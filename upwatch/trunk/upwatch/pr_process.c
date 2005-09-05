#include <config.h>
#include <generic.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <generic.h>
#define _UPWATCH
#include <probe.h>
#include "slot.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

//*******************************************************************
// For probes that use no cache: disable this field
//******************************************************************* 
void init_no_cache(module *probe)
{
  probe->needs_cache = 0;
}

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//******************************************************************* 
int set_result_value(trx *t, char *name, char *value)
{
  xmlNodePtr cur = t->cur;
  int found = FALSE;

  for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
    if ((!xmlStrcmp(cur->name, (const xmlChar *) name)) && (cur->ns == t->ns)) {
      xmlNodeSetContent(cur, value);
      found = TRUE;
      break;
    }
  }
  if (!found) {
    if (xmlNewChild(t->cur, NULL, name, value)) {
      found = TRUE;
    }
  }
  return found;
}

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//******************************************************************* 
int extract_info_from_xml(trx *t)
{
  struct probe_result *res;
  xmlNodePtr save = t->cur;

  res = g_malloc0(t->probe->res_size);
  if (res == NULL) {
    return 0;
  }
  t->res = res;

  res->name = strdup(t->cur->name);

  res->server = xmlGetPropInt(t->cur, (const xmlChar *) "server");
  res->probeid = xmlGetPropInt(t->cur, (const xmlChar *) "id");
  res->stattime = xmlGetPropUnsigned(t->cur, (const xmlChar *) "date");
  res->expires = xmlGetPropUnsigned(t->cur, (const xmlChar *) "expires");
  res->color = xmlGetPropUnsigned(t->cur, (const xmlChar *) "color");
  res->interval = xmlGetPropUnsigned(t->cur, (const xmlChar *) "interval");
  res->received = xmlGetPropUnsigned(t->cur, (const xmlChar *) "received");
  res->ipaddress = xmlGetProp(t->cur, (const xmlChar *) "ipaddress");
  res->realm = xmlGetProp(t->cur, (const xmlChar *) "domain"); // only one of domain/realm can be present
  res->realm = xmlGetProp(t->cur, (const xmlChar *) "realm");

  if (res->stattime > t->probe->lastseen) {
    t->probe->lastseen = res->stattime;
  }

  if (res->color == STAT_PURPLE) 
    return 1; // PURPLE results don't have details of course, they are created by uw_purple

  if (t->probe->xml_result_node) {
    t->probe->xml_result_node(t);
  }

  for (t->cur = t->cur->xmlChildrenNode; t->cur != NULL; t->cur = t->cur->next) {
    char *p;

    if (xmlIsBlankNode(t->cur)) continue;
    if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "color")) && (t->cur->ns == t->ns)) {
      res->color = xmlNodeListGetInt(t->doc, t->cur->xmlChildrenNode, 1);
      continue;
    }
    if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "info")) && (t->cur->ns == t->ns)) {
      p = xmlNodeListGetString(t->doc, t->cur->xmlChildrenNode, 1);
      if (p) { 
        res->message = strdup(p); 
        xmlFree(p);
      }
      continue;
    }
    if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "prevcolor")) && (t->cur->ns == t->ns)) {
      res->prevcolor = xmlNodeListGetInt(t->doc, t->cur->xmlChildrenNode, 1);
      continue;
    }
    if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "host")) && (t->cur->ns == t->ns)) {
      p = xmlNodeListGetString(t->doc, t->cur->xmlChildrenNode, 1);
      if (p) { 
        res->hostname = strdup(p); 
        xmlFree(p);
      }
      continue;
    }
    if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "host")) && (t->cur->ns == t->ns)) {
      xmlNodePtr hname;

      for (hname = t->cur->xmlChildrenNode; hname != NULL; hname = hname->next) {
        if (xmlIsBlankNode(hname)) continue;
        if ((!xmlStrcmp(hname->name, (const xmlChar *) "hostname")) && (hname->ns == t->ns)) {
          p = xmlNodeListGetString(t->doc, hname->xmlChildrenNode, 1);
          if (p) {
            res->hostname = strdup(p);
            xmlFree(p);
          }
          continue;
        }
        if ((!xmlStrcmp(hname->name, (const xmlChar *) "ipaddress")) && (hname->ns == t->ns)) {
          p = xmlNodeListGetString(t->doc, hname->xmlChildrenNode, 1);
          if (p) {
            res->ipaddress = strdup(p);
            xmlFree(p);
          }
          continue;
        }
      }
    }
    if (t->probe->get_from_xml) {
      t->probe->get_from_xml(t);
    }
  }
  t->cur = save;
  return 1;
}
//*******************************************************************
// Only used for debugging
//*******************************************************************
int accept_result(trx *t)
{
  LOG(LOG_DEBUG, "%s %s %d: %d stattime %u expires %u",
               t->res->realm, t->res->name, t->res->probeid, 
               t->res->color, t->res->stattime, t->res->expires);
  return 1; // ok
}

void delete_pr_status(trx *t, int id)
{
  MYSQL_RES *result;

  result = my_query(t->probe->db, 0,
           "delete from pr_status where class = '%u' and probe = '%u'",
           t->probe->class, id);
  mysql_free_result(result);
}

//*******************************************************************
// retrieve the definitions + status
// get it from the cache. if there but too old: delete
// in case of mysql-has-gone-away type errors, we keep on running, 
// it will be caught later-on.
//*******************************************************************
void *get_def(trx *t, int create)
{
  struct probe_result *res = (struct probe_result *)t->res;
  struct probe_def *def;
  MYSQL_RES *result;
  MYSQL_ROW row;
  time_t now = time(NULL);
  char *def_fields = "ipaddress, description, server, yellow, red, contact, hide, email, delay, pgroup";

  def = g_hash_table_lookup(t->probe->cache, &res->probeid);
  if (def && def->stamp < now - (120 + uw_rand(240))) { // older then 2 - 6 minutes?
     g_hash_table_remove(t->probe->cache, &res->probeid);
     def = NULL;
  }
  // if not there retrieve from database and insert in hash
  if (def == NULL) {
    def = g_malloc0(t->probe->def_size);
    def->stamp    = time(NULL);
    strcpy(def->hide, "no");

    result = my_query(t->probe->db, 0,
                      "select color "
                      "from   pr_status "
                      "where  class = '%u' and probe = '%u'", t->probe->class, res->probeid);
    if (result) {
      row = mysql_fetch_row(result);
      if (row) {
        if (row[0]) def->color   = atoi(row[0]);
      } else {
        LOG(LOG_NOTICE, "%s:%u@%s: pr_status record for %s id %u ip %s not found", 
            res->realm, res->stattime, t->fromhost, res->name, res->probeid, res->ipaddress);
      }
      mysql_free_result(result);
    }

    // Get the server, contact and yellow/red info from the def record. Note the yellow/red may 
    // have been changed by the user so need to be transported into the data files
    result = my_query(t->probe->db, 0, "select %s from pr_%s_def where  id = '%u'", 
                      t->probe->get_def_fields ? t->probe->get_def_fields : def_fields,
                      res->name, res->probeid);
    if (!result) return(NULL);

    if (mysql_num_rows(result) == 0) { // DEF RECORD NOT FOUND
      mysql_free_result(result);
      if (!create) {
        LOG(LOG_NOTICE, "%s:%u@%s: pr_%s_def id %u not found - skipped",
                         res->realm, res->stattime, t->fromhost, res->name, res->probeid);
        delete_pr_status(t, res->probeid);
        return(NULL);
      }
      if (res->server == 0) {
        LOG(LOG_NOTICE, "%s:%u@%s: pr_%s_def id %u not found trusted but we have no serverid - skipped",
                         res->realm, res->stattime, t->fromhost, res->name, res->probeid);
        delete_pr_status(t, res->probeid);
        return(NULL);
      }
      // at this point, we have a probe result, but we can't find the _def record
      // for it. We apparantly trust this result, so we can create the definition
      // ourselves. For that we need to fill in the server id and the ipaddress
      // and we look into the result if the is anything useful in there.
      result = my_query(t->probe->db, 0,
                        "insert into pr_%s_def set server = '%d', "
                        "        ipaddress = '%s', description = '%s'",
                        res->name, res->server, res->ipaddress?res->ipaddress:"",
                        t->fromhost ? t->fromhost : "automatically added");
      mysql_free_result(result);
      res->probeid = mysql_insert_id(t->probe->db);
      if (mysql_affected_rows(t->probe->db) == 0) { // nothing was actually inserted
        LOG(LOG_NOTICE, "%s:%u@%s: insert missing pr_%s_def id %u: %s", 
                         res->realm, res->stattime, t->fromhost,
                         res->name, res->probeid, mysql_error(t->probe->db));
      }
      result = my_query(t->probe->db, 0, "select %s from pr_%s_def where  id = '%u'", 
                        t->probe->get_def_fields ? t->probe->get_def_fields : def_fields,
                        res->name, res->probeid);
      if (!result) return(NULL);
    }
    if (t->probe->set_def_fields) {
      t->probe->set_def_fields(t, def, result);
    } else {
      row = mysql_fetch_row(result);
      if (row) {
        if (row[0]) def->ipaddress   = strdup(row[0]);
        if (row[1]) def->description = strdup(row[1]);
        if (row[2]) def->server    = atoi(row[2]);
        if (row[3]) def->yellow    = atof(row[3]);
        if (row[4]) def->red       = atof(row[4]);
        if (row[5]) def->contact   = atof(row[5]);
        strcpy(def->hide, row[6] ? row[6] : "no");
        strcpy(def->email, row[7] ? row[7] : "");
        if (row[8]) def->delay = atoi(row[8]);
        if (row[9]) def->pgroup = atoi(row[9]);
      }
    }
    mysql_free_result(result);

    result = my_query(t->probe->db, 0,
                      "select stattime from pr_%s_raw use index(probstat) "
                      "where probe = '%u' order by stattime desc limit 1",
                       res->name, res->probeid);
    if (result) {
      if (mysql_num_rows(result) > 0) {
        row = mysql_fetch_row(result);
        if (row && row[0]) {
          def->newest = atoi(row[0]);
        }
      }
      mysql_free_result(result);
    }

    def->probeid = res->probeid;
    g_hash_table_insert(t->probe->cache, guintdup(def->probeid), def);
  }
  res->probeid = def->probeid;
  return(def);
}


//*******************************************************************
// retrieve the definitions + status
// get it from the cache. if there but too old: delete
// If a probe definition does not exist, it will be created.
// in case of mysql-has-gone-away type errors, we keep on running, 
// it will be caught later-on.
//*******************************************************************
void *get_def_by_servid(trx *t, int create)
{
  struct probe_def *def;
  struct probe_result *res = (struct probe_result *)t->res;
  MYSQL_RES *result;
  MYSQL_ROW row;

  def = g_malloc0(t->probe->def_size);
  def->stamp    = time(NULL);
  def->server   = res->server;
  strcpy(def->hide, "no");

  // first find the definition based on the serverid
  result = my_query(t->probe->db, 0,
                    "select id, contact, hide, email, delay from pr_%s_def "
                    "where  server = '%u'", res->name, res->server);
  if (!result) {
    g_free(def);
    return(NULL);
  }
  if (mysql_num_rows(result) == 0) { // DEF RECORD NOT FOUND
    mysql_free_result(result);
    result = my_query(t->probe->db, 0,
                      "insert into pr_%s_def set server = '%u', ipaddress = '%s', " 
                      "       description = '%s'", 
                       res->name, res->server, res->ipaddress ? res->ipaddress : "", 
                       res->hostname);
    mysql_free_result(result);
    def->probeid = mysql_insert_id(t->probe->db);
    LOG(LOG_NOTICE, "%s:%u@%s: pr_%s_def created for %s, id = %u", 
        res->realm, res->stattime, t->fromhost, res->name, res->hostname, def->probeid);
    result = my_query(t->probe->db, 0,
                    "select id, contact, hide, email, delay from pr_%s_def "
                    "where  server = '%u'", res->server, res->name);
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
  if (row[4]) def->delay = atoi(row[4]);

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

  return(def);
}

/*
 * Reads a results file
*/
int handle_result_file(gpointer data, gpointer user_data)
{
  char *filename = (char *)data;
  trx *t = (trx *) user_data;
  char *p;
  time_t fromdate;
  struct stat st;
  int filesize;
  int i, ret = 0;

  LOG(LOG_INFO, "Processing %s", filename); 

  if (stat(filename, &st)) {
    LOG(LOG_WARNING, "%s: %m", filename);
    return ret;
  }
  ret++; /* file exists */
  filesize = (int) st.st_size;
  if (filesize == 0) {
    LOG(LOG_WARNING, "%s: size 0, removed", filename);
    return ret;
  }

  t->doc = xmlParseFile(filename);
  if (t->doc == NULL) {
    LOG(LOG_WARNING, "%s: %m", filename);
    return ret;
  }
  t->cur = xmlDocGetRootElement(t->doc);
  if (t->cur == NULL) {
    LOG(LOG_WARNING, "%s: empty document", filename);
    return ret;
  }
  t->ns = xmlSearchNsByHref(t->doc, t->cur, (const xmlChar *) NAMESPACE_URL);
  if (t->ns == NULL) {
    LOG(LOG_WARNING, "%s: wrong type, result namespace not found", filename);
    return ret;
  }
  if (xmlStrcmp(t->cur->name, (const xmlChar *) "result")) {
    LOG(LOG_WARNING, "%s: wrong type, root node is not 'result'", filename);
    return ret;
  }
  p = xmlGetProp(t->cur, (const xmlChar *) "fromhost");
  if (p) {
    t->fromhost = strdup(p);
    xmlFree(p);
  }
  fromdate = (time_t) xmlGetPropUnsigned(t->cur, (const xmlChar *) "date");

  /*
   * Now, walk the tree.
   */
  /* First level we expect just result */
  t->cur = t->cur->xmlChildrenNode;
  while (t->cur && xmlIsBlankNode(t->cur)) {
    t->cur = t->cur->next;
  }
  if (t->cur == 0) {
    LOG(LOG_WARNING, "%s: empty file", filename);
    return ret;
  }
  ret++; /* file contents syntax check succeeded */
  /* Second level is a list of probes, but be laxist */
  for (t->failed_count = 0; t->cur != NULL;) {
    int found = 0;
    int retval = 0;
    char buf[20];
    int count = 0;

    buf[0] = 0;
    if (xmlIsBlankNode(t->cur)) {
      t->cur = t->cur->next;
      continue;
    }
    for (i = 0; modules[i]; i++) {

      t->probe = modules[i];
      if (modules[i]->accept_probe) {
        if (!modules[i]->accept_probe(t, t->cur->name)) continue;
      } else if (strcmp(t->cur->name, modules[i]->module_name)) {
        continue;
      } 

      if (t->cur->ns != t->ns) {
        LOG(LOG_WARNING, "%s: namespace %s != %s", t->cur->name, t->cur->ns, t->ns);
        //continue;
      }
      found = 1;
      retval = extract_info_from_xml(t);
      if (retval) {
        if (buf[0] == 0 || count % 100 == 0) {
          strftime(buf, sizeof(buf), "%Y-%m-%d %T", gmtime(&fromdate));
          uw_setproctitle("%s %s@%s", buf, t->res->name, t->fromhost);
        }
        if (debug > 3) { fprintf(stderr, "%s %s@%s", buf, t->res->name, t->fromhost); }
        retval = t->process(t);
      }
      if (retval == 0 || retval == -1) { // error in processing this probe
        found = FALSE; // so it will go to the failed section
      } else if (retval == -2) {
        ret = 3;
        goto errexit; // fatal database error
      } else {
        count++;
      }
      break;
    }
    if (!found) {
      xmlNodePtr node, new;

      if (retval > 0) {
        LOG(LOG_WARNING, "can't find method: %s, saved as failure", t->cur->name);
      }
      t->failed_count++;
      if (!t->failed) {
        t->failed = UpwatchXmlDoc("result", t->fromhost);
      }
      node = t->cur;
      t->cur = t->cur->next;
      xmlUnlinkNode(node); // unlink, copy and paste
      new = xmlCopyNode(node, 1);
      xmlFreeNode(node);
      xmlAddChild(xmlDocGetRootElement(t->failed), new);
    } else {
     t->cur = t->cur->next;
    }
  }
  ret = 4;

errexit:
  if (t->fromhost) {
    free(t->fromhost);
    t->fromhost = NULL;
  }
  return ret;
}
