#include "config.h"
#include <string.h>
#include <generic.h>
#include "cmd_options.h"
#include "slot.h"
#include "uw_process.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct bb_cpu_result {
  STANDARD_PROBE_RESULT;
#include "../uw_acceptbb/probe.res_h"
  char *hostname;
};
extern module bb_cpu_module;

static void free_res(void *res)
{
  struct bb_cpu_result *r = (struct bb_cpu_result *)res;

  if (r->hostname) g_free(r->hostname);
}

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//*******************************************************************
static void *extract_info_from_xml_node(module *probe, xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns)
{
  struct bb_cpu_result *res;

  res = g_malloc0(sizeof(struct bb_cpu_result));
  if (res == NULL) {
    return(NULL);
  }

  // res->probeid will be filled in later;
  res->stattime = xmlGetPropUnsigned(cur, (const xmlChar *) "date");
  res->expires = xmlGetPropUnsigned(cur, (const xmlChar *) "expires");
  for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
    char *p;

    if (xmlIsBlankNode(cur)) continue;
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "color")) && (cur->ns == ns)) {
      res->color = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
      continue;
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "info")) && (cur->ns == ns)) {
      p = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (p) {
        res->message = strdup(p);
        xmlFree(p);
      }
      continue;
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "host")) && (cur->ns == ns)) {
      xmlNodePtr hname;

      for (hname = cur->xmlChildrenNode; hname != NULL; hname = hname->next) {
        if (xmlIsBlankNode(hname)) continue;
        if ((!xmlStrcmp(hname->name, (const xmlChar *) "hostname")) && (hname->ns == ns)) {
          p = xmlNodeListGetString(doc, hname->xmlChildrenNode, 1);
          if (p) {
            res->hostname = strdup(p);
            xmlFree(p);
          }
          continue;
        }
      }
    }
  }

  // Ok, we'll give it a try, decode the info string a bit.
  // [ntserver3.netland.nl] up: 33 days, 1 users, 22 procs, load=9%, PhysicalMem: 256MB(50%)
  // up: 43 days, 0 users, 88 procs, load=6
  if (res->message) {
    char *load = strstr(res->message, "load=");
    char *win = strstr(res->message, "PhysicalMem");

    if (load) {
      load += 5;
      if (win) {
        char *p_perc;
        int totalmem;
        
        res->user = atoi(load);  // compute system load
        res->idle = 100 - res->user;
        win += 13;               // compute memory use
        totalmem = atoi(win) * 1024 * 1024;
        p_perc = strchr(win, '(');
        if (p_perc) {
          int percentage = atoi(++p_perc);
           
          res->used = (totalmem / 100) * (percentage ? percentage : 1);
        }
        res->free = totalmem - res->used;
      } else {
        res->loadavg = atof(load) / 100.0;
      }
    }
  }
  if (debug) LOG(LOG_DEBUG, "%s: %s %d stattime:%u expires:%u loadavg:%.2f user:%u idle:%u used:%u free:%u",
                 probe->name, res->hostname, res->color, res->stattime, res->expires,
                 res->loadavg, res->user, res->idle, res->used, res->free);
  return(res);
}

//*******************************************************************
// retrieve the definitions + status
// get it from the cache. if there but too old: delete
// If a probe definition does not exist, it will be created.
// in case of mysql-has-gone-away type errors, we keep on running, 
// it will be caught later-on.
//*******************************************************************
static void *get_def(module *probe, void *probe_res)
{
  struct probe_def *def;
  struct bb_cpu_result *res = (struct bb_cpu_result *)probe_res;
  MYSQL_RES *result;
  MYSQL_ROW row;
  time_t now = time(NULL);

  // first we find the serverid, this will be used to find the probe definition in the hashtable
  result = my_query(OPT_ARG(SERVERQUERY), res->hostname, res->hostname, res->hostname, res->hostname, res->hostname);
  if (!result) {
    return(NULL);
  }
  row = mysql_fetch_row(result);
  if (row && row[0]) {
    res->server   = atoi(row[0]);
  } else {
    LOG(LOG_NOTICE, "server %s not found", res->hostname);
    mysql_free_result(result);
    return(NULL);
  }
  mysql_free_result(result);

  // look in the cache for the def
  def = g_hash_table_lookup(probe->cache, &res->server);
  if (def && def->stamp < now - 600) { // older then 10 minutes?
     g_hash_table_remove(probe->cache, &res->server);
     def = NULL;
  }
  // if not there retrieve from database and insert in hash
  if (def == NULL) {
    def = g_malloc0(sizeof(struct probe_result));
    def->stamp    = time(NULL);

    // first find the definition based on the serverid
    result = my_query("select id from pr_%s_def where server = '%u'", probe->name, res->server);
    if (!result) {
      g_free(def);
      return(NULL);
    }
    row = mysql_fetch_row(result);
    if (row && row[0]) {
      // definition found, get the pr_status
      res->probeid = atoi(row[0]);
      mysql_free_result(result);
      result = my_query("select color, stattime "
                        "from   pr_status "
                        "where  class = '%u' and probe = '%u'", probe->class, res->probeid);
      if (result) {
        row = mysql_fetch_row(result);
        if (row) {
          if (row[0]) def->color  = atoi(row[0]);
          if (row[1]) def->newest = atoi(row[1]);
        } else {
          LOG(LOG_NOTICE, "pr_status record for %s id %u not found", probe->name, res->probeid);
        }
        mysql_free_result(result);
      } else {
        // bad error on the select query
        def->color  = res->color;
        def->newest = res->stattime;
      }
    } else {
      // no def record found? Create one. And a pr_status record too.
      mysql_free_result(result);
      result = my_query("insert into pr_%s_def set server = '%u', description = '%s'", 
                         probe->name, res->server, res->hostname);
      mysql_free_result(result);
      res->probeid = mysql_insert_id(mysql);
      LOG(LOG_NOTICE, "pr_status and pr_%s_def created for %s, id = %u", probe->name, res->hostname, res->probeid);
    }
    def->probeid = res->probeid;
    def->server = res->server;

    result = my_query("select stattime from pr_%s_raw use index(probtime) "
                      "where probe = '%u' order by stattime desc limit 1",
                       probe->name, def->probeid);
    if (result) {
      row = mysql_fetch_row(result);
      if (row && mysql_num_rows(result) > 0) {
        if (row[0]) def->newest = atoi(row[0]);
      }
      mysql_free_result(result);
    }
    g_hash_table_insert(probe->cache, guintdup(def->server), def);
  }
  return(def);
}

//*******************************************************************
// STORE RAW RESULTS
//*******************************************************************
static gint store_raw_result(struct _module *probe, void *probe_def, void *probe_res)
{
  MYSQL_RES *result;
  struct bb_cpu_result *res = (struct bb_cpu_result *)probe_res;
  struct bb_cpu_result *def = (struct bb_cpu_result *)probe_def;
  int already_there = TRUE;
  char *escmsg = strdup("");

  if (res->message) {
    escmsg = g_malloc(strlen(res->message) * 2 + 1);
    mysql_real_escape_string(mysql, escmsg, res->message, strlen(res->message)) ;
  }

    
  result = my_query("insert into pr_bb_cpu_raw "
                    "set    probe = '%u', stattime = '%u', color = '%u', "
                    "       user = '%u',  idle = '%u', free = '%u', used = '%u', message = '%s'",
                    def->probeid, res->stattime, res->color,
                    res->user, res->idle, res->free, res->used, escmsg);
  mysql_free_result(result);
  if (mysql_affected_rows(mysql) > 0) { // something was actually inserted
    already_there = FALSE;
  }
  g_free(escmsg);
  return(already_there); // the record was already in the database
}

//*******************************************************************
// SUMMARIZE A TABLE INTO AN OLDER PERIOD
//*******************************************************************
static void summarize(void *probe_def, void *probe_res, char *from, char *into, guint slotlow, guint slothigh)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct bb_cpu_result *def = (struct bb_cpu_result *)probe_def;
  float avg_yellow, avg_red;
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
                    "from   pr_bb_cpu_%s use index(probtime) "
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
  avg_yellow  = atof(row[14]);
  avg_red     = atof(row[15]);
  mysql_free_result(result);

  result = my_query("insert into pr_bb_cpu_%s " 
                    "set    loadavg = '%f', user = '%u', system = '%u', idle = '%u', "
                    "       swapin = '%u', swapout = '%u', blockin = '%u', blockout = '%u', "
                    "       swapped = '%u', free = '%u', buffered = '%u', cached = '%u', "
                    "       used = '%u', probe = %d, color = '%u', stattime = %d, "
                    "       yellow = '%f', red = '%f'",
                    into, 
                    avg_loadavg, avg_user, avg_system, avg_idle, 
                    avg_swapin, avg_swapout, avg_blockin, avg_blockout, 
                    avg_swapped, avg_free, avg_buffered, avg_cached, 
                    avg_used, def->probeid, max_color, stattime,
                    avg_yellow, avg_red);
  mysql_free_result(result);
}

module bb_cpu_module  = {
  STANDARD_MODULE_STUFF(BB_CPU, "bb_cpu"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  store_raw_result,
  summarize
};

