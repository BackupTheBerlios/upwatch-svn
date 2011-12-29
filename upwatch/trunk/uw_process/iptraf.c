#include "config.h"
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <generic.h>
#include "slot.h"
#include "uw_process_glob.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static void iptraf_end_run(module *probe)
{
  mod_ic_flush(probe, "pr_iptraf_raw");
}

// this hook abused for setting the query string find-server-by-ip
int iptraf_accept_result(trx *t)
{
  int i;

  query_server_by_ip = NULL;

  if (t->res->realm == NULL) {
    query_server_by_ip = dblist[0].srvrbyip;
    return 1;
  }

  for (i=0; i < dblist_cnt; i++) {
    if (strcmp(t->res->realm, dblist[i].realm)) continue;
    query_server_by_ip = dblist[i].srvrbyip;
    break;
  }
  return 1;
}

//*******************************************************************
// STORE RAW RESULTS
//*******************************************************************
static gint iptraf_store_raw_result(trx *t)
{
  char buf[256];
  struct iptraf_result *res = (struct iptraf_result *)t->res;
  struct iptraf_def *def = (struct iptraf_def *)t->def;
    
  if (t->res->color == STAT_PURPLE) return 1;
  t->seen_before = FALSE;
  def->slotday_in += res->incoming;
  def->slotday_out += res->outgoing;
  if (res->color > def->slotday_max_color) {
    def->slotday_max_color = res->color;
  }
  sprintf(buf, "(DEFAULT, '%u', '%f', '%f', '%u', '%u', '%f', '%f', '')",
               def->probeid, def->yellow, def->red, res->stattime, res->color,
               res->incoming, res->outgoing);

  if (HAVE_OPT(MULTI_VALUE_INSERTS)) {
    mod_ic_add(t->probe, "pr_iptraf_raw", strdup(buf));
  } else {
    dbi_result result;
    const char *errmsg;

    result = db_query(t->probe->db, 0, "insert into pr_iptraf_raw "
                                       "            (id, probe, yellow, red, stattime, color, incoming, outgoing) "
                                       "            values %s", buf);
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
  return 1; // success
}

//*******************************************************************
// SUMMARIZE A TABLE INTO AN OLDER PERIOD
//*******************************************************************
static void iptraf_summarize(trx *t, char *from, char *into, guint slot, guint slotlow, guint slothigh, gint resummarize)
{
  dbi_result result;
  struct iptraf_def *def = (struct iptraf_def *)t->def;
  struct iptraf_result *res = (struct iptraf_result *)t->res;
  float avg_yellow, avg_red;
  float incoming, outgoing;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  if (strcmp(into, "day") == 0 && res->stattime > def->newest) {
    incoming = def->slotday_in;
    outgoing = def->slotday_out;
    max_color = def->slotday_max_color;
    avg_yellow = def->slotday_avg_yellow;
    avg_red = def->slotday_avg_red;
    def->slotday_in = 0;
    def->slotday_out = 0;
    def->slotday_max_color = 0;
    def->slotday_avg_yellow = def->yellow;
    def->slotday_avg_red = def->red;
  } else {
    result = db_query(t->probe->db, 0,
                      "select sum(incoming) as incoming, sum(outgoing) as outgoing, max(color) as max_color, "
                      "       avg(yellow) as avg_yellow, avg(red) as avg_red "
                      "from   pr_iptraf_%s use index(probstat) "
                      "where  probe = '%u' and stattime >= '%u' and stattime < '%u'",
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
      LOG(LOG_NOTICE, "nothing to summarize from %s for probe %u %u %u", 
                       from, def->probeid, slotlow, slothigh);
      dbi_result_free(result);
      return;
    }

    incoming    = dbi_result_get_float(result, "incoming");
    outgoing    = dbi_result_get_float(result, "outgoing");
    max_color   = dbi_result_get_int(result, "max_color");
    avg_yellow  = dbi_result_get_float(result, "avg_yellow");
    avg_red     = dbi_result_get_float(result, "avg_red");
    dbi_result_free(result);
  }

  if (resummarize) {
    // delete old values
    result = db_query(t->probe->db, 0,
                    "delete from pr_iptraf_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    dbi_result_free(result);
  }
  result = db_query(t->probe->db, 0,
                    "insert into pr_iptraf_%s (incoming, outgoing, probe, color, stattime, yellow, red, slot) "
                    "            values ('%f', '%f', '%u', '%u', '%u', '%f', '%f', '%u')",
                    into, incoming, outgoing, def->probeid, max_color, stattime, 
                    avg_yellow, avg_red, slot);
  dbi_result_free(result);
}

module iptraf_module  = {
  STANDARD_MODULE_STUFF(iptraf),
  NO_FREE_DEF,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  iptraf_xml_result_node,
  iptraf_get_from_xml,
  iptraf_accept_result,
  NO_GET_DEF_FIELDS,
  NO_SET_DEF_FIELDS,
  iptraf_get_def,
  iptraf_adjust_result,
  NO_END_RESULT,
  iptraf_end_run, 
  NO_EXIT,
  NO_FIND_DOMAIN,
  iptraf_store_raw_result,
  NO_NOTIFY_MAIL_SUBJECT_EXTRA,
  NO_NOTIFY_MAIL_BODY_PROBE_DEF,
  iptraf_summarize
};


