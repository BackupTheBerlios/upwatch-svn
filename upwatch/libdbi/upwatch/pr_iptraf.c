#include "config.h"
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <generic.h>
#include "slot.h"
#define _UPWATCH
#include "probe.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

const char *query_server_by_ip;

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//*******************************************************************
void iptraf_xml_result_node(trx *t)
{
  struct iptraf_result *res = (struct iptraf_result *)t->res;

  if (res->ipaddress) {
    inet_aton(res->ipaddress, &res->ipaddr);
  }
}

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//*******************************************************************
void iptraf_get_from_xml(trx *t)
{
  struct iptraf_result *res = (struct iptraf_result *)t->res;

  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "incoming")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->incoming = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "in")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->incoming = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "outgoing")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->outgoing = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "out")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->outgoing = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
}

//*******************************************************************
// find the real probeid from the ip address
// get it from the cache. if there but too old: delete
// the cache is used so we don't have to issue an SQL query for
// every probe we process.
//*******************************************************************
void *iptraf_get_def(trx *t, int create)
{
  struct iptraf_def *def;
  struct iptraf_result *res = (struct iptraf_result *)t->res;
  dbi_result result;
  char buffer[10];
  time_t now = time(NULL);

  if (res->color == STAT_PURPLE) {
    result = db_query(t->probe->db, 0,
                      "select ipaddress from pr_%s_def where id = '%u'", res->name, res->probeid);
    if (!result) return(NULL);

    if (dbi_result_get_numrows(result) == 0) {
      LOG(LOG_NOTICE, "%s:%u@%s: iptraf def %u not found", res->realm, res->stattime, t->fromhost, res->probeid);
      dbi_result_free(result);
      delete_pr_status(t, res->probeid);
      return(NULL);
    }

    dbi_result_next_row(result);
    strcpy(res->ipaddress, dbi_result_get_string(result, "ipaddress"));
    inet_aton(res->ipaddress, &res->ipaddr);
    dbi_result_free(result);
  }

  def = g_hash_table_lookup(t->probe->cache, &res->ipaddr);
  if (def && def->stamp < now - (120 + uw_rand(240))) { // older then 2 - 6 minutes?
     g_hash_table_remove(t->probe->cache, &res->ipaddr);
     def = NULL;
  }
  if (def == NULL) {    // if not there construct from database and insert in hash
    gulong slotlow, slothigh;

    def = g_malloc0(t->probe->def_size);
    def->stamp = now;
    strcpy(def->hide, "no");

    result = db_query(t->probe->db, 0,
                      "select id, server, yellow, red, contact, hide, email, sms, delay, pgroup "
                      "from   pr_%s_def "
                      "where  ipaddress = '%s'", res->name, res->ipaddress);
    if (!result) return(NULL);

    if (dbi_result_get_numrows(result) == 0) { // DEF RECORD NOT FOUND
      dbi_result_free(result);
      if (!create) {
        LOG(LOG_NOTICE, "%s:%u@%s: pr_%s_def ip %s not found - skipped",
                         res->realm, res->stattime, t->fromhost, res->name, res->ipaddress);
        return(NULL);
      }
      // okay, create a new probe definition.
      // first try to find out the server id
      if (!query_server_by_ip) {
        LOG(LOG_ERR, "No SQL query for how to find a server by ip address");
        return(NULL);
      }
      result = db_query(t->probe->db, 0, query_server_by_ip, res->ipaddress, res->ipaddress,
                        res->ipaddress, res->ipaddress, res->ipaddress);
      if (!result) return(NULL);

      if (dbi_result_get_numrows(result) == 0) {
        LOG(LOG_NOTICE, "%s:%u@%s: iptraf for ip %s added without server id", 
            res->realm, res->stattime, t->fromhost, res->ipaddress);
        res->server = 1;
      } else {
        dbi_result_next_row(result);
        res->server = dbi_result_get_uint_idx(result, 0);
        dbi_result_free(result);
      }

      result = db_query(t->probe->db, 0,
                        "insert into pr_%s_def set ipaddress = '%s', server = '%u', "
                        "        description = 'auto-added by system'",
                        res->name, res->ipaddress, res->server);
      dbi_result_free(result);
      if (dbi_result_get_numrows_affected(t->probe->db) == 0) { // nothing was actually inserted
        const char *errmsg;
        dbi_conn_error(t->probe->db, &errmsg);
        LOG(LOG_NOTICE, "%s:%u@%s: insert missing pr_%s_def id %s: %s", 
                         res->realm, res->stattime, t->fromhost, 
                         res->name, res->ipaddress, errmsg);
      } else {
        LOG(LOG_NOTICE, "%s:%u@%s: created pr_%s_def for ipaddress %s", 
                         res->realm, res->stattime, t->fromhost,
                         res->name, res->ipaddress);
      }
      result = db_query(t->probe->db, 0,
                        "select id, server, yellow, red, contact, hide, email, sms, lay, pgroup "
                        "from   pr_%s_def "
                        "where  ipaddress = '%s'", res->name, res->ipaddress);
      if (!result) return(NULL);
    }
  
    if (dbi_result_get_numrows(result) == 0) {
      dbi_result_free(result);
      return(NULL);
    }
    dbi_result_next_row(result);

    def->probeid = dbi_result_get_uint(result, "id");
    def->server = dbi_result_get_float(result, "server");
    def->yellow = dbi_result_get_float(result, "yellow");
    def->red = dbi_result_get_float(result, "red");
    def->contact = dbi_result_get_uint(result, "contact");
    strcpy(def->hide, dbi_result_get_string(result, "hide"));
    strcpy(def->email, dbi_result_get_string(result, "email"));
    strcpy(def->sms, dbi_result_get_string(result, "sms"));
    def->delay = dbi_result_get_uint(result, "delay");
    def->pgroup = dbi_result_get_uint(result, "pgroup");
    dbi_result_free(result);

    result = db_query(t->probe->db, 0,
                      "select color "
                      "from   pr_status "
                      "where  class = '%d' and probe = '%d'", t->probe->class, def->probeid);
    if (result) {
      if (dbi_result_get_numrows(result) == 0) {
        LOG(LOG_NOTICE, "%s:%u@%s: pr_status record for %s id %u (%s) not found", 
            res->realm, res->stattime, t->fromhost, res->name, def->probeid, res->ipaddress);
        dbi_result_free(result);
        result = db_query(t->probe->db, 0,
                          "insert into pr_status set class = '%d', probe = '%d', server = '%d'",
                          t->probe->class, def->probeid, def->server);
      } else {
        dbi_result_next_row(result);
        def->color   = dbi_result_get_uint(result, "color");
      }
      dbi_result_free(result);
    }
    if (!def->color) def->color = res->color;

    result = db_query(t->probe->db, 0,
                      "select stattime from pr_%s_raw use index(probstat) "
                      "where probe = '%u' order by stattime desc limit 1",
                       res->name, def->probeid);
    if (result) {
      if (dbi_result_get_numrows(result) > 0) {
        dbi_result_next_row(result);
        def->newest  = dbi_result_get_uint(result, "stattime");
        dbi_result_free(result);
      }
    }

    uw_slot(SLOT_DAY, res->stattime, &slotlow, &slothigh);
    result = db_query(t->probe->db, 0,
                      "select sum(incoming) as slotday_in, sum(outgoing) as slotday_out, "
                      "       max(color) as slotday_max_color, avg(yellow) as slotday_avg_yellow, "
                      "       avg(red) as slotday_avg_red "
                      "from   pr_iptraf_raw use index(probstat) "
                      "where  probe = '%u' and stattime >= '%u' and stattime < '%u'",
                      def->probeid, slotlow, slothigh);
    if (result) {
      if (dbi_result_get_numrows(result) > 0) {
        dbi_result_next_row(result);
        def->slotday_in  = dbi_result_get_uint(result, "slotday_in");
        def->slotday_out = dbi_result_get_uint(result, "slotday_out");
        def->slotday_max_color = dbi_result_get_uint(result, "slotday_max_color");
        def->slotday_avg_yellow = dbi_result_get_uint(result, "slotday_avg_yellow");
        def->slotday_avg_red = dbi_result_get_uint(result, "slotday_avg_red");
        dbi_result_free(result);
      } else {
        LOG(LOG_NOTICE, "%s:%u@%s: raw record for %s id %u not found between %u and %u", 
                        res->realm, res->stattime, t->fromhost, res->name, def->probeid, slotlow, slothigh);
      }
    }
    if (def->slotday_avg_yellow == 0) {
      def->slotday_avg_yellow = def->yellow;
    }
    if (def->slotday_avg_red == 0) {
      def->slotday_avg_red    = def->red;   
    }

    g_hash_table_insert(t->probe->cache, guintdup(*(unsigned *)&res->ipaddr), def);
  }
  res->probeid = def->probeid;
  sprintf(buffer, "%u", res->color);
  set_result_value(t, "color", buffer);
  return(def);
}

//*******************************************************************
// Adjust the result. In this case this means:
// don't trust the color we receive from the probe
//*******************************************************************
void iptraf_adjust_result(trx *t)
{
  char buffer[10];
  struct iptraf_result *res = (struct iptraf_result *)t->res;

  if (res->color == STAT_PURPLE) return;

  if (res->color == 0) { // no color given in the result?
    guint largest = (res->incoming > res->outgoing ? res->incoming : res->outgoing) / 8;
    res->color = STAT_GREEN;
    if (largest/(res->interval?res->interval:1) > (t->def->yellow*1024)) {
      res->color = STAT_YELLOW;
    }
    if (largest/(res->interval?res->interval:1) > (t->def->red*1024)) {
      LOG(LOG_DEBUG, "%u(%u): (%u/8)/%u (=%u) > %u", res->probeid, res->stattime, largest, res->interval, 
          largest/(res->interval?res->interval:1), t->def->red*1024);
      res->color = STAT_RED;
    }
    sprintf(buffer, "%u", res->color);
    set_result_value(t, "color", buffer);
  }
}

