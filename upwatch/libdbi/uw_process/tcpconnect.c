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
static void tcpconnect_set_def_fields(trx *t, struct probe_def *probedef, MYSQL_RES *result)
{
  struct tcpconnect_def *def = (struct tcpconnect_def *) probedef;
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
    if (row[8]) def->delay = atoi(row[8]);
    if (row[9]) def->port = atoi(row[9]);
  }
}

//*******************************************************************
// Format the probe definition fields for inclusion in the notification body
//*******************************************************************
static void tcpconnect_notify_mail_body_probe_def(trx *t, char *buf, size_t buflen)
{
  struct tcpconnect_def *def = (struct tcpconnect_def *)t->def;
  struct tcpconnect_result *res = (struct tcpconnect_result *)t->res;

  sprintf(buf, "%-20s: %s\n"
               "%-20s: %s\n"
               "%-20s: %u\n"
               "%-20s: %f\n"
               "%-20s: %f\n",
  "IP Address", def->ipaddress,
  "Description", def->description,
  "Port", def->port,
  "Connect time", res->connect,
  "Total time", res->total);
}

module tcpconnect_module  = {
  STANDARD_MODULE_STUFF(tcpconnect),
  NO_FREE_DEF,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  ct_get_from_xml,
  NO_ACCEPT_RESULT,
  "ipaddress, description, server, yellow, red, contact, hide, email, delay, "
  "port ",
  tcpconnect_set_def_fields,
  NO_GET_DEF,
  NO_ADJUST_RESULT,
  NO_END_RESULT,
  NO_END_RUN,
  NO_EXIT,
  NO_FIND_DOMAIN,
  ct_store_raw_result,
  NO_NOTIFY_MAIL_SUBJECT_EXTRA,
  tcpconnect_notify_mail_body_probe_def,
  ct_summarize
};

