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
};
struct bb_cpu_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};
extern module bb_cpu_module;

//*******************************************************************
// Ok, we'll give it a try, decode the info string a bit.
// [ntserver3.netland.nl] up: 33 days, 1 users, 22 procs, load=9%, PhysicalMem: 256MB(50%)
// nbslevel: 00229, up: 96 days, 0 users, 90 procs, load=21
// up: 43 days, 0 users, 88 procs, load=6
//*******************************************************************
static int fix_result(module *probe, void *probe_res)
{
  struct bb_cpu_result *res = (struct bb_cpu_result *)probe_res;

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
        res->loadavg = atof(load);
        res->loadavg /= 100.0;
      }
    }
  }
  if (debug > 1) {
    LOG(LOG_DEBUG, "%s: %s %d stattime:%u expires:%u loadavg:%.2f user:%u idle:%u used:%u free:%u",
                   res->name, res->hostname, res->color, res->stattime, res->expires,
                   res->loadavg, res->user, res->idle, res->used, res->free);
  }
  return 1;
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
  result = my_query(probe->db, 0, OPT_ARG(QUERY_SERVER_BY_NAME), res->hostname, res->hostname, 
                    res->hostname, res->hostname, res->hostname);
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
  if (def && def->stamp < now - (120 + uw_rand(240))) { // older then 2 - 6 minutes?
     g_hash_table_remove(probe->cache, &res->server);
     def = NULL;
  }
  // if not there retrieve from database and insert in hash
  if (def == NULL) {
    def = g_malloc0(probe->def_size);
    def->stamp    = time(NULL);
    def->server   = res->server;
    strcpy(def->hide, "no");

    // first find the definition based on the serverid
    result = my_query(probe->db, 0, "select id, yellow, red, contact, hide, email, redmins "
                                    "from pr_%s_def where server = '%u'", 
                      res->name, res->server);
    if (!result) {
      g_free(def);
      return(NULL);
    }
    if (mysql_num_rows(result) == 0) { // DEF RECORD NOT FOUND
      // no def record found? Create one. 
      mysql_free_result(result);
      result = my_query(probe->db, 0, 
                        "insert into pr_%s_def set server = '%u', description = '%s'", 
                         res->name, res->server, res->hostname);
      mysql_free_result(result);
      def->probeid = mysql_insert_id(probe->db);
      LOG(LOG_NOTICE, "pr_%s_def created for %s, id = %u", res->name, res->hostname, def->probeid);
      result = my_query(probe->db, 0, "select id, yellow, red, contact, hide, email, redmins "
                                      "from pr_%s_def where id = '%u'", 
                        res->name, def->probeid);
    }
    row = mysql_fetch_row(result);
    if (!row || !row[0]) {
      LOG(LOG_NOTICE, "no pr_%s_def found for server %u - skipped", res->name, res->server);
      mysql_free_result(result);
      g_free(def);
      return(NULL);
    } 
    if (row[0]) def->probeid = atoi(row[0]);
    if (row[1]) def->yellow = atof(row[1]);
    if (row[2]) def->red = atof(row[2]);
    if (row[3]) def->contact = atof(row[3]);
    strcpy(def->hide, row[4] ? row[4] : "no");
    strcpy(def->email, row[5] ? row[5] : "");
    if (row[6]) def->redmins = atoi(row[6]);

    mysql_free_result(result);
    result = my_query(probe->db, 0, 
                      "select color, changed, notified "
                      "from   pr_status "
                      "where  class = '%u' and probe = '%u'", probe->class, def->probeid);
    if (result) {
      row = mysql_fetch_row(result);
      if (row) {
        if (row[0]) def->color   = atoi(row[0]);
        if (row[1]) def->changed = atoi(row[1]);
        strcpy(def->notified, row[2] ? row[2] : "no");
      } else {
        LOG(LOG_NOTICE, "pr_status record for %s id %u not found", res->name, def->probeid);
      }
      mysql_free_result(result);
    }
    if (!def->color) def->color = res->color;

    result = my_query(probe->db, 0, 
                      "select stattime from pr_%s_raw use index(probstat) "
                      "where probe = '%u' order by stattime desc limit 1",
                       res->name, def->probeid);
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
static gint store_raw_result(struct _module *probe, void *probe_def, void *probe_res, guint *seen_before)
{
  MYSQL_RES *result;
  struct bb_cpu_result *res = (struct bb_cpu_result *)probe_res;
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
                    "insert into pr_bb_cpu_raw "
                    "set    probe = '%u', stattime = '%u', color = '%u', loadavg = '%f', "
                    "       user = '%u',  idle = '%u', free = '%u', used = '%u', message = '%s'",
                    def->probeid, res->stattime, res->color, res->loadavg,
                    res->user, res->idle, res->free, res->used, escmsg);
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
  float avg_yellow, avg_red;
  gfloat avg_loadavg;
  guint avg_user, avg_system, avg_idle;
  guint avg_swapped, avg_free, avg_buffered, avg_cached, avg_used;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = my_query(probe->db, 0,
                    "select avg(loadavg), avg(user), avg(system), avg(idle), "
                    "       avg(swapped), avg(free), avg(buffered), avg(cached), "
                    "       avg(used), max(color), avg(yellow), avg(red) " 
                    "from   pr_bb_cpu_%s use index(probstat) "
                    "where  probe = '%d' and stattime >= %d and stattime < %d",
                    from, def->probeid, slotlow, slothigh);
  
  if (!result) return;
  if (mysql_num_rows(result) == 0) { // no records found
    LOG(LOG_WARNING, "nothing to summarize from %s for probe %u %u %u",
                       from, def->probeid, slotlow, slothigh);
    mysql_free_result(result);
    return;
  }

  row = mysql_fetch_row(result);
  if (!row) {
    mysql_free_result(result);
    LOG(LOG_ERR, (char *)mysql_error(probe->db));
    return;
  }
  if (row[0] == NULL) {
    LOG(LOG_WARNING, "nothing to summarize from %s for probe %u %u %u", 
                       from, def->probeid, slotlow, slothigh);
    mysql_free_result(result);
    return;
  }

  avg_loadavg = atof(row[0]);
  avg_user    = atoi(row[1]);
  avg_system  = atoi(row[2]);
  avg_idle    = atoi(row[3]);
  avg_swapped = atoi(row[4]);
  avg_free    = atoi(row[5]);
  avg_buffered= atoi(row[6]);
  avg_cached  = atoi(row[7]);
  avg_used    = atoi(row[8]);
  max_color   = atoi(row[9]);
  avg_yellow  = atof(row[10]);
  avg_red     = atof(row[11]);
  mysql_free_result(result);

  if (resummarize) {
    // delete old values
    result = my_query(probe->db, 0,
                    "delete from pr_bb_cpu_%s where probe = '%u' and stattime = '%u'",
                    into, def->probeid, stattime);
    mysql_free_result(result);
  }
  result = my_query(probe->db, 0,
                    "insert into pr_bb_cpu_%s " 
                    "set    loadavg = '%f', user = '%u', system = '%u', idle = '%u', "
                    "       swapped = '%u', free = '%u', buffered = '%u', cached = '%u', "
                    "       used = '%u', probe = %d, color = '%u', stattime = %d, "
                    "       yellow = '%f', red = '%f', slot = '%u'",
                    into, 
                    avg_loadavg, avg_user, avg_system, avg_idle, 
                    avg_swapped, avg_free, avg_buffered, avg_cached, 
                    avg_used, def->probeid, max_color, stattime,
                    avg_yellow, avg_red, slot);
  mysql_free_result(result);
}

module bb_cpu_module  = {
  STANDARD_MODULE_STUFF(bb_cpu),
  NO_FREE_DEF,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  NO_GET_FROM_XML,
  fix_result,    
  get_def,
  store_raw_result,
  summarize,
  NO_END_PROBE,
  NO_END_RUN
};

