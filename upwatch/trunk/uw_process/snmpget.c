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
static void snmpget_set_def_fields(trx *t, struct probe_def *probedef, MYSQL_RES *result)
{
  struct snmpget_def *def = (struct snmpget_def *) probedef;
  MYSQL_ROW row = mysql_fetch_row(result);

  if (row) {
    if (row[0]) def->ipaddress = strdup(row[0]);
    if (row[1]) def->description = strdup(row[1]);
    if (row[2]) def->server   = atoi(row[2]);
    if (row[3]) def->yellow   = atof(row[3]);
    if (row[4]) def->red      = atof(row[4]);
    if (row[5]) def->contact  = atof(row[5]);
    strcpy(def->hide, row[6] ? row[6] : "no");
    strcpy(def->email, row[7] ? row[7] : "");
    strcpy(def->sms, row[8] ? row[8] : "");
    if (row[9]) def->delay = atoi(row[9]);
    if (row[10]) def->community = strdup(row[10]);/* community string for SNMPv1/v2c transactions */
    if (row[11]) def->OID = strdup(row[11]);      /* Object ID */
    if (row[12]) def->dispname = strdup(row[12]); /* Display Name */
    if (row[13]) def->dispunit = strdup(row[13]); /* Display Unit */
    if (row[14]) def->multiplier = atof(row[14]); /* Multiplier for result values */
    if (row[15]) strcpy(def->mode, row[15]);      /* plot absolute or relative values */
  }
}

//*******************************************************************
// STORE RAW RESULTS
//*******************************************************************
static gint snmpget_store_raw_result(trx *t)
{
  MYSQL_RES *result;
  struct snmpget_result *res = (struct snmpget_result *)t->res;
  struct probe_def *def = (struct probe_def *)t->def;
  char *escmsg;

  if (t->res->color == STAT_PURPLE) return 1;
  t->seen_before = FALSE;
  if (res->message) {
    escmsg = g_malloc(strlen(res->message) * 2 + 1);
    mysql_real_escape_string(t->probe->db, escmsg, res->message, strlen(res->message)) ;
  } else {
    escmsg = strdup("");
  }
    
  result = my_query(t->probe->db, 0,
                    "insert into pr_snmpget_raw "
                    "set    probe = '%u', yellow = '%f', red = '%f', stattime = '%u', color = '%u', "
                    "       value = '%f', "
                    "       message = '%s' ",
                    def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->value, escmsg);
  g_free(escmsg);
  if (result) mysql_free_result(result);
  if (mysql_errno(t->probe->db) == ER_DUP_ENTRY) {
    t->seen_before = TRUE;
  } else if (mysql_errno(t->probe->db)) {
    return 0; // other failure
  }
  return 1; // success
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
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct probe_def *def = (struct probe_def *)t->def;
  float avg_yellow, avg_red;
  float avg_value;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = my_query(t->probe->db, 0,
                    "select avg(value), "
                    "       max(color), avg(yellow), avg(red) "
                    "from   pr_snmpget_%s use index(probstat) "
                    "where  probe = '%d' and stattime >= %d and stattime < %d",
                    from, def->probeid, slotlow, slothigh);

  if (!result) return;
  if (mysql_num_rows(result) == 0) { // no records found
    LOG(LOG_NOTICE, "nothing to summarize from %s for probe %u %u %u",
                       from, def->probeid, slotlow, slothigh);
    mysql_free_result(result);
    return;
  }

  row = mysql_fetch_row(result);
  if (!row) {
    mysql_free_result(result);
    return;
  }
  if (row[0] == NULL) {
    LOG(LOG_NOTICE, "nothing to summarize from %s for probe %u %u %u", 
                       from, def->probeid, slotlow, slothigh);
    mysql_free_result(result);
    return;
  }

  avg_value = atof(row[0]);
  max_color   = atoi(row[1]);
  avg_yellow  = atof(row[2]);
  avg_red     = atof(row[3]);
  mysql_free_result(result);

  if (resummarize) {
    // delete old values
    result = my_query(t->probe->db, 0,
                    "delete from pr_snmpget_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    mysql_free_result(result);
  }

  result = my_query(t->probe->db, 0,
                    "insert into pr_snmpget_%s "
                    "set    value = '%f', "
                    "       probe = %d, color = '%u', stattime = %d, "
                    "       yellow = '%f', red = '%f', slot = '%u'",
                    into, avg_value, def->probeid, 
                    max_color, stattime, avg_yellow, avg_red, slot);

  mysql_free_result(result);
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

