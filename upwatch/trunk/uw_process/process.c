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

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct summ_spec summ_info[] = {
  { SLOT_DAY,   "raw",   "day"   },
  { SLOT_WEEK,  "day",   "week"  },
  { SLOT_MONTH, "week",  "month" },
  { SLOT_YEAR,  "month", "year"  },
  { SLOT_YEAR5, "year",  "5year" },
  { 0,          NULL,    NULL    }
};

static void free_res(void *res)
{
  struct probe_result *r = (struct probe_result *)res;

  if (r->name) g_free(r->name);
  if (r->message) g_free(r->message);
  if (r->hostname) g_free(r->hostname);
  if (r->ipaddress) g_free(r->ipaddress);
  g_free(r);
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

//*******************************************************************
// retrieve the definitions + status
// get it from the cache. if there but too old: delete
// in case of mysql-has-gone-away type errors, we keep on running, 
// it will be caught later-on.
//*******************************************************************
static void *get_def(module *probe, struct probe_result *res)
{
  struct probe_def *def;
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

    result = my_query(probe->db, 0,
                      "select server, color, stattime, yellow, red "
                      "from   pr_status "
                      "where  class = '%u' and probe = '%u'", probe->class, def->probeid);
    if (result) {
      row = mysql_fetch_row(result);
      if (row) {
        if (row[0]) def->server = atoi(row[0]);
        if (row[1]) def->color  = atoi(row[1]);
        if (row[2]) def->newest = atoi(row[2]);
        if (row[3]) def->yellow = atof(row[3]);
        if (row[4]) def->red    = atof(row[4]);
      } else {
        LOG(LOG_NOTICE, "pr_status record for %s id %u not found", res->name, def->probeid);
      }
      mysql_free_result(result);
    }

    if (!def->server) { 
      // couldn't find pr_status record? Will be created later, 
      // but get the server and yellow/red info from the def record for now
      result = my_query(probe->db, 0,
                        "select server, yellow, red "
                        "from   pr_%s_def "
                        "where  id = '%u'", res->name, def->probeid);
      if (!result) return(NULL);

      if (mysql_num_rows(result) == 0) { // DEF RECORD NOT FOUND
        mysql_free_result(result);
        if (!trust(res->name)) {
          LOG(LOG_NOTICE, "pr_%s_def id %u not found and not trusted - skipped",
                           res->name, def->probeid);
          return(NULL);
        }
        // at this point, we have a probe result, but we can't find the _def record
        // for it. We apparantly trust this result, so we can create the definition
        // ourselves. For that we need to fill in the server id and the ipaddress
        // and we look into the result if the is anything useful in there.
/*
        result = my_query(probe->db, 0,
                          "insert into pr_%s_def set server = '%d', "
                          "        ipaddress = '%s', description = 'auto-added by system'",
                          res->name, res->server, res->ipaddress);
        mysql_free_result(result);
        if (mysql_affected_rows(mysql) == 0) { // nothing was actually inserted
          LOG(LOG_NOTICE, "insert missing pr_%s_def id %u: %s", 
                           res->name, def->probeid, mysql_error(mysql));
        }
        result = my_query(probe->db, 0,
                          "select id, yellow, red "
                          "from   pr_%s_def "
                          "where  server = '%u'", res->name, res->server);
        if (!result) return(NULL);
*/
      }
      row = mysql_fetch_row(result);
      if (row) {
        if (row[0]) def->server   = atoi(row[0]);
        if (row[1]) def->yellow   = atof(row[1]);
        if (row[2]) def->red      = atof(row[2]);
      }
      mysql_free_result(result);
    }

    result = my_query(probe->db, 0,
                      "select stattime from pr_%s_raw use index(probstat) "
                      "where probe = '%u' order by stattime desc limit 1",
                       res->name, def->probeid);
    if (result) {
      row = mysql_fetch_row(result);
      if (row && mysql_num_rows(result) > 0) {
        if (row[0]) def->newest = atoi(row[0]);
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
static struct probe_result *get_previous_record(module *probe, struct probe_def *def, struct probe_result *res)
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

  if (!probe->store_results) return(prv); // we probably don't have a xxxx_raw table

  result = my_query(probe->db, 0,
                    "select   color, stattime "
                    "from     pr_%s_raw use index(probstat) "
                    "where    probe = '%u' and stattime < '%u' "
                    "order by stattime desc limit 1", 
                    res->name, def->probeid, res->stattime);
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
static struct probe_result *get_following_record(module *probe, struct probe_def *def, struct probe_result *res)
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

  if (!probe->store_results) return(nxt); // we probably don't have a xxxx_raw table

  result = my_query(probe->db, 0,
                    "select   color, stattime "
                    "from     pr_%s_raw use index(probstat) "
                    "where    probe = '%u' and stattime < '%u' "
                    "order by stattime desc limit 1", 
                    res->name, def->probeid, res->stattime);
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
static void update_pr_status(module *probe, struct probe_def *def, struct probe_result *res)
{
  MYSQL_RES *result;
  char *escmsg = strdup("");

  if (res->message) {
    escmsg = g_malloc(strlen(res->message) * 2 + 1);
    mysql_real_escape_string(probe->db, escmsg, res->message, strlen(res->message)) ;
  }

  result = my_query(probe->db, 0,
                    "update pr_status "
                    "set    stattime = '%u', expires = '%u', color = '%d', " 
                    "       message = '%s', yellow = '%f', red = '%f' "
                    "where  probe = '%u' and class = '%u'",
                    res->stattime, res->expires, res->color, 
                    escmsg, def->yellow, def->red, def->probeid, probe->class);
  mysql_free_result(result);
  if (mysql_affected_rows(probe->db) == 0) { // nothing was actually updated, probably already there
    LOG(LOG_NOTICE, "update_pr_status failed, inserting new record (class=%u, probe=%u)", 
                    probe->class, def->probeid);
    result = my_query(probe->db, 0,
                      "insert into pr_status "
                      "set    class =  '%u', probe = '%u', stattime = '%u', expires = '%u', "
                      "       server = '%u', color = '%u', message = '%s', yellow = '%f', red = '%f'",
                      probe->class, def->probeid, res->stattime, res->expires, def->server, res->color, 
                      escmsg, def->yellow, def->red);
    mysql_free_result(result);
  }
  g_free(escmsg);
}

//*******************************************************************
// CREATE PR_STATUS RECORD
//*******************************************************************
static void insert_pr_status(module *probe, struct probe_def *def, struct probe_result *res)
{
  MYSQL_RES *result;
  char *escmsg = strdup("");

  if (res->message) {
    escmsg = g_malloc(strlen(res->message) * 2 + 1);
    mysql_real_escape_string(probe->db, escmsg, res->message, strlen(res->message)) ;
  }

  result = my_query(probe->db, 0,
                    "insert into pr_status "
                    "set    class =  '%u', probe = '%u', stattime = '%u', expires = '%u', "
                    "       color = '%u', server = '%u', message = '%s', yellow = '%f', red = '%f'",
                    probe->class, def->probeid, res->stattime, res->expires, def->color, res->server, 
                    escmsg, def->yellow, def->red);
  mysql_free_result(result);
  if (mysql_affected_rows(probe->db) == 0 || (mysql_errno(probe->db) == ER_DUP_ENTRY)) { 
    // nothing was actually inserted, it was probably already there
    LOG(LOG_NOTICE, "insert_pr_status failed, updating current record (class=%u, probe=%u)", 
                    probe->class, def->probeid);
    result = my_query(probe->db, 0,
                      "update pr_status "
                      "set    stattime = '%u', expires = '%u', color = '%d', "
                      "       message = '%s', yellow = '%f', red = '%f' "
                      "where  probe = '%u' and class = '%u'",
                      res->stattime, res->expires, res->color, escmsg, 
                      def->yellow, def->red, def->probeid, probe->class);
    mysql_free_result(result);
  }
  g_free(escmsg);
}

//*******************************************************************
// CREATE PR_HIST
//*******************************************************************
static void create_pr_hist(module *probe, struct probe_def *def, struct probe_result *res, struct probe_result *prv)
{
  MYSQL_RES *result;
  char *escmsg = strdup("");

  if (res->message) {
    escmsg = g_malloc(strlen(res->message) * 2 + 1);
    mysql_real_escape_string(probe->db, escmsg, res->message, strlen(res->message)) ;
  }

  result = my_query(probe->db, 0,
                    "insert into pr_hist "
                    "set    server = '%u', class = '%u', probe = '%u', stattime = '%u', "
                    "       prv_color = '%d', color = '%d', message = '%s'",
                    def->server, probe->class, def->probeid, res->stattime,
                    prv->color, res->color, escmsg);
  mysql_free_result(result);
  g_free(escmsg);
}

//*******************************************************************
// REMOVE ALL OCCURRENCES OF A PROBE IN THE HISTORY FILES
//*******************************************************************
static void delete_history(module *probe, struct probe_def *def, struct probe_result *nxt)
{
  MYSQL_RES *result;

  result = my_query(probe->db, 0,
                    "delete from pr_hist "
                    "where stattime = '%u' and probe = '%u' and class = '%d'",
                    nxt->stattime, def->probeid, probe->class);
  mysql_free_result(result);
  result = my_query(probe->db, 0,
                    "delete from pr_status "
                    "where stattime = '%u' and probe = '%u' and class = '%d'",
                    nxt->stattime, def->probeid, probe->class);
  mysql_free_result(result);
}

//*******************************************************************
// UPDATE SERVER COLOR
//*******************************************************************
static void update_server_color(module *probe, struct probe_def *def, struct probe_result *res)
{
  MYSQL_RES *result;

  result = my_query(probe->db, 0,
                    "update %s set %s = '%u' where %s = '%u'",
                     OPT_ARG(SERVER_TABLE_NAME), OPT_ARG(SERVER_TABLE_COLOR_FIELD), 
                     res->color, OPT_ARG(SERVER_TABLE_ID_FIELD), def->server);
  mysql_free_result(result);
}

//*******************************************************************
// HAS THE LAST RECORD FOR THIS SLOT BEEN SEEN?
//*******************************************************************
int have_records_later_than(module *probe, char *name, guint probeid, char *from, guint slothigh)
{
  MYSQL_RES *result;
  int val = FALSE;

  result = my_query(probe->db, 0,
                    "select id from pr_%s_%s use index(probstat) "
                    "where  probe = '%u' and stattime > '%u' limit 1",
                    name, from, probeid, slothigh);
  if (!result) return(FALSE);
  if (mysql_num_rows(result) > 0) {
    val = TRUE;
  }
  mysql_free_result(result);
  return(val);
}

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
int process(module *probe, xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns)
{
  int seen_before=0, must_update_def=0;
  struct probe_def *def=NULL;
  struct probe_result *res=NULL, *prv=NULL;

  res = extract_info_from_xml(probe, doc, cur, ns); // EXTRACT INFO FROM XML NODE
  if (!res) return 1;

  if (probe->fix_result) {
    probe->fix_result(probe, res); // do some final calculations on the result
  }

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

  if (res->stattime > def->newest) { // IF CURRENT RAW RECORD IS THE MOST RECENT 
    prv = g_malloc0(sizeof(struct probe_result));
    prv->color = def->color;  // USE PREVIOUS COLOR FROM DEF RECORD
    prv->stattime = def->newest;
  } else {
    prv = get_previous_record(probe, def, res); // RETRIEVE PRECEDING RAW RECORD FROM DATABASE
  }

  if (def->newest == 0) { // IF THIS IS THE FIRST RESULT EVER FOR THIS PROBE
    insert_pr_status(probe, def, res);
    must_update_def = TRUE;
  } else {
    if (!seen_before) {
      // IF COLOR DIFFERS FROM PRECEDING RAW RECORD
      if (res->color != prv->color) {
        struct probe_result *nxt;
        create_pr_hist(probe, def, res, prv); // CREATE PR_HIST
        nxt = get_following_record(probe, def, res); // RETRIEVE FOLLOWING RAW RECORD
        if (nxt && nxt->color) { // IF FOUND
          if (nxt->color == res->color) {  // IF COLOR OF FOLLOWING IS THE SAME AS CURRENT
            delete_history(probe, def, nxt); // DELETE POSSIBLE HISTORY RECORDS FOR FOLLOWING RECORD
          }
          g_free(nxt);
        }
        if (res->stattime > def->newest) { // IF THIS RAW RECORD IS THE MOST RECENT EVER RECEIVED
          update_pr_status(probe, def, res);  // UPDATE PR_STATUS
          update_server_color(probe, def, res); // UPDATE SERVER COLOR
          must_update_def = TRUE;
        }
      } else {
        if (res->stattime > def->newest) { // IF THIS RAW RECORD IS THE MOST RECENT EVER RECEIVED
          must_update_def = TRUE;
        }
      }
      if (probe->summarize) { // if we have a summarisation function
        if (res->stattime > def->newest) { // IF CURRENT RAW RECORD IS THE MOST RECENT
          guint cur_slot, prev_slot;
          gulong slotlow, slothigh;
          gulong dummy_low, dummy_high;
          gint i;

          for (i=0; summ_info[i].period > 0; i++) { // FOR EACH PERIOD
            prev_slot = uw_slot(summ_info[i].period, prv->stattime, &slotlow, &slothigh);
            cur_slot = uw_slot(summ_info[i].period, res->stattime, &dummy_low, &dummy_high);
            if (cur_slot != prev_slot) { // IF WE ENTERED A NEW SLOT, SUMMARIZE PREVIOUS SLOT
              //if (strcmp(res->name, "sysstat") == 0) {
              //  LOG(LOG_DEBUG, "cur(%u for %u) != prv(%u for %u), summarizing %s from %u to %u",
              //                 cur_slot, res->stattime, prev_slot, prv->stattime,
              //                 summ_info[i].from, slotlow, slothigh);
              //}
              probe->summarize(probe, def, res, summ_info[i].from, summ_info[i].to, 
                               cur_slot, slotlow, slothigh, 0);
            }
          }
        } else {
          guint cur_slot;
          gulong slotlow, slothigh;
          gulong not_later_then = UINT_MAX;
          gint i;

          if (debug > 2) {
            LOG(LOG_DEBUG, "stattime = %u, newest = %u for %s %u", res->stattime, def->newest,
              res->name, res->probeid);
          }
          for (i=0; summ_info[i].period > 0; i++) { // FOR EACH PERIOD
            cur_slot = uw_slot(summ_info[i].period, res->stattime, &slotlow, &slothigh);
            if (slothigh > not_later_then) continue; // we already know there are none later then this
            // IF THE LAST RECORD FOR THIS SLOT HAS BEEN SEEN
            if (have_records_later_than(probe, res->name, def->probeid, summ_info[i].from, slothigh)) { 
              // RE-SUMMARIZE CURRENT SLOT
              probe->summarize(probe, def, res, summ_info[i].from, summ_info[i].to, 
                               cur_slot, slotlow, slothigh, 0);
            } else {
              not_later_then = slothigh;
            }
          }
        }
      }
    }
  }
  if (must_update_def) {
    def->newest = res->stattime;
    def->color = res->color;
  }
  g_free(prv);

  // free the result block
  if (res) {
    if (probe->free_res) {
      probe->free_res(res); // the probe specific part...
    }
    free_res(res); // .. and the generic part
  }

  // note we don't free the *def here, because that structure is owned by the hashtable
  return 1;
}

