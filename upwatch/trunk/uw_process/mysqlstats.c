#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#include "uw_process_glob.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

//*******************************************************************
// STORE RAW RESULTS
//*******************************************************************
static gint mysqlstats_store_raw_result(trx *t)
{
  dbi_result result;
  struct mysqlstats_result *res = (struct mysqlstats_result *)t->res;
  struct probe_def *def = (struct probe_def *)t->def;
  char *escmsg;
  const char *errmsg;

  if (t->res->color == STAT_PURPLE) return 1;
  t->seen_before = FALSE;
  if (res->message) {
    dbi_conn_quote_string_copy(t->probe->db->conn, t->res->message, &escmsg);
  } else {
    escmsg = strdup("");
  }
    
  result = db_query(t->probe->db, 0,
                    "insert into pr_mysqlstats_raw (probe, yellow, red, stattime, color, selectq, insertq, "
                    "                               updateq, deleteq, message) "
                    "            values ('%u', '%f', '%f', '%u', '%u', '%u', '%u', "
                    "                    '%u', '%u', '%s')",
                    def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->selectq, res->insertq, res->updateq, res->deleteq,escmsg);
  g_free(escmsg);
  if (result) {
    dbi_result_free(result);
    return 1; // success
  }
  if (dbi_duplicate_entry(t->probe->db->conn)) {
    t->seen_before = TRUE;
    return 1; // success
  }
  if (dbi_conn_error(t->probe->db->conn, &errmsg) == DBI_ERROR_NONE) {
    return 1; // success
  }
  LOG(LOG_ERR, "%s", errmsg);
  return 0; // Failure
}

//*******************************************************************
// Create a meaningful subject line for the notification
//*******************************************************************
static int mysqlstats_notify_mail_subject(trx *t, FILE *fp, char *servername)
{
  fprintf(fp, "Subject: %s: %s %s (was %s)\n", servername,
                 t->probe->module_name, /* t->def->dispname, */
                 color2string(t->res->color),
                 color2string(t->res->prevhistcolor));
}
//*******************************************************************
// SUMMARIZE A TABLE INTO AN OLDER PERIOD
//*******************************************************************
static void mysqlstats_summarize(trx *t, char *from, char *into, guint slot, guint slotlow, guint slothigh, gint resummarize)
{
  dbi_result result;
  struct probe_def *def = (struct probe_def *)t->def;
  float avg_yellow, avg_red;
  int avg_selectq;
  int avg_insertq;
  int avg_updateq;
  int avg_deleteq;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = db_query(t->probe->db, 0,
                    "select avg(selectq) as avg_selectq, avg(insertq) as avg_insertq, avg(updateq) as avg_updateq, "
                    "       avg(deleteq) as avg_deleteq, max(color) as max_color, avg(yellow) as avg_yellow, avg(red) as avg_red"
                    "from   pr_mysqlstats_%s use index(probstat) "
                    "where  probe = '%d' and stattime >= %d and stattime < %d",
                    from, def->probeid, slotlow, slothigh);

  if (!result) return;
  if (dbi_result_get_numrows(result) == 0) { // no records found
    LOG(LOG_NOTICE, "nothing to summarize from %s for probe %u %u %u",
                       from, def->probeid, slotlow, slothigh);
    dbi_result_free(result);
    return;
  }

  if (!dbi_result_next_row(result)) {
    dbi_result_free(result);
    return;
  }
  if (dbi_result_field_is_null_idx(result, 0)) {
    LOG(LOG_NOTICE, "nothing to summarize from %s for probe %u %u %u", 
                       from, def->probeid, slotlow, slothigh);
    dbi_result_free(result);
    return;
  }

  avg_selectq = dbi_result_get_int(result, "avg_selectq");
  avg_insertq = dbi_result_get_int(result, "avg_insertq");
  avg_updateq = dbi_result_get_int(result, "avg_updateq");
  avg_deleteq = dbi_result_get_int(result, "avg_deleteq");
  max_color   = dbi_result_get_int(result, "max_color");
  avg_yellow  = dbi_result_get_float(result, "avg_yellow");
  avg_red     = dbi_result_get_float(result, "avg_red");
  dbi_result_free(result);

  if (resummarize) {
    // delete old values
    result = db_query(t->probe->db, 0,
                    "delete from pr_mysqlstats_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    dbi_result_free(result);
  }

  result = db_query(t->probe->db, 0,
                    "insert into pr_mysqlstats_%s (selectq, insertq, updateq, deleteq, probe, color, stattime, "
                    "                              yellow, red, slot) "
                    "            values ('%u', '%u', '%u', '%u', '%d', '%u', '%d', "
                    "                    '%f', '%f', '%u')",
                    into,  avg_selectq, avg_insertq, avg_updateq, avg_deleteq, def->probeid, 
                    max_color, stattime, avg_yellow, avg_red, slot);

  dbi_result_free(result);
}

module mysqlstats_module  = {
  STANDARD_MODULE_STUFF(mysqlstats),
  NO_FREE_DEF,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  mysqlstats_get_from_xml,
  NO_ACCEPT_RESULT,
  NO_GET_DEF_FIELDS,
  NO_SET_DEF_FIELDS,
  NO_GET_DEF,
  NO_ADJUST_RESULT,
  NO_END_RESULT,
  NO_END_RUN,
  NO_EXIT,
  NO_FIND_DOMAIN,
  mysqlstats_store_raw_result,
  NO_NOTIFY_MAIL_SUBJECT_EXTRA,
  NO_NOTIFY_MAIL_BODY_PROBE_DEF,
  mysqlstats_summarize
};

