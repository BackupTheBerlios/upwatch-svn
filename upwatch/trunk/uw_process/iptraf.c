#include "config.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <generic.h>
#include "cmd_options.h"
#include "slot.h"
#include "uw_process.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct iptraf_result {
  STANDARD_PROBE_RESULT;
  gchar ip[16];
  struct in_addr ipaddr;
#include "../uw_iptraf/probe.res_h"
};

struct iptraf_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
  float slotday_in_total;   // in-memory counters for the current slot in the 
  float slotday_out_total;  // pr_iptraf_day table. To speed things up a bit.
  guint slotday_max_color;  // same with color
  float slotday_avg_yellow; // 
  float slotday_avg_red;    // 
};

extern module iptraf_module;

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//*******************************************************************
static void xml_result_node(module *probe, xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns, void *probe_res)
{
  struct iptraf_result *res = (struct iptraf_result *)probe_res;
  char *p;

  p = xmlGetProp(cur, (const xmlChar *) "id");
  if (p) {
    strcpy(res->ip, p); 
    inet_aton(p, &res->ipaddr);
    xmlFree(p);
  }
}

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//*******************************************************************
static void get_from_xml(module *probe, xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns, void *probe_res)
{
  struct iptraf_result *res = (struct iptraf_result *)probe_res;

  if ((!xmlStrcmp(cur->name, (const xmlChar *) "in")) && (cur->ns == ns)) {
    res->in_total = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "out")) && (cur->ns == ns)) {
    res->out_total = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
    return;
  }
}

//*******************************************************************
// find the real probeid from the ip address
// get it from the cache. if there but too old: delete
//*******************************************************************
static void *get_def(module *probe, void *probe_res)
{
  struct iptraf_def *def;
  struct iptraf_result *res = (struct iptraf_result *)probe_res;
  MYSQL_RES *result;
  MYSQL_ROW row;

  def = g_hash_table_lookup(probe->cache, &res->ipaddr);
  if (def == NULL) {    // if not there construct from database and insert in hash
    gulong slotlow, slothigh;

    def = g_malloc0(sizeof(struct iptraf_def));
    def->stamp = time(NULL);

    result = my_query(probe->db, 0,
                      "select id, yellow, red "
                      "from   pr_%s_def "
                      "where  ipaddress = '%s'", res->name, res->ip);
    if (!result) return(NULL);

    if (mysql_num_rows(result) == 0) { // DEF RECORD NOT FOUND
      mysql_free_result(result);
      if (!trust(res->name)) {
        LOG(LOG_NOTICE, "pr_%s_def ip %s not found and not trusted - skipped",
                         res->name, res->ip);
        return(NULL);
      }
      result = my_query(probe->db, 0,
                        "insert into pr_%s_def set ipaddress = '%s', "
                        "        description = 'auto-added by system'",
                        res->name, res->ip);
      mysql_free_result(result);
      if (mysql_affected_rows(probe->db) == 0) { // nothing was actually inserted
        LOG(LOG_NOTICE, "insert missing pr_%s_def id %s: %s", 
                         res->name, res->ip, mysql_error(probe->db));
      }
      result = my_query(probe->db, 0,
                        "select id, yellow, red "
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
    def->yellow   = atof(row[1]);
    def->red      = atof(row[2]);
    mysql_free_result(result);

    result = my_query(probe->db, 0,
                      "select server, color, stattime "
                      "from   pr_status "
                      "where  class = '%d' and probe = '%d'", probe->class, def->probeid);
    if (result) {
      row = mysql_fetch_row(result);
      if (row) {
        def->server = atoi(row[0]);
        def->color  = atoi(row[1]);
        def->newest = atoi(row[2]); // just in case the next query fails
      }
      mysql_free_result(result);
    } else {
      LOG(LOG_NOTICE, "pr_status record for %s id %u not found", res->name, def->probeid);
    }

    if (!def->server) {
      // couldn't find pr_status record? Will be created later,
      // but get the server from the def record for now
      result = my_query(probe->db, 0,
                        "select server "
                        "from   pr_%s_def "
                        "where  id = '%u'", res->name, def->probeid);
      if (result) {
        row = mysql_fetch_row(result);
        if (row) def->server   = atoi(row[0]);
        mysql_free_result(result);
      }
    }

    uw_slot(SLOT_DAY, res->stattime, &slotlow, &slothigh);
    result = my_query(probe->db, 0,
                      "select sum(in_total), sum(out_total), max(color), avg(yellow), avg(red), "
                      "       max(stattime) "
                      "from   pr_iptraf_raw use index(probstat) "
                      "where  probe = '%u' and stattime >= '%u' and stattime < '%u'",
                      def->probeid, slotlow, slothigh);
    if (result) {
      row = mysql_fetch_row(result);
      if (row) {
        if (row[0]) def->slotday_in_total   = atoi(row[0]);
        if (row[1]) def->slotday_out_total  = atoi(row[1]);
        if (row[2]) def->slotday_max_color  = atoi(row[2]);
        if (row[3]) def->slotday_avg_yellow = atof(row[3]);
        if (row[4]) def->slotday_avg_red    = atof(row[4]);
        if (row[5]) def->newest             = atoi(row[5]);
      }
      mysql_free_result(result);
    } else {
      LOG(LOG_NOTICE, "raw record for %s id %u not found between %u and %u", 
                      res->name, def->probeid, slotlow, slothigh);
      def->slotday_avg_yellow = def->yellow;
      def->slotday_avg_red    = def->red;   
    }

    g_hash_table_insert(probe->cache, guintdup(*(unsigned *)&res->ipaddr), def);
  }
  if (res->color == 0) { // no color given in the result?
    float total = res->in_total + res->out_total;
    res->color = 200;
    if ((total*8)/60 > (def->yellow*1024)) {
      res->color = STAT_YELLOW;
    }
    if ((total*8)/60 > (def->red*1024)) {
      res->color = STAT_RED;
    }
  }
  return(def);
}

//*******************************************************************
// STORE RAW RESULTS
//*******************************************************************
static gint store_raw_result(struct _module *probe, void *probe_def, void *probe_res, guint *seen_before)
{
  MYSQL_RES *result;
  struct iptraf_result *res = (struct iptraf_result *)probe_res;
  struct iptraf_def *def = (struct iptraf_def *)probe_def;
    
  *seen_before = FALSE;
  def->slotday_in_total += res->in_total;
  def->slotday_out_total += res->out_total;
  if (res->color > def->slotday_max_color) {
    def->slotday_max_color = res->color;
  }

  result = my_query(probe->db, 0,
                    "insert into pr_iptraf_raw "
                    "set    probe = '%u', yellow = '%f', red = '%f', stattime = '%u', color = '%u', "
                    "       in_total = '%f', out_total = '%f'",
                    def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->in_total, res->out_total);
  if (result) mysql_free_result(result);
  if (mysql_errno(probe->db) == ER_DUP_ENTRY) {
    *seen_before = TRUE;
  } else if (mysql_errno(probe->db)) {
    return 0; // other failure
  }
  return 1; // success
}

//*******************************************************************
// SUMMARIZE A TABLE INTO AN OLDER PERIOD
//*******************************************************************
static void summarize(module *probe, void *probe_def, void *probe_res, char *from, char *into, guint slot, guint slotlow, guint slothigh, gint resummarize)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct iptraf_def *def = (struct iptraf_def *)probe_def;
  struct iptraf_result *res = (struct iptraf_result *)probe_res;
  float avg_yellow, avg_red;
  float in_total, out_total;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  if (strcmp(into, "day") == 0 && res->stattime > def->newest) {
    in_total = def->slotday_in_total;
    out_total = def->slotday_out_total;
    max_color = def->slotday_max_color;
    avg_yellow = def->slotday_avg_yellow;
    avg_red = def->slotday_avg_red;
    def->slotday_in_total = 0;
    def->slotday_out_total = 0;
    def->slotday_max_color = 0;
    def->slotday_avg_yellow = def->yellow;
    def->slotday_avg_red = def->red;
  } else {
    result = my_query(probe->db, 0,
                      "select sum(in_total), sum(out_total), max(color), avg(yellow), avg(red) "
                      "from   pr_iptraf_%s use index(probstat) "
                      "where  probe = '%u' and stattime >= '%u' and stattime < '%u'",
                      from, def->probeid, slotlow, slothigh);

    if (!result) return;
    if (mysql_num_rows(result) == 0) { // no records found
      LOG(LOG_WARNING, "nothing to summarize from %s for probe %u %u %u",
                         from, def->probeid, slotlow, slothigh);
      return;
    }

    row = mysql_fetch_row(result);
    if (!row) {
      mysql_free_result(result);
      LOG(LOG_ERR, mysql_error(probe->db));
      return;
    }
    if (row[0] == NULL) {
      LOG(LOG_WARNING, "nothing to summarize from %s for probe %u %u %u", 
                       from, def->probeid, slotlow, slothigh);
      return;
    }

    in_total    = atof(row[0]);
    out_total   = atof(row[1]);
    max_color   = atoi(row[2]);
    avg_yellow  = atof(row[3]);
    avg_red     = atof(row[4]);
    mysql_free_result(result);
  }

  if (resummarize) {
    // delete old values
    result = my_query(probe->db, 0,
                    "delete from pr_iptraf_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    mysql_free_result(result);
  }
  result = my_query(probe->db, 0,
                    "insert into pr_iptraf_%s "
                    "set    in_total = '%f', out_total = '%f', "
                    "       probe = '%u', color = '%u', stattime = '%u', "
                    "       yellow = '%f', red = '%f', slot = '%u'",
                    into, in_total, out_total, def->probeid, max_color, stattime, 
                    avg_yellow, avg_red, slot);
  mysql_free_result(result);
}

module iptraf_module  = {
  STANDARD_MODULE_STUFF(iptraf),
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  xml_result_node,
  get_from_xml,
  NULL,
  get_def,
  store_raw_result,
  summarize,
  NULL
};

