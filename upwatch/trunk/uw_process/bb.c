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
  gchar *hostname;
};

static void free_res(void *res)
{
  struct bb_generic_result *r = (struct bb_generic_result *)res;

  if (r->hostname) g_free(r->hostname);
  if (r->message) g_free(r->message);
  g_free(r);
}

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//*******************************************************************
static void *extract_info_from_xml_node(module *probe, xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns)
{
  struct bb_generic_result *res;

  res = g_malloc0(sizeof(struct bb_generic_result));
  if (res == NULL) {
    return(NULL);
  }

  // res->probeid will be filled in later;
  res->stattime = xmlGetPropUnsigned(cur, (const xmlChar *) "date");
  res->expires = xmlGetPropUnsigned(cur, (const xmlChar *) "expires");
  for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
    char *p;

    if (xmlIsBlankNode(cur)) continue;
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "color")) && (cur->ns == ns)) {
      res->color = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
      continue;
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "info")) && (cur->ns == ns)) {
      p = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (p) {
        res->message = strdup(p);
        xmlFree(p);
      }
      continue;
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "host")) && (cur->ns == ns)) {
      xmlNodePtr hname;

      for (hname = cur->xmlChildrenNode; hname != NULL; hname = hname->next) {
        if (xmlIsBlankNode(hname)) continue;
        if ((!xmlStrcmp(hname->name, (const xmlChar *) "hostname")) && (hname->ns == ns)) {
          p = xmlNodeListGetString(doc, hname->xmlChildrenNode, 1);
          if (p) {
            res->hostname = strdup(p);
            xmlFree(p);
          }
          continue;
        }
      }
    }
  }
  if (debug) LOG(LOG_DEBUG, "%s: %s %d stattime %u expires %u", 
                 probe->name, res->hostname, res->color, res->stattime, res->expires);
  return(res);
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
  time_t now = time(NULL);

  // first we find the serverid, this will be used to find the probe definition in the hashtable
  result = my_query(OPT_ARG(SERVERQUERY), res->hostname, res->hostname, res->hostname, res->hostname, res->hostname);
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
  if (def && def->stamp < now - 600) { // older then 10 minutes?
     g_hash_table_remove(probe->cache, &res->server);
     def = NULL;
  }
  // if not there retrieve from database and insert in hash
  if (def == NULL) {
    def = g_malloc0(sizeof(struct probe_result));
    def->stamp    = time(NULL);

    // first find the definition based on the serverid
    result = my_query("select id from pr_%s_def where server = '%u'", probe->name, res->server);
    if (!result) {
      g_free(def);
      return(NULL);
    }
    row = mysql_fetch_row(result);
    if (row && row[0]) {
      // definition found, get the pr_status
      res->probeid = atoi(row[0]);
      mysql_free_result(result);
      result = my_query("select color, stattime "
                        "from   pr_status "
                        "where  class = '%u' and probe = '%u'", probe->class, res->probeid);
      if (result) {
        row = mysql_fetch_row(result);
        if (row) {
          if (row[0]) def->color  = atoi(row[0]);
          if (row[1]) def->newest = atoi(row[1]);
        } else {
          LOG(LOG_NOTICE, "pr_status record for %s id %u (%s) not found", probe->name, res->probeid, def->server);
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
      result = my_query("insert into pr_%s_def set server = '%u', description = '%s'", 
                         probe->name, res->server, res->hostname);
      mysql_free_result(result);
      res->probeid = mysql_insert_id(mysql);
      LOG(LOG_NOTICE, "pr_%s_def created for %s, id = %u", probe->name, res->hostname, res->probeid);
    }
    def->probeid = res->probeid;
    def->server = res->server;

    g_hash_table_insert(probe->cache, guintdup(def->server), def);
  }
  return(def);
}

module bb_disk_module  = {
  STANDARD_MODULE_STUFF(BB_DISK, "bb_disk"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  NULL,
  NULL
};

module bb_bgp_module  = {
  STANDARD_MODULE_STUFF(BB_BGP, "bb_bgp"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  NULL,
  NULL
};

module bb_conn_module  = {
  STANDARD_MODULE_STUFF(BB_CONN, "bb_conn"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  NULL,
  NULL
};

module bb_dhcp_module  = {
  STANDARD_MODULE_STUFF(BB_DHCP, "bb_dhcp"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  NULL,
  NULL
};

module bb_dns_module  = {
  STANDARD_MODULE_STUFF(BB_DNS, "bb_dns"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  NULL,
  NULL
};

module bb_ftp_module  = {
  STANDARD_MODULE_STUFF(BB_FTP, "bb_ftp"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  NULL,
  NULL
};

module bb_http_module  = {
  STANDARD_MODULE_STUFF(BB_HTTP, "bb_http"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  NULL,
  NULL
};

module bb_hylafax_module  = {
  STANDARD_MODULE_STUFF(BB_HYLAFAX, "bb_hylafax"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  NULL,
  NULL
};

module bb_istmp_module  = {
  STANDARD_MODULE_STUFF(BB_ISMTP, "bb_ismtp"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  NULL,
  NULL
};

module bb_msgs_module  = {
  STANDARD_MODULE_STUFF(BB_MSGS, "bb_msgs"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  NULL,
  NULL
};

module bb_mysql_module  = {
  STANDARD_MODULE_STUFF(BB_MYSQL, "bb_mysql"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  NULL,
  NULL
};

module bb_pop3_module  = {
  STANDARD_MODULE_STUFF(BB_POP3, "bb_pop3"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  NULL,
  NULL
};

module bb_procs_module  = {
  STANDARD_MODULE_STUFF(BB_PROCS, "bb_procs"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  NULL,
  NULL
};

module bb_radius_module  = {
  STANDARD_MODULE_STUFF(BB_RADIUS, "bb_radius"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  NULL,
  NULL
};

module bb_smtp_module  = {
  STANDARD_MODULE_STUFF(BB_SMTP, "bb_smtp"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  NULL,
  NULL
};

module bb_squid_module  = {
  STANDARD_MODULE_STUFF(BB_SQUID, "bb_squid"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  NULL,
  NULL
};

module bb_svcs_module  = {
  STANDARD_MODULE_STUFF(BB_SVCS, "bb_svcs"),
  NULL,
  free_res,
  extract_info_from_xml_node,
  get_def,
  NULL,
  NULL
};

