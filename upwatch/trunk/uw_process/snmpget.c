#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#include "uw_process_glob.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

//*******************************************************************
// free the probe_def structure.
//*******************************************************************
static void snmpget_free_def(void *probedef)
{
  struct snmpget_def *def = (struct snmpget_def *) probedef;

  if (def->ipaddress) g_free(def->ipaddress);
  if (def->description) g_free(def->description);
  if (def->community) g_free(def->community);
  if (def->OID)       g_free(def->OID);
  if (def->dispname)  g_free(def->dispname);
  if (def->dispunit)  g_free(def->dispunit);
  g_free(def);
}

//*******************************************************************
// Get the results of the MySQL query into our probe_def structure
//*******************************************************************
static void snmpget_set_def_fields(trx *t, struct probe_def *probedef, dbi_result result)
{
  struct snmpget_def *def = (struct snmpget_def *) probedef;

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
    def->community   = dbi_result_get_string_copy(result, "community"); /* community string for SNMPv1/v2c transactions */
    def->OID         = dbi_result_get_string_copy(result, "OID");       /* Object ID */
    def->dispname    = dbi_result_get_string_copy(result, "dispname");  /* Display Name */
    def->dispunit    = dbi_result_get_string_copy(result, "dispunit");  /* Display Unit */
    def->multiplier  = dbi_result_get_float(result, "multiplier");      /* Multiplier for result values */
    strcpy(def->mode, dbi_result_get_string(result, "mode"));           /* plot absolute or relative values */
  }
}

//*******************************************************************
// STORE RAW RESULTS
//*******************************************************************
static gint snmpget_store_raw_result(trx *t)
{
  dbi_result result;
  struct snmpget_result *res = (struct snmpget_result *)t->res;
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
                    "insert into pr_snmpget_raw (probe, yellow, red, stattime, color, value, message)"
                    "            values ('%u', '%f', '%f', '%u', '%u', '%f', '%s')",
                    def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->value, escmsg);
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
static void snmpget_notify_mail_subject_extra(trx *t, char *buf, size_t buflen)
{
  struct snmpget_def *def = (struct snmpget_def *)t->def;

  sprintf(buf, "%s", def->dispname);
}

//*******************************************************************
// Format the probe definition fields for inclusion in the notification body
//*******************************************************************
static void snmpget_notify_mail_body_probe_def(trx *t, char *buf, size_t buflen)
{
  struct snmpget_def *def = (struct snmpget_def *)t->def;
  struct snmpget_result *res = (struct snmpget_result *)t->res;

  sprintf(buf, "%-20s: %s\n"
               "%-20s: %s\n"
               "%-20s: %s\n"
               "%-20s: %s\n"
               "%-20s: %s\n"
               "%-20s: %s\n"
               "%-20s: %.2f\n"
               "%-20s: %s\n"
               "%-20s: %.2f%s\n",
  "IP adres", def->ipaddress, 
  "Description", def->description, 
  "Community", "********", 
  "Object ID", def->OID, 
  "Display Name", def->dispname,
  "Display Unit", def->dispunit,
  "Multiplier", def->multiplier,
  "Mode", def->mode,
  "Current Value", res->value, def->dispunit);
}
//*******************************************************************
// SUMMARIZE A TABLE INTO AN OLDER PERIOD
//*******************************************************************
static void snmpget_summarize(trx *t, char *from, char *into, guint slot, guint slotlow, guint slothigh, gint resummarize)
{
  dbi_result result;
  struct probe_def *def = (struct probe_def *)t->def;
  float avg_yellow, avg_red;
  float avg_value;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = db_query(t->probe->db, 0,
                    "select avg(value) as avg_value, max(color) as max_color , avg(yellow) as avg_yellow, avg(red) as avg_red"
                    "from   pr_snmpget_%s use index(probstat) "
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

  avg_value   = dbi_result_get_float(result, "avg_value");
  max_color   = dbi_result_get_int(result, "max_color");
  avg_yellow  = dbi_result_get_float(result, "avg_yellow");
  avg_red     = dbi_result_get_float(result, "avg_red");
  dbi_result_free(result);

  if (resummarize) {
    // delete old values
    result = db_query(t->probe->db, 0,
                    "delete from pr_snmpget_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    dbi_result_free(result);
  }

  result = db_query(t->probe->db, 0,
                    "insert into pr_snmpget_%s (value, probe, color, stattime, yellow, red, slot) "
                    "            values ('%f', '%d', '%u', '%d', '%f', '%f', '%u')",
                    into, avg_value, def->probeid, 
                    max_color, stattime, avg_yellow, avg_red, slot);

  dbi_result_free(result);
}

module snmpget_module  = {
  STANDARD_MODULE_STUFF(snmpget),
  snmpget_free_def,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  snmpget_get_from_xml,
  NO_ACCEPT_RESULT,
  "ipaddress, description, server, yellow, red, contact, hide, email, sms, delay, "
  "community, OID, dispname, dispunit, multiplier, mode ",
  snmpget_set_def_fields,
  NO_GET_DEF,
  NO_ADJUST_RESULT,
  NO_END_RESULT,
  NO_END_RUN,
  NO_EXIT,
  NO_FIND_DOMAIN,
  snmpget_store_raw_result,
  snmpget_notify_mail_subject_extra,
  snmpget_notify_mail_body_probe_def,
  snmpget_summarize
};

