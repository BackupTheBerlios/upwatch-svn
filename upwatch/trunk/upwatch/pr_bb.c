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
  MYSQL_RES *result;
  MYSQL_ROW row;

  def = g_malloc0(t->probe->def_size);
  def->stamp    = time(NULL);
  def->server   = res->server;
  def->pgroup   = 1;
  strcpy(def->hide, "no");

  if (res->color == STAT_PURPLE && res->probeid) {
    // find the definition based on the probe id
    result = my_query(t->probe->db, 0,
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
      result = my_query(t->probe->db, 0, query_server_by_name, res->hostname, res->hostname, 
                        res->hostname, res->hostname, res->hostname);
      if (!result) {
        g_free(def);
        return(NULL);
      }
      row = mysql_fetch_row(result);
      if (row && row[0]) {
        res->server   = atoi(row[0]);
      } else {
        LOG(LOG_WARNING, "%s:%u@%s: server %s not found", res->realm, res->stattime, t->fromhost, res->hostname);
        mysql_free_result(result);
        g_free(def);
        return(NULL);
      }
      mysql_free_result(result);
    }

    // first find the definition based on the serverid
    result = my_query(t->probe->db, 0,
                      "select id, contact, hide, email, sms, delay from pr_bb_def "
                      "where  bbname = '%s' and server = '%u'", res->bbname, res->server);
    if (!result) {
      g_free(def);
      return(NULL);
    }
  }
  if (mysql_num_rows(result) == 0) { // DEF RECORD NOT FOUND
    mysql_free_result(result);
    result = my_query(t->probe->db, 0,
                      "insert into pr_%s_def set server = '%u', ipaddress = '%s', " 
                      "       description = '%s', bbname = '%s'", 
                       res->name, res->server, res->ipaddress ? res->ipaddress : "", 
                       res->hostname, res->bbname);
    mysql_free_result(result);
    def->probeid = mysql_insert_id(t->probe->db);
    LOG(LOG_NOTICE, "%s:%u@%s: pr_bb_def %s created for %s, id = %u", 
        res->realm, res->stattime, t->fromhost, res->bbname, res->hostname, def->probeid);
    result = my_query(t->probe->db, 0,
                    "select id, contact, hide, email, sms, delay from pr_bb_def "
                    "where  bbname = '%s' and server = '%u'", res->bbname, res->server);
    if (!result) return(NULL);
  }
  row = mysql_fetch_row(result);
  if (!row || !row[0]) {
    LOG(LOG_NOTICE, "%s:%u@%s: no pr_%s_def found for server %u - skipped", 
         res->realm, res->stattime, t->fromhost, res->name, res->server);
    mysql_free_result(result);
    return(NULL);
  }

  if (row[0])  def->probeid = atoi(row[0]);
  if (row[1])  def->contact = atoi(row[1]);
  strcpy(def->hide, row[2] ? row[2] : "no");
  strcpy(def->email, row[3] ? row[3] : "");
  strcpy(def->sms, row[4] ? row[4] : "");
  if (row[4]) def->delay = atoi(row[5]);

  mysql_free_result(result);

  // definition found, get the pr_status
  result = my_query(t->probe->db, 0,
                    "select color, stattime "
                    "from   pr_status "
                    "where  class = '%u' and probe = '%u'", t->probe->class, def->probeid);
  if (result) {
    row = mysql_fetch_row(result);
    if (row) {
      if (row[0]) def->color   = atoi(row[0]);
      if (row[1]) def->newest  = atoi(row[1]);
    } else {
      LOG(LOG_NOTICE, "%s:%u@%s: pr_status record for %s id %u (server %s) not found", 
                       res->realm, res->stattime, t->fromhost, res->name, def->probeid, res->hostname);
    }
    mysql_free_result(result);
  }
  if (!def->color) def->color = res->color;
  res->probeid = def->probeid;

  return(def);
}

