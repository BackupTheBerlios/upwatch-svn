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

char *query_server_by_ip;

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

  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "incoming")) && (t->cur->ns == t->ns)) {
    res->incoming = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "in")) && (t->cur->ns == t->ns)) {
    res->incoming = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "outgoing")) && (t->cur->ns == t->ns)) {
    res->outgoing = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "out")) && (t->cur->ns == t->ns)) {
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
  MYSQL_RES *result;
  MYSQL_ROW row;
  char buffer[10];
  time_t now = time(NULL);

  if (res->color == STAT_PURPLE) {
    result = my_query(t->probe->db, 0,
                      "select ipaddress from pr_%s_def where id = '%u'", res->name, res->probeid);
    if (!result) return(NULL);

    row = mysql_fetch_row(result);
    if (row && row[0]) {
      res->ipaddress = strdup(row[0]);
      inet_aton(res->ipaddress, &res->ipaddr);
    } else {
      LOG(LOG_NOTICE, "iptraf def %u not found", res->probeid);
      mysql_free_result(result);
      return(NULL);
    }
    mysql_free_result(result);
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

    result = my_query(t->probe->db, 0,
                      "select id, server, yellow, red, contact, hide, email, delay "
                      "from   pr_%s_def "
                      "where  ipaddress = '%s'", res->name, res->ipaddress);
    if (!result) return(NULL);

    if (mysql_num_rows(result) == 0) { // DEF RECORD NOT FOUND
      mysql_free_result(result);
      if (!create) {
        LOG(LOG_NOTICE, "pr_%s_def ip %s not found - skipped",
                         res->name, res->ipaddress);
        return(NULL);
      }
      // okay, create a new probe definition.
      // first try to find out the server id
      if (!query_server_by_ip) {
        LOG(LOG_ERR, "Don't know how to find a server by ip address");
        return(NULL);
      }
      result = my_query(t->probe->db, 0, query_server_by_ip, res->ipaddress, res->ipaddress,
                        res->ipaddress, res->ipaddress, res->ipaddress);
      if (!result) return(NULL);
      row = mysql_fetch_row(result);
      if (row && row[0]) {
        res->server   = atoi(row[0]);
      } else {
        LOG(LOG_NOTICE, "iptraf for ip %s added without server id", res->ipaddress);
        res->server = 1;
      }
      mysql_free_result(result);

      result = my_query(t->probe->db, 0,
                        "insert into pr_%s_def set ipaddress = '%s', server = '%u', "
                        "        description = 'auto-added by system'",
                        res->name, res->ipaddress, res->server);
      mysql_free_result(result);
      if (mysql_affected_rows(t->probe->db) == 0) { // nothing was actually inserted
        LOG(LOG_NOTICE, "insert missing pr_%s_def id %s: %s", 
                         res->name, res->ipaddress, mysql_error(t->probe->db));
      } else {
        LOG(LOG_NOTICE, "created pr_%s_def for ipaddress %s", 
                         res->name, res->ipaddress);
      }
      result = my_query(t->probe->db, 0,
                        "select id, server, yellow, red, contact, hide, email, delay "
                        "from   pr_%s_def "
                        "where  ipaddress = '%s'", res->name, res->ipaddress);
      if (!result) return(NULL);
    }
  
    row = mysql_fetch_row(result);
    if (!row) {
      mysql_free_result(result);
      return(NULL);
    }
    def->probeid  = atoi(row[0]);
    def->server   = atoi(row[1]);
    def->yellow   = atof(row[2]);
    def->red      = atof(row[3]);
    def->contact  = atof(row[4]);
    strcpy(def->hide, row[5] ? row[5] : "no");
    strcpy(def->email, row[6] ? row[6] : "");
    if (row[7]) def->delay = atoi(row[7]);

    mysql_free_result(result);

    result = my_query(t->probe->db, 0,
                      "select color "
                      "from   pr_status "
                      "where  class = '%d' and probe = '%d'", t->probe->class, def->probeid);
    if (result) {
      row = mysql_fetch_row(result);
      if (row) {
        def->color   = atoi(row[0]);
      } else {
        LOG(LOG_NOTICE, "pr_status record for %s id %u (%s) not found", res->name, def->probeid, res->ipaddress);
        mysql_free_result(result);
        result = my_query(t->probe->db, 0,
                          "insert into pr_status set class = '%d', probe = '%d', server = '%d'",
                          t->probe->class, def->probeid, def->server);
      }
      mysql_free_result(result);
    } else {
      LOG(LOG_NOTICE, "pr_status record for %s id %u (%s) not found", res->name, def->probeid, res->ipaddress);
    }

    result = my_query(t->probe->db, 0,
                      "select stattime from pr_%s_raw use index(probstat) "
                      "where probe = '%u' order by stattime desc limit 1",
                       res->name, def->probeid);
    if (result) {
      if (mysql_num_rows(result) > 0) {
        row = mysql_fetch_row(result);
        if (row && row[0]) {
          def->newest = atoi(row[0]);
        }
      }
      mysql_free_result(result);
    }

    uw_slot(SLOT_DAY, res->stattime, &slotlow, &slothigh);
    result = my_query(t->probe->db, 0,
                      "select sum(incoming), sum(outgoing), max(color), avg(yellow), avg(red) "
                      "from   pr_iptraf_raw use index(probstat) "
                      "where  probe = '%u' and stattime >= '%u' and stattime < '%u'",
                      def->probeid, slotlow, slothigh);
    if (result) {
      row = mysql_fetch_row(result);
      if (row) {
        if (row[0]) def->slotday_in   = atoi(row[0]);
        if (row[1]) def->slotday_out  = atoi(row[1]);
        if (row[2]) def->slotday_max_color  = atoi(row[2]);
        if (row[3]) def->slotday_avg_yellow = atof(row[3]);
        if (row[4]) def->slotday_avg_red    = atof(row[4]);
      }
      mysql_free_result(result);
    } else {
      LOG(LOG_NOTICE, "raw record for %s id %u not found between %u and %u", 
                      res->name, def->probeid, slotlow, slothigh);
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

