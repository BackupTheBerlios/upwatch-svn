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

struct ct_result {
  STANDARD_PROBE_RESULT;
  float connect;	/* time for connection to complete */
  float total;		/* total time needed */
};

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//*******************************************************************
void ct_get_from_xml(module *probe, xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns, void *probe_res)
{
  struct ct_result *res = (struct ct_result *)probe_res;

  if ((!xmlStrcmp(cur->name, (const xmlChar *) "connect")) && (cur->ns == ns)) {
    res->connect = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "total")) && (cur->ns == ns)) {
    res->total = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
    return;
  }
}

//*******************************************************************
// STORE RAW RESULTS
//*******************************************************************
gint ct_store_raw_result(struct _module *probe, void *probe_def, void *probe_res, guint *seen_before)
{
  MYSQL_RES *result;
  struct ct_result *res = (struct ct_result *)probe_res;
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
                    "insert into pr_%s_raw "
                    "set    probe = '%u', yellow = '%f', red = '%f', stattime = '%u', color = '%u', "
                    "       connect = '%f', total = '%f', "
                    "       message = '%s' ",
                    res->name, def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->connect, res->total, escmsg);
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
void ct_summarize(module *probe, void *probe_def, void *probe_res, char *from, char *into, guint slot, guint slotlow, guint slothigh, gint resummarize)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct ct_result *res = (struct ct_result *)probe_res;
  struct probe_def *def = (struct probe_def *)probe_def;
  float avg_yellow, avg_red;
  float avg_connect, avg_total;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = my_query(probe->db, 0,
                    "select avg(connect), avg(total), "
                    "       max(color), avg(yellow), avg(red) "
                    "from   pr_%s_%s use index(probstat) "
                    "where  probe = '%d' and stattime >= %d and stattime < %d",
                    res->name, from, def->probeid, slotlow, slothigh);

  if (!result) return;
  if (mysql_num_rows(result) == 0) { // no records found
    LOG(LOG_WARNING, "nothing to summarize from %s for probe %u %u %u",
                       from, def->probeid, slotlow, slothigh);
    mysql_free_result(result);
    return;
  }

  row = mysql_fetch_row(result);
  if (!row) {
    LOG(LOG_ERR, (char *) mysql_error(probe->db));
    mysql_free_result(result);
    return;
  }
  if (row[0] == NULL) {
    LOG(LOG_WARNING, "nothing to summarize from %s for probe %u %u %u", 
                       from, def->probeid, slotlow, slothigh);
    mysql_free_result(result);
    return;
  }

  avg_connect = atof(row[0]);
  avg_total = atof(row[1]);
  max_color   = atoi(row[2]);
  avg_yellow  = atof(row[3]);
  avg_red     = atof(row[4]);
  mysql_free_result(result);

  if (resummarize) {
    // delete old values
    result = my_query(probe->db, 0,
                    "delete from pr_%s_%s where probe = '%u' and stattime = '%u'",
                    res->name, into, def->probeid, stattime);
    mysql_free_result(result);
  }

  result = my_query(probe->db, 0,
                    "insert into pr_%s_%s "
                    "set    connect = '%f', total = '%f', "
                    "       probe = %d, color = '%u', stattime = %d, "
                    "       yellow = '%f', red = '%f', slot = '%u'",
                    res->name, into, avg_connect, avg_total, def->probeid, 
                    max_color, stattime, avg_yellow, avg_red, slot);

  mysql_free_result(result);
}


