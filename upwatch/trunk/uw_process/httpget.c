#include "config.h"
#include <string.h>
#include <generic.h>
#include "cmd_options.h"
#include "slot.h"
#include "uw_process.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct httpget_result {
  STANDARD_PROBE_RESULT;
  gfloat lookup;
  gfloat connect;
  gfloat pretransfer;
  gfloat total;
};
extern module httpget_module;

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//*******************************************************************
static void *extract_info_from_xml_node(module *probe, xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns)
{
  struct httpget_result *res;

  res = g_malloc0(sizeof(struct httpget_result));
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
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "lookup")) && (cur->ns == ns)) {
      res->lookup = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
      continue;
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "connect")) && (cur->ns == ns)) {
      res->connect = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
      continue;
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "pretransfer")) && (cur->ns == ns)) {
      res->pretransfer = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
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
  struct httpget_result *res = (struct httpget_result *)probe_res;
  struct probe_def *def = (struct probe_def *)probe_def;
  int already_there = TRUE;
    
  result = my_query("insert into pr_httpget_raw "
                    "set    probe = '%u', yellow = '%u', red = '%u', stattime = '%u', color = '%u', "
                    "       lookup = '%f', connect = '%f', pretransfer = '%f', total = '%f', "
                    "       message = '%s' ",
                    def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->lookup, res->connect, res->pretransfer, res->total,
                    res->message ? res->message : "");

  mysql_free_result(result);
  if (mysql_affected_rows(mysql) > 0) { // something was actually inserted
    already_there = FALSE;
  }
  return(already_there); // the record was already in the database
}

//*******************************************************************
// SUMMARIZE A TABLE INTO AN OLDER PERIOD
//*******************************************************************
static void summarize(void *probe_def, void *probe_res, char *from, char *into, guint slotlow, guint slothigh)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct httpget_result *def = (struct httpget_result *)probe_def;
  gint avg_yellow, avg_red;
  float avg_lookup, avg_connect, avg_pretransfer, avg_total;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = my_query("select avg(lookup), avg(connect), avg(pretransfer), "
                    "       avg(total), max(color), avg(yellow), avg(red) "
                    "from   pr_httpget_%s use index(probtime) "
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

  avg_lookup = atof(row[0]);
  avg_connect = atof(row[1]);
  avg_pretransfer = atof(row[2]);
  avg_total = atof(row[3]);
  max_color   = atoi(row[4]);
  avg_yellow  = atoi(row[5]);
  avg_red     = atoi(row[6]);
  mysql_free_result(result);

  result = my_query("insert into pr_httpget_%s "
                    "set    lookup = '%f', connect = '%f', pretransfer = '%f', total = '%f', "
                    "       probe = %d, color = '%u', stattime = %d, yellow = '%d', red = '%d'",
                    into, avg_lookup, avg_connect, avg_pretransfer, avg_total, def->probeid, 
                    max_color, stattime, avg_yellow, avg_red);

  mysql_free_result(result);
}

module httpget_module  = {
  STANDARD_MODULE_STUFF(HTTPGET, "httpget"),
  NULL,
  NULL,
  extract_info_from_xml_node,
  NULL,
  store_raw_result,
  summarize
};

