#include "config.h"
#include <netinet/in_systm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <netinet/in.h>
#include <curl/curl.h>
#include <netdb.h>

#include <generic.h>
#include "cmd_options.h"
#include "uw_process.h"
#include "slot.h"
#include "modules.inc"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static int handle_file(gpointer data, gpointer user_data);
extern  int process(module *module, xmlDocPtr, xmlNodePtr, xmlNsPtr);
extern struct summ_spec summ_info[]; 

static void modules_end_run(void)
{
  int i, total = 0;
  char buf[1024];

  buf[0] = 0;

  for (i = 0; modules[i]; i++) {
    char wrk[50];
    if (modules[i]->end_run) {
      modules[i]->end_run();
    }
    close_database(modules[i]->db);
    modules[i]->db = NULL;
    if (debug && modules[i]->count) {
      sprintf(wrk, "%s:%u ", modules[i]->module_name, modules[i]->count);
      total += modules[i]->count;
      modules[i]->count = 0;
      strcat(buf, wrk);
    }
  }
  if (debug) LOG(LOG_DEBUG, "Processed: Total:%u (%s)", total, buf);
}

static void modules_cleanup(void)
{
  int i;

  for (i = 0; modules[i]; i++) {
/*
    if (modules[i]->cleanup) {
      modules[i]->cleanup();
    }
*/
    if (modules[i]->cache) {
      g_hash_table_destroy(modules[i]->cache);
    }
  }
}

static void modules_start_run(void)
{
  int i;

  for (i = 0; modules[i]; i++) {
    modules[i]->db = open_database(OPT_ARG(DBHOST), OPT_ARG(DBNAME),
                                     OPT_ARG(DBUSER), OPT_ARG(DBPASSWD),
                                     OPT_VALUE_DBCOMPRESS);
    if (modules[i]->start_run) {
      modules[i]->start_run();
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
                            modules[i]->free_def? modules[i]->free_def : g_free);
    }
  }
}

int init(void)
{
  daemonize = TRUE;
  if (HAVE_OPT(RUN_QUEUE) || HAVE_OPT(SUMMARIZE)) {
    every = ONE_SHOT;
  } else {
    every = EVERY_5SECS;
  }

  // check trust option strings
  if (HAVE_OPT(TRUST)) {
    int i, found=0;
    int     ct  = STACKCT_OPT( TRUST );
    char**  pn = STACKLST_OPT( TRUST );

    for (ct--; ct; ct--) {
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
  //g_thread_init(NULL);
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
int run(void)
{
  int count = 0;
  char path[PATH_MAX];
  G_CONST_RETURN gchar *filename;
  GError *error=NULL;
  GDir *dir;
  GPtrArray *arr = g_ptr_array_new();
  int i;
  int files = 0;
  int got_fatal = 0;
extern int forever;
static int resummarize(void);

  if (debug > 3) LOG(LOG_DEBUG, "run()");

  if (HAVE_OPT(SUMMARIZE)) {
    return(resummarize()); // --summarize
  }
  sprintf(path, "%s/%s/new", OPT_ARG(SPOOLDIR), OPT_ARG(INPUT));
  dir = g_dir_open (path, 0, &error);
  if (dir == NULL) {
    LOG(LOG_NOTICE, "g_dir_open: %s", error);
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

  modules_start_run();
  for (i=0; i < arr->len && forever; i++) {
    //printf("%s\n", g_ptr_array_index(arr,i));
    if (!got_fatal) {
      got_fatal = handle_file(g_ptr_array_index(arr,i), NULL);
    }
    free(g_ptr_array_index(arr,i));
    count++;
  }
  modules_end_run();

  g_ptr_array_free(arr, TRUE);
  return(count);
}

/*
 * Reads a results file
*/
static int handle_file(gpointer data, gpointer user_data)
{
  char *filename = (char *)data;
  xmlDocPtr doc; 
  xmlNsPtr ns;
  xmlNodePtr cur;
  char *p, *fromhost=NULL;
  time_t fromdate = 0;
  int failures=0;
  int probe_count = 0;
  struct stat st;
  int filesize;
  int i, fatal = FALSE;

  if (debug) LOG(LOG_DEBUG, "Processing %s", filename);

  if (stat(filename, &st)) {
    LOG(LOG_WARNING, "%s: %m", filename);
    return 0;
  }
  filesize = (int) st.st_size;
  if (filesize == 0) {
    unlink(filename);
    LOG(LOG_WARNING, "%s: size 0, removed", filename);
    return 0;
  }

  doc = xmlParseFile(filename);
  if (doc == NULL) {
    char cmd[2048];

    LOG(LOG_NOTICE, "%s: %m", filename);
    sprintf(cmd, "cat %s >> %s", filename, OPT_ARG(FAILURES));
    system(cmd);
    unlink(filename);
    return 0;
  }

  if (HAVE_OPT(COPY) && strcmp(OPT_ARG(COPY), "none")) {
    spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(COPY), doc, NULL);
  }

  cur = xmlDocGetRootElement(doc);
  if (cur == NULL) {
    LOG(LOG_NOTICE, "%s: empty document", filename);
    xmlFreeDoc(doc);
    unlink(filename);
    return 0;
  }
  ns = xmlSearchNsByHref(doc, cur, (const xmlChar *) NAMESPACE_URL);
  if (ns == NULL) {
    LOG(LOG_NOTICE, "%s: wrong type, result namespace not found", filename);
    xmlFreeDoc(doc);
    unlink(filename);
    return 0;
  }
  if (xmlStrcmp(cur->name, (const xmlChar *) "result")) {
    LOG(LOG_NOTICE, "%s: wrong type, root node is not 'result'", filename);
    xmlFreeDoc(doc);
    unlink(filename);
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
    LOG(LOG_NOTICE, "%s: wrong type, empty file", filename);
    if (fromhost) g_free(fromhost);
    xmlFreeDoc(doc);
    unlink(filename);
    return 0;
  }
  /* Second level is a list of probes, but be laxist */
  for (failures = 0; cur != NULL;) {
    int found = 0;

    if (xmlIsBlankNode(cur)) {
      cur = cur->next;
      continue;
    }
    for (i = 0; modules[i]; i++) {
      int ret;
      char buf[20];

      if (modules[i]->accept_probe) {
        if (!modules[i]->accept_probe(modules[i], cur->name)) continue;
      } else if (strcmp(cur->name, modules[i]->module_name)) {
        continue;
      } 

      if (cur->ns != ns) {
        LOG(LOG_ERR, "method found, but namespace incorrect on %s", cur->name);
        continue;
      }
      found = 1;
      probe_count++;
      //xmlDocFormatDump(stderr, doc, 1);
      strftime(buf, sizeof(buf), "%Y-%m-%d %T", gmtime(&fromdate));
      uw_setproctitle("%s %s@%s", buf, cur->name, fromhost);
      ret = process(modules[i], doc, cur, ns);
      if (ret == 0 || ret == -1) {
        failures++;
        cur = cur->next;
      } else if (ret == -2) {
        fatal = TRUE;
        break;
      } else {
        xmlNodePtr del = cur;
        cur = cur->next;
        xmlUnlinkNode(del); // succeeded, remove this node from the XML tree
        xmlFreeNode(del);
        modules[i]->count++;
      }
      break;
    }
    if (!found) {
      LOG(LOG_ERR, "can't find method: %s", cur->name);
      failures++;
      cur = cur->next;
    }
    if (fatal) break;
  }
  if (failures) {
    xmlSaveFormatFile(OPT_ARG(FAILURES), doc, 1);
  }
  xmlFreeDoc(doc);
  if (!fatal) {
    if (debug > 1) LOG(LOG_DEBUG, "Processed %d probes", probe_count);
    if (debug > 1) LOG(LOG_DEBUG, "unlink(%s)", filename);
    unlink(filename);
  }
  if (fromhost) g_free(fromhost);
  return fatal;
}

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

  mysql = open_database(OPT_ARG(DBHOST), OPT_ARG(DBNAME), OPT_ARG(DBUSER), OPT_ARG(DBPASSWD),
                        OPT_VALUE_DBCOMPRESS);
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
        if (debug > 2) fprintf(stderr, "%8u records processed\r", found);
      }
    }
    mysql_free_result(presult);

  }
  mysql_free_result(result);
  modules_end_run();
  close_database(mysql);
  return(found);
}
