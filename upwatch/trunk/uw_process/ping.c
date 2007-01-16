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
static void ping_set_def_fields(trx *t, struct probe_def *probedef, MYSQL_RES *result)
{
  struct ping_def *def = (struct ping_def *) probedef;
  MYSQL_ROW row = mysql_fetch_row(result);

  if (row) {
    if (row[0]) def->ipaddress   = strdup(row[0]);
    if (row[1]) def->description = strdup(row[1]);
    if (row[2]) def->server   = atoi(row[2]);
    if (row[3]) def->yellow   = atof(row[3]);
    if (row[4]) def->red      = atof(row[4]);
    if (row[5]) def->contact  = atof(row[5]);
    strcpy(def->hide, row[6] ? row[6] : "no");
    strcpy(def->email, row[7] ? row[7] : "");
    strcpy(def->sms, row[8] ? row[8] : "");
    if (row[9]) def->delay = atoi(row[9]);
    if (row[10]) def->count = atoi(row[10]);
  }
}

//*******************************************************************
// STORE RAW RESULTS
//*******************************************************************
static gint ping_store_raw_result(trx *t)
{
  MYSQL_RES *result;
  struct ping_result *res = (struct ping_result *)t->res;
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
                    "insert into pr_ping_raw "
                    "set    probe = '%u', yellow = '%f', red = '%f', stattime = '%u', color = '%u', "
                    "       average = '%f', lowest = '%f', highest = '%f', message = '%s'",
                    def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->average, res->lowest, res->highest, escmsg);
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
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct probe_def *def = (struct probe_def *)t->def;
  float avg_yellow, avg_red;
  float avg_average, min_lowest, max_highest; 
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = my_query(t->probe->db, 0,
                    "select avg(lowest), avg(average), avg(highest), "
                    "       max(color), avg(yellow), avg(red) " 
                    "from   pr_ping_%s use index(probstat) "
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

  min_lowest  = atof(row[0]);
  avg_average = atof(row[1]);
  max_highest = atof(row[2]);
  max_color   = atoi(row[3]);
  avg_yellow  = atof(row[4]);
  avg_red     = atof(row[5]);
  mysql_free_result(result);

  if (resummarize) {
    // delete old values
    result = my_query(t->probe->db, 0,
                    "delete from pr_ping_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    mysql_free_result(result);
  }

  result = my_query(t->probe->db, 0,
                    "insert into pr_ping_%s " 
                    "set    average = %f, lowest = %f, highest = %f, probe = %d, color = '%u', " 
                    "       stattime = %d, yellow = '%f', red = '%f', slot = '%u'",
                    into, avg_average, min_lowest, max_highest, def->probeid, 
                    max_color, stattime, avg_yellow, avg_red, slot);
  mysql_free_result(result);
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
  "ipaddress, description, server, yellow, red, contact, hide, email, sms, delay, "
  "count ",
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

