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
  MYSQL_RES *result;
  struct bb_cpu_result *res = (struct bb_cpu_result *)t->res;
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
                    "insert into pr_bb_cpu_raw "
                    "set    probe = '%u', stattime = '%u', color = '%u', loadavg = '%f', "
                    "       user = '%u',  idle = '%u', free = '%u', used = '%u', message = '%s'",
                    def->probeid, res->stattime, res->color, res->loadavg,
                    res->user, res->idle, res->free, res->used, escmsg);
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
static void bb_cpu_summarize(trx *t, char *from, char *into, guint slot, guint slotlow, guint slothigh, gint resummarize)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct probe_def *def = (struct probe_def *)t->def;
  float avg_yellow, avg_red;
  gfloat avg_loadavg;
  guint avg_user, avg_system, avg_idle;
  guint avg_swapped, avg_free, avg_buffered, avg_cached, avg_used;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = my_query(t->probe->db, 0,
                    "select avg(loadavg), avg(user), avg(system), avg(idle), "
                    "       avg(swapped), avg(free), avg(buffered), avg(cached), "
                    "       avg(used), max(color), avg(yellow), avg(red) " 
                    "from   pr_bb_cpu_%s use index(probstat) "
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
    LOG(LOG_NOTICE, "nothing to summarize from %s for probe %u %u %u", 
                       from, def->probeid, slotlow, slothigh);
    mysql_free_result(result);
    return;
  }

  avg_loadavg = atof(row[0]);
  avg_user    = atoi(row[1]);
  avg_system  = atoi(row[2]);
  avg_idle    = atoi(row[3]);
  avg_swapped = atoi(row[4]);
  avg_free    = atoi(row[5]);
  avg_buffered= atoi(row[6]);
  avg_cached  = atoi(row[7]);
  avg_used    = atoi(row[8]);
  max_color   = atoi(row[9]);
  avg_yellow  = atof(row[10]);
  avg_red     = atof(row[11]);
  mysql_free_result(result);

  if (resummarize) {
    // delete old values
    result = my_query(t->probe->db, 0,
                    "delete from pr_bb_cpu_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    mysql_free_result(result);
  }
  result = my_query(t->probe->db, 0,
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
  mysql_free_result(result);
}

module bb_cpu_module  = {
  STANDARD_MODULE_STUFF(bb_cpu),
  NO_FREE_DEF,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  NO_GET_FROM_XML,
  bb_cpu_accept_result,    
  bb_cpu_get_def,
  NO_ADJUST_RESULT,
  NO_END_RESULT,
  NO_END_RUN,
  NO_EXIT,
  bb_cpu_find_realm,
  bb_cpu_store_raw_result,
  NO_NOTIFY_MAIL_SUBJECT,
  bb_cpu_summarize
};

