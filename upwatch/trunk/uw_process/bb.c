#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#include "uw_process_glob.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

// determine - given the servername - the db realm it belongs to.
int bb_find_realm(trx *t)
{
  int i, server = 1;

  query_server_by_name = NULL;
  for (i=0; i < dblist_cnt; i++) {
    t->probe->db = open_realm(dblist[i].realm);
    server = realm_server_by_name(dblist[i].realm, t->res->hostname);
    if (server > 1) {
      t->res->server = server;
      t->res->realm = strdup(dblist[i].realm);
      query_server_by_name = dblist[i].srvrbyname;
      break;
    }
  }
  return 1;
}

module bb_module  = {
  STANDARD_MODULE_STUFF(bb),
  NO_FREE_DEF,
  bb_free_res,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  bb_xml_result_node,
  NO_GET_FROM_XML,
  accept_result,
  NO_GET_DEF_FIELDS,
  NO_SET_DEF_FIELDS,
  bb_get_def,
  NO_ADJUST_RESULT,
  NO_END_RESULT,
  NO_END_RUN,
  NO_EXIT,
  bb_find_realm,
  NO_STORE_RESULTS,
  NO_NOTIFY_MAIL_SUBJECT_EXTRA,
  NO_NOTIFY_MAIL_BODY_PROBE_DEF,
  NO_SUMMARIZE
};

