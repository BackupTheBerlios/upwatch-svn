#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#define _UPWATCH
#include <probe.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

//*******************************************************************
// Ok, we'll give it a try, decode the info string a bit.
// [ntserver3.netland.nl] up: 33 days, 1 users, 22 procs, load=9%, PhysicalMem: 256MB(50%)
// nbslevel: 00229, up: 96 days, 0 users, 90 procs, load=21
// up: 43 days, 0 users, 88 procs, load=6
//*******************************************************************
int bb_cpu_accept_result(trx *t)
{
  struct bb_cpu_result *res = (struct bb_cpu_result *)t->res;

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
  LOG(LOG_DEBUG, "%s: %s %d stattime:%u expires:%u loadavg:%.2f user:%u idle:%u used:%u free:%u",
                 res->name, res->hostname, res->color, res->stattime, res->expires,
                 res->loadavg, res->user, res->idle, res->used, res->free);
  return 1;
}

//*******************************************************************
// retrieve the definitions + status
// get it from the cache. if there but too old: delete
// If a probe definition does not exist, it will be created.
// in case of mysql-has-gone-away type errors, we keep on running, 
// it will be caught later-on.
//*******************************************************************
void *bb_cpu_get_def(trx *t, int create)
{
  struct probe_def *def;
  struct bb_cpu_result *res = (struct bb_cpu_result *)t->res;
  dbi_result result;
  time_t now = time(NULL);

  if (res->color != STAT_PURPLE && res->server == 0) { 
    // first we find the serverid, this will be used to find the probe definition in the hashtable
    result = db_query(t->probe->db, 0, query_server_by_name, res->hostname, res->hostname, 
                      res->hostname, res->hostname, res->hostname);
    if (!result) {
      return(NULL);
    }
    if (dbi_result_next_row(result)) {
      res->server   = dbi_result_get_int_idx(result, 0);
    } else {
      LOG(LOG_NOTICE, "%s:%u@%s: server %s not found", res->realm, res->stattime, t->fromhost, res->hostname);
      dbi_result_free(result);
      return(NULL);
    }
    dbi_result_free(result);
  }

  // look in the cache for the def
  def = g_hash_table_lookup(t->probe->cache, &res->server);
  if (def && def->stamp < now - (120 + uw_rand(240))) { // older then 2 - 6 minutes?
     g_hash_table_remove(t->probe->cache, &res->server);
     def = NULL;
  }
  // if not there retrieve from database and insert in hash
  if (def == NULL) {
    def = g_malloc0(t->probe->def_size);
    def->stamp    = time(NULL);
    def->server   = res->server;
    def->pgroup   = 1;
    strcpy(def->hide, "no");

    // first find the definition based on the serverid
    result = db_query(t->probe->db, 0, "select id, yellow, red, contact, hide, email, sms, delay "
                                    "from pr_%s_def where server = '%u'", 
                      res->name, res->server);
    if (!result) {
      g_free(def);
      return(NULL);
    }
    if (dbi_result_get_numrows(result) == 0) { // DEF RECORD NOT FOUND
      char sequence[40];
      // no def record found? Create one. 
      dbi_result_free(result);
      result = db_query(t->probe->db, 0, 
                        "insert into pr_%s_def (server, description) values ('%u', '%s')", 
                         res->name, res->server, res->hostname);
      dbi_result_free(result);
      sprintf(sequence, "pr_%s_def_id_seq", res->name);
      def->probeid = dbi_conn_sequence_last(t->probe->db, sequence);
      LOG(LOG_NOTICE, "%s:%u@%s: pr_%s_def created for %s, id = %u", 
          res->realm, res->stattime, t->fromhost, res->name, res->hostname, def->probeid);
      result = db_query(t->probe->db, 0, "select id, yellow, red, contact, hide, email, sms, delay "
                                      "from pr_%s_def where id = '%u'", 
                        res->name, def->probeid);
    }
    if (!dbi_result_next_row(result)) {
      LOG(LOG_NOTICE, "%s:%u@%s: no pr_%s_def found for server %u - skipped", 
          res->realm, res->stattime, t->fromhost, res->name, res->server);
      dbi_result_free(result);
      g_free(def);
      return(NULL);
    }

    def->probeid = dbi_result_get_int(result, "id");
    def->yellow  = dbi_result_get_float(result, "yellow");
    def->red     = dbi_result_get_float(result, "red");
    def->contact = dbi_result_get_int(result, "contact");
    strcpy(def->hide, dbi_result_get_string_default(result, "hide", "no"));
    strcpy(def->email, dbi_result_get_string_default(result, "email", ""));
    strcpy(def->sms, dbi_result_get_string_default(result, "sms", ""));
    def->delay   = dbi_result_get_int(result, "delay");

    dbi_result_free(result);
    result = db_query(t->probe->db, 0, 
                      "select color "
                      "from   pr_status "
                      "where  class = '%u' and probe = '%u'", t->probe->class, def->probeid);
    if (result) {
      if (dbi_result_next_row(result)) {
        def->color   = dbi_result_get_int(result, "color");
      } else {
        LOG(LOG_NOTICE, "%s:%u@%s: pr_status record for %s id %u not found", 
           res->realm, res->stattime, t->fromhost, res->name, def->probeid);
      }
      dbi_result_free(result);
    }
    if (!def->color) def->color = res->color;

    result = db_query(t->probe->db, 0, 
                      "select stattime from pr_%s_raw use index(probstat) "
                      "where probe = '%u' order by stattime desc limit 1",
                       res->name, def->probeid);
    if (result) {
      if (dbi_result_next_row(result)) {
        def->newest = dbi_result_get_int(result, "stattime");
      }
      dbi_result_free(result);
    }
    g_hash_table_insert(t->probe->cache, guintdup(def->server), def);
  }
  res->probeid = def->probeid;
  return(def);
}


