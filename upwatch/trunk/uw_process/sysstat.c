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
  gfloat loadavg;
  guint user;
  guint system;
  guint idle;
  guint swapin;
  guint swapout;
  guint blockin;
  guint blockout;
  guint swapped;
  guint free;
  guint buffered;
  guint cached;
  guint used;
};

extern module sysstat_module;

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//*******************************************************************
static void *extract_info_from_xml_node(module *probe, xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns)
{
  struct sysstat_result *res;

  res = g_malloc0(sizeof(struct sysstat_result));
  if (res == NULL) {
    return(NULL);
  }

  res->server = xmlGetPropInt(cur, (const xmlChar *) "server");
  res->stattime = xmlGetPropUnsigned(cur, (const xmlChar *) "date");
  res->expires = xmlGetPropUnsigned(cur, (const xmlChar *) "expires");
  for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
    char *p;

    if (xmlIsBlankNode(cur)) continue;
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "color")) && (cur->ns == ns)) {
      res->color = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "loadavg")) && (cur->ns == ns)) {
      res->loadavg = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "user")) && (cur->ns == ns)) {
      res->user = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "system")) && (cur->ns == ns)) {
      res->system = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "idle")) && (cur->ns == ns)) {
      res->idle = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "swapin")) && (cur->ns == ns)) {
      res->swapin = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "swapout")) && (cur->ns == ns)) {
      res->swapout = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "blockin")) && (cur->ns == ns)) {
      res->blockin = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "blockout")) && (cur->ns == ns)) {
      res->blockout = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "swapped")) && (cur->ns == ns)) {
      res->swapped = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "free")) && (cur->ns == ns)) {
      res->free = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "buffered")) && (cur->ns == ns)) {
      res->buffered = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "cached")) && (cur->ns == ns)) {
      res->cached = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "used")) && (cur->ns == ns)) {
      res->used = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "info")) && (cur->ns == ns)) {
      p = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (p) {
        res->message = strdup(p);
        xmlFree(p);
      }
    }
  }
  return(res);
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
  if (def && def->stamp < now - 600) { // older then 10 minutes?
     g_hash_table_remove(probe->cache, &res->server);
     def = NULL;
  }
  // if not there retrieve from database and insert in hash
  if (def == NULL) {
    def = g_malloc0(sizeof(struct probe_result));
    def->stamp = time(NULL);
    
    result = my_query("select id, yellow, red "
                      "from   pr_%s_def "
                      "where  server = '%u'", probe->name, res->server);
    if (!result) return(NULL);
    row = mysql_fetch_row(result); 
    if (!row) {
      mysql_free_result(result);
      return(NULL);
    }
    if (row[0]) def->probeid  = atoi(row[0]);
    if (row[1]) def->yellow   = atoi(row[1]);
    if (row[2]) def->red      = atoi(row[2]);
    mysql_free_result(result);

    result = my_query("select server, color, stattime "
                      "from   pr_status "
                      "where  class = '%d' and probe = '%d'", probe->class, def->probeid);
    if (result) {
      row = mysql_fetch_row(result);
      if (row) {
        def->server   = atoi(row[0]);
        def->color    = atoi(row[1]); 
        def->stattime = atoi(row[2]); // just in case the next query fails
      }
      mysql_free_result(result);
    } else {
      LOG(LOG_NOTICE, "pr_status record for %s id %u not found", probe->name, def->probeid);
    }

    if (!def->server) {
      // couldn't find pr_status record? Will be created later,
      // but get the server from the def record for now
      result = my_query("select server "
                        "from   pr_%s_def " 
                        "where  id = '%u'", probe->name, def->probeid);
      if (result) {
        row = mysql_fetch_row(result);
        if (row) def->server   = atoi(row[0]);
        mysql_free_result(result);
      }
    }

    g_hash_table_insert(probe->cache, guintdup(res->server), def);
  }
  return(def);
}   


//*******************************************************************
// STORE RAW RESULTS
//*******************************************************************
static gint store_raw_result(struct _module *probe, void *probe_def, void *probe_res)
{
  MYSQL_RES *result;
  struct sysstat_result *res = (struct sysstat_result *)probe_res;
  struct probe_def *def = (struct probe_def *)probe_def;
  int already_there = TRUE;
    
  result = my_query("insert into pr_sysstat_raw "
                    "set    probe = '%u', yellow = '%d', red = '%d', stattime = '%u', color = '%u', "
                    "       loadavg = '%f', user = '%u', system = '%u', idle = '%u', "
                    "       swapin = '%u', swapout = '%u', blockin = '%u', blockout = '%u', "
                    "       swapped = '%u', free = '%u', buffered = '%u', cached = '%u', "
                    "       used = '%u', message = '%s'",
                    def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->loadavg,   res->user, res->system, res->idle,
                    res->swapin, res->swapout, res->blockin, res->blockout,
                    res->swapped, res->free, res->buffered, res->cached,
                    res->used, res->message ? res->message : "");
  mysql_free_result(result);
  if (mysql_affected_rows(mysql) > 0) { // something was actually inserted
    already_there = FALSE;
  }
  return(already_there); // the record was already in the database
}

//*******************************************************************
// SUMMARIZE A TABLE INTO AN OLDER PERIOD
//*******************************************************************
static void summarize(void *probe_def, void *probe_res, char *from, char *into, guint slotlow, guint slothigh)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct sysstat_result *def = (struct sysstat_result *)probe_def;
  gint avg_yellow, avg_red;
  gfloat avg_loadavg;
  guint avg_user, avg_system, avg_idle, avg_swapin, avg_swapout, avg_blockin;
  guint avg_blockout, avg_swapped, avg_free, avg_buffered, avg_cached, avg_used;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = my_query("select avg(loadavg), avg(user), avg(system), avg(idle), "
                    "       avg(swapin), avg(swapout), avg(blockin), avg(blockout), "
                    "       avg(swapped), avg(free), avg(buffered), avg(cached), "
                    "       avg(used), max(color), avg(yellow), avg(red) " 
                    "from   pr_sysstat_%s use index(probtime) "
                    "where  probe = '%d' and stattime >= %d and stattime < %d",
                    from, def->probeid, slotlow, slothigh);
  
  row = mysql_fetch_row(result);
  if (!row) {
    mysql_free_result(result);
    LOG(LOG_ERR, mysql_error(mysql));
    return;
  }
  if (row[0] == NULL) {
    LOG(LOG_WARNING, "Internal error: nothing to summarize %s: %d %d", into, slotlow, slothigh);
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
  max_color   = atoi(row[13]);
  avg_yellow  = atoi(row[14]);
  avg_red     = atoi(row[15]);
  mysql_free_result(result);

  result = my_query("insert into pr_sysstat_%s " 
                    "set    loadavg = '%f', user = '%u', system = '%u', idle = '%u', "
                    "       swapin = '%u', swapout = '%u', blockin = '%u', blockout = '%u', "
                    "       swapped = '%u', free = '%u', buffered = '%u', cached = '%u', "
                    "       used = '%u', probe = %d, color = '%u', stattime = %d, "
                    "       yellow = '%d', red = '%d'",
                    into, 
                    avg_loadavg, avg_user, avg_system, avg_idle, 
                    avg_swapin, avg_swapout, avg_blockin, avg_blockout, 
                    avg_swapped, avg_free, avg_buffered, avg_cached, 
                    avg_used, def->probeid, max_color, stattime,
                    avg_yellow, avg_red);
  mysql_free_result(result);
}

module sysstat_module  = {
  STANDARD_MODULE_STUFF(SYSSTAT, "sysstat"),
  NULL,
  NULL,
  extract_info_from_xml_node,
  get_def,
  store_raw_result,
  summarize
};

