#include "config.h"
#include <string.h>
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
  sprintf(buf, "(DEFAULT, '%u', '%f', '%f', '0', '%u', '%u', '%f', '%f', '')",
               def->probeid, def->yellow, def->red, res->stattime, res->color,
               res->incoming, res->outgoing);

  if (HAVE_OPT(MULTI_VALUE_INSERTS)) {
    mod_ic_add(t->probe, "pr_iptraf_raw", strdup(buf));
  } else {
    MYSQL_RES *result;

    result = my_query(t->probe->db, 0, "insert into pr_iptraf_raw values %s", buf);
    if (result) mysql_free_result(result);
    if (mysql_errno(t->probe->db) == ER_DUP_ENTRY) {
      t->seen_before = TRUE;
    } else if (mysql_errno(t->probe->db)) {
      return 0; // other failure
    }
  }
  return 1; // success
}

//*******************************************************************
// SUMMARIZE A TABLE INTO AN OLDER PERIOD
//*******************************************************************
static void iptraf_summarize(trx *t, char *from, char *into, guint slot, guint slotlow, guint slothigh, gint resummarize)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
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
    result = my_query(t->probe->db, 0,
                      "select sum(incoming), sum(outgoing), max(color), avg(yellow), avg(red) "
                      "from   pr_iptraf_%s use index(probstat) "
                      "where  probe = '%u' and stattime >= '%u' and stattime < '%u'",
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

    incoming    = atof(row[0]);
    outgoing    = atof(row[1]);
    max_color   = atoi(row[2]);
    avg_yellow  = atof(row[3]);
    avg_red     = atof(row[4]);
    mysql_free_result(result);
  }

  if (resummarize) {
    // delete old values
    result = my_query(t->probe->db, 0,
                    "delete from pr_iptraf_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    mysql_free_result(result);
  }
  result = my_query(t->probe->db, 0,
                    "insert into pr_iptraf_%s "
                    "set    incoming = '%f', outgoing = '%f', "
                    "       probe = '%u', color = '%u', stattime = '%u', "
                    "       yellow = '%f', red = '%f', slot = '%u'",
                    into, incoming, outgoing, def->probeid, max_color, stattime, 
                    avg_yellow, avg_red, slot);
  mysql_free_result(result);
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
  iptraf_get_def,
  iptraf_adjust_result,
  NO_END_RESULT,
  iptraf_end_run, 
  NO_EXIT,
  NO_FIND_DOMAIN,
  iptraf_store_raw_result,
  NO_NOTIFY_MAIL_SUBJECT,
  iptraf_summarize
};


