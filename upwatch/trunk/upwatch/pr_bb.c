#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#define _UPWATCH
#include <probe.h>
#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *query_server_by_name;

void bb_free_res(void *res)
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
void bb_xml_result_node(trx *t)
{
  struct bb_result *res = (struct bb_result *)t->res;

  res->bbname = xmlGetProp(t->cur, (const xmlChar *) "bbname");
}

//*******************************************************************
// retrieve the definitions + status
// get it from the cache. if there but too old: delete
// If a probe definition does not exist, it will be created.
// in case of mysql-has-gone-away type errors, we keep on running, 
// it will be caught later-on.
//*******************************************************************
void *bb_get_def(trx *t, int create)
{
  struct probe_def *def;
  struct bb_result *res = (struct bb_result *)t->res;
  dbi_result result;

  def = g_malloc0(t->probe->def_size);
  def->stamp    = time(NULL);
  def->server   = res->server;
  def->pgroup   = 1;
  strcpy(def->hide, "no");

  if (res->color == STAT_PURPLE && res->probeid) {
    // find the definition based on the probe id
    result = db_query(t->probe->db, 0,
                      "select id, contact, hide, email, sms, delay from pr_bb_def "
                      "where  id = '%u'", res->probeid);
    if (!result) {
      g_free(def);
      return(NULL);
    }
  } else {
    if (res->server == 0) {
      // first we find the serverid, this will be used to find the probe definition in the database
      if (!query_server_by_name) {
        LOG(LOG_WARNING, "%s:%u@%s: don't know how to find %s by name", 
            res->realm, res->stattime, t->fromhost, res->hostname);
        g_free(def);
        return(NULL);
      }
      result = db_query(t->probe->db, 0, query_server_by_name, res->hostname, res->hostname, 
                        res->hostname, res->hostname, res->hostname);
      if (!result) {
        g_free(def);
        return(NULL);
      }
      if (dbi_result_next_row(result)) {
        res->server   = dbi_result_get_int_idx(result, 0);
      } else {
        LOG(LOG_WARNING, "%s:%u@%s: server %s not found", res->realm, res->stattime, t->fromhost, res->hostname);
        dbi_result_free(result);
        g_free(def);
        return(NULL);
      }
      dbi_result_free(result);
    }

    // first find the definition based on the serverid
    result = db_query(t->probe->db, 0,
                      "select id, contact, hide, email, sms, delay from pr_bb_def "
                      "where  bbname = '%s' and server = '%u'", res->bbname, res->server);
    if (!result) {
      g_free(def);
      return(NULL);
    }
  }
  if (dbi_result_get_numrows(result) == 0) { // DEF RECORD NOT FOUND
    char sequence[40];
    dbi_result_free(result);
    result = db_query(t->probe->db, 0,
                      "insert into pr_%s_def (server, ipaddress, description, bbname) "
                      "            values ('%u', '%s', '%s', '%s')", 
                      res->name, res->server, res->ipaddress ? res->ipaddress : "", 
                      res->hostname, res->bbname);
    dbi_result_free(result);
    sprintf(sequence, "pr_%s_def_id_seq", res->name);
    def->probeid  = dbi_conn_sequence_last(t->probe->db, sequence);
    LOG(LOG_NOTICE, "%s:%u@%s: pr_bb_def %s created for %s, id = %u", 
        res->realm, res->stattime, t->fromhost, res->bbname, res->hostname, def->probeid);
    result = db_query(t->probe->db, 0,
                    "select id, contact, hide, email, sms, delay from pr_bb_def "
                    "where  bbname = '%s' and server = '%u'", res->bbname, res->server);
    if (!result) return(NULL);
  }
  if (!dbi_result_next_row(result)) {
    LOG(LOG_NOTICE, "%s:%u@%s: no pr_%s_def found for server %u - skipped", 
         res->realm, res->stattime, t->fromhost, res->name, res->server);
    dbi_result_free(result);
    return(NULL);
  }
  def->probeid = dbi_result_get_int(result, "id");
  def->contact = dbi_result_get_int(result, "contact");
  strcpy(def->hide, dbi_result_get_string_default(result, "hide", "no"));
  strcpy(def->email, dbi_result_get_string_default(result, "email", ""));
  strcpy(def->sms, dbi_result_get_string_default(result, "sms", ""));
  def->delay = dbi_result_get_int(result, "delay");

  dbi_result_free(result);

  // definition found, get the pr_status
  result = db_query(t->probe->db, 0,
                    "select color, stattime "
                    "from   pr_status "
                    "where  class = '%u' and probe = '%u'", t->probe->class, def->probeid);
  if (result) {
    if (dbi_result_next_row(result)) {
      def->color   = dbi_result_get_int(result, "color");
      def->newest  = dbi_result_get_int(result, "stattime");
    } else {
      LOG(LOG_NOTICE, "%s:%u@%s: pr_status record for %s id %u (server %s) not found", 
                       res->realm, res->stattime, t->fromhost, res->name, def->probeid, res->hostname);
    }
    dbi_result_free(result);
  }
  if (!def->color) def->color = res->color;
  res->probeid = def->probeid;

  return(def);
}

