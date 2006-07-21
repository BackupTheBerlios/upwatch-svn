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
static gint httpget_store_raw_result(trx *t)
{
  dbi_result result;
  struct httpget_result *res = (struct httpget_result *)t->res;
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
                    "insert into pr_httpget_raw "
                    "set    probe = '%u', yellow = '%f', red = '%f', stattime = '%u', color = '%u', "
                    "       lookup = '%f', connect = '%f', pretransfer = '%f', total = '%f', "
                    "       message = '%s' ",
                    def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->lookup, res->connect, res->pretransfer, res->total, escmsg);
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
static void httpget_summarize(trx *t, char *from, char *into, guint slot, guint slotlow, guint slothigh, gint resummarize)
{
  dbi_result result;
  struct probe_def *def = (struct probe_def *)t->def;
  float avg_yellow, avg_red;
  float avg_lookup, avg_connect, avg_pretransfer, avg_total;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = db_query(t->probe->db, 0,
                    "select avg(lookup) as avg_lookup, avg(connect) as avg_connect, "
                    "       avg(pretransfer) as avg_pretransfer, avg(total) as avg_total, "
                    "       max(color) as max_color, avg(yellow) avg_yellow, avg(red) as avg_red "
                    "from   pr_httpget_%s use index(probstat) "
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
  if (dbi_result_get_string(result, "avg_lookup") == NULL) {
    LOG(LOG_NOTICE, "nothing to summarize from %s for probe %u %u %u", 
                       from, def->probeid, slotlow, slothigh);
    dbi_result_free(result);
    return;
  }

  avg_lookup = dbi_result_get_float(result, "avg_lookup");
  avg_connect = dbi_result_get_float(result, "avg_connect");
  avg_pretransfer = dbi_result_get_float(result, "avg_pretransfer");
  avg_total = dbi_result_get_float(result, "avg_total");
  max_color   = dbi_result_get_uint(result, "max_color");
  avg_yellow  = dbi_result_get_float(result, "avg_yellow");
  avg_red     = dbi_result_get_float(result, "avg_red");
  dbi_result_free(result);

  if (resummarize) {
    // delete old values
    result = db_query(t->probe->db, 0,
                    "delete from pr_httpget_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    dbi_result_free(result);
  }

  result = db_query(t->probe->db, 0,
                    "insert into pr_httpget_%s "
                    "set    lookup = '%f', connect = '%f', pretransfer = '%f', total = '%f', "
                    "       probe = %d, color = '%u', stattime = %d, "
                    "       yellow = '%f', red = '%f', slot = '%u'",
                    into, avg_lookup, avg_connect, avg_pretransfer, avg_total, def->probeid, 
                    max_color, stattime, avg_yellow, avg_red, slot);

  dbi_result_free(result);
}

module httpget_module  = {
  STANDARD_MODULE_STUFF(httpget),
  NO_FREE_DEF,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  httpget_get_from_xml,
  NO_ACCEPT_RESULT,
  NO_GET_DEF_FIELDS,
  NO_SET_DEF_FIELDS,
  NO_GET_DEF,
  NO_ADJUST_RESULT,
  NO_END_RESULT,
  NO_END_RUN,
  NO_EXIT,
  NO_FIND_DOMAIN,
  httpget_store_raw_result,
  NO_NOTIFY_MAIL_SUBJECT_EXTRA,
  NO_NOTIFY_MAIL_BODY_PROBE_DEF,
  httpget_summarize
};

