#include "config.h"
#include <string.h>
#include <generic.h>
#include "cmd_options.h"
#include "slot.h"

static void summarize_httpget(int probe, char *tbl, char *newtbl, guint slotlow, guint slothigh);

/*
 * General procedure: 
 * Get current httpget_definitions
 * Write performance stats into pr_httpget_perf
 * Write status into pr_httpget_def
 * 
 * 
 */

int process_httpget(xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  gchar user[4096];
  gchar passwd[4096];
  gchar dummy[4096];
  gint probe;
  gulong stattime;
  gulong expires;
  gchar ip[4096];
  guint color;
  gfloat lookup, connect, pretransfer, total;
  gchar hostname[4096];
  gchar message[4096];
  gint fields;

  guint server = 0;
  guint prv_color = 0;
  gulong prv_hist_id = 0;
  gulong hist_id = 0;
  guint cur_slot, prev_slot;
  gulong prv_stattime;
  gulong slotlow, slothigh;
  gulong dummy_low, dummy_high;

  hostname[0] = 0;

/*
    <httpget id="1" date="1033425402" expires="1033425522">
      <host>
        <hostname>netland-01.services.netland.nl</hostname>
        <ipaddress>217.170.32.83</ipaddress>
      </host>
      <color>200</color>
      <lookup>16.976</lookup>
      <connect>16.976</connect>
      <pretransfer>20.196</pretransfer>
      <total>28.454</total>
    </httpget>
*/						  
  probe = xmlGetPropInt(cur, (const xmlChar *) "id");
  stattime = xmlGetPropLong(cur, (const xmlChar *) "date");
  expires = xmlGetPropLong(cur, (const xmlChar *) "expires");
  for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
    char *p;
    
    if (xmlIsBlankNode(cur)) continue;
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "color")) && (cur->ns == ns)) {
      color = xmlNodeListGetInt(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "lookup")) && (cur->ns == ns)) {
      lookup = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "connect")) && (cur->ns == ns)) {
      connect = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "pretransfer")) && (cur->ns == ns)) {
      pretransfer = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "total")) && (cur->ns == ns)) {
      total = xmlNodeListGetFloat(doc, cur->xmlChildrenNode, 1);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "info")) && (cur->ns == ns)) {
      p = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (p) {
	strcpy(message, p);
        xmlFree(p);
      }
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "host")) && (cur->ns == ns)) {
      xmlNodePtr hname;

      for (hname = cur->xmlChildrenNode; hname != NULL; hname = hname->next) {
	if (xmlIsBlankNode(hname)) continue;
        if ((!xmlStrcmp(hname->name, (const xmlChar *) "hostname")) && (hname->ns == ns)) {
          p = xmlNodeListGetString(doc, hname->xmlChildrenNode, 1);
	  strcpy(hostname, p);
	  xmlFree(p);
        } 
        if ((!xmlStrcmp(hname->name, (const xmlChar *) "ipaddress")) && (hname->ns == ns)) {
          p = xmlNodeListGetString(doc, hname->xmlChildrenNode, 1);
	  strcpy(ip, p);
	  xmlFree(p);
        } 
      }
    }
  }

  if (open_database() != 0) { // will do nothing if already open
    return FALSE;
  }
      
  result = my_query("select server, color, lasthist, stattime "
                    "from   pr_httpget_def where id = %d", probe);
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
  mysql_free_result(result);
 
  if (stattime <= prv_stattime) return(1);  // already processed?

  //my_transaction("begin");
  if (prv_stattime == 0) { // first time?
    prv_stattime = stattime; // fake

    result = my_query("insert into pr_status "
                      "set    class = '%d', probe = '%d', server = '%d', color = '%d'",
                      PROBE_HTTPGET, probe, server, color);
    mysql_free_result(result);
  }

  if (prv_color == 0 || prv_color != color) { // status changed?
    guint max_color;

    result = my_query("insert into pr_httpget_hist "
                      "set    probe = '%d', stattime = %d, color = '%d', message = '%s'",
                      probe, stattime, color,  message);
    if (mysql_affected_rows(mysql) > 0) {
      hist_id = mysql_insert_id(mysql);
    }
    mysql_free_result(result);

    result = my_query("insert into pr_hist "
                      "set    server = '%d', class = '%d', probe = '%d', stattime = %d, "
                      "       prv_color = '%d', prv_hist = '%d', color = '%d', hist = '%d', message = '%s'",
                      server, PROBE_HTTPGET, probe, stattime, prv_color, prv_hist_id, color,  hist_id, message);
    mysql_free_result(result);

    result = my_query("update pr_status "
                      "set    color = '%d' "
                      "where  class = '%d' and probe = '%d'",
                      color, PROBE_HTTPGET, probe);
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

    result = my_query("update server set color = '%d' where id = '%d'", max_color, server);
    mysql_free_result(result);
  }

  result = my_query("insert into pr_httpget_raw "
                    "set    probe = '%d',  hist = '%d', "
                    "       stattime = %d, color = '%d', "
                    "       lookup = '%f', connect = '%f', pretransfer = '%f', total = '%f' ",
                    probe, hist_id, stattime, color, lookup, connect, pretransfer, total);
  mysql_free_result(result);

  prev_slot = uw_slot(SLOT_DAY, prv_stattime, &slotlow, &slothigh);
  cur_slot = uw_slot(SLOT_DAY, stattime, &dummy_low, &dummy_high);
  if (cur_slot != prev_slot) { // new slot. summarize records with old slot index
    summarize_httpget(probe, "raw", "day", slotlow, slothigh);
  }

  prev_slot = uw_slot(SLOT_WEEK, prv_stattime, &slotlow, &slothigh);
  cur_slot = uw_slot(SLOT_WEEK, stattime, &dummy_low, &dummy_high);
  if (cur_slot != prev_slot) { // new slot. summarize records with old slot index
    summarize_httpget(probe, "day", "week", slotlow, slothigh);
  }

  prev_slot = uw_slot(SLOT_MONTH, prv_stattime, &slotlow, &slothigh);
  cur_slot = uw_slot(SLOT_MONTH, stattime, &dummy_low, &dummy_high);
  if (cur_slot != prev_slot) { // new slot. summarize records with old slot index
    summarize_httpget(probe, "week", "month", slotlow, slothigh);
  }

  prev_slot = uw_slot(SLOT_YEAR, prv_stattime, &slotlow, &slothigh);
  cur_slot = uw_slot(SLOT_YEAR, stattime, &dummy_low, &dummy_high);
  if (cur_slot != prev_slot) { // new slot. summarize records with old slot index
    summarize_httpget(probe, "month", "year", slotlow, slothigh);
  }

  prev_slot = uw_slot(SLOT_YEAR5, prv_stattime, &slotlow, &slothigh);
  cur_slot = uw_slot(SLOT_YEAR5, stattime, &dummy_low, &dummy_high);
  if (cur_slot != prev_slot) { // new slot. summarize records with old slot index
    summarize_httpget(probe, "year", "5year", slotlow, slothigh);
  }

  result = my_query("update pr_httpget_def "
                    "set    stattime = %d, expires = %d, color = '%d', lasthist = '%d', "
                    "       message = '%s' "
                    "where  id = '%d'",
                    stattime, expires, color, hist_id, message, probe);
  mysql_free_result(result);

  xmlUnlinkNode(cur); // succeeded, remove this node from the XML tree
  xmlFreeNode(cur);
  //my_transaction("commit");
  return 1;  
}

static void summarize_httpget(int probe, char *from, char *into, guint slotlow, guint slothigh)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  float avg_lookup, avg_connect, avg_pretransfer, avg_total; 
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = my_query("select avg(lookup), avg(connect), avg(pretransfer), " 
                    "       avg(total), max(color) " 
                    "from   pr_httpget_%s "
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

  avg_lookup = atof(row[0]);
  avg_connect = atof(row[1]);
  avg_pretransfer = atof(row[2]);
  avg_total = atof(row[3]);
  max_color   = atoi(row[4]);
  mysql_free_result(result);

  result = my_query("insert into pr_httpget_%s " 
                    "set    lookup = '%f', connect = '%f', pretransfer = '%f', total = '%f', "
		    "       probe = %d, color = '%u', stattime = %d",
                    into, avg_lookup, avg_connect, avg_pretransfer, avg_total, probe, max_color, stattime);
  mysql_free_result(result);
}

