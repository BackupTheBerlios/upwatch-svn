#include <config.h>
#include <generic.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <probe.h>
#include "uw_process_glob.h"

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <generic.h>
#include "slot.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct summ_spec summ_info[] = {
  { SLOT_DAY,   0,  "raw",   "day"   },
  { SLOT_WEEK,  7,  "day",   "week"  },
  { SLOT_MONTH, 4,  "week",  "month" },
  { SLOT_YEAR,  12, "month", "year"  },
  { SLOT_YEAR5, 5,  "year",  "5year" },
  { -1,         0,  NULL,    NULL    }
};

void free_res(void *res)
{
  struct probe_result *r = (struct probe_result *)res;

  if (r->name) g_free(r->name);
  if (r->message) g_free(r->message);
  if (r->hostname) g_free(r->hostname);
  if (r->ipaddress) g_free(r->ipaddress);
  if (r->realm) g_free(r->realm);
  g_free(r);
}

//*******************************************************************
// RETRIEVE PRECEDING RAW RECORD FROM DATABASE (well just the color)
//*******************************************************************
static struct probe_result *get_previous_record(trx *t)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct probe_result *prv;

  prv = g_malloc0(sizeof(struct probe_result));
  if (prv == NULL) {
    return(NULL);
  }
  prv->stattime = t->res->stattime;
  prv->color = t->res->color;

  if (!t->probe->store_results) return(prv); // we probably don't have a xxxx_raw table

  result = my_query(t->probe->db, 0,
                    "select   color, stattime "
                    "from     pr_%s_raw use index(probstat) "
                    "where    probe = '%u' and stattime < '%u' "
                    "order by stattime desc limit 1", 
                    t->res->name, t->def->probeid, t->res->stattime);
  if (!result) return(prv);
  row = mysql_fetch_row(result);
  if (row) {
    prv->color = atoi(row[0]);
    prv->stattime = atoi(row[1]);
  }
  mysql_free_result(result);
  t->res->prevcolor = prv->color;
  return(prv);
}

//*******************************************************************
// If we have an email address, add notify info with 
// the previous color
//*******************************************************************
static void set_result_prev_color(trx *t, struct probe_result *prv)
{
  char buffer[10];

  sprintf(buffer, "%d", prv->color);
  xmlNewChild(t->cur, NULL, "prevcolor", buffer);
}

//*******************************************************************
// RETRIEVE FOLLOWING RAW RECORD FROM DATABASE (well just the color)
//*******************************************************************
static struct probe_result *get_following_record(trx *t)
{
  MYSQL_RES *result;
  struct probe_result *nxt = NULL;

  if (!t->probe->store_results) return(NULL); // we probably don't have a xxxx_raw table

  result = my_query(t->probe->db, 0,
                    "select   color, stattime "
                    "from     pr_%s_raw use index(probstat) "
                    "where    probe = '%u' and stattime > '%u' "
                    "order by stattime asc limit 1", 
                    t->res->name, t->def->probeid, t->res->stattime);
  if (result) {
    MYSQL_ROW row;

    row = mysql_fetch_row(result);
    if (row) {
      nxt = g_malloc0(sizeof(struct probe_result));

      nxt->color = atoi(row[0]);
      nxt->stattime = atoi(row[1]);
    }
    mysql_free_result(result);
  }
  return(nxt);
}

//*******************************************************************
// UPDATE PR_STATUS
//*******************************************************************
static void update_pr_status(trx *t, struct probe_result *prv)
{
  MYSQL_RES *result;
  char *qry;

  qry = g_malloc(512 + (t->res->message ? strlen(t->res->message)*2 : 0));

  sprintf(qry, "update pr_status "
               "set    stattime = '%u', expires = '%u', hide = '%s', " 
               "       contact = '%u', server = '%u'", 
               t->res->stattime, t->res->expires, t->def->hide, 
               t->def->contact, t->def->server);

  if (t->probe->fuse) {
    if (t->res->color > prv->color || prv->color == STAT_PURPLE) {
    // if this probe acts like a fuse, only update color if new color is higher then old color
    // because colors must be set to green (= fuse replaced) by a human
      sprintf(&qry[strlen(qry)], ", color = '%u'", t->res->color);
    } 
  } else {
    sprintf(&qry[strlen(qry)], ", color = '%u'", t->res->color);
  }

  if (t->res->message) {
    char *escmsg;

    escmsg = g_malloc(strlen(t->res->message) * 2 + 1);
    mysql_real_escape_string(t->probe->db, escmsg, t->res->message, strlen(t->res->message));

    if (t->res->color != prv->color) {
      sprintf(&qry[strlen(qry)], ", message = '%s'", escmsg);
    } else {
      sprintf(&qry[strlen(qry)], ", message = concat(message,'%s')", escmsg);
    }
    free(escmsg);
  }

  sprintf(&qry[strlen(qry)], " where probe = '%u' and class = '%u'", t->def->probeid, t->probe->class);
  result = my_rawquery(t->probe->db, 0, qry);
  mysql_free_result(result);
  g_free(qry);
}

//*******************************************************************
// CREATE PR_STATUS RECORD
//*******************************************************************
static void insert_pr_status(trx *t)
{
  MYSQL_RES *result;
  char *escmsg;

  if (t->res->message) {
    escmsg = g_malloc(strlen(t->res->message) * 2 + 1);
    mysql_real_escape_string(t->probe->db, escmsg, t->res->message, strlen(t->res->message)) ;
  } else {
    escmsg = strdup("");
  }

  result = my_query(t->probe->db, 0,
                    "insert into pr_status "
                    "set    class =  '%u', probe = '%u', stattime = '%u', expires = '%u', "
                    "       color = '%u', server = '%u', message = '%s', "
                    "       contact = '%u', hide = '%s'",
                    t->probe->class, t->def->probeid, t->res->stattime, t->res->expires, 
                    t->def->color, t->def->server, escmsg, t->def->contact, t->def->hide);
  mysql_free_result(result);
  g_free(escmsg);
}

//*******************************************************************
// CREATE PR_HIST
//*******************************************************************
static void create_pr_hist(trx *t, struct probe_result *prv)
{
  MYSQL_RES *result;
  char *escmsg;

  if (t->res->message) {
    escmsg = g_malloc(strlen(t->res->message) * 2 + 1);
    mysql_real_escape_string(t->probe->db, escmsg, t->res->message, strlen(t->res->message)) ;
  } else {
    escmsg = strdup("");
  }

  result = my_query(t->probe->db, 0,
                    "insert into pr_hist "
                    "set    server = '%u', class = '%u', probe = '%u', stattime = '%u', "
                    "       prv_color = '%d', color = '%d', message = '%s', contact = '%u', "
                    "       hide = '%s', pgroup = '%u'",
                    t->def->server, t->probe->class, t->def->probeid, t->res->stattime,
                    /* (t->res->received > t->res->expires) ? STAT_PURPLE : */ prv->color, 
                    t->res->color, escmsg, t->def->contact, t->def->hide, t->def->pgroup);
  mysql_free_result(result);
  g_free(escmsg);
}

//*******************************************************************
// REMOVE ALL OCCURRENCES OF A PROBE IN THE HISTORY FILES
//*******************************************************************
static void delete_history(trx *t, struct probe_result *nxt)
{
  MYSQL_RES *result;

  result = my_query(t->probe->db, 0,
                    "delete from pr_hist "
                    "where stattime = '%u' and probe = '%u' and class = '%d'",
                    nxt->stattime, t->def->probeid, t->probe->class);
  mysql_free_result(result);
  result = my_query(t->probe->db, 0,
                    "delete from pr_status "
                    "where stattime = '%u' and probe = '%u' and class = '%d'",
                    nxt->stattime, t->def->probeid, t->probe->class);
  mysql_free_result(result);
}

//*******************************************************************
// UPDATE SERVER COLOR
//*******************************************************************
static void update_server_color(trx *t, struct probe_result *prv)
{
  MYSQL_RES *result;
  int maxcolor, newcolor;

  if (t->def->server < 2) return;

  if (t->probe->fuse && (t->res->color < prv->color)) {
    // if this probe acts like a fuse, use the previous color if it's higher
    newcolor = prv->color;
  } else {
    newcolor = t->res->color;
  }
  maxcolor = newcolor; // init

  result = my_query(t->probe->db, 0,
                    "select max(color) as color from pr_status where server = '%u'", t->def->server);
  if (result) {
    MYSQL_ROW row;
    row = mysql_fetch_row(result);
    if (row && row[0]) {
      maxcolor = atoi(row[0]);
    }
  }
  mysql_free_result(result);
  if (t->res->color <= maxcolor) return;

  result = my_query(t->probe->db, 0,
                    // update server set color = '200' where id = '345'
                    "update %s set %s = '%u' where %s = '%u'",
                     OPT_ARG(SERVER_TABLE_NAME), OPT_ARG(SERVER_TABLE_COLOR_FIELD), 
                     t->res->color, OPT_ARG(SERVER_TABLE_ID_FIELD), t->def->server);
  mysql_free_result(result);
}

//*******************************************************************
// SEE IF THE USER HAS BEEN NOTIFIED FOR THIS COLOR
//*******************************************************************
void get_previous_pr_hist(trx *t)
{
  MYSQL_RES *result;
  MYSQL_ROW row;

  t->res->changed = 0;
  strcpy(t->res->notified, "yes"); // if we cannot find pr_hist record, assume the user has already been warned
  result = my_query(t->probe->db, 0,
                    "select   id, stattime, notified, prv_color "
                    "from     pr_hist  "
                    "where    probe = '%u' and class = '%u' and stattime <= '%u' "
                    "order by stattime desc limit 1",
                    t->def->probeid, t->probe->class, t->res->stattime);
  if (!result) return;
  row = mysql_fetch_row(result);
  if (row) {
    t->res->prevhistid = atoll(row[0]);
    t->res->changed = atoi(row[1]);
    strcpy(t->res->notified, row[2]);
    t->res->prevhistcolor = atoi(row[3]);
  }
  mysql_free_result(result);
  return;
}

//*******************************************************************
// INDICATE WE HAVE NOTIFIED THIS USER
//*******************************************************************
void set_pr_hist_notified(trx *t)
{
  MYSQL_RES *result;

  result = my_query(t->probe->db, 0,
                    "update pr_hist set notified = 'yes' "
                    "where  id = '%ull'", t->res->prevhistid);
  mysql_free_result(result);
}

//*******************************************************************
// IS THIS SLOT COMPLETELY FILLED?
//*******************************************************************
int slot_is_complete(trx *t, int i, guint slotlow, guint slothigh)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  int val = FALSE;

  /* 
   * don't know how many raw records go into a day slot, 
   * so we fake it is filled, so it'll always be summarized
   */
  if (i == 0) return 1;
  result = my_query(t->probe->db, 0,
                    "select count(*) from pr_%s_%s use index(probstat) "
                    "where  probe = '%u' and stattime >= '%u' and stattime <= '%u'",
                    t->res->name, summ_info[i].from, t->def->probeid, slotlow, slothigh);
  if (!result) return(FALSE);
  row = mysql_fetch_row(result);
  if (row && row[0]) {
    if (atoi(row[0]) >= summ_info[i].perslot) {
      val = TRUE;
    }
  }
  mysql_free_result(result);
  return(val);
}

/*
 * PSEUDO CODE:
 *
 * EXTRACT INFO FROM XML NODE
 * RETRIEVE PROBE DEFINITION RECORD FROM DATABASE
 * RETRIEVE PRECEDING RAW RECORD FROM DATABASE
 * STORE RAW RESULTS
 * IF THIS IS THE FIRST RESULT EVER FOR THIS PROBE
 *   CREATE PR_STATUS RECORD
 * ELSE
 *   IF WE HAVEN'T PROCESSED THIS RECORD BEFORE
 *     IF COLOR DIFFERS FROM PRECEDING RAW RECORD
 *       CREATE PR_HIST
 *       RETRIEVE FOLLOWING RAW RECORD
 *       IF FOUND AND COLOR OF FOLLOWING IS THE SAME AS CURRENT
 *         DELETE POSSIBLE HISTORY RECORDS FOR FOLLOWING RECORD
 *       ENDIF
 *       IF THIS RAW RECORD IS THE MOST RECENT EVER RECEIVED
 *         WRITE NEW RECENT TIME INTO PROBE DEFINITION RECORD
 *         UPDATE PR_STATUS
 *         UPDATE SERVER COLOR
 *       ENDIF
 *     ELSE
 *       IF THIS RAW RECORD IS THE MOST RECENT EVER RECEIVED
 *         WRITE NEW RECENT TIME INTO PROBE DEFINITION RECORD
 *       ENDIF
 *     ENDIF
 *     IF CURRENT RAW RECORD IS THE MOST RECENT
 *       FOR EACH PERIOD
 *         IF WE ENTERED A NEW SLOT
 *           SUMMARIZE PREVIOUS SLOT
 *         ENDIF
 *       ENDFOR
 *     ELSE
 *       FOR EACH PERIOD
 *         IF THE LAST RECORD FOR THIS SLOT HAS BEEN SEEN
 *           RE-SUMMARIZE CURRENT SLOT
 *         ENDIF
 *       ENDFOR
 *     ENDIF
 *   ENDIF
 * ENDIF
 *
 * returns:
 *  1 in case of success
 *  0 in case of database failure where trying again later might help
 * -1 in case of malformed input, never try again
 * -2 in case of a fatal error, just skip this batch
 */
int process(trx *t)
{
  int must_update_def=0;
  struct probe_result *prv=NULL;
  int err = 1; /* default ok */

  if (!realm_exists(t->res->realm)) {
    return -1;
  }

  if (t->res->realm && t->res->realm[0]) {
    t->probe->db = open_realm(t->res->realm);
  } else {
    if (t->probe->find_realm) {
      t->probe->find_realm(t);
    } else {
      t->probe->db = open_realm(NULL);
    }
  }
  if (!t->probe->db) return -2;

  if (t->probe->resultcount % 400 == 0) {
    update_last_seen(t->probe);
  }

  if (debug > 3) fprintf(stderr, "accept_result\n");
  if (t->probe->accept_result) {
    t->probe->accept_result(t); // do some final calculations on the result
  }

  if (debug > 3) fprintf(stderr, "get_def\n");
  if (t->probe->get_def) {
    if (debug > 3) fprintf(stderr, "RETRIEVE PROBE DEFINITION RECORD FROM DATABASE\n");
    t->def = t->probe->get_def(t, trust(t->res->name)); // RETRIEVE PROBE DEFINITION RECORD FROM DATABASE
  } else {
    t->def = get_def(t, trust(t->res->name));
  }
  if (!t->def) {  // Oops, def record not found. Skip this probe
    if (debug > 3) fprintf(stderr, "def NOT found\n");
    err = -1; /* malformed input FIXME should make distinction between db errors and def not found */
    goto exit_with_res;
  }
  if (t->probe->adjust_result) {
    t->probe->adjust_result(t);
  }

  if (debug > 3) fprintf(stderr, "STORE RAW RESULTS\n");
  if (t->probe->store_results) {
    int ret = t->probe->store_results(t); // STORE RAW RESULTS
    if (!ret) { /* error return? */
      if (debug > 3) fprintf(stderr, "error in store_results\n");
      err = -2; /* database fatal error - try again later */
      goto exit_with_res;
    }
  } else {
    t->seen_before = FALSE;
  }

  if (t->res->stattime > t->def->newest) { // IF CURRENT RAW RECORD IS THE MOST RECENT 
    if (debug > 3) fprintf(stderr, "CURRENT RAW RECORD IS THE MOST RECENT\n");
    prv = g_malloc0(sizeof(struct probe_result));
    prv->color = t->def->color;  // USE PREVIOUS COLOR FROM DEF RECORD
    prv->stattime = t->def->newest;
  } else {
    if (debug > 3) fprintf(stderr, "RETRIEVE PRECEDING RAW RECORD FROM DATABASE\n");
    prv = get_previous_record(t); // RETRIEVE PRECEDING RAW RECORD FROM DATABASE
  }

  set_result_prev_color(t, prv); // indicate previous color in result set

  if (t->def->email[0]) { // and if email address given, add simple notification record
    xmlNodePtr notify;

    notify = xmlNewChild(t->cur, NULL, "notify", NULL);
    xmlSetProp(notify, "proto", "smtp");
    xmlSetProp(notify, "target", t->def->email);
  }

  if (t->def->newest == 0) { // IF THIS IS THE FIRST RESULT EVER FOR THIS PROBE
    if (debug > 3) fprintf(stderr, "THIS IS THE FIRST RESULT EVER FOR THIS PROBE\n");
    insert_pr_status(t);
    must_update_def = TRUE;
    goto finish;
  }
  if (t->seen_before) {
    goto finish;
  }

  // IF COLOR DIFFERS FROM PRECEDING RAW RECORD
  if (t->res->color != prv->color) {
    struct probe_result *nxt;

    if (t->probe->fuse) {
      if (t->res->color > prv->color || prv->color == STAT_PURPLE) {
        if (debug > 3) fprintf(stderr, "FUSE WITH HIGHER COLOR - CREATE PR_HIST\n");
        create_pr_hist(t, prv); // CREATE PR_HIST
      }
    } else {
      if (debug > 3) fprintf(stderr, "COLOR DIFFERS FROM PRECEDING RAW RECORD - CREATE PR_HIST\n");
      create_pr_hist(t, prv); // CREATE PR_HIST
    }
    if (t->res->stattime > t->def->newest) { // IF THIS RAW RECORD IS THE MOST RECENT EVER RECEIVED
      if (debug > 3) fprintf(stderr, "THIS RAW RECORD IS THE MOST RECENT EVER RECEIVED - UPDATE PR_STATUS\n");
      update_pr_status(t, prv);    // UPDATE PR_STATUS
      if (mysql_affected_rows(t->probe->db) == 0) { // nothing was actually updated, need to insert new
        insert_pr_status(t); 
      }
      if (debug > 3) fprintf(stderr, "UPDATE SERVER COLOR\n");
      update_server_color(t, prv); // UPDATE SERVER COLOR
      must_update_def = TRUE;
    } else {
      if (debug > 3) fprintf(stderr, "RETRIEVE FOLLOWING RAW RECORD\n");
      nxt = get_following_record(t); // RETRIEVE FOLLOWING RAW RECORD
      if (nxt && nxt->color) { // IF FOUND
        if (debug > 3) fprintf(stderr, "FOLLOWING RECORD IS FOUND\n");
        if (nxt->color == t->res->color) {  // IF COLOR OF FOLLOWING IS THE SAME AS CURRENT
          if (debug > 3) fprintf(stderr, "SAME COLOR: DELETE POSSIBLE HISTORY RECORDS\n");
          delete_history(t, nxt); // DELETE POSSIBLE HISTORY RECORDS FOR FOLLOWING RECORD
        }
        g_free(nxt);
      }
    }
  } else {
    if (debug > 3) fprintf(stderr, "COLOR SAME AS PRECEDING RAW RECORD\n");
    if (t->res->stattime > t->def->newest) { // IF THIS RAW RECORD IS THE MOST RECENT EVER RECEIVED
      if (debug > 3) fprintf(stderr, "THIS RAW RECORD IS THE MOST RECENT EVER RECEIVED - UPDATE PR_STATUS\n");
      update_pr_status(t, prv);  // UPDATE PR_STATUS (not for the color, but for the expiry time)
      if (mysql_affected_rows(t->probe->db) == 0) { // nothing was actually updated, need to insert new
        insert_pr_status(t); 
      }
      must_update_def = TRUE;
    }
  }

  if (t->def->email[0]) { // if we have an address
    // RETRIEVE LAST HIST ENTRY FOR THIS PROBE
    get_previous_pr_hist(t);

    // notify if needed
    if (strcmp(t->res->notified, "yes")) { // not already notified
      if (notify(t)) {
        set_pr_hist_notified(t);
      }
    }
  }

  if (t->probe->summarize && t->res->color != STAT_PURPLE) { 
    if (t->res->stattime > t->def->newest) { // IF CURRENT RAW RECORD IS THE MOST RECENT
      guint cur_slot, prev_slot;
      gulong slotlow, slothigh;
      gulong dummy_low, dummy_high;
      gint i;

      if (debug > 3) fprintf(stderr, "SUMMARIZING. CURRENT RAW RECORD IS THE MOST RECENT\n");
      for (i=0; summ_info[i].period != -1; i++) { // FOR EACH PERIOD
        prev_slot = uw_slot(summ_info[i].period, prv->stattime, &slotlow, &slothigh);
        cur_slot = uw_slot(summ_info[i].period, t->res->stattime, &dummy_low, &dummy_high);
        if (cur_slot != prev_slot) { // IF WE ENTERED A NEW SLOT, SUMMARIZE PREVIOUS SLOT
          if (debug > 3)  fprintf(stderr, "cur(%u for %u) != prv(%u for %u), summarizing %s from %lu to %lu",
                                          cur_slot, t->res->stattime, prev_slot, prv->stattime,
                                          summ_info[i].from, slotlow, slothigh);
          t->probe->summarize(t, summ_info[i].from, summ_info[i].to, 
                           cur_slot, slotlow, slothigh, 0);
        }
      }
    } else {
      guint cur_slot;
      gulong slotlow, slothigh;
      gulong not_later_then = UINT_MAX;
      gint i;

      if (debug > 3) {
        fprintf(stderr, "SUMMARIZING. CURRENT RAW RECORD IS AN OLD ONE\n");
        LOG(LOG_DEBUG, "stattime = %u, newest = %u for %s %u", t->res->stattime, t->def->newest,
          t->res->name, t->def->probeid);
      }
      for (i=0; summ_info[i].period != -1; i++) { // FOR EACH PERIOD
        cur_slot = uw_slot(summ_info[i].period, t->res->stattime, &slotlow, &slothigh);
        if (slothigh > not_later_then) continue; // we already know there are none later then this
        // IF THIS SLOT IS COMPLETE
        if (slot_is_complete(t, i, slotlow, slothigh)) {
          // RE-SUMMARIZE CURRENT SLOT
          if (debug > 3) fprintf(stderr, "SLOT IS COMPLETE - RE-SUMMARIZE CURRENT SLOT\n");
          t->probe->summarize(t, summ_info[i].from, summ_info[i].to, 
                           cur_slot, slotlow, slothigh, 0);
        } else {
          not_later_then = slothigh;
        }
      }
    }
  }

finish:
  if (must_update_def) {
    t->def->newest = t->res->stattime;
    t->def->color = t->res->color;
  }
  g_free(prv);

exit_with_res:
  if (t->probe->end_result) {
    t->probe->end_result(t);
  }

  // free the result block
  if (t->res) {
    if (t->probe->free_res) {
      t->probe->free_res(t->res); // the probe specific part...
    }
    free_res(t->res); // .. and the generic part
  }

  // note we don't free the *t->def here, because that structure is owned by the hashtable
  return err;
}

