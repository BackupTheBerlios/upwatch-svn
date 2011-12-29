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
static gint hwstat_store_raw_result(trx *t)
{
  dbi_result result;
  struct hwstat_result *res = (struct hwstat_result *)t->res;
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
                    "insert into pr_hwstat_raw (probe, yellow, red, stattime, color, temp1, temp2, temp3, "
                    "                           rot1, rot2, rot3, vc0, vc1, v33, v50p, v12p, v12n, v50n, message) "
                    "            values ('%u', '%f', '%f', '%u', '%u', '%f', '%f', '%f', "
                    "                    '%d', '%d', '%d', '%f', '%f', '%f', '%f', '%f', '%f', '%f', '%s')",
                    def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->temp1, res->temp2, res->temp3, 
                    res->rot1, res->rot2, res->rot3, 
                    res->vc0, res->vc1, res->v33, 
                    res->v50p, res->v12p, res->v12n, res->v50n, 
                    escmsg);
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
static void hwstat_summarize(trx *t, char *from, char *into, guint slot, guint slotlow, guint slothigh, gint resummarize)
{
  dbi_result result;
  struct probe_def *def = (struct probe_def *)t->def;
  gfloat avg_yellow, avg_red;
  gfloat avg_temp1, avg_temp2, avg_temp3;
  guint avg_rot1, avg_rot2, avg_rot3;
  gfloat avg_vc0, avg_vc1, avg_v33,  avg_v50p,  avg_v12p,  avg_v12n, avg_v50n;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = db_query(t->probe->db, 0,
                    "select avg(temp1) as avg_temp1, avg(temp2) as avg_temp2, avg(temp3) as avg_temp3, "
                    "       avg(rot1) as avg_rot1, avg(rot2) as avg_rot2, avg(rot3) as avg_rot3, "
                    "       avg(vc0) as avg_vc0, avg(vc1) as avg_vc1, avg(v33) as avg_v33, "
                    "       avg(v50p) as avg_v50p, avg(v50n) as avg_v50n, avg(v12p) as avg_v12p, avg(v12n) as avg_v12n, "
                    "       max(color) as max_color, avg(yellow) as avg_yellow, avg(red) as avg_red " 
                    "from   pr_hwstat_%s use index(probstat) "
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

  avg_temp1   = dbi_result_get_float(result, "avg_temp1");
  avg_temp2   = dbi_result_get_float(result, "avg_temp2");
  avg_temp3   = dbi_result_get_float(result, "avg_temp3");
  avg_rot1    = dbi_result_get_int(result, "avg_rot1");
  avg_rot2    = dbi_result_get_int(result, "avg_rot2");
  avg_rot3    = dbi_result_get_int(result, "avg_rot3");
  avg_vc0     = dbi_result_get_float(result, "avg_vc0");
  avg_vc1     = dbi_result_get_float(result, "avg_vc1");
  avg_v33     = dbi_result_get_float(result, "avg_v33");
  avg_v50p    = dbi_result_get_float(result, "avg_v50p");
  avg_v50n    = dbi_result_get_float(result, "avg_v50n");
  avg_v12p    = dbi_result_get_float(result, "avg_v12p");
  avg_v12n    = dbi_result_get_float(result, "avg_v12n");
  max_color   = dbi_result_get_int(result, "max_color");
  avg_yellow  = dbi_result_get_float(result, "avg_yellow");
  avg_red     = dbi_result_get_float(result, "avg_red");
  dbi_result_free(result);

  if (resummarize) {
    // delete old values
    result = db_query(t->probe->db, 0,
                    "delete from pr_hwstat_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    dbi_result_free(result);
  }

  result = db_query(t->probe->db, 0,
                    "insert into pr_hwstat_%s (temp1, temp2, temp3, rot1, rot2, rot3, vc0, vc1, v33, v50p, v50n, v12p, v12n, "
                    "                          probe, color, stattime, yellow, red, slot) "
                    "            values ('%f', '%f', '%f', '%u', '%u', '%u', '%f', '%f', '%f', '%f', '%f', '%f', '%f', "
                    "                    '%d', '%u', '%d', '%f', '%f', '%u')",
                    into, 
                    avg_temp1, avg_temp2, avg_temp3,
                    avg_rot1, avg_rot2, avg_rot3, 
                    avg_vc0, avg_vc1, avg_v33, avg_v50p, 
                    avg_v12p, avg_v12n, avg_v50n, 
                    def->probeid, max_color, stattime,
                    avg_yellow, avg_red, slot);
  dbi_result_free(result);
}

module hwstat_module  = {
  STANDARD_MODULE_STUFF(hwstat),
  NO_FREE_DEF,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  hwstat_get_from_xml,
  NO_ACCEPT_RESULT,
  NO_GET_DEF_FIELDS,
  NO_SET_DEF_FIELDS,
  hwstat_get_def,
  hwstat_adjust_result,
  NO_END_RESULT,
  NO_END_RUN,
  NO_EXIT,
  NO_FIND_DOMAIN,
  hwstat_store_raw_result,
  NO_NOTIFY_MAIL_SUBJECT_EXTRA,
  NO_NOTIFY_MAIL_BODY_PROBE_DEF,
  hwstat_summarize
};

