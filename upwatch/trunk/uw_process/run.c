#include "config.h"
#include <generic.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef UW_PROCESS
#include "uw_process_glob.h"
#endif
#ifdef UW_NOTIFY
#include "uw_notify_glob.h"
#endif

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include "slot.h"
#include "modules.inc"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static int handle_file(gpointer data, gpointer user_data);
extern  int process(module *module, trx *t);
extern struct summ_spec summ_info[]; 

static int master = 1;
static pid_t childpid[256];
static int childpidcnt;

static void modules_end_run(void)
{
  int i, total = 0;
  char buf[1024];

  buf[0] = 0;

  for (i = 0; modules[i]; i++) {
    char wrk[50];
    if (modules[i]->end_run) {
      modules[i]->end_run(modules[i]);
    }
    if (modules[i]->db) {
      MYSQL_RES *result;

      result = my_query(modules[i]->db, 0, 
                        "update probe set lastseen = UNIX_TIMESTAMP() where name = '%s'", 
                        modules[i]->module_name);
      if (result) mysql_free_result(result);
      close_database(modules[i]->db);
      modules[i]->db = NULL;
    }
    if (debug && modules[i]->count) {
      sprintf(wrk, "%s:%u ", modules[i]->module_name, modules[i]->count);
      total += modules[i]->count;
      modules[i]->count = 0;
      strcat(buf, wrk);
    }
  }
  if (debug > 1 && total) { 
    LOG(LOG_DEBUG, "Processed: Total:%u (%s)", total, buf);
  }
}

static void modules_cleanup(void)
{
  int i;
extern void free_res(void *res);

  for (i = 0; modules[i]; i++) {
/*
    if (modules[i]->cleanup) {
      modules[i]->cleanup();
    }
*/
    if (modules[i]->queue) {
#if 0
      trx *t;

      while (1) {
        g_static_mutex_lock (&modules[i]->queue_mutex);
        t = g_queue_pop_head(modules[i]->queue);
        g_static_mutex_unlock (&modules[i]->queue_mutex);
        if (t == NULL) break;

        // free the result block
        if (t->res) {
          if (modules[i]->free_res) {
            modules[i]->free_res(t->res); // the probe specific part...
          }
          free_res(t->res); // .. and the generic part
        }
        resfile_decr(t->rf);
        g_free(t);
      }
#endif
      g_queue_free(modules[i]->queue);
    }
    if (modules[i]->insertc) {
      g_ptr_array_free(modules[i]->insertc, TRUE);
      modules[i]->insertc = NULL;
    }
    if (modules[i]->cache) {
      g_hash_table_destroy(modules[i]->cache);
      modules[i]->cache = NULL;
    }
  }
}

static void modules_start_run(void)
{
  int i;

  for (i = 0; modules[i]; i++) {
    if (runcounter % 50 == 1) { // the first time or every 50 runs
      MYSQL_RES *result;

      modules[i]->db = open_database(OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME),
                                     OPT_ARG(DBUSER), OPT_ARG(DBPASSWD),
                                     OPT_VALUE_DBCOMPRESS);
      if (modules[i]->db) {
        result = my_query(modules[i]->db, 0, "select fuse from probe where id = '%d'", modules[i]->class);
        if (result) {
          MYSQL_ROW row;
          row = mysql_fetch_row(result);
          if (row) {
            if (row[0]) modules[i]->fuse  = (strcmp(row[0], "yes") == 0) ? 1 : 0;
          } else {
            LOG(LOG_NOTICE, "probe record for id %u not found", modules[i]->class);
          }
          mysql_free_result(result);
        }
        close_database(modules[i]->db);
        modules[i]->db = NULL;
      }
    }

    if (modules[i]->start_run) {
      modules[i]->start_run(modules[i]);
    }
  }
}

static void modules_init(void)
{
  int i;

  for (i = 0; modules[i]; i++) {
    if (modules[i]->init) {
      modules[i]->init();
    }
    if (modules[i]->cache == NULL) {
      modules[i]->cache = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, 
                            modules[i]->free_def ? modules[i]->free_def : g_free);
    }
    if (modules[i]->insertc == NULL) {
      modules[i]->insertc =  g_ptr_array_new();
    }
    if (modules[i]->queue == NULL) {
      modules[i]->queue = g_queue_new();
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
        LOG(LOG_NOTICE, "waitpid: %u %m", errno);
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
      LOG(LOG_NOTICE, "cure dumped");
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
#ifdef UW_PROCESS
  if (HAVE_OPT(SUMMARIZE)) {
    every = ONE_SHOT;
  }

  // check trust option strings
  if (HAVE_OPT(TRUST)) {
    int i, found=0;
    int     ct  = STACKCT_OPT( TRUST );
    char**  pn = STACKLST_OPT( TRUST );

    while (ct--) {
      for (i=0; modules[i]; i++) {
        if (modules[i]->accept_probe) {
          if (modules[i]->accept_probe(modules[i], pn[ct])) {
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
#endif
  if (!HAVE_OPT(INPUT)) {
    LOG(LOG_NOTICE, "Error: no input queue given");
    return(0);
  }

  if (HAVE_OPT(SLAVE)) {
    sprintf(path, "%s/%s/new", OPT_ARG(SPOOLDIR), OPT_ARG(SLAVE));
    master = 0;
  }

  /* set up SIGCHLD handler */
  new_action.sa_handler = child_termination_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = SA_RESTART|SA_NOCLDSTOP; // not interested in children that stopped
  sigaction (SIGCHLD, &new_action, NULL);

  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  modules_init();
  atexit(modules_cleanup);
  return(1);
}

#ifdef UW_PROCESS
// return true if the given probename is trusted
int trust(char *name)
{
  int trust;
  int     ct  = STACKCT_OPT( TRUST );
  char**  pn = STACKLST_OPT( TRUST );

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
#endif

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
    LOG(LOG_NOTICE, "g_dir_open %s: (%m) %s", path, error);
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
  g_ptr_array_sort(arr, mystrcmp);
  if (debug > 3) { fprintf(stderr, "%u files in directory\n", files); sleep(3); }

  // now we have a sorted list of files 
  // 
  for (i=0; i < arr->len && forever; i++) {
    if (debug > 3) { fprintf(stderr, "%u: %s: ", i, (char *)g_ptr_array_index(arr,i)); }
    uw_setproctitle("reading %s", (char *)g_ptr_array_index(arr,i));
    handle_file(g_ptr_array_index(arr,i), NULL); // extract probe results and queue them
    count++;
  }
  g_ptr_array_free(arr, TRUE);
  return(count);
}


int run(void)
{
  int count = 0;
  struct hostent *host;
static int resummarize(void);

  if (debug > 3) { LOG(LOG_DEBUG, "run()"); }

#ifdef UW_PROCESS
  if (HAVE_OPT(SUMMARIZE)) {
    return(resummarize()); // --summarize
  }
#endif

  // the following statement ensures the nss-*.so libraries are loaded
  // before fork() is called. If not, AND this program is run under gdb
  // children will get a SIGTRAP when THEY load nss-*.so libs, and will
  // be killed.
  host = gethostbyname("localhost");

  if (master) {
    int     ct  = STACKCT_OPT( INPUT );
    char**  pn = STACKLST_OPT( INPUT );

    childpidcnt = ct;
    while (ct--) {
      if (childpid[ct] == 0) { 
        pid_t pid;

        sprintf(path, "%s/%s/new", OPT_ARG(SPOOLDIR), pn[ct]);
        pid = fork();
        if (pid == -1) {
          LOG(LOG_NOTICE, "fork: %m");
          return 1;
        }
        if (pid == 0) {
          master = FALSE; 
          break;
        }
        LOG(LOG_NOTICE, "started [%u] on %s", pid, path);
        childpid[ct] = pid;
        sleep(1);
      }
    } 
  }
  if (master) return 0;

  uw_setproctitle("listing %s", path);

  modules_start_run();
  count = read_input_files(path);
  modules_end_run();
 
  return(count);
}

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//******************************************************************* 
static void *extract_info_from_xml(module *probe, xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns)
{
  struct probe_result *res;

  res = g_malloc0(probe->res_size);
  if (res == NULL) {
    return(NULL);
  }

  res->name = strdup(cur->name);

  res->server = xmlGetPropInt(cur, (const xmlChar *) "server");
  res->probeid = xmlGetPropInt(cur, (const xmlChar *) "id");
  res->stattime = xmlGetPropUnsigned(cur, (const xmlChar *) "date");
  res->expires = xmlGetPropUnsigned(cur, (const xmlChar *) "expires");
  res->ipaddress = xmlGetProp(cur, (const xmlChar *) "ipaddress");

  if (probe->xml_result_node) {
    probe->xml_result_node(probe, doc, cur, ns, res);
  }

  for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
    char *p;

    if (xmlIsBlankNode(cur)) continue;
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "color")) && (cur->ns == ns)) {
      res->color = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
      continue;
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "info")) && (cur->ns == ns)) {
      p = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (p) { 
        res->message = strdup(p); 
        xmlFree(p);
      }
      continue;
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "notify")) && (cur->ns == ns)) {
      res->proto = xmlGetProp(cur->xmlChildrenNode, (const xmlChar *) "proto");
      res->target = xmlGetProp(cur->xmlChildrenNode, (const xmlChar *) "target");
      continue;
    }
    if (probe->get_from_xml) {
      probe->get_from_xml(probe, doc, cur, ns, res);
    }
  }
  return(res);
}

/*
 * Reads a results file
*/
static int handle_file(gpointer data, gpointer user_data)
{
  char *filename = (char *)data;
  xmlDocPtr doc, failed, notify;
  xmlNsPtr ns;
  xmlNodePtr cur;
  char *p;
  char *fromhost = NULL;
  time_t fromdate;
  int failures=0;
  struct stat st;
  int filesize;
  int i;
  int output_ct = STACKCT_OPT(OUTPUT);
  char **output_pn = STACKLST_OPT(OUTPUT);
#ifdef UW_PROCESS
  int notify_ct = STACKCT_OPT(NOTIFY);
  char **notify_pn = STACKLST_OPT(NOTIFY);
#endif

  if (debug) { LOG(LOG_DEBUG, "Processing %s", filename); }

  if (stat(filename, &st)) {
    LOG(LOG_WARNING, "%s: %m", filename);
    unlink(filename);
    free(filename);
    return 0;
  }
  filesize = (int) st.st_size;
  if (filesize == 0) {
    LOG(LOG_WARNING, "%s: size 0, removed", filename);
    unlink(filename);
    free(filename);
    return 0;
  }

  doc = xmlParseFile(filename);
  if (doc == NULL) {
    char cmd[2048];

    LOG(LOG_NOTICE, "%s: %m", filename);
    sprintf(cmd, "cat %s >> %s", filename, OPT_ARG(FAILURES));
    system(cmd);
    unlink(filename);
    free(filename);
    return 0;
  }
  if (HAVE_OPT(COPY) && strcmp(OPT_ARG(COPY), "none")) {
    spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(COPY), doc, NULL);
  }

  cur = xmlDocGetRootElement(doc);
  if (cur == NULL) {
    LOG(LOG_NOTICE, "%s: empty document", filename);
    unlink(filename);
    free(filename);
    xmlFreeDoc(doc);
    return 0;
  }
  ns = xmlSearchNsByHref(doc, cur, (const xmlChar *) NAMESPACE_URL);
  if (ns == NULL) {
    LOG(LOG_NOTICE, "%s: wrong type, result namespace not found", filename);
    unlink(filename);
    free(filename);
    xmlFreeDoc(doc);
    return 0;
  }
  if (xmlStrcmp(cur->name, (const xmlChar *) "result")) {
    LOG(LOG_NOTICE, "%s: wrong type, root node is not 'result'", filename);
    unlink(filename);
    free(filename);
    xmlFreeDoc(doc);
    return 0;
  }
  p = xmlGetProp(cur, (const xmlChar *) "fromhost");
  if (p) {
    fromhost = strdup(p);
    xmlFree(p);
  }
  fromdate = (time_t) xmlGetPropUnsigned(cur, (const xmlChar *) "date");

  /*
   * Now, walk the tree.
   */
  /* First level we expect just result */
  cur = cur->xmlChildrenNode;
  while (cur && xmlIsBlankNode(cur)) {
    cur = cur->next;
  }
  if (cur == 0) {
    LOG(LOG_NOTICE, "%s: empty file", filename);
    unlink(filename);
    free(filename);
    xmlFreeDoc(doc);
    return 0;
  }
  failed = UpwatchXmlDoc("result");
  xmlSetDocCompressMode(failed, OPT_VALUE_COMPRESS);
#ifdef UW_PROCESS
  notify = UpwatchXmlDoc("result");
  xmlSetDocCompressMode(notify, OPT_VALUE_COMPRESS);
#endif

  /* Second level is a list of probes, but be laxist */
  for (failures = 0; cur != NULL;) {
    int found = 0;
    char buf[20];
    int count = 0;
    trx *t = NULL;

    buf[0] = 0;
    if (xmlIsBlankNode(cur)) {
      cur = cur->next;
      continue;
    }
    for (i = 0; modules[i]; i++) {
      int ret;

      if (modules[i]->accept_probe) {
        if (!modules[i]->accept_probe(modules[i], cur->name)) continue;
      } else if (strcmp(cur->name, modules[i]->module_name)) {
        continue;
      } 

      if (cur->ns != ns) {
        LOG(LOG_ERR, "%s: namespace %s != %s", cur->name, cur->ns, ns);
        //continue;
      }
      found = 1;
      t = (trx *)g_malloc0(sizeof(trx));
      t->res = extract_info_from_xml(modules[i], doc, cur, ns);
      t->doc = doc;
      t->node = cur;

      if (buf[0] == 0 || count % 100 == 0) {
        strftime(buf, sizeof(buf), "%Y-%m-%d %T", gmtime(&fromdate));
        uw_setproctitle("%s %s@%s", buf, t->res->name, fromhost);
      }
      if (debug > 3) { fprintf(stderr, "%s %s@%s", buf, t->res->name, fromhost); }
      ret = process(modules[i], t);
      if (ret == 0 || ret == -1) { // error in processing this probe
        failures++; 
        found = FALSE; // so it will go to the failed section
      } else if (ret == -2) {
        free(t);
        goto errexit; // fatal database error
      } else {
        count++;
      }
      break;
    }
    if (found) {
#ifdef UW_PROCESS
      xmlNodePtr node, new;

      if (t->notify) {
        node = cur;
        xmlUnlinkNode(node); // unlink, copy and paste
        new = xmlCopyNode(node, 1);
        xmlFreeNode(node);
        xmlAddChild(xmlDocGetRootElement(notify), new);
      }
#endif
    } else {
      xmlNodePtr node, new;

      LOG(LOG_ERR, "can't find method: %s, saved to %s", cur->name, OPT_ARG(FAILURES));
      failures++;
      node = cur;
      xmlUnlinkNode(node); // unlink, copy and paste
      new = xmlCopyNode(node, 1);
      xmlFreeNode(node);
      xmlAddChild(xmlDocGetRootElement(failed), new);
    }
    cur = cur->next;
    if (t) free(t);
  }
  if (failures) {
    spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(FAILURES), failed, NULL);
  }
  for (i=0; i < output_ct; i++) {
    spool_result(OPT_ARG(SPOOLDIR), output_pn[i], doc, NULL);
  }
#ifdef UW_PROCESS
  for (i=0; i < notify_ct; i++) {
    spool_result(OPT_ARG(SPOOLDIR), notify_pn[i], doc, NULL);
  }
#endif
  unlink(filename);

errexit:
  free(filename);
  xmlFreeDoc(doc);
  xmlFreeDoc(notify);
  xmlFreeDoc(failed);
  if (fromhost) free(fromhost);
  return 0;
}

#ifdef UW_PROCESS
static int resummarize(void)
{
  gint idx, found;
  struct probe_def def;
  struct probe_result res;
  MYSQL *mysql;
  MYSQL_RES *result;
  MYSQL_ROW row;
extern int forever;
  guint lowtime, hightime; 
  char *p;

  res.name = strtok(OPT_ARG(SUMMARIZE), ",");
  lowtime = 0;
  hightime = time(NULL);
  p = strtok(NULL, ",");
  if (p && atoi(p)) lowtime = atoi(p);
  p = strtok(NULL, ",");
  if (p && atoi(p)) hightime = atoi(p);
  
//  printf("%s: %s", res.name, ctime(&res.stattime));

  memset(&def, 0, sizeof(def));
  memset(&res, 0, sizeof(res));
  for (found = 0, idx = 0; modules[idx]; idx++) {
    if (modules[idx]->accept_probe) {
      if (modules[idx]->accept_probe(modules[idx], res.name)) {
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

  mysql = open_database(OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME), 
			OPT_ARG(DBUSER), OPT_ARG(DBPASSWD), OPT_VALUE_DBCOMPRESS);
  if (!mysql) {
    return 0;
  }
  modules_start_run();
  found = 0;
  result = my_query(mysql, 1, "select id, server from pr_%s_def where id > 1", res.name);
  if (result == NULL) return(0);
  while ((row = mysql_fetch_row(result)) && forever) {
    MYSQL_RES *presult;
    MYSQL_ROW prow;

    def.probeid = atoi(row[0]);
    def.server = atoi(row[1]);
    //printf("%u server %u\n", def.probeid, def.server);

    presult = my_query(mysql, 1, "select stattime from pr_%s_raw where probe = '%u' "
                                 "and stattime >= '%u' and stattime <= '%u'", 
                                 res.name, def.probeid, lowtime, hightime);
    if (presult == NULL) continue;
    while ((prow = mysql_fetch_row(presult)) && forever) {
      guint cur_slot, prev_slot;
      gulong dummy_low, dummy_high;
      gulong slotlow, slothigh;
      int i;

      res.stattime = atoi(prow[0]);
      if (def.newest == 0) def.newest = res.stattime;
      found++;

      //printf("stattime = %u, prev = %u\n", res.stattime, def.newest);

      for (i=0; summ_info[i].period != -1; i++) { // FOR EACH PERIOD
        prev_slot = uw_slot(summ_info[i].period, def.newest, &slotlow, &slothigh);
        cur_slot = uw_slot(summ_info[i].period, res.stattime, &dummy_low, &dummy_high);

        if (cur_slot != prev_slot) {
          modules[idx]->summarize(modules[idx], &def, &res, summ_info[i].from, summ_info[i].to, 
                                  cur_slot, slotlow, slothigh, 1 /* delete any previous values */);
          //printf("\ncurslot=%u, prev=%u\n", cur_slot, prev_slot);
        }
      }

      def.newest = res.stattime;
      if (found %500 == 0) {
        if (debug > 2) { fprintf(stderr, "%8u records processed\r", found); }
      }
    }
    mysql_free_result(presult);

  }
  mysql_free_result(result);
  modules_end_run();
  close_database(mysql);
  return(found);
}
#endif

