#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#include "uw_process_glob.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

//*******************************************************************
// Get the results of the MySQL query into our probe_def structure
//*******************************************************************
static void ping_set_def_fields(trx *t, struct probe_def *probedef, dbi_result result)
{
  struct ping_def *def = (struct ping_def *) probedef;

  if (dbi_result_next_row(result)) {
    def->ipaddress   = dbi_result_get_string_copy(result, "ipaddress");
    def->description = dbi_result_get_string_copy(result, "description");
    def->server      = dbi_result_get_int(result, "server");
    def->yellow      = dbi_result_get_float(result, "yellow");
    def->red         = dbi_result_get_float(result, "red");
    def->contact     = dbi_result_get_int(result, "contact");
    strcpy(def->hide, dbi_result_get_string_default(result, "hide", "no"));
    strcpy(def->email, dbi_result_get_string_default(result, "email", ""));
    strcpy(def->sms, dbi_result_get_string_default(result, "sms", ""));
    def->delay       = dbi_result_get_int(result, "delay");
    def->count       = dbi_result_get_int(result, "count");
  }
}

//*******************************************************************
// STORE RAW RESULTS
//*******************************************************************
static gint ping_store_raw_result(trx *t)
{
  dbi_result result;
  struct ping_result *res = (struct ping_result *)t->res;
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
                    "insert into pr_ping_raw (probe, yellow, red, stattime, color, average, lowest, highest, message)"
                    "            values ('%u', '%f', '%f', '%u', '%u', '%f', '%f', '%f', '%s')",
                    def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->average, res->lowest, res->highest, escmsg);
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
// Format the probe definition fields for inclusion in the notification body
//*******************************************************************
static void ping_notify_mail_body_probe_def(trx *t, char *buf, size_t buflen)
{
  struct ping_def *def = (struct ping_def *)t->def;
  struct ping_result *res = (struct ping_result *)t->res;

  sprintf(buf, "%-20s: %s\n"
               "%-20s: %s\n"
               "%-20s: %u\n"
               "%-20s: %f\n"
               "%-20s: %f\n",
  "IP Address", def->ipaddress,
  "Description", def->description,
  "# packets", def->count,
  "Lowest", res->lowest,
  "Average", res->average,
  "Highest", res->highest);
}

//*******************************************************************
// SUMMARIZE A TABLE INTO AN OLDER PERIOD
//*******************************************************************
static void ping_summarize(trx *t, char *from, char *into, guint slot, guint slotlow, guint slothigh, gint resummarize)
{
  dbi_result result;
  struct probe_def *def = (struct probe_def *)t->def;
  float avg_yellow, avg_red;
  float avg_average, avg_lowest, avg_highest; 
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = db_query(t->probe->db, 0,
                    "select avg(lowest) as avg_lowest, avg(average) as avg_average, avg(highest) as avg_highest, "
                    "       max(color) as max_color, avg(yellow) as avg_yellow, avg(red) as avg_red " 
                    "from   pr_ping_%s use index(probstat) "
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

  avg_lowest  = dbi_result_get_float(result, "avg_lowest");
  avg_average = dbi_result_get_float(result, "avg_average");
  avg_highest = dbi_result_get_float(result, "avg_highest");
  max_color   = dbi_result_get_int(result, "max_color");
  avg_yellow  = dbi_result_get_float(result, "avg_yellow");
  avg_red     = dbi_result_get_float(result, "avg_red");
  dbi_result_free(result);

  if (resummarize) {
    // delete old values
    result = db_query(t->probe->db, 0,
                    "delete from pr_ping_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    dbi_result_free(result);
  }

  result = db_query(t->probe->db, 0,
                    "insert into pr_ping_%s " 
                    "set    average = %f, lowest = %f, highest = %f, probe = %d, color = '%u', " 
                    "       stattime = %d, yellow = '%f', red = '%f', slot = '%u'",
                    into, avg_average, avg_lowest, avg_highest, def->probeid, 
                    max_color, stattime, avg_yellow, avg_red, slot);
  dbi_result_free(result);
}

module ping_module  = {
  STANDARD_MODULE_STUFF(ping),
  NO_FREE_DEF,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  ping_get_from_xml,
  NO_ACCEPT_RESULT,
  //"ipaddress, description, server, yellow, red, contact, hide, email, sms, delay, count ",
  ping_set_def_fields,
  NO_GET_DEF,
  NO_ADJUST_RESULT,
  NO_END_RESULT,
  NO_END_RUN,
  NO_EXIT,
  NO_FIND_DOMAIN,
  ping_store_raw_result,
  NO_NOTIFY_MAIL_SUBJECT_EXTRA,
  ping_notify_mail_body_probe_def,
  ping_summarize
};

