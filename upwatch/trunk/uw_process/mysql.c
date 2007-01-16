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
static void mysql_free_def(void *probedef)
{
  struct mysql_def *def = (struct mysql_def *) probedef;

  if (def->ipaddress) g_free(def->ipaddress);
  if (def->description) g_free(def->description);
  if (def->dbname) g_free(def->dbname);
  if (def->dbuser) g_free(def->dbuser);
  if (def->dbpasswd) g_free(def->dbpasswd);
  if (def->query) g_free(def->query);
  g_free(def);
}

//*******************************************************************
// Get the results of the MySQL query into our probe_def structure
//*******************************************************************
static void mysql_set_def_fields(trx *t, struct probe_def *probedef, MYSQL_RES *result)
{
  struct mysql_def *def = (struct mysql_def *) probedef;
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
    if (row[10]) def->dbname = strdup(row[10]);
    if (row[11]) def->dbuser = strdup(row[11]);
    if (row[12]) def->dbpasswd = strdup(row[12]);
    if (row[13]) def->query = strdup(row[13]);
  }
}

//*******************************************************************
// Format the probe definition fields for inclusion in the notification body
//*******************************************************************
static void mysql_notify_mail_body_probe_def(trx *t, char *buf, size_t buflen)
{
  struct mysql_def *def = (struct mysql_def *)t->def;
  struct mysql_result *res = (struct mysql_result *)t->res;

  sprintf(buf, "%-20s: %s\n"
               "%-20s: %s\n"
               "%-20s: %s\n"
               "%-20s: %s\n"
               "%-20s: %s\n"
               "%-20s: %s\n"
               "%-20s: %f\n"
               "%-20s: %f\n",
  "IP Address", def->ipaddress,
  "Description", def->description,
  "DB Name", def->dbname,
  "DB User", def->dbuser,
  "DB Password", "********",
  "Query", def->query,
  "Connect time", res->connect,
  "Total time", res->total);
}

module mysql_module  = {
  STANDARD_MODULE_STUFF(mysql),
  mysql_free_def,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  ct_get_from_xml,
  NO_ACCEPT_RESULT,
  "ipaddress, description, server, yellow, red, contact, hide, email, sms, delay, "
  "dbname, dbuser, dbpasswd, query ",
  mysql_set_def_fields,
  NO_GET_DEF,
  NO_ADJUST_RESULT,
  NO_END_RESULT,
  NO_END_RUN,
  NO_EXIT,
  NO_FIND_DOMAIN,
  ct_store_raw_result,
  NO_NOTIFY_MAIL_SUBJECT_EXTRA,
  mysql_notify_mail_body_probe_def,
  ct_summarize
};

