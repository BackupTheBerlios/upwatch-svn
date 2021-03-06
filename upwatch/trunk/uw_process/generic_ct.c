#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#include "uw_process_glob.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct ct_result {
  STANDARD_PROBE_RESULT;
  float connect;	/* time for connection to complete */
  float total;		/* total time needed */
};

//*******************************************************************
// STORE RAW RESULTS
//*******************************************************************
gint ct_store_raw_result(trx *t)
{
  dbi_result result;
  struct ct_result *res = (struct ct_result *)t->res;
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
                    "insert into pr_%s_raw (probe, yellow, red, stattime, color, connect, total, message)"
                    "            values ('%u', '%f', '%f', '%u', '%u', '%f', '%f', '%s')",
                    res->name, def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->connect, res->total, escmsg);
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
void ct_summarize(trx *t, char *from, char *into, guint slot, guint slotlow, guint slothigh, gint resummarize)
{
  dbi_result result;
  struct ct_result *res = (struct ct_result *)t->res;
  struct probe_def *def = (struct probe_def *)t->def;
  float avg_yellow, avg_red;
  float avg_connect, avg_total;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = db_query(t->probe->db, 0,
                    "select avg(connect) as avg_connect, avg(total) as avg_total, "
                    "       max(color) as max_color, avg(yellow) as avg_yellow, avg(red) as avg_red "
                    "from   pr_%s_%s use index(probstat) "
                    "where  probe = '%d' and stattime >= %d and stattime < %d",
                    res->name, from, def->probeid, slotlow, slothigh);

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
    LOG(LOG_NOTICE, "nothing to summarize from %s for probe %u %u %u", 
                       from, def->probeid, slotlow, slothigh);
    dbi_result_free(result);
    return;
  }

  avg_connect = dbi_result_get_float(result, "avg_connect");
  avg_total   = dbi_result_get_float(result, "avg_total");
  max_color   = dbi_result_get_int(result, "max_color");
  avg_yellow  = dbi_result_get_float(result, "avg_yellow");
  avg_red     = dbi_result_get_float(result, "avg_red");
  dbi_result_free(result);

  if (resummarize) {
    // delete old values
    result = db_query(t->probe->db, 0,
                    "delete from pr_%s_%s where probe = '%u' and stattime = '%u'",
                    res->name, into, def->probeid, stattime);
    dbi_result_free(result);
  }

  result = db_query(t->probe->db, 0,
                    "insert into pr_%s_%s (connect, total, probe, color, stattime, yellow, red, slot)"
                    "            values ('%f', '%f', '%d', '%u', '%d', '%f', '%f', '%u')",
                    res->name, into, avg_connect, avg_total, def->probeid, 
                    max_color, stattime, avg_yellow, avg_red, slot);

  dbi_result_free(result);
}

