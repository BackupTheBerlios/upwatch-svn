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
static int process(module *module, xmlDocPtr, xmlNodePtr, xmlNsPtr);

static void cleanup(void)
{
  int i;

  for (i = 0; modules[i]; i++) {
    if (modules[i]->cache) {
      g_hash_table_destroy(modules[i]->cache);
    }
  }
}

static void free_res(void *res)
{
  struct probe_result *r = (struct probe_result *)res;

  if (r->message) g_free(r->message);
  g_free(r);
}

int init(void)
{
  daemonize = TRUE;
  every = EVERY_5SECS /* ONE_SHOT */;
  g_thread_init(NULL);
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  atexit(cleanup);
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
int run(void)
{
  int count = 0;
  char path[PATH_MAX];
  G_CONST_RETURN gchar *filename;
  GDir *dir;
  GPtrArray *arr = g_ptr_array_new();
  int i;
  int files = 0;
  int got_fatal = 0;
extern int forever;

  if (debug > 3) LOG(LOG_DEBUG, "run()");
  sprintf(path, "%s/%s/new", OPT_ARG(SPOOLDIR), OPT_ARG(INPUT));
  dir = g_dir_open (path, 0, NULL);
  while ((filename = g_dir_read_name(dir)) != NULL) {
    char buffer[PATH_MAX];

    if (open_database() != 0) { // will do nothing if already open
      break;
    }
    if (filename[0] == '.') continue;  // skip hidden files
    sprintf(buffer, "%s/%s", path, filename);
    g_ptr_array_add(arr, strdup(buffer));
    files++;
  }
  g_dir_close(dir);
  if (files) {
    g_ptr_array_sort(arr, mystrcmp);
  }

  for (i=0; i < arr->len && forever; i++) {
    //printf("%s\n", g_ptr_array_index(arr,i));
    if (!got_fatal) {
      got_fatal = handle_file(g_ptr_array_index(arr,i), NULL);
    }
    free(g_ptr_array_index(arr,i));
    count++;
  }

  g_ptr_array_free(arr, TRUE);
  close_database();
  return(count);
}

static int handle_file(gpointer data, gpointer user_data)
{
  char *filename = (char *)data;
  xmlDocPtr doc; 
  xmlNsPtr ns;
  xmlNodePtr cur;
  int found=0, failures=0;
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
    LOG(LOG_NOTICE, "%s: %m", filename);
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
  /*
   * Now, walk the tree.
   */
  /* First level we expect just result */
  cur = cur->xmlChildrenNode;
  while (cur && xmlIsBlankNode(cur)) {
    cur = cur->next;
  }
  if (cur == 0) {
    LOG(LOG_NOTICE, "%s: wrong type, empty file'", filename);
    xmlFreeDoc(doc);
    unlink(filename);
    return 0;
  }
  /* Second level is a list of probes, but be laxist */
  for (failures = 0; cur != NULL;) {
    if (xmlIsBlankNode(cur)) {
      cur = cur->next;
      continue;
    }
    for (found = 0, i = 0; modules[i]; i++) {
      if (!xmlStrcmp(cur->name, (const xmlChar *) modules[i]->name)) {
        int ret;

	if (cur->ns != ns) {
          LOG(LOG_ERR, "method found, but namespace incorrect on %s", cur->name);
	  continue;
	}
        found = 1;
        probe_count++;
        //xmlDocFormatDump(stderr, doc, 1);
        ret = process(modules[i], doc, cur, ns);
        if (ret == 0) {
          failures++;
          cur = cur->next;
        } else if (ret == -1) {
          fatal = TRUE;
          break;
        } else {
          xmlNodePtr del = cur;
          cur = cur->next;
          xmlUnlinkNode(del); // succeeded, remove this node from the XML tree
          xmlFreeNode(del);
        }
        break;
      }
    }
    if (!found) {
      LOG(LOG_ERR, "can't find method: %s", cur->name);
      failures++;
    }
    if (fatal) break;
  }
  if (failures) {
    xmlSaveFormatFile(OPT_ARG(FAILURES), doc, 1);
  }
  //xmlDocFormatDump(stderr, doc, 1);
  xmlFreeDoc(doc);
  if (!fatal) {
    if (debug > 1) LOG(LOG_DEBUG, "Processed %d probes", probe_count);
    if (debug > 1) LOG(LOG_DEBUG, "unlink(%s)", filename);
    unlink(filename);
  }
  return fatal;
}

//*******************************************************************
// retrieve the definitions + status
// get it from the cache. if there but too old: delete
// in case of mysql-has-gone-away type errors, we keep on running, 
// it will be caught later-on.
//*******************************************************************
static void *get_def(module *probe, struct probe_result *res)
{
  struct probe_result *def;
  MYSQL_RES *result;
  MYSQL_ROW row;
  time_t now = time(NULL);

  def = g_hash_table_lookup(probe->cache, &res->probeid);
  if (def && def->stamp < now - 600) { // older then 10 minutes?
     g_hash_table_remove(probe->cache, &res->probeid);
     def = NULL;
  }
  // if not there retrieve from database and insert in hash
  if (def == NULL) {
    def = g_malloc0(sizeof(struct probe_result));
    def->stamp    = time(NULL);
    def->probeid  = res->probeid;

    result = my_query("select server, color, stattime "
                      "from   pr_status "
                      "where  class = '%u' and probe = '%u'", probe->class, def->probeid);
    if (result) {
      row = mysql_fetch_row(result);
      if (row) {
        if (row[0]) def->server   = atoi(row[0]);
        if (row[1]) def->color    = atoi(row[1]);
        if (row[2]) def->stattime = atoi(row[2]);
      } else {
        LOG(LOG_NOTICE, "pr_status record for %s id %u not found", probe->name, def->probeid);
      }
      mysql_free_result(result);
    }

    if (!def->server) { 
      // couldn't find pr_status record? Will be created later, 
      // but get the server from the def record for now
      result = my_query("select server "
                        "from   pr_%s_def "
                        "where  id = '%u'", probe->name, def->probeid);
      if (result) {
        row = mysql_fetch_row(result);
        if (row && row[0]) def->server   = atoi(row[0]);
        mysql_free_result(result);
      }
    }

    result = my_query("select stattime from pr_%s_raw use index(probtime) "
                      "where probe = '%u' order by stattime desc limit 1",
                       probe->name, def->probeid);
    if (result) {
      row = mysql_fetch_row(result);
      if (row && mysql_num_rows(result) > 0) {
        if (row[0]) def->stattime = atoi(row[1]);
      }
      mysql_free_result(result);
    }

    g_hash_table_insert(probe->cache, guintdup(def->probeid), def);
  }
  return(def);
}

//*******************************************************************
// RETRIEVE PRECEDING RAW RECORD FROM DATABASE (well just the color)
//*******************************************************************
static struct probe_result *get_previous_record(char *name, struct probe_result *def, struct probe_result *res)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct probe_result *prv;

  prv = g_malloc0(sizeof(struct probe_result));
  if (prv == NULL) {
    return(NULL);
  }
  prv->stattime = res->stattime;
  prv->color = res->color;

  result = my_query("select   color, stattime "
                    "from     pr_%s_raw use index(probtime) "
                    "where    probe = '%u' and stattime < '%u' "
                    "order by stattime desc limit 1", 
                    name, def->probeid, res->stattime);
  if (!result) return(prv);
  row = mysql_fetch_row(result);
  if (row) {
    prv->color = atoi(row[0]);
    prv->stattime = atoi(row[1]);
  }
  mysql_free_result(result);
  return(prv);
}

//*******************************************************************
// RETRIEVE FOLLOWING RAW RECORD FROM DATABASE (well just the color)
//*******************************************************************
static struct probe_result *get_following_record(char *name, struct probe_result *def, struct probe_result *res)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct probe_result *nxt;

  nxt = g_malloc0(sizeof(struct probe_result));
  if (nxt == NULL) {
    return(NULL);
  }
  nxt->stattime = res->stattime;
  nxt->color = res->color;

  result = my_query("select   color, stattime "
                    "from     pr_%s_raw use index(probtime) "
                    "where    probe = '%u' and stattime < '%u' "
                    "order by stattime desc limit 1", 
                    name, def->probeid, res->stattime);
  if (!result) return(nxt);
  row = mysql_fetch_row(result);
  if (row) {
    nxt->color = atoi(row[0]);
    nxt->stattime = atoi(row[1]);
  }
  mysql_free_result(result);
  return(nxt);
}

//*******************************************************************
// UPDATE PR_STATUS
//*******************************************************************
static void update_pr_status(int class, struct probe_result *def, struct probe_result *res)
{
  MYSQL_RES *result;

  result = my_query("update pr_status "
                    "set    stattime = '%u', expires = '%u', color = '%d', message = '%s' "
                    "where  probe = '%u' and class = '%u'",
                    res->stattime, res->expires, res->color, 
                    res->message ? res->message : "", def->probeid, class);
  mysql_free_result(result);
  if (mysql_affected_rows(mysql) == 0) { // nothing was actually updated, it was probably already there
    LOG(LOG_NOTICE, "update_pr_status failed, inserting new record (class=%u, probe=%u)", class, def->probeid);
    result = my_query("insert into pr_status "
                      "set    class =  '%d', probe = '%u', stattime = '%u', expires = '%u', "
                      "       server = '%u', color = '%d', message = '%s'",
                      class, def->probeid, res->stattime, res->expires, def->server, res->color, 
                      res->message ? res->message : "");
    mysql_free_result(result);
  }
}

//*******************************************************************
// CREATE PR_STATUS RECORD
//*******************************************************************
static void insert_pr_status(int class, struct probe_result *def, struct probe_result *res)
{
  MYSQL_RES *result;

  result = my_query("insert into pr_status "
                    "set    class =  '%d', probe = '%u', stattime = '%u', expires = '%u', "
                    "       server = '%u', color = '%d', msg = '%s'",
                    class, def->probeid, res->stattime, res->expires, def->server, res->color, 
                    res->message ? res->message : "");
  mysql_free_result(result);
  if (mysql_affected_rows(mysql) == 0) { // nothing was actually inserted, it was probably already there
    LOG(LOG_NOTICE, "insert_pr_status failed, updating current record (class=%u, probe=%u)", class, def->probeid);
    result = my_query("update pr_status "
                      "set    stattime = '%u', expires = '%u', color = '%d', message = '%s' "
                      "where  probe = '%u' and class = '%u'",
                      res->stattime, res->expires, res->color,
                      res->message ? res->message : "", def->probeid, class);
    mysql_free_result(result);
  }
}

//*******************************************************************
// CREATE PR_HIST
//*******************************************************************
static void create_pr_hist(int class, struct probe_result *def, struct probe_result *res, struct probe_result *prv)
{
  MYSQL_RES *result;

  result = my_query("insert into pr_hist "
                    "set    server = '%u', class = '%u', probe = '%u', stattime = '%u', "
                    "       prv_color = '%d', color = '%d', message = '%s'",
                    def->server, class, def->probeid, res->stattime,
                    prv->color, res->color, res->message ? res->message : "");
  mysql_free_result(result);
}

//*******************************************************************
// REMOVE ALL OCCURRENCES OF A PROBE IN THE HISTORY FILES
//*******************************************************************
static void delete_history(int class, struct probe_result *def, struct probe_result *nxt)
{
  MYSQL_RES *result;

  result = my_query("delete from pr_hist "
                    "where stattime = '%u' and probe = '%u' and class = '%d'",
                    nxt->stattime, def->probeid, class);
  mysql_free_result(result);
  result = my_query("delete from pr_status "
                    "where stattime = '%u' and probe = '%u' and class = '%d'",
                    nxt->stattime, def->probeid, class);
  mysql_free_result(result);
}

//*******************************************************************
// UPDATE SERVER COLOR
//*******************************************************************
static void update_server_color(struct probe_result *def, struct probe_result *res)
{
  MYSQL_RES *result;

  result = my_query("update %s set %s = '%u' where %s = '%u'",
                     OPT_ARG(SERVER_TABLE_NAME), OPT_ARG(SERVER_TABLE_COLOR_FIELD), 
                     res->color, OPT_ARG(SERVER_TABLE_ID_FIELD), def->server);
  mysql_free_result(result);
}

//*******************************************************************
// HAS THE LAST RECORD FOR THIS SLOT BEEN SEEN?
//*******************************************************************
int have_records_later_than(char *name, guint probe, char *from, guint slothigh)
{
  MYSQL_RES *result;
  int val = FALSE;

  result = my_query("select id from pr_%s_%s use index(probtime) where probe = '%u' and stattime > '%u' limit 1",
                    name, from, probe, slothigh);
  if (!result) return(FALSE);
  if (mysql_num_rows(result) > 0) {
    val = TRUE;
  }
  mysql_free_result(result);
  return(val);
}

static struct summ_spec {
  int period;
  char *from, *to;
} summ_info[] = {
  { SLOT_DAY,   "raw",   "day"   },
  { SLOT_WEEK,  "day",   "week"  },
  { SLOT_MONTH, "week",  "month" },
  { SLOT_YEAR,  "month", "year"  },
  { SLOT_YEAR5, "year",  "5year" },
  { 0,          NULL,    NULL    }
};

/*
 * PSEUDO CODE:
 *
 * EXTRACT INFO FROM XML NODE
 * RETRIEVE PROBE DEFINITION RECORD FROM DATABASE
 * RETRIEVE PRECEDING RAW RECORD FROM DATABASE
 * STORE RAW RESULTS
 * IF THIS IS THE FIRST RESULT EVER FOR THIS PROBE
 *   CREATE PR_STATUS RECORD
 * ELSE
 *   IF WE HAVEN'T PROCESSED THIS RECORD BEFORE
 *     IF COLOR DIFFERS FROM PRECEDING RAW RECORD
 *       CREATE PR_HIST
 *       RETRIEVE FOLLOWING RAW RECORD
 *       IF FOUND AND COLOR OF FOLLOWING IS THE SAME AS CURRENT
 *         DELETE POSSIBLE HISTORY RECORDS FOR FOLLOWING RECORD
 *       ENDIF
 *       IF THIS RAW RECORD IS THE MOST RECENT EVER RECEIVED
 *         WRITE NEW RECENT TIME INTO PROBE DEFINITION RECORD
 *         UPDATE PR_STATUS
 *         UPDATE SERVER COLOR
 *       ENDIF
 *     ELSE
 *       IF THIS RAW RECORD IS THE MOST RECENT EVER RECEIVED
 *         WRITE NEW RECENT TIME INTO PROBE DEFINITION RECORD
 *       ENDIF
 *     ENDIF
 *     IF CURRENT RAW RECORD IS THE MOST RECENT
 *       FOR EACH PERIOD
 *         IF WE ENTERED A NEW SLOT
 *           SUMMARIZE PREVIOUS SLOT
 *         ENDIF
 *       ENDFOR
 *     ELSE
 *       FOR EACH PERIOD
 *         IF THE LAST RECORD FOR THIS SLOT HAS BEEN SEEN
 *           RE-SUMMARIZE CURRENT SLOT
 *         ENDIF
 *       ENDFOR
 *     ENDIF
 *   ENDIF
 * ENDIF
 *
 */
static int process(module *probe, xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns)
{
  int seen_before=0, must_update_def=0;
  struct probe_result *def=NULL;
  struct probe_result *res=NULL, *prv=NULL;

  if (open_database() != 0) {
    return -1; // fatal error
  }

  if (!probe->cache) {
    probe->cache = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, 
            probe->free_def? probe->free_def : g_free);
  }

  res = probe->extract_from_xml(doc, cur, ns); // EXTRACT INFO FROM XML NODE
  if (!res) return 1;
  if (probe->get_def) {
    def = probe->get_def(probe, res); // RETRIEVE PROBE DEFINITION RECORD FROM DATABASE
  } else {
    def = get_def(probe, res);
  }
  if (!def) {  // Oops, def record not found. Skip this probe
    if (probe->free_res) {
      if (res) probe->free_res(res);
    } else {
      if (res) g_free(res);
    }
    return 1;  
  }

  if (probe->store_results) {
    seen_before = probe->store_results(probe, def, res); // STORE RAW RESULTS
  } else {
    seen_before = FALSE;
  }

  if (res->stattime > def->stattime) { // IF CURRENT RAW RECORD IS THE MOST RECENT 
    prv = g_malloc0(sizeof(struct probe_result));
    prv->color = def->color;  // USE PREVIOUS COLOR FROM DEF RECORD
    prv->stattime = def->stattime;
  } else {
    prv = get_previous_record(probe->name, def, res); // RETRIEVE PRECEDING RAW RECORD FROM DATABASE
  }

  if (def->stattime == 0) { // IF THIS IS THE FIRST RESULT EVER FOR THIS PROBE
    insert_pr_status(probe->class, def, res);
    must_update_def = TRUE;
  } else {
    if (!seen_before) {
      // IF COLOR DIFFERS FROM PRECEDING RAW RECORD
      if (res->color != prv->color) {
        struct probe_result *nxt;
        create_pr_hist(probe->class, def, res, prv); // CREATE PR_HIST
        nxt = get_following_record(probe->name, def, res); // RETRIEVE FOLLOWING RAW RECORD
        if (nxt && nxt->color) { // IF FOUND
          if (nxt->color == res->color) {  // IF COLOR OF FOLLOWING IS THE SAME AS CURRENT
            delete_history(probe->class, def, nxt); // DELETE POSSIBLE HISTORY RECORDS FOR FOLLOWING RECORD
          }
          g_free(nxt);
        }
        if (res->stattime > def->stattime) { // IF THIS RAW RECORD IS THE MOST RECENT EVER RECEIVED
          update_pr_status(probe->class, def, res);  // UPDATE PR_STATUS
          update_server_color(def, res); // UPDATE SERVER COLOR
          must_update_def = TRUE;
        }
      } else {
        if (res->stattime > def->stattime) { // IF THIS RAW RECORD IS THE MOST RECENT EVER RECEIVED
          must_update_def = TRUE;
        }
      }
      if (probe->summarize) { // if we have a summarisation function
        if (res->stattime > def->stattime) { // IF CURRENT RAW RECORD IS THE MOST RECENT
          guint cur_slot, prev_slot;
          gulong slotlow, slothigh;
          gulong dummy_low, dummy_high;
          gint i;

          for (i=0; summ_info[i].period > 0; i++) { // FOR EACH PERIOD
            prev_slot = uw_slot(summ_info[i].period, prv->stattime, &slotlow, &slothigh);
            cur_slot = uw_slot(summ_info[i].period, res->stattime, &dummy_low, &dummy_high);
            if (cur_slot != prev_slot) { // IF WE ENTERED A NEW SLOT, SUMMARIZE PREVIOUS SLOT
              probe->summarize(def, res, summ_info[i].from, summ_info[i].to, slotlow, slothigh);
            }
          }
        } else {
          guint cur_slot;
          gulong slotlow, slothigh;
          gulong not_later_then = UINT_MAX;
          gint i;

          for (i=0; summ_info[i].period > 0; i++) { // FOR EACH PERIOD
            cur_slot = uw_slot(summ_info[i].period, res->stattime, &slotlow, &slothigh);
            if (slothigh > not_later_then) continue; // we already know there are no records later then this
            // IF THE LAST RECORD FOR THIS SLOT HAS BEEN SEEN
            if (have_records_later_than(probe->name, def->probeid, summ_info[i].from, slothigh)) { 
              // RE-SUMMARIZE CURRENT SLOT
              probe->summarize(def, res, summ_info[i].from, summ_info[i].to, slotlow, slothigh);
            } else {
              not_later_then = slothigh;
            }
          }
        }
      }
    }
  }
  if (must_update_def) {
    def->stattime = res->stattime;
    def->color = res->color;
  }
  g_free(prv);
  if (probe->free_res) {
    if (res) probe->free_res(res);
  } else {
    if (res) free_res(res);
  }
  // note we don't free the *def here, because that structure is owned by the hashtable
  return 1;
}

