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
extern  int process(module *module, trx *t);
extern struct summ_spec summ_info[]; 

struct resfile {
  char *filename;
  char *fromhost;
  time_t fromdate;
  int count; // number of results in this file
};
GStaticRecMutex resfile_mutex = G_STATIC_REC_MUTEX_INIT;
GPtrArray *resfile_arr;

static void resfile_free(void *p)
{
  struct resfile *rf = (struct resfile *)p;

  if (rf->filename) g_free(rf->filename);
  if (rf->fromhost) g_free(rf->fromhost);
  g_free(rf);
}

void resfile_remove(struct resfile *rf, int ondisk)
{
  g_static_rec_mutex_lock (&resfile_mutex);
  if (ondisk) unlink(rf->filename);
  g_ptr_array_remove(resfile_arr, rf);
  resfile_free(rf);
  g_static_rec_mutex_lock (&resfile_mutex);
}

void resfile_incr(struct resfile *rf)
{
  g_static_rec_mutex_lock (&resfile_mutex);
  rf->count++;
  g_static_rec_mutex_unlock (&resfile_mutex);
}

void resfile_decr(struct resfile *rf)
{
  g_static_rec_mutex_lock (&resfile_mutex);
  rf->count--;
  if (rf->count < 1) {
    resfile_remove(rf, TRUE);
  }
  g_static_rec_mutex_unlock (&resfile_mutex);
}

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
    if (modules[i]->queue) {
      g_queue_free(modules[i]->queue);
    }
  }
}

static void modules_start_run(void)
{
  int i;

  for (i = 0; modules[i]; i++) {
    modules[i]->db = open_database(OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME),
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
                            modules[i]->free_def ? modules[i]->free_def : g_free);
    }
    if (modules[i]->queue == NULL) {
      modules[i]->queue = g_queue_new();
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
#ifdef G_THREADS_ENABLED
  //g_thread_init(NULL);
#endif
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  resfile_arr = g_ptr_array_new();
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
  int failures = 0;
extern int forever;
static int resummarize(void);

  if (debug > 3) LOG(LOG_DEBUG, "run()");

  if (HAVE_OPT(SUMMARIZE)) {
    return(resummarize()); // --summarize
  }
  sprintf(path, "%s/%s/new", OPT_ARG(SPOOLDIR), OPT_ARG(INPUT));
  uw_setproctitle("listing %s", path);
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

  // now we have a sorted list of files 
  // walk the list, and add resfile descriptions for files we aren't processing yet
  // 
  for (i=0; i < arr->len && forever; i++) {
    struct resfile *rf;
    int j, found = 0;

    for (j=0; j < resfile_arr->len; j++) { // are we already working on this file?
      rf = g_ptr_array_index(resfile_arr, j);
      if (!strcmp(rf->filename, g_ptr_array_index(arr,i))) {
        found = 1; // file already in the list -> already working on it
        break;
      }
    }
    if (found || resfile_arr->len >= 100) {
      free(g_ptr_array_index(arr,i));
      continue;
    }
    rf = g_malloc0(sizeof(struct resfile));
    rf->filename = g_ptr_array_index(arr,i);
    uw_setproctitle("reading %s", rf->filename);
    g_ptr_array_add(resfile_arr, rf); 
    handle_file(rf, NULL); // extract probe results and queue them
    count++;
  }
  g_ptr_array_free(arr, TRUE);
  if (debug) LOG(LOG_DEBUG, "processing %u file entries", resfile_arr->len);

  // Now we have the queues updated, process all results
  //
  for (i = 0; modules[i]; i++) {
    unsigned count = 0;
    char buf[20];

    modules[i]->db = open_database(OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME),
                                     OPT_ARG(DBUSER), OPT_ARG(DBPASSWD),
                                     OPT_VALUE_DBCOMPRESS);
    if (modules[i]->start_run) {
      modules[i]->start_run();
    }
    buf[0] = 0;
    while (forever) {
      trx *t;
      int ret;

      g_static_mutex_lock (&modules[i]->queue_mutex);
      t = g_queue_pop_head(modules[i]->queue);
      g_static_mutex_unlock (&modules[i]->queue_mutex);
      if (t == NULL) break;

      if (buf[0] == 0 || count % 100 == 0) {
        strftime(buf, sizeof(buf), "%Y-%m-%d %T", gmtime(&t->rf->fromdate));
        uw_setproctitle("%s %s@%s", buf, t->res->name, t->rf->fromhost);
      }
      ret = process(modules[i], t);
      if (ret == 0 || ret == -1) { // error in processing this probe
        failures++; // should log this somewhere
      } else if (ret == -2) {
        break; // fatal database error
      } else {
        count++;
      }
      resfile_decr(t->rf);
      g_free(t);
    }
    if (modules[i]->end_run) { 
      modules[i]->end_run();
    }
    close_database(modules[i]->db);
    modules[i]->db = NULL;
    if (debug && count) {
      LOG(LOG_DEBUG, "Processed: %s:%u", modules[i]->module_name, count);
    }
  }

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
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "host")) && (cur->ns == ns)) {
      xmlNodePtr hname;

      for (hname = cur->xmlChildrenNode; hname != NULL; hname = hname->next) {
        if (xmlIsBlankNode(hname)) continue;
        if ((!xmlStrcmp(hname->name, (const xmlChar *) "hostname")) && (hname->ns == ns)) {
          p = xmlNodeListGetString(doc, hname->xmlChildrenNode, 1);
          if (p) {
            res->hostname = strdup(p);
            xmlFree(p);
          }
          continue;
        }
        if ((!xmlStrcmp(hname->name, (const xmlChar *) "ipaddress")) && (hname->ns == ns)) {
          p = xmlNodeListGetString(doc, hname->xmlChildrenNode, 1);
          if (p) {
            res->ipaddress = strdup(p);
            xmlFree(p);
          }
          continue;
        }
      }
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
  struct resfile *rf = (struct resfile *)data;
  xmlDocPtr doc; 
  xmlNsPtr ns;
  xmlNodePtr cur;
  char *p;
  int failures=0;
  struct stat st;
  int filesize;
  int i;

  if (debug) LOG(LOG_DEBUG, "Processing %s", rf->filename);

  if (stat(rf->filename, &st)) {
    LOG(LOG_WARNING, "%s: %m", rf->filename);
    resfile_remove(rf, FALSE);
    return 0;
  }
  filesize = (int) st.st_size;
  if (filesize == 0) {
    LOG(LOG_WARNING, "%s: size 0, removed", rf->filename);
    resfile_remove(rf, TRUE);
    return 0;
  }

  doc = xmlParseFile(rf->filename);
  if (doc == NULL) {
    char cmd[2048];

    LOG(LOG_NOTICE, "%s: %m", rf->filename);
    sprintf(cmd, "cat %s >> %s", rf->filename, OPT_ARG(FAILURES));
    system(cmd);
    resfile_remove(rf, TRUE);
    return 0;
  }
  if (HAVE_OPT(COPY) && strcmp(OPT_ARG(COPY), "none")) {
    spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(COPY), doc, NULL);
  }

  cur = xmlDocGetRootElement(doc);
  if (cur == NULL) {
    LOG(LOG_NOTICE, "%s: empty document", rf->filename);
    resfile_remove(rf, TRUE);
    xmlFreeDoc(doc);
    return 0;
  }
  ns = xmlSearchNsByHref(doc, cur, (const xmlChar *) NAMESPACE_URL);
  if (ns == NULL) {
    LOG(LOG_NOTICE, "%s: wrong type, result namespace not found", rf->filename);
    resfile_remove(rf, TRUE);
    xmlFreeDoc(doc);
    return 0;
  }
  if (xmlStrcmp(cur->name, (const xmlChar *) "result")) {
    LOG(LOG_NOTICE, "%s: wrong type, root node is not 'result'", rf->filename);
    resfile_remove(rf, TRUE);
    xmlFreeDoc(doc);
    return 0;
  }
  p = xmlGetProp(cur, (const xmlChar *) "fromhost");
  if (p) {
    rf->fromhost = strdup(p);
    xmlFree(p);
  }
  rf->fromdate = (time_t) xmlGetPropUnsigned(cur, (const xmlChar *) "date");

  /*
   * Now, walk the tree.
   */
  /* First level we expect just result */
  cur = cur->xmlChildrenNode;
  while (cur && xmlIsBlankNode(cur)) {
    cur = cur->next;
  }
  if (cur == 0) {
    LOG(LOG_NOTICE, "%s: empty file", rf->filename);
    resfile_remove(rf, TRUE);
    xmlFreeDoc(doc);
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
      trx *t;
      xmlNodePtr del = cur;

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
      resfile_incr(rf);
      t = (trx *)g_malloc0(sizeof(trx));
      t->rf = rf;
      t->res = extract_info_from_xml(modules[i], doc, cur, ns);
      g_static_mutex_lock (&modules[i]->queue_mutex);
      g_queue_push_tail(modules[i]->queue, t);
      g_static_mutex_unlock (&modules[i]->queue_mutex);
      cur = cur->next;
      xmlUnlinkNode(del); // succeeded, remove this node from the XML tree
      xmlFreeNode(del);
      break;
    }
    if (!found) {
      LOG(LOG_ERR, "can't find method: %s", cur->name);
      failures++;
      cur = cur->next;
    }
  }
  if (failures) {
    xmlSaveFormatFile(OPT_ARG(FAILURES), doc, 1);
  }
  xmlFreeDoc(doc);
  return 0;
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
