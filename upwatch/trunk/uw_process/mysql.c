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

static int accept_probe(const char *name)
{
  return(strcmp(name, "mysql") == 0);
}

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//*******************************************************************
static void get_from_xml(module *probe, xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns, void *probe_res)
{
  struct mysql_result *res = (struct mysql_result *)probe_res;

  if ((!xmlStrcmp(cur->name, (const xmlChar *) "connect")) && (cur->ns == ns)) {
    res->connect = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(cur->name, (const xmlChar *) "total")) && (cur->ns == ns)) {
    res->total = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
    return;
  }
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
  char *escmsg;

  if (res->message) {
    escmsg = g_malloc(strlen(res->message) * 2 + 1);
    mysql_real_escape_string(probe->db, escmsg, res->message, strlen(res->message)) ;
  } else {
    escmsg = strdup("");
  }
    
  result = my_query(probe->db, 0,
                    "insert into pr_mysql_raw "
                    "set    probe = '%u', yellow = '%f', red = '%f', stattime = '%u', color = '%u', "
                    "       connect = '%f', total = '%f', "
                    "       message = '%s' ",
                    def->probeid, def->yellow, def->red, res->stattime, res->color, 
                    res->connect, res->total, escmsg);

  mysql_free_result(result);
  if (mysql_affected_rows(probe->db) > 0) { // something was actually inserted
    already_there = FALSE;
  }
  g_free(escmsg);
  return(already_there); // the record was already in the database
}

//*******************************************************************
// SUMMARIZE A TABLE INTO AN OLDER PERIOD
//*******************************************************************
static void summarize(module *probe, void *probe_def, void *probe_res, char *from, char *into, guint slot, guint slotlow, guint slothigh, gint ignore_dupes)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct probe_def *def = (struct probe_def *)probe_def;
  float avg_yellow, avg_red;
  float avg_connect, avg_total;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = my_query(probe->db, 0,
                    "select avg(connect), avg(total), "
                    "       max(color), avg(yellow), avg(red) "
                    "from   pr_mysql_%s use index(probtime) "
                    "where  probe = '%d' and stattime >= %d and stattime < %d",
                    from, def->probeid, slotlow, slothigh);

  if (!result) return;
  if (mysql_num_rows(result) == 0) { // no records found
    LOG(LOG_WARNING, "nothing to summarize from %s for probe %u %u %u",
                       from, def->probeid, slotlow, slothigh);
    return;
  }

  row = mysql_fetch_row(result);
  if (!row) {
    mysql_free_result(result);
    LOG(LOG_ERR, mysql_error(probe->db));
    return;
  }
  if (row[0] == NULL) {
    LOG(LOG_WARNING, "nothing to summarize from %s for probe %u %u %u", 
                       from, def->probeid, slotlow, slothigh);
    return;
  }

  avg_connect = atof(row[0]);
  avg_total = atof(row[1]);
  max_color   = atoi(row[2]);
  avg_yellow  = atof(row[3]);
  avg_red     = atof(row[4]);
  mysql_free_result(result);

  result = my_query(probe->db, ignore_dupes,
                    "insert into pr_mysql_%s "
                    "set    connect = '%f', total = '%f', "
                    "       probe = %d, color = '%u', stattime = %d, "
                    "       yellow = '%f', red = '%f', slot = '%u'",
                    into, avg_connect, avg_total, def->probeid, 
                    max_color, stattime, avg_yellow, avg_red, slot);

  mysql_free_result(result);
}

module mysql_module  = {
  STANDARD_MODULE_STUFF(mysql),
  NULL,
  NULL,
  NULL,
  NULL,
  accept_probe,
  NULL,
  get_from_xml,
  NULL,
  NULL,
  store_raw_result,
  summarize,
  NULL
};

