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
  MYSQL_RES *result;
  struct hwstat_result *res = (struct hwstat_result *)t->res;
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
  if (result) mysql_free_result(result);
  if (mysql_errno(t->probe->db) == ER_DUP_ENTRY) {
    t->seen_before = TRUE;
  } else if (mysql_errno(t->probe->db)) {
    return 0; // other failure
  }
  return 1; // success
}

//*******************************************************************
// SUMMARIZE A TABLE INTO AN OLDER PERIOD
//*******************************************************************
static void hwstat_summarize(trx *t, char *from, char *into, guint slot, guint slotlow, guint slothigh, gint resummarize)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct probe_def *def = (struct probe_def *)t->def;
  gfloat avg_yellow, avg_red;
  gfloat avg_temp1, avg_temp2, avg_temp3;
  guint avg_rot1, avg_rot2, avg_rot3;
  gfloat avg_vc0, avg_vc1, avg_v33,  avg_v50p,  avg_v12p,  avg_v12n, avg_v50n;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = my_query(t->probe->db, 0,
                    "select avg(temp1), avg(temp2), avg(temp3) "
                    "       avg(rot1), avg(rot2), avg(rot3), "
                    "       avg(vc0), avg(vc1), avg(v33), avg(v50p), "
                    "       avg(v50n), avg(v50n), max(color), avg(yellow), avg(red) " 
                    "from   pr_hwstat_%s use index(probstat) "
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
    LOG(LOG_NOTICE, "NULL values found in summarizing from %s for probe %u %u %u", 
                      from, def->probeid, slotlow, slothigh);
    mysql_free_result(result);
    return;
  }

  avg_temp1   = atof(row[0]);
  avg_temp2   = atof(row[1]);
  avg_temp3   = atof(row[2]);
  avg_rot1    = atoi(row[3]);
  avg_rot2    = atoi(row[4]);
  avg_rot3    = atoi(row[5]);
  avg_vc0     = atof(row[6]);
  avg_vc1     = atof(row[7]);
  avg_v33     = atof(row[8]);
  avg_v50p    = atof(row[9]);
  avg_v12p    = atof(row[10]);
  avg_v12n    = atof(row[11]);
  avg_v50n    = atof(row[12]);
  max_color   = atoi(row[13]);
  avg_yellow  = atof(row[14]);
  avg_red     = atof(row[15]);
  mysql_free_result(result);

  if (resummarize) {
    // delete old values
    result = my_query(t->probe->db, 0,
                    "delete from pr_hwstat_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    mysql_free_result(result);
  }

  result = my_query(t->probe->db, 0,
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
  mysql_free_result(result);
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
  hwstat_get_def,
  hwstat_adjust_result,
  NO_END_RESULT,
  NO_END_RUN,
  NO_EXIT,
  NO_FIND_DOMAIN,
  hwstat_store_raw_result,
  NO_NOTIFY_MAIL_SUBJECT,
  hwstat_summarize
};

