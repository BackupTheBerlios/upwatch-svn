#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#include "uw_process_glob.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

//*******************************************************************
// STORE RAW RESULTS
//*******************************************************************
static gint sysstat_store_raw_result(trx *t)
{
  dbi_result result;
  struct sysstat_result *res = (struct sysstat_result *)t->res;
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
                    "insert into pr_sysstat_raw (probe, yellow, red, stattime, color, loadavg, user, system, idle, "
                    "                            swapin, swapout, blockin, blockout, swapped, free, buffered, "
                    "                            cached, used, systemp, message) "
                    "            values ('%u', '%f', '%f', '%u', '%u', '%f', '%u', '%u', '%u', "
                    "                    '%u', '%u', '%u', '%u', '%u', '%u', '%u', "
                    "                    '%u', '%u', '%d', '%s')",
                    def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->loadavg,   res->user, res->system, res->idle,
                    res->swapin, res->swapout, res->blockin, res->blockout,
                    res->swapped, res->free, res->buffered, res->cached,
                    res->used, res->systemp, escmsg);
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
// SUMMARIZE A TABLE INTO AN OLDER PERIOD
//*******************************************************************
static void sysstat_summarize(trx *t, char *from, char *into, guint slot, guint slotlow, guint slothigh, gint resummarize)
{
  dbi_result result;
  struct probe_def *def = (struct probe_def *)t->def;
  gfloat avg_yellow, avg_red;
  gfloat avg_loadavg;
  guint avg_user, avg_system, avg_idle, avg_swapin, avg_swapout, avg_blockin;
  guint avg_blockout, avg_swapped, avg_free, avg_buffered, avg_cached, avg_used;
  gint avg_systemp;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = db_query(t->probe->db, 0,
                    "select avg(loadavg) as avg_loadavg, avg(user) as avg_user, avg(system) as avg_system, "
                    "       avg(idle) as avg_idle, avg(swapin) as avg_swapin, avg(swapout) as avg_swapout, "
                    "       avg(blockin) as avg_blockin, avg(blockout) as avg_blockout, avg(swapped) as avg_swapped, "
                    "       avg(free) as avg_free, avg(buffered) as avg_buffered, avg(cached) as avg_cached, "
                    "       avg(used) as avg_used, avg(systemp) as avg_systemp, max(color) as max_color, "
                    "       avg(yellow) as avg_yellow, avg(red) as avg_red " 
                    "from   pr_sysstat_%s use index(probstat) "
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
    LOG(LOG_NOTICE, "NULL values found in summarizing from %s for probe %u %u %u", 
                      from, def->probeid, slotlow, slothigh);
    dbi_result_free(result);
    return;
  }

  avg_loadavg = dbi_result_get_float(result, "avg_loadavg");
  avg_user    = dbi_result_get_int(result, "avg_user");
  avg_system  = dbi_result_get_int(result, "avg_system");
  avg_idle    = dbi_result_get_int(result, "avg_idle");
  avg_swapin  = dbi_result_get_int(result, "avg_swapin");
  avg_swapout = dbi_result_get_int(result, "avg_swapout");
  avg_blockin = dbi_result_get_int(result, "avg_blockin");
  avg_blockout= dbi_result_get_int(result, "avg_blockout");
  avg_swapped = dbi_result_get_int(result, "avg_swapped");
  avg_free    = dbi_result_get_int(result, "avg_free");
  avg_buffered= dbi_result_get_int(result, "avg_buffered");
  avg_cached  = dbi_result_get_int(result, "avg_cached");
  avg_used    = dbi_result_get_int(result, "avg_used");
  avg_systemp = dbi_result_get_int(result, "avg_systemp");
  max_color   = dbi_result_get_int(result, "max_color");
  avg_yellow  = dbi_result_get_float(result, "avg_yellow");
  avg_red     = dbi_result_get_float(result, "avg_red");
  dbi_result_free(result);

  if (resummarize) {
    // delete old values
    result = db_query(t->probe->db, 0,
                    "delete from pr_sysstat_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    dbi_result_free(result);
  }

  result = db_query(t->probe->db, 0,
                    "insert into pr_sysstat_%s (loadavg, user, system, idle, swapin, swapout, blockin, blockout, " 
                    "                           swapped, free, buffered, cached, used, systemp, probe, color, "
                    "                           stattime, yellow, red, slot) "
                    "            values ('%f', '%u', '%u', '%u', '%u', '%u', '%u', '%u', "
                    "                    '%u', '%u', '%u', '%u', '%u', '%d', '%d', '%u', "
                    "                    '%d', '%f', '%f', '%u')",
                    into, 
                    avg_loadavg, avg_user, avg_system, avg_idle, 
                    avg_swapin, avg_swapout, avg_blockin, avg_blockout, 
                    avg_swapped, avg_free, avg_buffered, avg_cached, 
                    avg_used, avg_systemp, def->probeid, max_color, stattime,
                    avg_yellow, avg_red, slot);
  dbi_result_free(result);
}

module sysstat_module  = {
  STANDARD_MODULE_STUFF(sysstat),
  NO_FREE_DEF,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  sysstat_get_from_xml,
  NO_ACCEPT_RESULT,
  NO_GET_DEF_FIELDS,
  NO_SET_DEF_FIELDS,
  sysstat_get_def,
  sysstat_adjust_result,
  NO_END_RESULT,
  NO_END_RUN,
  NO_EXIT,
  NO_FIND_DOMAIN,
  sysstat_store_raw_result,
  NO_NOTIFY_MAIL_SUBJECT_EXTRA,
  NO_NOTIFY_MAIL_BODY_PROBE_DEF,
  sysstat_summarize
};

