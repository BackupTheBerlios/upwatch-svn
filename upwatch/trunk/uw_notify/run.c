#include "config.h"
#include <generic.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "uw_notify_glob.h"

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
  if (total) { 
    LOG(LOG_INFO, "Processed: Total:%u (%s)", total, buf);
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
      LOG(LOG_NOTICE, "cure dumped");
    }
#endif
  }
}

static char path[PATH_MAX];
int init(void)
{
  struct sigaction new_action;

  if (!HAVE_OPT(OUTPUT)) {
    LOG(LOG_ERR, "missing output option");
    return 0;
  }
  daemonize = TRUE;
  if (HAVE_OPT(RUN_QUEUE)) {
    every = ONE_SHOT;
  } else {
    every = EVERY_5SECS;
  }
  query_server_by_name = OPT_ARG(QUERY_SERVER_BY_NAME);
  query_server_by_ip = OPT_ARG(QUERY_SERVER_BY_IP);

  if (!HAVE_OPT(INPUT)) {
    LOG(LOG_ERR, "Error: no input queue given");
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
    LOG(LOG_WARNING, "g_dir_open %s: (%m) %s", path, error);
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
  if (debug > 3) { fprintf(stderr, "%u files in directory\n", files); }

  // now we have a sorted list of files 
  // 
  for (i=0; i < arr->len && forever; i++) {
    trx t;
    int j;
    int output_ct = STACKCT_OPT(OUTPUT);
    char **output_pn = STACKLST_OPT(OUTPUT);

    memset(&t, 0, sizeof(t));
    t.process = process; // callback for each result
    t.failed = UpwatchXmlDoc("result");
    char *filebase;

    filebase = strrchr((char *)g_ptr_array_index(arr,i), '/');
    if (filebase) filebase++;
    else filebase = (char *)g_ptr_array_index(arr,i);

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
      }
      if (t.failed_count) {
        char *name = filebase;
        xmlSetDocCompressMode(t.failed, OPT_VALUE_COMPRESS);
        spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(FAILURES), t.failed, &name);
      }
      for (j=0; j < output_ct; j++) {
        char *name = filebase;
        spool_result(OPT_ARG(SPOOLDIR), output_pn[j], t.doc, &name);
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


int run(void)
{
  int count = 0;
  struct hostent *host;
static int resummarize(void);

  if (debug > 3) { LOG(LOG_DEBUG, "run()"); }

  // the following statement ensures the nss-*.so libraries are loaded
  // before fork() is called. If not, AND this program is run under gdb,
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

        sprintf(path, "%s/%s", OPT_ARG(SPOOLDIR), pn[ct]);
        chdir(path);
        sprintf(path, "%s/%s/new", OPT_ARG(SPOOLDIR), pn[ct]);
        pid = fork();
        if (pid == -1) {
          LOG(LOG_ERR, "fork: %m");
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
  if (master) {
    chdir("/tmp");
    return 0;
  }

  uw_setproctitle("listing %s", path);

  modules_start_run();
  count = read_input_files(path);
  modules_end_run();
 
  return(count);
}
