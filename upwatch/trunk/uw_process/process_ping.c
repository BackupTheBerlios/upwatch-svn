#include "config.h"
#include <string.h>
#include <generic.h>
#include "cmd_options.h"
#include "slot.h"

static void summarize_ping(int probe, char *tbl, char *newtbl, guint slotlow, guint slothigh);

/*
 * General procedure: 
 * Get current ping_definitions
 * Write performance stats into pr_ping_perf
 * Write status into pr_ping_def
 * 
 * 
 */

int process_ping(xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns)
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
  gfloat lowest, value, highest;
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
    <ping id="1" date="1033425402" expires="1033425522">
      <host>
        <hostname>netland-01.services.netland.nl</hostname>
        <ipaddress>217.170.32.83</ipaddress>
      </host>
      <color>200</color>
      <min>16.976</min>
      <avg>20.196</avg>
      <max>28.454</max>
    </ping>
*/						  
  probe = atoi(xmlGetProp(cur, (const xmlChar *) "id"));
  stattime = atol(xmlGetProp(cur, (const xmlChar *) "date"));
  expires = atol(xmlGetProp(cur, (const xmlChar *) "expires"));
  for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next) {
    char *p;
    
    if (xmlIsBlankNode(cur)) continue;
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "color")) && (cur->ns == ns)) {
      color = atoi(xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "color")) && (cur->ns == ns)) {
      color = atoi(xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "min")) && (cur->ns == ns)) {
      lowest = atof(xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "avg")) && (cur->ns == ns)) {
      value = atof(xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "max")) && (cur->ns == ns)) {
      highest = atof(xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "info")) && (cur->ns == ns)) {
      p = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      strcpy(message, p);
      xmlFree(p);
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

  result = my_query("select server, color, lasthist, stattime "
                    "from   pr_ping_def where id = %d", probe);
  if (!result) {
    close_database();
    return(FALSE);
  }
  row = mysql_fetch_row(result);
  if (!row) {
    mysql_free_result(result);
    close_database();
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
                      PROBE_PING, probe, server, color);
    mysql_free_result(result);
  }

  if (prv_color == 0 || prv_color != color) { // status changed?
    guint max_color;

    result = my_query("insert into pr_ping_hist "
                      "set    probe = '%d', stattime = %d, color = '%d', message = '%s'",
                      probe, stattime, color,  message);
    if (mysql_affected_rows(mysql) > 0) {
      hist_id = mysql_insert_id(mysql);
    }
    mysql_free_result(result);

    result = my_query("insert into pr_hist "
                      "set    server = '%d', class = '%d', probe = '%d', stattime = %d, "
                      "       prv_color = '%d', prv_hist = '%d', color = '%d', hist = '%d', message = '%s'",
                      server, PROBE_PING, probe, stattime, prv_color, prv_hist_id, color,  hist_id, message);
    mysql_free_result(result);

    result = my_query("update pr_status "
                      "set    color = '%d' "
                      "where  class = '%d' and probe = '%d'",
                      color, PROBE_PING, probe);
    mysql_free_result(result);

    result = my_query("select max(color) as max_color from pr_status where server = '%d'", server);
    row = mysql_fetch_row(result);
    if (!row) {
      mysql_free_result(result);
      //my_transaction("rollback");
      close_database();
      return(FALSE);
    }
    max_color = row[0] ? atoi(row[0]) : STAT_GREEN;
    mysql_free_result(result);

    result = my_query("update server set color = '%d' where id = '%d'", max_color, server);
    mysql_free_result(result);
  }

  result = my_query("insert into pr_ping_raw "
                    "set    probe = '%d',  hist = '%d', "
                    "       stattime = %d, color = '%d', "
                    "       lowest = '%f', value = '%f', highest = '%f' ",
                    probe, hist_id, stattime, color, lowest, value, highest);
  mysql_free_result(result);

  prev_slot = uw_slot(SLOT_DAY, prv_stattime, &slotlow, &slothigh);
  cur_slot = uw_slot(SLOT_DAY, stattime, &dummy_low, &dummy_high);
  if (cur_slot != prev_slot) { // new slot. summarize records with old slot index
    summarize_ping(probe, "raw", "day", slotlow, slothigh);
  }

  prev_slot = uw_slot(SLOT_WEEK, prv_stattime, &slotlow, &slothigh);
  cur_slot = uw_slot(SLOT_WEEK, stattime, &dummy_low, &dummy_high);
  if (cur_slot != prev_slot) { // new slot. summarize records with old slot index
    summarize_ping(probe, "day", "week", slotlow, slothigh);
  }

  prev_slot = uw_slot(SLOT_MONTH, prv_stattime, &slotlow, &slothigh);
  cur_slot = uw_slot(SLOT_MONTH, stattime, &dummy_low, &dummy_high);
  if (cur_slot != prev_slot) { // new slot. summarize records with old slot index
    summarize_ping(probe, "week", "month", slotlow, slothigh);
  }

  prev_slot = uw_slot(SLOT_YEAR, prv_stattime, &slotlow, &slothigh);
  cur_slot = uw_slot(SLOT_YEAR, stattime, &dummy_low, &dummy_high);
  if (cur_slot != prev_slot) { // new slot. summarize records with old slot index
    summarize_ping(probe, "month", "year", slotlow, slothigh);
  }

  prev_slot = uw_slot(SLOT_YEAR5, prv_stattime, &slotlow, &slothigh);
  cur_slot = uw_slot(SLOT_YEAR5, stattime, &dummy_low, &dummy_high);
  if (cur_slot != prev_slot) { // new slot. summarize records with old slot index
    summarize_ping(probe, "year", "year5", slotlow, slothigh);
  }

  result = my_query("update pr_ping_def "
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

static void summarize_ping(int probe, char *from, char *into, guint slotlow, guint slothigh)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  float avg_value, min_lowest, max_highest; 
  guint stattime;
  guint max_color;

  stattime = slotlow + ((slothigh-slotlow)/2);

  result = my_query("select avg(lowest), avg(value), " 
                    "       avg(highest), max(color) " 
                    "from   pr_ping_%s "
                    "where  probe = '%d' and stattime >= %d and stattime < %d",
                    from, probe, slotlow, slothigh);
  
  row = mysql_fetch_row(result);
  if (!row) {
    mysql_free_result(result);
    LOG(LOG_ERR, mysql_error(mysql));
    return;
  }
  if (row[0] == NULL) {
    LOG(LOG_ERR, "Internal error: nothing to summarize %s: %d %d", into, slotlow, slothigh);
    return;
  }

  min_lowest  = atof(row[0]);
  avg_value   = atof(row[1]);
  max_highest = atof(row[2]);
  max_color   = atoi(row[3]);
  mysql_free_result(result);

  result = my_query("insert into pr_ping_%s " 
                    "set    value = %f, lowest = %f, highest = %f, probe = %d, color = '%u', stattime = %d",
                    into, avg_value, min_lowest, max_highest, probe, max_color, stattime);
  mysql_free_result(result);
}

