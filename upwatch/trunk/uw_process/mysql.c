#include "config.h"
#include <string.h>
#include <generic.h>
#include "cmd_options.h"
#include "slot.h"
#include "uw_process.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct mysql_result {
  STANDARD_PROBE_RESULT;
#include "../uw_mysql/probe.res_h"
};
extern module mysql_module;

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//*******************************************************************
static void *extract_info_from_xml_node(module *probe, xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns)
{
  struct mysql_result *res;

  res = g_malloc0(sizeof(struct mysql_result));
  if (res == NULL) {
    return(NULL);
  }

  res->probeid = xmlGetPropInt(cur, (const xmlChar *) "id");
  res->stattime = xmlGetPropUnsigned(cur, (const xmlChar *) "date");
  res->expires = xmlGetPropUnsigned(cur, (const xmlChar *) "expires");
  for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
    char *p;

    if (xmlIsBlankNode(cur)) continue;
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "color")) && (cur->ns == ns)) {
      res->color = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
      continue;
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "connect")) && (cur->ns == ns)) {
      res->connect = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
      continue;
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "total")) && (cur->ns == ns)) {
      res->total = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
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
  }
  return(res);
}

//*******************************************************************
// STORE RAW RESULTS
//*******************************************************************
static gint store_raw_result(struct _module *probe, void *probe_def, void *probe_res)
{
  MYSQL_RES *result;
  struct mysql_result *res = (struct mysql_result *)probe_res;
  struct probe_def *def = (struct probe_def *)probe_def;
  int already_there = TRUE;
  char *escmsg = strdup("");

  if (res->message) {
    escmsg = g_malloc(strlen(res->message) * 2 + 1);
    mysql_real_escape_string(mysql, escmsg, res->message, strlen(res->message)) ;
  }
    
  result = my_query("insert into pr_mysql_raw "
                    "set    probe = '%u', yellow = '%f', red = '%f', stattime = '%u', color = '%u', "
                    "       connect = '%f', total = '%f', "
                    "       message = '%s' ",
                    def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->connect, res->total, escmsg);

  mysql_free_result(result);
  if (mysql_affected_rows(mysql) > 0) { // something was actually inserted
    already_there = FALSE;
  }
  g_free(escmsg);
  return(already_there); // the record was already in the database
}

//*******************************************************************
// SUMMARIZE A TABLE INTO AN OLDER PERIOD
//*******************************************************************
static void summarize(void *probe_def, void *probe_res, char *from, char *into, guint slotlow, guint slothigh)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct mysql_result *def = (struct mysql_result *)probe_def;
  float avg_yellow, avg_red;
  float avg_connect, avg_total;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = my_query("select avg(connect), avg(total), "
                    "       max(color), avg(yellow), avg(red) "
                    "from   pr_mysql_%s use index(probtime) "
                    "where  probe = '%d' and stattime >= %d and stattime < %d",
                    from, def->probeid, slotlow, slothigh);

  row = mysql_fetch_row(result);
  if (!row) {
    mysql_free_result(result);
    LOG(LOG_ERR, mysql_error(mysql));
    return;
  }
  if (row[0] == NULL) {
    LOG(LOG_WARNING, "Internal error: nothing to summarize %s: %d %d", into, slotlow, slothigh);
    return;
  }

  avg_connect = atof(row[0]);
  avg_total = atof(row[1]);
  max_color   = atoi(row[2]);
  avg_yellow  = atof(row[3]);
  avg_red     = atof(row[4]);
  mysql_free_result(result);

  result = my_query("insert into pr_mysql_%s "
                    "set    connect = '%f', total = '%f', "
                    "       probe = %d, color = '%u', stattime = %d, yellow = '%f', red = '%f'",
                    into, avg_connect, avg_total, def->probeid, 
                    max_color, stattime, avg_yellow, avg_red);

  mysql_free_result(result);
}

module mysql_module  = {
  STANDARD_MODULE_STUFF(MYSQL, "mysql"),
  NULL,
  NULL,
  extract_info_from_xml_node,
  NULL,
  store_raw_result,
  summarize
};

