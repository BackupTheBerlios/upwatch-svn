#include "config.h"
#include <string.h>
#include <generic.h>
#include "cmd_options.h"
#include "slot.h"
#include "uw_process.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct bb_result {
  STANDARD_PROBE_RESULT;
  char *bbname;
};

static void free_res(void *res)
{
  struct bb_result *r = (struct bb_result *)res;

  if (r->bbname) {
    g_free(r->bbname);
  }
}

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//*******************************************************************
static void xml_result_node(module *probe, xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns, void *probe_res)
{
  struct bb_result *res = (struct bb_result *)probe_res;

  res->bbname = xmlGetProp(cur, (const xmlChar *) "bbname");
}

//*******************************************************************
// Only used for debugging
//*******************************************************************
static int fix_result(module *probe, void *probe_res)
{
  struct bb_result *res = (struct bb_result *)probe_res;

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
  struct bb_result *res = (struct bb_result *)probe_res;
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
  strcpy(def->hide, "no");

  // first find the definition based on the serverid
  result = my_query(probe->db, 0,
                    "select id, contact, hide from pr_bb_def "
                    "where  bbname = '%s' and server = '%u'", res->bbname, res->server);
  if (!result) {
    g_free(def);
    return(NULL);
  }
  row = mysql_fetch_row(result);
  if (row && row[0]) {
    // definition found, get the pr_status
    res->probeid = atoi(row[0]);
    def->contact = atoi(row[1]);
    strcpy(def->hide, row[2] ? row[2] : "no");
    mysql_free_result(result);
    result = my_query(probe->db, 0,
                      "select color "
                      "from   pr_status "
                      "where  class = '%u' and probe = '%u'", probe->class, res->probeid);
    if (result) {
      row = mysql_fetch_row(result);
      if (row) {
        if (row[0]) def->color  = atoi(row[0]);
      } else {
        LOG(LOG_NOTICE, "pr_status record for %s id %u (server %s) not found", 
                         res->name, res->probeid, res->hostname);
      }
      mysql_free_result(result);
    } else {
      // bad error on the select query
      def->color  = res->color;
    }
  } else {
    // no def record found? Create one. pr_status will be done later.
    mysql_free_result(result);
    result = my_query(probe->db, 0,
                      "insert into pr_%s_def set server = '%u', " 
                      "       description = '%s', bbname = '%s'", 
                       res->name, res->server, res->hostname, res->bbname);
    mysql_free_result(result);
    res->probeid = mysql_insert_id(probe->db);
    LOG(LOG_NOTICE, "pr_bb_def %s created for %s, id = %u", res->bbname, res->hostname, res->probeid);
  }
  def->probeid = res->probeid;
  def->server = res->server;
  def->newest = res->stattime;

  return(def);
}

static void end_probe(module *probe, void *def, void *res)
{
  g_free(def);
}

module bb_module  = {
  STANDARD_MODULE_STUFF(bb),
  NO_FREE_DEF,
  free_res,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  xml_result_node,
  NO_GET_FROM_XML,
  fix_result,
  get_def,
  NO_STORE_RESULTS,
  NO_SUMMARIZE,
  end_probe,
  NO_END_RUN
};

