#include "config.h"
#include <string.h>
#include <generic.h>
#include "cmd_options.h"
#include "slot.h"

static void summarize_iptraf(int probe, char *tbl, char *newtbl, guint slotlow, guint slothigh);

/*
 * General procedure: 
 * Get current iptraf_definitions
 * Write performance stats into pr_iptraf_perf
 * Write status into pr_iptraf_def
 * 
 * 
 */

int process_iptraf(xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  gint probe;
  gulong stattime;
  gulong expires;
  gchar ip[4096];
  guint color = 200;
  guint in = 0, out = 0;
  gchar hostname[4096];

  guint server = 1;
  guint prv_color = 0;
  gulong prv_hist_id = 0;
  gulong hist_id = 0;
  guint cur_slot, prev_slot;
  gulong prv_stattime;
  gulong slotlow, slothigh;
  gulong dummy_low, dummy_high;
  char *p;

  hostname[0] = 0;

/*
    <iptraf id="192.168.170.5" date="1033425402" expires="1033425522">
      <in>200</in>
      <out>16976</out>
    </iptraf>
*/						  
  p = xmlGetProp(cur, (const xmlChar *) "id");
  strcpy(ip, p);
  xmlFree(p);
  stattime = xmlGetPropLong(cur, (const xmlChar *) "date");
  expires = xmlGetPropLong(cur, (const xmlChar *) "expires");
  for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
    if (xmlIsBlankNode(cur)) continue;
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "in")) && (cur->ns == ns)) {
      in = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "out")) && (cur->ns == ns)) {
      out = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
    }
  }

  if (open_database() != 0) { // will do nothing if already open
    return FALSE;
  }
      
  // find the real probeid from the ip address
  result = my_query("select server, color, lasthist, stattime, id " 
                    "from   pr_iptraf_def " 
                    "where  ipaddress = '%s'", ip);
  if (!result) {
    return(FALSE);
  }
  row = mysql_fetch_row(result);
  if (!row) {
    mysql_free_result(result);
    return(FALSE);
  }
  server       = atoi(row[0]);
  prv_color    = atoi(row[1]);
  prv_hist_id  = atoi(row[2]);
  prv_stattime = atoi(row[3]);
  probe        = atoi(row[4]);
  mysql_free_result(result);
 
  if (stattime <= prv_stattime) return(1);  // already processed?

  //my_transaction("begin");
  if (prv_stattime == 0) { // first time?
    prv_stattime = stattime; // fake

    result = my_query("insert into pr_status "
                      "set    class = '%d', probe = '%d', server = '%d', color = '%d'",
                      PROBE_IPTRAF, probe, server, color);
    mysql_free_result(result);
  }

  if (prv_color == 0 || prv_color != color) { // status changed?
    guint max_color;

    result = my_query("insert into pr_iptraf_hist "
                      "set    probe = '%d', stattime = %d, color = '%d'",
                      probe, stattime, color);
    if (mysql_affected_rows(mysql) > 0) {
      hist_id = mysql_insert_id(mysql);
    }
    mysql_free_result(result);

    result = my_query("insert into pr_hist "
                      "set    server = '%d', class = '%d', probe = '%d', stattime = %d, "
                      "       prv_color = '%d', prv_hist = '%d', color = '%d', hist = '%d'",
                      server, PROBE_IPTRAF, probe, stattime, prv_color, prv_hist_id, color,  hist_id);
    mysql_free_result(result);

    result = my_query("update pr_status "
                      "set    color = '%d' "
                      "where  class = '%d' and probe = '%d'",
                      color, PROBE_IPTRAF, probe);
    mysql_free_result(result);

    result = my_query("select max(color) as max_color from pr_status where server = '%d'", server);
    row = mysql_fetch_row(result);
    if (!row) {
      mysql_free_result(result);
      //my_transaction("rollback");
      return(FALSE);
    }
    max_color = row[0] ? atoi(row[0]) : STAT_GREEN;
    mysql_free_result(result);

    if (server > 1) {
      result = my_query("update %s set %s = '%d' where %s = '%d'",        
                        OPT_ARG(SERVER_TABLE_NAME), OPT_ARG(SERVER_TABLE_COLOR_FIELD), max_color,
                        OPT_ARG(SERVER_TABLE_ID_FIELD), server);

      mysql_free_result(result);
    }
  }

  result = my_query("insert into pr_iptraf_raw "
                    "set    probe = '%d',  hist = '%d', "
                    "       stattime = %d, color = '%d', "
                    "       in_total = '%u', out_total = '%u' ",
                    probe, hist_id, stattime, color, in, out);
  mysql_free_result(result);

  prev_slot = uw_slot(SLOT_DAY, prv_stattime, &slotlow, &slothigh);
  cur_slot = uw_slot(SLOT_DAY, stattime, &dummy_low, &dummy_high);
  if (cur_slot != prev_slot) { // new slot. summarize records with old slot index
    summarize_iptraf(probe, "raw", "day", slotlow, slothigh);
  }

  prev_slot = uw_slot(SLOT_WEEK, prv_stattime, &slotlow, &slothigh);
  cur_slot = uw_slot(SLOT_WEEK, stattime, &dummy_low, &dummy_high);
  if (cur_slot != prev_slot) { // new slot. summarize records with old slot index
    summarize_iptraf(probe, "day", "week", slotlow, slothigh);
  }

  prev_slot = uw_slot(SLOT_MONTH, prv_stattime, &slotlow, &slothigh);
  cur_slot = uw_slot(SLOT_MONTH, stattime, &dummy_low, &dummy_high);
  if (cur_slot != prev_slot) { // new slot. summarize records with old slot index
    summarize_iptraf(probe, "week", "month", slotlow, slothigh);
  }

  prev_slot = uw_slot(SLOT_YEAR, prv_stattime, &slotlow, &slothigh);
  cur_slot = uw_slot(SLOT_YEAR, stattime, &dummy_low, &dummy_high);
  if (cur_slot != prev_slot) { // new slot. summarize records with old slot index
    summarize_iptraf(probe, "month", "year", slotlow, slothigh);
  }

  prev_slot = uw_slot(SLOT_YEAR5, prv_stattime, &slotlow, &slothigh);
  cur_slot = uw_slot(SLOT_YEAR5, stattime, &dummy_low, &dummy_high);
  if (cur_slot != prev_slot) { // new slot. summarize records with old slot index
    summarize_iptraf(probe, "year", "5year", slotlow, slothigh);
  }

  result = my_query("update pr_iptraf_def "
                    "set    stattime = %d, expires = %d, color = '%d', lasthist = '%d' "
                    "where  id = '%d'",
                    stattime, expires, color, hist_id, probe);
  mysql_free_result(result);

  xmlUnlinkNode(cur); // succeeded, remove this node from the XML tree
  xmlFreeNode(cur);
  //my_transaction("commit");
  return 1;  
}

static void summarize_iptraf(int probe, char *from, char *into, guint slotlow, guint slothigh)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  guint in_total, out_total;
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = my_query("select sum(in_total), sum(out_total), max(color) " 
                    "from   pr_iptraf_%s "
                    "where  probe = '%d' and stattime >= %d and stattime < %d",
                    from, probe, slotlow, slothigh);
  
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

  in_total = atof(row[0]);
  out_total = atof(row[1]);
  max_color   = atoi(row[2]);
  mysql_free_result(result);

  result = my_query("insert into pr_iptraf_%s " 
                    "set    in_total = '%u', out_total = '%u', "
		    "       probe = %d, color = '%u', stattime = %d",
                    into, in_total, out_total, probe, max_color, stattime);
  mysql_free_result(result);
}

