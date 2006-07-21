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
  int lasterr;

  if (t->res->color == STAT_PURPLE) return 1;
  t->seen_before = FALSE;
  if (res->message) {
    escmsg = strdup(res->message);
    dbi_conn_quote_string(t->probe->db, &escmsg);
  } else {
    escmsg = strdup("");
  }
    
  result = db_query(t->probe->db, 0,
                    "insert into pr_hwstat_raw "
                    "set    probe = '%u', yellow = '%f', red = '%f', stattime = '%u', color = '%u', "
                    "       temp1 = '%f', temp2 = '%f', temp3 = '%f', "
                    "       rot1 = '%d', rot2 = '%d', rot3 = '%d', "
                    "       vc0 = '%f', vc1 = '%f', v33 = '%f', " 
                    "       v50p = '%f', v12p = '%f', v12n = '%f', v50n = '%f', "
                    "       message = '%s'",
                    def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->temp1, res->temp2, res->temp3, 
                    res->rot1, res->rot2, res->rot3, 
                    res->vc0, res->vc1, res->v33, 
                    res->v50p, res->v12p, res->v12n, res->v50n, 
                    escmsg);
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
                    "       avg(vc0) as avg_vc0, avg(vc1) as avg_vc1, avg(v33) as avg_v33, avg(v50p) as avg_v50p, "
                    "       avg(v12p) as avg_v12p, avg(v12n) as avg_v12n, avg(v50n) as avg_v50n, "
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
  if (dbi_result_get_string(result, "avg_temp1") == NULL) {
    LOG(LOG_NOTICE, "NULL values found in summarizing from %s for probe %u %u %u", 
                      from, def->probeid, slotlow, slothigh);
    dbi_result_free(result);
    return;
  }

  avg_temp1   = dbi_result_get_float(result, "avg_temp1");
  avg_temp2   = dbi_result_get_float(result, "avg_temp2");
  avg_temp3   = dbi_result_get_float(result, "avg_temp3");
  avg_rot1    = dbi_result_get_uint(result, "avg_rot1");
  avg_rot2    = dbi_result_get_uint(result, "avg_rot2");
  avg_rot3    = dbi_result_get_uint(result, "avg_rot3");
  avg_vc0     = dbi_result_get_float(result, "avg_vc0");
  avg_vc1     = dbi_result_get_float(result, "avg_vc1");
  avg_v33     = dbi_result_get_float(result, "avg_v33");
  avg_v50p    = dbi_result_get_float(result, "avg_v50p");
  avg_v12p    = dbi_result_get_float(result, "avg_v12p");
  avg_v12n    = dbi_result_get_float(result, "avg_v12n");
  avg_v50n    = dbi_result_get_float(result, "avg_yellow");
  max_color   = dbi_result_get_uint(result, "max_color");
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
                    "insert into pr_hwstat_%s " 
                    "set    temp1 = '%f', temp2 = '%f', temp3 = '%f', "
                    "       rot1 = '%u', rot2 = '%u', rot3 = '%u', "
                    "       vc0 = '%f', vc1 = '%f', v33 = '%f', v50p = '%f', "
                    "       v12p = '%f', v12n = '%f', v50n = '%f', "
                    "       probe = %d, color = '%u', stattime = %d, "
                    "       yellow = '%f', red = '%f', slot = '%u'",
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

