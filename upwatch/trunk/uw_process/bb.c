#include "config.h"
#include <string.h>
#include <generic.h>
#include "cmd_options.h"
#include "slot.h"
#include "uw_process.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct bb_generic_result {
  STANDARD_PROBE_RESULT;
};

static int accept_probe(module *probe, const char *name)
{
  return(strncmp(name, "bb_", 3) == 0);
}

//*******************************************************************
// Only used for debugging
//*******************************************************************
static int fix_result(module *probe, void *probe_res)
{
  struct bb_generic_result *res = (struct bb_generic_result *)probe_res;

  if (debug) LOG(LOG_DEBUG, "%s: %s %d stattime %u expires %u", 
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
  struct bb_generic_result *res = (struct bb_generic_result *)probe_res;
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

  // first find the definition based on the serverid
  result = my_query(probe->db, 0,
                    "select id from pr_bb_generic_def "
                    "where  bbname = '%s' and server = '%u'", res->name, res->name, res->server);
  if (!result) {
    g_free(def);
    return(NULL);
  }
  row = mysql_fetch_row(result);
  if (row && row[0]) {
    // definition found, get the pr_status
    res->probeid = atoi(row[0]);
    mysql_free_result(result);
    result = my_query(probe->db, 0,
                      "select color, stattime "
                      "from   pr_status "
                      "where  class = '%u' and probe = '%u'", probe->class, res->probeid);
    if (result) {
      row = mysql_fetch_row(result);
      if (row) {
        if (row[0]) def->color  = atoi(row[0]);
        if (row[1]) def->newest = atoi(row[1]);
      } else {
        LOG(LOG_NOTICE, "pr_status record for %s id %u (%s) not found", res->name, res->probeid, def->server);
      }
      mysql_free_result(result);
    } else {
      // bad error on the select query
      def->color  = res->color;
      def->newest = res->stattime;
    }
  } else {
    // no def record found? Create one. pr_status will be done later.
    mysql_free_result(result);
    result = my_query(probe->db, 0,
                      "insert into pr_bb_generic_def set server = '%u', " 
                      "       description = '%s', bbname = '%s'", 
                       res->name, res->server, res->hostname, res->name);
    mysql_free_result(result);
    res->probeid = mysql_insert_id(probe->db);
    LOG(LOG_NOTICE, "pr_generic_def %s created for %s, id = %u", res->name, res->hostname, res->probeid);
  }
  def->probeid = res->probeid;
  def->server = res->server;

  return(def);
}

module bb_generic_module  = {
  STANDARD_MODULE_STUFF(bb_generic),
  NULL,
  NULL,
  NULL,
  NULL,
  accept_probe,
  NULL,
  NULL,
  fix_result,
  get_def,
  NULL,
  NULL,
  NULL
};
