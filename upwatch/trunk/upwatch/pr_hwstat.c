#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#define _UPWATCH
#include "probe.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//*******************************************************************
void hwstat_get_from_xml(trx *t)
{
  struct hwstat_result *res = (struct hwstat_result *)t->res;

  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "temp1")) && (t->cur->ns == t->ns)) {
    res->temp1 = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "temp2")) && (t->cur->ns == t->ns)) {
    res->temp2 = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "temp3")) && (t->cur->ns == t->ns)) {
    res->temp3 = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "rot1")) && (t->cur->ns == t->ns)) {
    res->rot1 = xmlNodeListGetInt(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "rot2")) && (t->cur->ns == t->ns)) {
    res->rot2 = xmlNodeListGetInt(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "rot3")) && (t->cur->ns == t->ns)) {
    res->rot3 = xmlNodeListGetInt(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "vc0")) && (t->cur->ns == t->ns)) {
    res->vc0 = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "vc1")) && (t->cur->ns == t->ns)) {
    res->vc1 = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "v33")) && (t->cur->ns == t->ns)) {
    res->v33 = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "v50p")) && (t->cur->ns == t->ns)) {
    res->v50p = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "v12p")) && (t->cur->ns == t->ns)) {
    res->v12p = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "v12n")) && (t->cur->ns == t->ns)) {
    res->v12n = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "v50n")) && (t->cur->ns == t->ns)) {
    res->v50n = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
}

//*******************************************************************
// find the real probeid from the server id
// get it from the cache. if there but too old: delete
//*******************************************************************
void *hwstat_get_def(trx *t, int create)
{ 
  struct probe_def *def;
  struct hwstat_result *res = (struct hwstat_result *)t->res;
  MYSQL_RES *result;
  MYSQL_ROW row;
  time_t now = time(NULL);

  if (res->color == STAT_PURPLE) { // generated by uw_purple, which knows the probe id but not the serverid
    result = my_query(t->probe->db, 0,
                      "select server from pr_%s_def where id = '%u'", res->name, res->probeid);
    if (!result) return(NULL);

    row = mysql_fetch_row(result);
    if (row && row[0]) {
      res->server = atoi(row[0]);
    } else {
      LOG(LOG_NOTICE, "%s def %u not found", res->name, res->probeid);
      mysql_free_result(result);
      return(NULL);
    }
    mysql_free_result(result);
  }

  def = g_hash_table_lookup(t->probe->cache, &res->server);
  if (def && def->stamp < now - (120 + uw_rand(240))) { // older then 2 - 6 minutes?
     g_hash_table_remove(t->probe->cache, &res->server);
     def = NULL;
  }
  // if not there retrieve from database and insert in hash
  if (def == NULL) {
    def = g_malloc0(t->probe->def_size);
    def->stamp = time(NULL);
    strcpy(def->hide, "no");
    def->server = res->server;
    
    result = my_query(t->probe->db, 0,
                      "select id, yellow, red, contact, hide, email, delay "
                      "from   pr_%s_def "
                      "where  server = '%u'", res->name, res->server);
    if (!result) return(NULL);

    if (mysql_num_rows(result) == 0) { // DEF RECORD NOT FOUND
      mysql_free_result(result);
      if (!create) {
        LOG(LOG_NOTICE, "pr_%s_def for server %u not found and not trusted - skipped", 
                         res->name, def->server);
        return(NULL);
      }
      result = my_query(t->probe->db, 0,
                        "insert into pr_%s_def set server = '%d', "
                        "        ipaddress = '%s', description = '%s'", 
                        res->name, res->server, t->res->ipaddress ? t->res->ipaddress : "127.0.0.1", 
                        t->fromhost ? t->fromhost : "automatically added");
      mysql_free_result(result);
      if (mysql_affected_rows(t->probe->db) == 0) { // nothing was actually inserted
        LOG(LOG_NOTICE, "insert missing pr_%s_def id %u: %s", 
                         res->name, def->probeid, mysql_error(t->probe->db));
      }
      result = my_query(t->probe->db, 0,
                        "select id, yellow, red, contact, hide, email, delay "
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
    if (row[3]) def->contact  = atoi(row[3]);
    strcpy(def->hide, row[4] ? row[4] : "no");
    strcpy(def->email, row[5] ? row[5] : "");
    if (row[6]) def->delay = atoi(row[6]);

    mysql_free_result(result);

    result = my_query(t->probe->db, 0,
                      "select color "
                      "from   pr_status "
                      "where  class = '%d' and probe = '%d'", t->probe->class, def->probeid);
    if (result) {
      row = mysql_fetch_row(result);
      if (row) {
        def->color   = atoi(row[0]); 
      }
      mysql_free_result(result);
    } 

    result = my_query(t->probe->db, 0,
                      "select stattime from pr_%s_raw use index(probstat) "
                      "where probe = '%u' order by stattime desc limit 1",
                       res->name, def->probeid);
    if (result) {
      if (mysql_num_rows(result) > 0) {
        row = mysql_fetch_row(result);
        if (row && row[0]) {
          def->newest = atoi(row[0]);
        }
      }
      mysql_free_result(result);
    }

    g_hash_table_insert(t->probe->cache, guintdup(res->server), def);
  }
  res->probeid = def->probeid;
  return(def);
}   

//*******************************************************************
// Adjust the result. In this case this means:
// don't trust the color we receive from the probe
//*******************************************************************
void hwstat_adjust_result(trx *t)
{ 
  struct hwstat_result *res = (struct hwstat_result *)t->res;

  if (res->color != STAT_PURPLE) {
    char buffer[10];

    res->color = STAT_GREEN;
    if (res->temp1 > t->def->yellow) res->color = STAT_YELLOW;
    if (res->temp1 > t->def->red) res->color = STAT_RED;
    sprintf(buffer, "%u", res->color);
    set_result_value(t, "color", buffer);
  }
}