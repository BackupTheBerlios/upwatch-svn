#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#ifdef UW_PROCESS
#include "uw_process_glob.h"
#endif
#ifdef UW_NOTIFY
#include "uw_notify_glob.h"
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct ping_result {
  STANDARD_PROBE_RESULT;
#include "../uw_ping/probe.res_h"
};
struct ping_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};

extern module ping_module;

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//*******************************************************************
static void ping_get_from_xml(module *probe, xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns, void *probe_res)
{
  struct ping_result *res = (struct ping_result *)probe_res;

  if ((!xmlStrcmp(cur->name, (const xmlChar *) "lowest")) && (cur->ns == ns)) {
    res->lowest = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "value")) && (cur->ns == ns)) {
    res->value = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "highest")) && (cur->ns == ns)) {
    res->highest = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
    return;
  }
}

#ifdef UW_PROCESS
//*******************************************************************
// STORE RAW RESULTS
//*******************************************************************
static gint ping_store_raw_result(struct _module *probe, void *probe_def, void *probe_res, guint *seen_before)
{
  MYSQL_RES *result;
  struct ping_result *res = (struct ping_result *)probe_res;
  struct probe_def *def = (struct probe_def *)probe_def;
  char *escmsg;

  *seen_before = FALSE;
  if (res->message) {
    escmsg = g_malloc(strlen(res->message) * 2 + 1);
    mysql_real_escape_string(probe->db, escmsg, res->message, strlen(res->message)) ;
  } else {
    escmsg = strdup("");
  }

  result = my_query(probe->db, 0,
                    "insert into pr_ping_raw "
                    "set    probe = '%u', yellow = '%f', red = '%f', stattime = '%u', color = '%u', "
                    "       value = '%f', lowest = '%f', highest = '%f', message = '%s'",
                    def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->value, res->lowest, res->highest, escmsg);
  g_free(escmsg);
  if (result) mysql_free_result(result);
  if (mysql_errno(probe->db) == ER_DUP_ENTRY) {
    *seen_before = TRUE;
  } else if (mysql_errno(probe->db)) {
    return 0; // other failure
  }
  return 1; // success
}

//*******************************************************************
// SUMMARIZE A TABLE INTO AN OLDER PERIOD
//*******************************************************************
static void ping_summarize(module *probe, void *probe_def, void *probe_res, char *from, char *into, guint slot, guint slotlow, guint slothigh, gint resummarize)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct probe_def *def = (struct probe_def *)probe_def;
  float avg_yellow, avg_red;
  float avg_value, min_lowest, max_highest; 
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = my_query(probe->db, 0,
                    "select avg(lowest), avg(value), avg(highest), "
                    "       max(color), avg(yellow), avg(red) " 
                    "from   pr_ping_%s use index(probstat) "
                    "where  probe = '%d' and stattime >= %d and stattime < %d",
                    from, def->probeid, slotlow, slothigh);
  
  if (!result) return;
  if (mysql_num_rows(result) == 0) { // no records found
    LOG(LOG_WARNING, "nothing to summarize from %s for probe %u %u %u",
                       from, def->probeid, slotlow, slothigh);
    mysql_free_result(result);
    return;
  }

  row = mysql_fetch_row(result);
  if (!row) {
    LOG(LOG_ERR, (char *)mysql_error(probe->db));
    mysql_free_result(result);
    return;
  }
  if (row[0] == NULL) {
    LOG(LOG_WARNING, "nothing to summarize from %s for probe %u %u %u", 
                       from, def->probeid, slotlow, slothigh);
    mysql_free_result(result);
    return;
  }

  min_lowest  = atof(row[0]);
  avg_value   = atof(row[1]);
  max_highest = atof(row[2]);
  max_color   = atoi(row[3]);
  avg_yellow  = atof(row[4]);
  avg_red     = atof(row[5]);
  mysql_free_result(result);

  if (resummarize) {
    // delete old values
    result = my_query(probe->db, 0,
                    "delete from pr_ping_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    mysql_free_result(result);
  }

  result = my_query(probe->db, 0,
                    "insert into pr_ping_%s " 
                    "set    value = %f, lowest = %f, highest = %f, probe = %d, color = '%u', " 
                    "       stattime = %d, yellow = '%f', red = '%f', slot = '%u'",
                    into, avg_value, min_lowest, max_highest, def->probeid, 
                    max_color, stattime, avg_yellow, avg_red, slot);
  mysql_free_result(result);
}
#endif

module ping_module  = {
  STANDARD_MODULE_STUFF(ping),
  NO_FREE_DEF,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  ping_get_from_xml,
  NO_FIX_RESULT,
  NO_GET_DEF,
#ifdef UW_PROCESS
  ping_store_raw_result,
  ping_summarize,
#endif
  NO_END_PROBE,
  NO_END_RUN
};

