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
static void postgresql_free_def(void *probedef)
{
  struct postgresql_def *def = (struct postgresql_def *) probedef;

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
static void postgresql_set_def_fields(trx *t, struct probe_def *probedef, dbi_result result)
{
  struct postgresql_def *def = (struct postgresql_def *) probedef;

  if (dbi_result_next_row(result)) {
    def->ipaddress = dbi_result_get_string_copy(result, "ipaddress");
    def->description = dbi_result_get_string_copy(result, "description");
    def->server  = dbi_result_get_int(result, "server");
    def->yellow  = dbi_result_get_float(result, "yellow");
    def->red     = dbi_result_get_float(result, "red");
    def->contact = dbi_result_get_int(result, "contact");
    strcpy(def->hide, dbi_result_get_string_default(result, "hide", "no"));
    strcpy(def->email, dbi_result_get_string_default(result, "email", ""));
    strcpy(def->sms, dbi_result_get_string_default(result, "sms", ""));
    def->delay   = dbi_result_get_int(result, "delay");
    def->dbname = dbi_result_get_string_copy(result, "dbname");
    def->dbuser = dbi_result_get_string_copy(result, "dbuser");
    def->dbpasswd = dbi_result_get_string_copy(result, "dbpasswd");
    def->query = dbi_result_get_string_copy(result, "query");
  }
}

//*******************************************************************
// Format the probe definition fields for inclusion in the notification body
//*******************************************************************
static void postgresql_notify_mail_body_probe_def(trx *t, char *buf, size_t buflen)
{
  struct postgresql_def *def = (struct postgresql_def *)t->def;
  struct postgresql_result *res = (struct postgresql_result *)t->res;

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

module postgresql_module  = {
  STANDARD_MODULE_STUFF(postgresql),
  postgresql_free_def,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  ct_get_from_xml,
  NO_ACCEPT_RESULT,
  "ipaddress, description, server, yellow, red, contact, hide, email, sms, delay, "
  "dbname, dbuser, dbpasswd, query ",
  postgresql_set_def_fields,
  NO_GET_DEF,
  NO_ADJUST_RESULT,
  NO_END_RESULT,
  NO_END_RUN,
  NO_EXIT,
  NO_FIND_DOMAIN,
  ct_store_raw_result,
  NO_NOTIFY_MAIL_SUBJECT_EXTRA,
  postgresql_notify_mail_body_probe_def,
  ct_summarize
};

