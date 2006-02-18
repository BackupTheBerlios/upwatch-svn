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
  MYSQL_RES *result;
  struct sysstat_result *res = (struct sysstat_result *)t->res;
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
                    "insert into pr_sysstat_raw "
                    "set    probe = '%u', yellow = '%f', red = '%f', stattime = '%u', color = '%u', "
                    "       loadavg = '%f', user = '%u', system = '%u', idle = '%u', "
                    "       swapin = '%u', swapout = '%u', blockin = '%u', blockout = '%u', "
                    "       swapped = '%u', free = '%u', buffered = '%u', cached = '%u', "
                    "       used = '%u', systemp = '%d', message = '%s'",
                    def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->loadavg,   res->user, res->system, res->idle,
                    res->swapin, res->swapout, res->blockin, res->blockout,
                    res->swapped, res->free, res->buffered, res->cached,
                    res->used, res->systemp, escmsg);
  g_free(escmsg);
  if (result) mysql_free_result(result);
  if (mysql_errno(t->probe->db) == ER_DUP_ENTRY) {
    t->seen_before = TRUE;
  } else if (mysql_errno(t->probe->db)) {
    LOG(LOG_WARNING, "%s:[%u] %s", "insert into pr_sysstat_raw", mysql_errno(t->probe->db), mysql_error(t->probe->db));
    return 0; // other failure
  }
  return 1; // success
}

//*******************************************************************
// SUMMARIZE A TABLE INTO AN OLDER PERIOD
//*******************************************************************
static void sysstat_summarize(trx *t, char *from, char *into, guint slot, guint slotlow, guint slothigh, gint resummarize)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct probe_def *def = (struct probe_def *)t->def;
  gfloat avg_yellow, avg_red;
  gfloat avg_loadavg;
  guint avg_user, avg_system, avg_idle, avg_swapin, avg_swapout, avg_blockin;
  guint avg_blockout, avg_swapped, avg_free, avg_buffered, avg_cached, avg_used;
  gint avg_systemp;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = my_query(t->probe->db, 0,
                    "select avg(loadavg), avg(user), avg(system), avg(idle), "
                    "       avg(swapin), avg(swapout), avg(blockin), avg(blockout), "
                    "       avg(swapped), avg(free), avg(buffered), avg(cached), "
                    "       avg(used), avg(systemp), max(color), avg(yellow), avg(red) " 
                    "from   pr_sysstat_%s use index(probstat) "
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

  avg_loadavg = atof(row[0]);
  avg_user    = atoi(row[1]);
  avg_system  = atoi(row[2]);
  avg_idle    = atoi(row[3]);
  avg_swapin  = atoi(row[4]);
  avg_swapout = atoi(row[5]);
  avg_blockin = atoi(row[6]);
  avg_blockout= atoi(row[7]);
  avg_swapped = atoi(row[8]);
  avg_free    = atoi(row[9]);
  avg_buffered= atoi(row[10]);
  avg_cached  = atoi(row[11]);
  avg_used    = atoi(row[12]);
  avg_systemp = atoi(row[13]);
  max_color   = atoi(row[14]);
  avg_yellow  = atof(row[15]);
  avg_red     = atof(row[16]);
  mysql_free_result(result);

  if (resummarize) {
    // delete old values
    result = my_query(t->probe->db, 0,
                    "delete from pr_sysstat_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    mysql_free_result(result);
  }

  result = my_query(t->probe->db, 0,
                    "insert into pr_sysstat_%s " 
                    "set    loadavg = '%f', user = '%u', system = '%u', idle = '%u', "
                    "       swapin = '%u', swapout = '%u', blockin = '%u', blockout = '%u', "
                    "       swapped = '%u', free = '%u', buffered = '%u', cached = '%u', "
                    "       used = '%u', systemp = '%d', probe = %d, color = '%u', stattime = %d, "
                    "       yellow = '%f', red = '%f', slot = '%u'",
                    into, 
                    avg_loadavg, avg_user, avg_system, avg_idle, 
                    avg_swapin, avg_swapout, avg_blockin, avg_blockout, 
                    avg_swapped, avg_free, avg_buffered, avg_cached, 
                    avg_used, avg_systemp, def->probeid, max_color, stattime,
                    avg_yellow, avg_red, slot);
  mysql_free_result(result);
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

