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

  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "temp1")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->temp1 = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "temp2")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->temp2 = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "temp3")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->temp3 = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "rot1")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->rot1 = xmlNodeListGetInt(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "rot2")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->rot2 = xmlNodeListGetInt(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "rot3")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->rot3 = xmlNodeListGetInt(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "vc0")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->vc0 = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "vc1")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->vc1 = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "v33")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->v33 = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "v50p")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->v50p = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "v12p")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->v12p = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "v12n")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->v12n = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "v50n")) && (xmlNsEqual(t->cur->ns, t->ns))) {
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
  struct hwstat_def *def;
  struct hwstat_result *res = (struct hwstat_result *)t->res;
  dbi_result result;
  time_t now = time(NULL);

  if (res->color == STAT_PURPLE) { // generated by uw_purple, which knows the probe id but not the serverid
    result = db_query(t->probe->db, 0,
                      "select server from pr_%s_def where id = '%u'", res->name, res->probeid);
    if (!result) return(NULL);

    if (dbi_result_next_row(result)) {
      res->server = dbi_result_get_int(result, "server");
    } else {
      LOG(LOG_NOTICE, "%s:%u@%s: %s def %u not found", 
          res->realm, res->stattime, t->fromhost, res->name, res->probeid);
      dbi_result_free(result);
      delete_pr_status(t, res->probeid);
      return(NULL);
    }
    dbi_result_free(result);
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

    result = db_query(t->probe->db, 0,
                      "select id, contact, hide, email, sms, delay, "
                      "       temp1_yellow, temp1_red, temp2_yellow, temp2_red, "
                      "       temp3_yellow, temp3_red, rot1_red, rot2_red, rot3_red, pgroup "
                      "from   pr_%s_def "
                      "where  server = '%u'", res->name, res->server);
    if (!result) return(NULL);

    if (dbi_result_get_numrows(result) == 0) { // DEF RECORD NOT FOUND
      dbi_result_free(result);
      if (!create) {
        LOG(LOG_NOTICE, "%s:%u@%s: pr_%s_def for server %u not found and not trusted - skipped", 
                         res->realm, res->stattime, t->fromhost, res->name, def->server);
        return(NULL);
      }
      result = db_query(t->probe->db, 0,
                        "insert into pr_%s_def (server, ipaddress, description) values ('%d', '%s', '%s')", 
                        res->name, res->server, t->res->ipaddress ? t->res->ipaddress : "127.0.0.1", 
                        t->fromhost ? t->fromhost : "automatically added");
      dbi_result_free(result);
      if (dbi_result_get_numrows_affected(t->probe->db) == 0) { // nothing was actually inserted
        LOG(LOG_NOTICE, "insert missing pr_%s_def id %u: %s", 
                         res->name, def->probeid, mysql_error(t->probe->db));
      }
      result = db_query(t->probe->db, 0,
                        "select id, contact, hide, email, sms, delay, "
                        "       temp1_yellow, temp1_red, temp2_yellow, temp2_red, "
                        "       temp3_yellow, temp3_red, rot1_red, rot2_red, rot3_red, pgroup "
                        "from   pr_%s_def "
                        "where  server = '%u'", res->name, res->server);
      if (!result) return(NULL);
    }
    if (!dbi_result_next_row(result)) {
      LOG(LOG_NOTICE, "%s:%u@%s: no pr_%s_def found for server %u - skipped", 
          res->realm, res->stattime, t->fromhost, res->name, res->server);
      dbi_result_free(result);
      return(NULL);
    }
    def->probeid      = dbi_result_get_int(result, "id");
    def->contact      = dbi_result_get_int(result, "contact");
    strcpy(def->hide, dbi_result_get_string_default(result, "hide", "no"));
    strcpy(def->email, dbi_result_get_string_default(result, "email", ""));
    strcpy(def->sms, dbi_result_get_string_default(result, "sms", ""));
    def->delay        = dbi_result_get_int(result, "delay");
    def->temp1_yellow = dbi_result_get_int(result, "temp1_yellow"); 
    def->temp1_red    = dbi_result_get_int(result, "temp1_red");
    def->temp2_yellow = dbi_result_get_int(result, "temp2_yellow");
    def->temp2_red    = dbi_result_get_int(result, "temp2_red");
    def->temp3_yellow = dbi_result_get_int(result, "temp3_yellow");
    def->temp3_red    = dbi_result_get_int(result, "temp3_red");
    def->rot1_red     = dbi_result_get_int(result, "rot1_red");
    def->rot2_red     = dbi_result_get_int(result, "rot2_red");
    def->rot3_red     = dbi_result_get_int(result, "rot3_red");
    def->pgroup       = dbi_result_get_int(result, "pgroup");

    dbi_result_free(result);

    result = db_query(t->probe->db, 0,
                      "select color "
                      "from   pr_status "
                      "where  class = '%d' and probe = '%d'", t->probe->class, def->probeid);
    if (result) {
      if (dbi_result_next_row(result)) {
        def->color   = dbi_result_get_int(result, "color");
      }
      dbi_result_free(result);
    } 

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
  struct hwstat_def *def = (struct hwstat_def *)t->def;

  if (res->color != STAT_PURPLE) {
    char buffer[100];
    GString *errmsg = g_string_new(res->message);

    res->color = STAT_GREEN;
    if (res->temp1 >= def->temp1_red)    {
      res->color = max(res->color, STAT_RED);
      sprintf(buffer, "temp1 (%.1f�C) has reached the RED limit: %u�C\n", res->temp1, def->temp1_red);
      errmsg = g_string_append(errmsg, buffer);
    } else if (res->temp1 >= def->temp1_yellow) { 
      res->color = max(res->color, STAT_YELLOW);
      sprintf(buffer, "temp1 (%.1f�C) has reached the YELLOW limit: %u�C\n", res->temp1, def->temp1_yellow);
      errmsg = g_string_append(errmsg, buffer);
    }
    if (res->temp2 >= def->temp2_red)    {
      res->color = max(res->color, STAT_RED);
      sprintf(buffer, "temp2 (%.1f�C) has reached the RED limit: %u�C\n", res->temp2, def->temp2_red);
      errmsg = g_string_append(errmsg, buffer);
    } else if (res->temp2 >= def->temp2_yellow) { 
      res->color = max(res->color, STAT_YELLOW);
      sprintf(buffer, "temp2 (%.1f�C) has reached the YELLOW limit: %u�C\n", res->temp2, def->temp2_yellow);
      errmsg = g_string_append(errmsg, buffer);
    }
    if (res->temp3 >= def->temp3_red)    {
      res->color = max(res->color, STAT_RED);
      sprintf(buffer, "temp3 (%.1f�C) has reached the RED limit: %u�C\n", res->temp3, def->temp3_red);
      errmsg = g_string_append(errmsg, buffer);
    } else if (res->temp3 >= def->temp3_yellow) { 
      res->color = max(res->color, STAT_YELLOW);
      sprintf(buffer, "temp3 (%.1f�C) has reached the YELLOW limit: %u�C\n", res->temp3, def->temp3_yellow);
      errmsg = g_string_append(errmsg, buffer);
    }
    if (res->rot1 <  def->rot1_red) {
      res->color = max(res->color, STAT_RED);
      sprintf(buffer, "Fan 1 is running too slow (%u, which is below %u)\n", res->rot1, def->rot1_red);
      errmsg = g_string_append(errmsg, buffer);
    }
    if (res->rot2 <  def->rot2_red) {
      res->color = max(res->color, STAT_RED);
      sprintf(buffer, "Fan 2 is running too slow (%u, which is below %u)\n", res->rot1, def->rot1_red);
      errmsg = g_string_append(errmsg, buffer);
    }
    if (res->rot3 <  def->rot3_red) {
      res->color = max(res->color, STAT_RED);
      sprintf(buffer, "Fan 3 is running too slow (%u, which is below %u)\n", res->rot1, def->rot1_red);
      errmsg = g_string_append(errmsg, buffer);
    }
    sprintf(buffer, "%u", res->color);
    set_result_value(t, "color", buffer);

    if (res->message) { free(res->message); res->message = NULL; }
    if (errmsg->len) res->message = strdup(errmsg->str);
    //set_result_value(t, "info", errmsg->str);
    g_string_free(errmsg, TRUE);
  }
}
