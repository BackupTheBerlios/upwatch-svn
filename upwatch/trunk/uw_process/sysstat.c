#include "config.h"
#include <string.h>
#include <generic.h>
#include "cmd_options.h"
#include "slot.h"
#include "uw_process.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct sysstat_result {
  STANDARD_PROBE_RESULT;
#include "../uw_sysstat/probe.res_h"
};

extern module sysstat_module;

static int accept_probe(module *probe, const char *name)
{
  return(strcmp(name, "sysstat") == 0);
}

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//*******************************************************************
static void get_from_xml(module *probe, xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns, void *probe_res)
{
  struct sysstat_result *res = (struct sysstat_result *)probe_res;

  if ((!xmlStrcmp(cur->name, (const xmlChar *) "loadavg")) && (cur->ns == ns)) {
    res->loadavg = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "user")) && (cur->ns == ns)) {
    res->user = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "system")) && (cur->ns == ns)) {
    res->system = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "idle")) && (cur->ns == ns)) {
    res->idle = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "swapin")) && (cur->ns == ns)) {
    res->swapin = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "swapout")) && (cur->ns == ns)) {
    res->swapout = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "blockin")) && (cur->ns == ns)) {
    res->blockin = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "blockout")) && (cur->ns == ns)) {
    res->blockout = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "swapped")) && (cur->ns == ns)) {
    res->swapped = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "free")) && (cur->ns == ns)) {
    res->free = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "buffered")) && (cur->ns == ns)) {
    res->buffered = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "cached")) && (cur->ns == ns)) {
    res->cached = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "used")) && (cur->ns == ns)) {
    res->used = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "systemp")) && (cur->ns == ns)) {
    res->systemp = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    return;
  }
}

//*******************************************************************
// find the real probeid from the ip address
// get it from the cache. if there but too old: delete
//*******************************************************************
static void *get_def(module *probe, void *probe_res)
{ 
  struct probe_def *def;
  struct sysstat_result *res = (struct sysstat_result *)probe_res;
  MYSQL_RES *result;
  MYSQL_ROW row;
  time_t now = time(NULL);

  def = g_hash_table_lookup(probe->cache, &res->server);
  if (def && def->stamp < now - (120 + uw_rand(240))) { // older then 2 - 6 minutes?
     g_hash_table_remove(probe->cache, &res->server);
     def = NULL;
  }
  // if not there retrieve from database and insert in hash
  if (def == NULL) {
    def = g_malloc0(sizeof(struct probe_result));
    def->stamp = time(NULL);
    def->server = res->server;
    
    result = my_query(probe->db, 0,
                      "select id, yellow, red "
                      "from   pr_%s_def "
                      "where  server = '%u'", res->name, res->server);
    if (!result) return(NULL);

    if (mysql_num_rows(result) == 0) { // DEF RECORD NOT FOUND
      mysql_free_result(result);
      if (!trust(res->name)) {
        LOG(LOG_NOTICE, "pr_%s_def id %u not found and not trusted - skipped", 
                         res->name, def->probeid);
        return(NULL);
      }
      result = my_query(probe->db, 0,
                        "insert into pr_%s_def set server = '%d', "
                        "        ipaddress = '127.0.0.1', description = 'auto-added by system'", 
                        res->name, res->server);
      mysql_free_result(result);
      if (mysql_affected_rows(probe->db) == 0) { // nothing was actually inserted
        LOG(LOG_NOTICE, "insert missing pr_%s_def id %u: %s", 
                         res->name, def->probeid, mysql_error(probe->db));
      }
      result = my_query(probe->db, 0,
                        "select id, yellow, red "
                        "from   pr_%s_def "
                        "where  server = '%u'", res->name, res->server);
      if (!result) return(NULL);
    }
    row = mysql_fetch_row(result); 
    if (!row || !row[0]) {
      LOG(LOG_NOTICE, "no pr_%s_def found for server %u - skipped", res->name, res->server);
      mysql_free_result(result);
      return(NULL);
    }
    if (row[0]) def->probeid  = atoi(row[0]);
    if (row[1]) def->yellow   = atoi(row[1]);
    if (row[2]) def->red      = atoi(row[2]);
    mysql_free_result(result);

    result = my_query(probe->db, 0,
                      "select color "
                      "from   pr_status "
                      "where  class = '%d' and probe = '%d'", probe->class, def->probeid);
    if (result) {
      row = mysql_fetch_row(result);
      if (row) {
        def->color  = atoi(row[0]); 
      }
      mysql_free_result(result);
    } 

    result = my_query(probe->db, 0,
                      "select stattime from pr_%s_raw use index(probstat) "
                      "where probe = '%u' order by stattime desc limit 1",
                       res->name, def->probeid);
    if (result && mysql_num_rows(result) > 0) {
      row = mysql_fetch_row(result);
      if (row && row[0]) {
        def->newest = atoi(row[0]);
      }
      mysql_free_result(result);
    }

    g_hash_table_insert(probe->cache, guintdup(res->server), def);
  }
  return(def);
}   


//*******************************************************************
// STORE RAW RESULTS
//*******************************************************************
static gint store_raw_result(struct _module *probe, void *probe_def, void *probe_res, guint *seen_before)
{
  MYSQL_RES *result;
  struct sysstat_result *res = (struct sysstat_result *)probe_res;
  struct probe_def *def = (struct probe_def *)probe_def;
  char *escmsg;

  *seen_before = FALSE;
  if (res->message) {
    escmsg = g_malloc(strlen(res->message) * 2 + 1);
    mysql_real_escape_string(probe->db, escmsg, res->message, strlen(res->message)) ;
  } else {
    escmsg = strdup("");
  }
    
  result = my_query(probe->db, 0,
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
  struct probe_def *def = (struct probe_def *)probe_def;
  gfloat avg_yellow, avg_red;
  gfloat avg_loadavg;
  guint avg_user, avg_system, avg_idle, avg_swapin, avg_swapout, avg_blockin;
  guint avg_blockout, avg_swapped, avg_free, avg_buffered, avg_cached, avg_used;
  gint avg_systemp;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = my_query(probe->db, 0,
                    "select avg(loadavg), avg(user), avg(system), avg(idle), "
                    "       avg(swapin), avg(swapout), avg(blockin), avg(blockout), "
                    "       avg(swapped), avg(free), avg(buffered), avg(cached), "
                    "       avg(used), avg(systemp), max(color), avg(yellow), avg(red) " 
                    "from   pr_sysstat_%s use index(probstat) "
                    "where  probe = '%d' and stattime >= %d and stattime < %d",
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
    LOG(LOG_WARNING, "NULL values found in summarizing from %s for probe %u %u %u", 
                      from, def->probeid, slotlow, slothigh);
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
    result = my_query(probe->db, 0,
                    "delete from pr_sysstat_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    mysql_free_result(result);
  }

  result = my_query(probe->db, 0,
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
  accept_probe,
  NO_XML_RESULT_NODE,
  get_from_xml,
  NO_FIX_RESULT,
  get_def,
  store_raw_result,
  summarize,
  NO_END_PROBE,
  NO_END_RUN
};

