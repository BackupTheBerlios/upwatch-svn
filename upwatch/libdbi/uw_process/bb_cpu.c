#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#include "uw_process_glob.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

// determine - given the servername - the db realm it belongs to.
int bb_cpu_find_realm(trx *t)
{
  int i, server;

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

//*******************************************************************
// STORE RAW RESULTS
//*******************************************************************
static gint bb_cpu_store_raw_result(trx *t)
{
  dbi_result result;
  struct bb_cpu_result *res = (struct bb_cpu_result *)t->res;
  struct probe_def *def = (struct probe_def *)t->def;
  char *escmsg;
  const char *errmsg;
  int lasterr;

  if (t->res->color == STAT_PURPLE) return 1;
  t->seen_before = FALSE;
  if (res->message) {
    escmsg = strdup(res->message);
    dbi_conn_quote_string(t->probe->db, &escmsg);
  } else {
    escmsg = strdup("''");
  }
    
  result = db_query(t->probe->db, 0,
                    "insert into pr_bb_cpu_raw "
                    "set    probe = '%u', stattime = '%u', color = '%u', loadavg = '%f', "
                    "       user = '%u',  idle = '%u', free = '%u', used = '%u', message = %s",
                    def->probeid, res->stattime, res->color, res->loadavg,
                    res->user, res->idle, res->free, res->used, escmsg);
  g_free(escmsg);
  if (result) dbi_result_free(result);
  lasterr = dbi_conn_error(t->probe->db, &errmsg);
  if (errmsg && (lasterr == 1062)) { // MySQL ER_DUP_ENTRY(1062)
    t->seen_before = TRUE;
  } else if (lasterr > -1) { // otther error
    return 0; // other failure
  }
  return 1; // success
}

//*******************************************************************
// SUMMARIZE A TABLE INTO AN OLDER PERIOD
//*******************************************************************
static void bb_cpu_summarize(trx *t, char *from, char *into, guint slot, guint slotlow, guint slothigh, gint resummarize)
{
  dbi_result result;
  struct probe_def *def = (struct probe_def *)t->def;
  float avg_yellow, avg_red;
  gfloat avg_loadavg;
  guint avg_user, avg_system, avg_idle;
  guint avg_swapped, avg_free, avg_buffered, avg_cached, avg_used;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = db_query(t->probe->db, 0,
                    "select avg(loadavg) as avg_loadavg, avg(user) as avg_user, avg(system) as avg_system, "
                    "       avg(idle) as avg_idle, avg(swapped) avg_swapped, avg(free) as avg_free, " 
                    "       avg(buffered) as avg_buffered, avg(cached) as avg_cached, "
                    "       avg(used) as avg_used, max(color) as max_color, "
                    "       avg(yellow) as avg_yellow, avg(red) as avg_red " 
                    "from   pr_bb_cpu_%s use index(probstat) "
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
  if (dbi_result_get_string(result, "avg_loadavg") == NULL) {
    LOG(LOG_NOTICE, "nothing to summarize from %s for probe %u %u %u", 
                       from, def->probeid, slotlow, slothigh);
    dbi_result_free(result);
    return;
  }

  avg_loadavg = dbi_result_get_float(result, "avg_loadavg");
  avg_user    = dbi_result_get_uint(result, "avg_user");
  avg_system  = dbi_result_get_uint(result, "avg_system");
  avg_idle    = dbi_result_get_uint(result, "avg_idle");
  avg_swapped = dbi_result_get_uint(result, "avg_swapped");
  avg_free    = dbi_result_get_uint(result, "avg_free");
  avg_buffered= dbi_result_get_uint(result, "avg_buffered");
  avg_cached  = dbi_result_get_uint(result, "avg_cached");
  avg_used    = dbi_result_get_uint(result, "avg_used");
  max_color   = dbi_result_get_uint(result, "max_color");
  avg_yellow  = dbi_result_get_float(result, "avg_yellow");
  avg_red     = dbi_result_get_float(result, "avg_red");
  dbi_result_free(result);

  if (resummarize) {
    // delete old values
    result = db_query(t->probe->db, 0,
                    "delete from pr_bb_cpu_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    dbi_result_free(result);
  }
  result = db_query(t->probe->db, 0,
                    "insert into pr_bb_cpu_%s " 
                    "set    loadavg = '%f', user = '%u', system = '%u', idle = '%u', "
                    "       swapped = '%u', free = '%u', buffered = '%u', cached = '%u', "
                    "       used = '%u', probe = %d, color = '%u', stattime = %d, "
                    "       yellow = '%f', red = '%f', slot = '%u'",
                    into, 
                    avg_loadavg, avg_user, avg_system, avg_idle, 
                    avg_swapped, avg_free, avg_buffered, avg_cached, 
                    avg_used, def->probeid, max_color, stattime,
                    avg_yellow, avg_red, slot);
  dbi_result_free(result);
}

module bb_cpu_module  = {
  STANDARD_MODULE_STUFF(bb_cpu),
  NO_FREE_DEF,
  NO_FREE_RES,
  INIT_NO_CACHE,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  NO_GET_FROM_XML,
  bb_cpu_accept_result,    
  NO_GET_DEF_FIELDS,
  NO_SET_DEF_FIELDS,
  bb_cpu_get_def,
  NO_ADJUST_RESULT,
  NO_END_RESULT,
  NO_END_RUN,
  NO_EXIT,
  bb_cpu_find_realm,
  bb_cpu_store_raw_result,
  NO_NOTIFY_MAIL_SUBJECT_EXTRA,
  NO_NOTIFY_MAIL_BODY_PROBE_DEF,
  bb_cpu_summarize
};

