#include "config.h"
#include <string.h>
#include <generic.h>
#include "cmd_options.h"
#include "slot.h"
#include "uw_process.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct errlog_result {
  STANDARD_PROBE_RESULT;
};

extern module errlog_module;

//*******************************************************************
// Only used for debugging
//*******************************************************************
static int fix_result(module *probe, void *probe_res)
{
  struct errlog_result *res = (struct errlog_result *)probe_res;

  if (debug > 1) LOG(LOG_DEBUG, "%s: %s %d stattime %u expires %u", 
                 res->name, res->hostname, res->color, res->stattime, res->expires);
  return 1; // ok
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
  struct errlog_result *res = (struct errlog_result *)probe_res;
  MYSQL_RES *result;
  MYSQL_ROW row;

  // first we find the serverid, this will be used to find the probe definition in the hashtable
  result = my_query(probe->db, 0,
                    OPT_ARG(SERVERQUERY), res->hostname, res->hostname, 
                    res->hostname, res->hostname, res->hostname);
  if (!result) return(NULL);
  row = mysql_fetch_row(result);
  if (row && row[0]) {
    res->server   = atoi(row[0]);
  } else {
    LOG(LOG_NOTICE, "server %s not found", res->hostname);
    mysql_free_result(result);
    return(NULL);
  }
  mysql_free_result(result);

  def = g_malloc0(sizeof(struct probe_result));
  def->stamp    = time(NULL);
  def->server   = res->server;
  strcpy(def->hide, "no");

  // first find the definition based on the serverid
  result = my_query(probe->db, 0,
                    "select id, contact, hide from pr_errlog_def "
                    "where  server = '%u'", res->server);
  if (!result) {
    g_free(def);
    return(NULL);
  }
  if (mysql_num_rows(result) == 0) { // DEF RECORD NOT FOUND
    mysql_free_result(result);
    result = my_query(probe->db, 0,
                      "insert into pr_%s_def set server = '%u', ipaddress = '%s', " 
                      "       description = '%s'", 
                       res->name, res->server, res->ipaddress ? res->ipaddress : "", 
                       res->hostname);
    mysql_free_result(result);
    def->probeid = mysql_insert_id(probe->db);
    LOG(LOG_NOTICE, "pr_errlog_def created for %s, id = %u", res->hostname, def->probeid);
    result = my_query(probe->db, 0,
                    "select id, contact, hide from pr_errlog_def "
                    "where  server = '%u'", res->server);
    if (!result) return(NULL);
  }
  row = mysql_fetch_row(result);
  if (!row || !row[0]) {
    LOG(LOG_NOTICE, "no pr_%s_def found for server %u - skipped", res->name, res->server);
    mysql_free_result(result);
    return(NULL);
  }

  if (row[0])  def->probeid = atoi(row[0]);
  if (row[1])  def->contact = atoi(row[1]);
  strcpy(def->hide, row[2] ? row[2] : "no");
  mysql_free_result(result);

  // definition found, get the pr_status
  result = my_query(probe->db, 0,
                    "select color, stattime "
                    "from   pr_status "
                    "where  class = '%u' and probe = '%u'", probe->class, def->probeid);
  if (result) {
    row = mysql_fetch_row(result);
    if (row) {
      if (row[0]) def->color  = atoi(row[0]);
      if (row[1]) def->newest = atoi(row[1]);
    } else {
      LOG(LOG_NOTICE, "pr_status record for %s id %u (server %s) not found", 
                       res->name, def->probeid, res->hostname);
    }
    mysql_free_result(result);
  }
  if (!def->color) def->color = res->color;

  return(def);
}

static void end_probe(module *probe, void *def, void *res)
{
  g_free(def);
}

module errlog_module  = {
  STANDARD_MODULE_STUFF(errlog),
  NO_FREE_DEF,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  NO_GET_FROM_XML,
  fix_result,
  get_def,
  NO_STORE_RESULTS,
  NO_SUMMARIZE,
  end_probe,
  NO_END_RUN
};

