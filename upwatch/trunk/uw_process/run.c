#include "config.h"
#include <generic.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <unistd.h>
#include "uw_process_glob.h"

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include "slot.h"
#include "modules.inc"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

extern int process(trx *t);
extern struct summ_spec summ_info[]; 

static int master = 1;
static pid_t childpid[256];
static int childpidcnt;

struct dbspec *dblist;
int dblist_cnt;

void update_last_seen(module *probe)
{
  dbi_result result;

  result = db_query(probe->db, 0,
                    "update probe set lastseen = %u where id = '%u'",
                    probe->lastseen, probe->class);
  if (result) dbi_result_free(result);
}

static void modules_end_run(void)
{
  int i;
  int filetotal = 0;
  int resulttotal = 0;
  char buf[1024];

  buf[0] = 0;

  for (i=0; i < dblist_cnt; i++) {
    if (dblist[i].realm) free(dblist[i].realm);
    if (dblist[i].host) free(dblist[i].host);
    if (dblist[i].dbtype) free(dblist[i].dbtype);
    if (dblist[i].dbname) free(dblist[i].dbname);
    if (dblist[i].dbuser) free(dblist[i].dbuser);
    if (dblist[i].dbpassword) free(dblist[i].dbpassword);
    if (dblist[i].srvrbyname) free(dblist[i].srvrbyname);
    if (dblist[i].srvrbyid) free(dblist[i].srvrbyid);
    if (dblist[i].srvrbyip) free(dblist[i].srvrbyip);

    if (dblist[i].db) {
      close_database(dblist[i].db);
    }
  }
  free(dblist);
  dblist = NULL; 
  dblist_cnt = 0;

  for (i = 0; modules[i]; i++) {
    char wrk[50];
    if (modules[i]->end_run) {
      modules[i]->end_run(modules[i]);
    }
    if (modules[i]->resultcount) {
      update_last_seen(modules[i]);
      sprintf(wrk, "%s:%u ", modules[i]->module_name, modules[i]->resultcount);
      resulttotal += modules[i]->resultcount;
      strcat(buf, wrk);

      filetotal += modules[i]->filecount;
    }
  }
  if (resulttotal) { 
    LOG(LOG_INFO, "Processed: %u files, %u results (%s)", filetotal, resulttotal, buf);
  }
}

static void modules_cleanup(void)
{
  int i;
extern void free_res(void *res);

  for (i = 0; modules[i]; i++) {
    if (modules[i]->insertc) {
      g_ptr_array_free(modules[i]->insertc, TRUE);
      modules[i]->insertc = NULL;
    }
    if (modules[i]->cache) {
      g_hash_table_destroy(modules[i]->cache);
      modules[i]->cache = NULL;
    }
    if (modules[i]->exit) {
      modules[i]->exit();
    }
  }
}

static void modules_start_run(void)
{
  int i;
  database *db;

  db = open_database(OPT_ARG(DBTYPE), OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME),
                     OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
  if (db) {
    dbi_result result;
    dblist = calloc(100, sizeof(struct dbspec));
    dblist_cnt = 0;
    
    result = db_query(db, 0, "select pr_realm.name, pr_realm.host, "
                             "       pr_realm.port, pr_realm.dbtype, pr_realm.dbname, pr_realm.dbuser, "
                             "       pr_realm.dbpassword, pr_realm.srvrbyname, "
                             "       pr_realm.srvrbyid, pr_realm.srvrbyip "
                             "from   pr_realm "
                             "where  pr_realm.id > 1");
    if (result) {
      while (dbi_result_next_row(result)) {
        strcpy(dblist[dblist_cnt].realm, dbi_result_get_string(result, "name"));
        strcpy(dblist[dblist_cnt].host, dbi_result_get_string_copy(result, "host"));
        dblist[dblist_cnt].port = dbi_result_get_int(result, "port");
        strcpy(dblist[dblist_cnt].dbtype, dbi_result_get_string(result, "dbtype"));
        strcpy(dblist[dblist_cnt].dbname, dbi_result_get_string(result, "dbname"));
        strcpy(dblist[dblist_cnt].dbuser, dbi_result_get_string(result, "dbuser"));
        strcpy(dblist[dblist_cnt].dbpassword, dbi_result_get_string(result, "dbpassword"));
        strcpy(dblist[dblist_cnt].srvrbyname, dbi_result_get_string(result, "srvrbyname"));
        strcpy(dblist[dblist_cnt].srvrbyid, dbi_result_get_string(result, "srvrbyid"));
        strcpy(dblist[dblist_cnt].srvrbyip, dbi_result_get_string(result, "srvrbyip"));
        dblist_cnt++;
      }
      dbi_result_free(result);
    }
    close_database(db);
  }

  for (i = 0; modules[i]; i++) {
    dbi_result result;

    modules[i]->filecount = 0;
    modules[i]->resultcount = 0;
    modules[i]->errors = 0;

    modules[i]->db = open_database(OPT_ARG(DBTYPE), OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME),
                                   OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
    if (modules[i]->db) {
      result = db_query(modules[i]->db, 0, "select fuse, lastseen from probe where id = '%d'", modules[i]->class);
      if (result) {
        if (dbi_result_next_row(result)) {
          modules[i]->fuse  = (strcmp(dbi_result_get_string(result, "fuse"), "yes") == 0) ? 1 : 0;
          modules[i]->lastseen = dbi_result_get_int(result, "lastseen");
        } else {
          LOG(LOG_NOTICE, "recordid %u not found in probe table (probetype='%s'), db=%s", modules[i]->class, modules[i]->module_name, OPT_ARG(DBNAME));
        }
        dbi_result_free(result);
      }
      close_database(modules[i]->db);
      modules[i]->db = NULL;
    }

    if (modules[i]->start_run) {
      modules[i]->start_run(modules[i]);
    }
  }
}

static void free_def(void *p)
{
  struct probe_def *def = (struct probe_def *) p;

  if (def->ipaddress) g_free(def->ipaddress);
  if (def->description) g_free(def->description);
  g_free(def);
}

static void modules_init(void)
{
  int i;

  for (i = 0; modules[i]; i++) {
    if (modules[i]->init) {
      modules[i]->init(modules[i]);
    }
    if (modules[i]->needs_cache) {
      if (modules[i]->cache == NULL) {
        modules[i]->cache = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, 
                              modules[i]->free_def ? modules[i]->free_def : free_def);
      }
    }
    if (modules[i]->insertc == NULL) {
      modules[i]->insertc =  g_ptr_array_new();
    }
  }
}

void child_termination_handler (int signum)
{
  for (;;) {
    pid_t pid;
    int i, status;

    pid = waitpid(-1, &status, WNOHANG);
    if (pid < 0) {
      if (errno != ECHILD) {
        LOG(LOG_ERR, "waitpid: %u %m", errno);
      }
      return;
    }
    if (pid == 0) return;
    for (i = 0; i < childpidcnt; i++) {
      if (childpid[i] == pid) { // wipe pid from list
        childpid[i] = 0;
        break;
      }
    }
    if (WIFEXITED(status)) {
      LOG(LOG_NOTICE, "child %u exited with status %d", pid, WEXITSTATUS(status));
    }
    if (WIFSIGNALED(status)) {
      LOG(LOG_NOTICE, "child %u exited by signal %d", pid, WTERMSIG(status));
    }
#ifdef WCOREDUMP
    if (WCOREDUMP(status)) {
      LOG(LOG_NOTICE, "pid %u dumped core", pid);
    }
#endif
  }
}

static char path[PATH_MAX];
int init(void)
{
  struct sigaction new_action;

  daemonize = TRUE;
  if (HAVE_OPT(RUN_QUEUE)) {
    every = ONE_SHOT;
  } else {
    every = EVERY_5SECS;
  }

  if (HAVE_OPT(SUMMARIZE)) {
    every = ONE_SHOT;
  }

  if (!HAVE_OPT(OUTPUT)) {
    LOG(LOG_ERR, "missing output option");
    return 0;
  }

  // check trust option strings
  if (HAVE_OPT(TRUST)) {
    int i, found=0;
    int     ct  = STACKCT_OPT( TRUST );
    char**  pn = (char **) &STACKLST_OPT( TRUST );

    while (ct--) {
      for (i=0; modules[i]; i++) {
        if (modules[i]->accept_probe) {
          trx t;

          t.probe = modules[i];
          if (modules[i]->accept_probe(&t, pn[ct])) {
            found = 1; break;
          } 
        } else {
          if (!strcmp(pn[ct], modules[i]->module_name)) {
            found = 1; break;
          }
        }
      } 
      if (strcmp(pn[ct], "all") == 0 ||
          strcmp(pn[ct], "none") == 0) { 
          found = 1;
      }
      if (!found) {
        LOG(LOG_NOTICE, "warning: unknown trust `%s' ignored", pn[ct]);
      } 
    }
  }
  if (!HAVE_OPT(INPUT)) {
    LOG(LOG_ERR, "Error: no input queue given");
    return(0);
  }

  if (HAVE_OPT(SLAVE)) {
    sprintf(path, "%s/%s/new", OPT_ARG(SPOOLDIR), OPT_ARG(SLAVE));
    master = 0;
  } else { // master process
    struct hostent *host;

    chdir("/tmp"); // for core dumps

    // the following statement ensures the nss-*.so libraries are loaded
    // before fork() is called. If not, AND this program is run under gdb,
    // children will get a SIGTRAP when THEY load nss-*.so libs, and will
    // be killed.
    host = gethostbyname("localhost");

    /* set up SIGCHLD handler */
    new_action.sa_handler = child_termination_handler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = SA_RESTART|SA_NOCLDSTOP; // not interested in children that stopped
    sigaction (SIGCHLD, &new_action, NULL);
  }

  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  modules_init();
  atexit(modules_cleanup);
  return(1);
}

// return true if the given probename is trusted
int trust(char *name)
{
  int trust;
  int     ct  = STACKCT_OPT( TRUST );
  char**  pn = (char **) &STACKLST_OPT( TRUST );

  for (trust=0; trust < ct; trust++) {
    if (strcmp(pn[trust], "all") == 0) {
      return 1;
    }
    if (strcmp(pn[trust], "none") == 0) {
      return 0;
    }
    if (strcmp(pn[trust], name) == 0) {
      return 1;
    }
  }
  return 0;
}

static int mystrcmp(char **a, char **b)
{
  int ret = strcmp(*a, *b);

  if (ret < 0) return(-1);
  if (ret > 0) return(1);
  return(0);
 
}

//************************************************************************
// read all files in the directory, sort and process them
//***********************************************************************
int read_input_files(char *path)
{
  int count = 0;
  G_CONST_RETURN gchar *filename;
  GError *error=NULL;
  GDir *dir;
  GPtrArray *arr = g_ptr_array_new();
  int i;
  int files = 0;
extern int forever;
  dir = g_dir_open (path, 0, &error);
  if (dir == NULL) {
    LOG(LOG_ERR, "g_dir_open %s: (%m) %s", path, error);
    g_ptr_array_free(arr, TRUE);
    return 0;
  }
  while ((filename = g_dir_read_name(dir)) != NULL) {
    char buffer[PATH_MAX];

    if (filename[0] == '.') continue;  // skip hidden files
    sprintf(buffer, "%s/%s", path, filename);
    g_ptr_array_add(arr, strdup(buffer));
    files++;
  }
  g_dir_close(dir);

  if (!files) {
    g_ptr_array_free(arr, TRUE);
    return 0;
  }
  g_ptr_array_sort(arr, (GCompareFunc) mystrcmp);
  if (debug > 3) { fprintf(stderr, "%u files in directory\n", files); sleep(3); }

  // now we have a sorted list of files 
  // 
  for (i=0; i < arr->len && forever; i++) {
    trx t;
    int j;
    int output_ct = STACKCT_OPT(OUTPUT);
    char **output_pn = (char **) &STACKLST_OPT(OUTPUT);
    char *filebase;

    filebase = strrchr((char *)g_ptr_array_index(arr,i), '/');
    if (filebase) filebase++;
    else filebase = (char *)g_ptr_array_index(arr,i);

    memset(&t, 0, sizeof(t));
    t.process = process; // callback for each result

    if (debug > 3) { fprintf(stderr, "%u: %s: ", i, (char *)g_ptr_array_index(arr,i)); }
    uw_setproctitle("reading %s", (char *)g_ptr_array_index(arr,i));
    switch (handle_result_file(g_ptr_array_index(arr,i), &t)) { // extract probe results and queue them
    case 0: /* file not found */
      break;
    case 1: /* file empty */
      unlink(g_ptr_array_index(arr,i));
      break;
    case 2: /* illegal contents */
      if (HAVE_OPT(COPY) && strcmp(OPT_ARG(COPY), "none")) {
        char *name = filebase;
        spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(COPY), t.doc, &name);
	free(name);
      }
      unlink(g_ptr_array_index(arr,i));
      break;
    case 3: /* try again later */
      break;
    case 4: /* successfully processed */
      xmlSetDocCompressMode(t.doc, OPT_VALUE_COMPRESS);
      if (HAVE_OPT(COPY) && strcmp(OPT_ARG(COPY), "none")) {
        char *name = filebase;
        spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(COPY), t.doc, &name);
	free(name);
      }
      if (t.failed_count) {
        char *name = filebase;
        xmlSetDocCompressMode(t.failed, OPT_VALUE_COMPRESS);
        spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(FAILURES), t.failed, &name);
	free(name);
      }
      for (j=0; j < output_ct; j++) {
        char *name = filebase;
        spool_result(OPT_ARG(SPOOLDIR), output_pn[j], t.doc, &name);
	free(name);
      }
      unlink(g_ptr_array_index(arr,i));
      break;
    }
    free(g_ptr_array_index(arr,i));

    if (t.doc) xmlFreeDoc(t.doc);
    if (t.failed) xmlFreeDoc(t.failed);
    count++;
  }
  g_ptr_array_free(arr, TRUE);
  return(count);
}

int master_checks(void)
{
  int i;
  int     ct  = STACKCT_OPT( INPUT );
  char**  pn = (char **) &STACKLST_OPT( INPUT );

  childpidcnt = ct;
  if (debug > 2) fprintf(stderr, "pondering..\n");
  while (ct--) {
    pid_t pid;

    if (childpid[ct] > 0) continue; // child still running

    sprintf(path, "%s/%s", OPT_ARG(SPOOLDIR), pn[ct]);
    chdir(path); // for coredumps
    sprintf(path, "%s/%s/new", OPT_ARG(SPOOLDIR), pn[ct]);
    pid = fork();
    if (pid == -1) {
      LOG(LOG_ERR, "fork: %m");
      return 1;
    }
    if (pid == 0) {
      master = FALSE; 
      return 0;
    }
    if (debug > 2) fprintf(stderr, "started [%u] on %s\n", pid, path);
    LOG(LOG_NOTICE, "started [%u] on %s", pid, path);
    childpid[ct] = pid;
    sleep(1);
  } 

  for (i = 0; modules[i]; i++) {
    modules[i]->db = open_database(OPT_ARG(DBTYPE), OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME),
                                   OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
    if (modules[i]->db) {
      dbi_result result;

      result = db_query(modules[i]->db, 0, "select lastseen, maxlag, lagwarn from probe where id = '%u'", 
                                           modules[i]->class);
      if (result) {
        if (dbi_result_next_row(result)) {
          unsigned now;
          int lastseen = dbi_result_get_int(result, "lastseen");
          int maxlag = dbi_result_get_int(result, "maxlag");
          int lagwarn = strcmp(dbi_result_get_string(result, "lagwarn"), "yes") == 0;

          now = (int) time(NULL);
          if ((now - lastseen) > maxlag) {
            if (!lagwarn) {
              char subject[256];

              sprintf(subject, "UPWATCH: probe %s is lagging in processing", modules[i]->module_name);
              mail((char *) &OPT_ARG(NOC_MAIL), subject, subject, (time_t)NULL);
              result = db_query(modules[i]->db, 0, "update probe set lagwarn = 'yes' where id = '%u'",
                                           modules[i]->class);
              dbi_result_free(result);
            }
          } else {
            if (lagwarn) {
              char subject[256];

              sprintf(subject, "UPWATCH: probe %s is up-to-date again", modules[i]->module_name);
              mail((char *) &OPT_ARG(NOC_MAIL), subject, subject, (time_t)NULL);
              result = db_query(modules[i]->db, 0, "update probe set lagwarn = 'no' where id = '%u'",
                                           modules[i]->class);
              dbi_result_free(result);
            }
          }
        } else {
          LOG(LOG_NOTICE, "%s probe record for id %u not found", modules[i]->module_name, modules[i]->class);
        }
        dbi_result_free(result);
      }
      close_database(modules[i]->db);
      modules[i]->db = NULL;
    }
  }
  return 0;
}

int run(void)
{
  int count = 0;

  if (debug > 3) { LOG(LOG_DEBUG, "run()"); }

  if (HAVE_OPT(SUMMARIZE)) {
    int resummarize(void);
    return(resummarize()); // --summarize
  }

  if (master) {
    uw_setproctitle("doing checks");
    count = master_checks();
  } else {
    modules_start_run();
    uw_setproctitle("listing %s", path);
    count = read_input_files(path);
    modules_end_run();
  }
 
  return(count);
}

int resummarize(void)
{
  gint idx, found;
  trx t;
  struct probe_result res;
  struct probe_def def;
  database *db;
  dbi_result result;
extern int forever;
  guint lowtime, hightime; 
  char *p;

  res.name = strtok((char *) &OPT_ARG(SUMMARIZE), ",");
  lowtime = 0;
  hightime = time(NULL);
  p = strtok(NULL, ",");
  if (p && atoi(p)) lowtime = atoi(p);
  p = strtok(NULL, ",");
  if (p && atoi(p)) hightime = atoi(p);
  
//  printf("%s: %s", res.name, ctime(&res.stattime));

  memset(&t, 0, sizeof(t));
  memset(&def, 0, sizeof(def));
  memset(&res, 0, sizeof(res));
  t.def = &def;
  t.res = &res;
  for (found = 0, idx = 0; modules[idx]; idx++) {
    if (modules[idx]->accept_probe) {
      if (modules[idx]->accept_probe(&t, res.name)) {
        t.probe = modules[idx];
        found = 1; break;
      }
    } else {
      if (!strcmp(res.name, modules[idx]->module_name)) {
        found = 1; break;
      }
    }
  }
  if (!found) {
    char buf[256];

    sprintf(buf, "unknown probe %s", res.name);
    fprintf(stderr, "%s\n", buf);
    LOG(LOG_NOTICE, buf);
    return 0;
  }

  db = open_database(OPT_ARG(DBTYPE), OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME), 
			OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
  if (!db) {
    return 0;
  }
  modules_start_run();
  found = 0;
  result = db_query(db, 1, "select id, server from pr_%s_def where id > 1", res.name);
  if (result == NULL) return(0);
  while (dbi_result_next_row(result) && forever) {
    dbi_result presult;

    def.probeid = dbi_result_get_int(result, "id");
    def.server = dbi_result_get_int(result, "server");
    //printf("%u server %u\n", def.probeid, def.server);

    presult = db_query(db, 1, "select stattime from pr_%s_raw where probe = '%u' "
                                 "and stattime >= '%u' and stattime <= '%u'", 
                                 res.name, def.probeid, lowtime, hightime);
    if (presult == NULL) continue;
    while (dbi_result_next_row(presult) && forever) {
      guint cur_slot, prev_slot;
      gulong dummy_low, dummy_high;
      gulong slotlow, slothigh;
      int i;

      res.stattime = dbi_result_get_int(presult, "stattime");
      if (def.newest == 0) def.newest = res.stattime;
      found++;

      //printf("stattime = %u, prev = %u\n", res.stattime, def.newest);

      for (i=0; summ_info[i].period != -1; i++) { // FOR EACH PERIOD
        prev_slot = uw_slot(summ_info[i].period, def.newest, &slotlow, &slothigh);
        cur_slot = uw_slot(summ_info[i].period, res.stattime, &dummy_low, &dummy_high);

        if (cur_slot != prev_slot) {
          modules[idx]->summarize(&t, summ_info[i].from, summ_info[i].to, 
                                  cur_slot, slotlow, slothigh, 1 /* delete any previous values */);
          //printf("\ncurslot=%u, prev=%u\n", cur_slot, prev_slot);
        }
      }

      def.newest = res.stattime;
      if (found %500 == 0) {
        if (debug > 2) { fprintf(stderr, "%8u records processed\r", found); }
      }
    }
    dbi_result_free(presult);

  }
  dbi_result_free(result);
  modules_end_run();
  close_database(db);
  return(found);
}

