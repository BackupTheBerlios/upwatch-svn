#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#include "uw_process_glob.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

//*******************************************************************
// Format the probe definition fields for inclusion in the notification body
//*******************************************************************
static void local_notify_mail_body_probe_def(trx *t, char *buf, size_t buflen)
{
  struct local_def *def = (struct local_def *)t->def;
  struct local_result *res = (struct local_result *)t->res;

  sprintf(buf, "%-20s: %s\n"
               "%-20s: %s\n",
  "IP Address", def->ipaddress,
  "Description", def->description);
}


module local_module  = {
  STANDARD_MODULE_STUFF(local),
  NO_FREE_DEF,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  ct_get_from_xml,
  accept_result,
  NO_GET_DEF_FIELDS,
  NO_SET_DEF_FIELDS,
  get_def_by_servid,
  NO_ADJUST_RESULT,
  NO_END_RESULT,
  NO_END_RUN,
  NO_EXIT,
  NO_FIND_DOMAIN,
  NO_STORE_RESULTS,
  NO_NOTIFY_MAIL_SUBJECT_EXTRA,
  NO_NOTIFY_MAIL_BODY_PROBE_DEF,
  NO_SUMMARIZE
};

