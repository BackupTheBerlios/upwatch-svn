#include <config.h>
#include <generic.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <probe.h>
#include "uw_notify_glob.h"

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <generic.h>
#include "slot.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

void free_res(void *res)
{
  struct probe_result *r = (struct probe_result *)res;

  if (r->name) g_free(r->name);
  if (r->message) g_free(r->message);
  if (r->hostname) g_free(r->hostname);
  if (r->ipaddress) g_free(r->ipaddress);
  g_free(r);
}

void get_previous_pr_hist(trx *t)
{
  MYSQL_RES *result;
  MYSQL_ROW row;

  t->def->changed = 0;
  strcpy(t->def->notified, "yes"); // if we cannot find pr_hist record, assume the user has already been warned
  result = my_query(t->probe->db, 0,
                    "select   id, stattime, notified "
                    "from     pr_hist  "
                    "where    probe = '%u' and class = '%u' and stattime <= '%u' "
                    "order by stattime desc limit 1",
                    t->def->probeid, t->probe->class, t->res->stattime);
  if (!result) return;
  row = mysql_fetch_row(result);
  if (row) {
    t->def->prevhistid = atoll(row[0]);
    t->def->changed = atoi(row[1]);
    strcpy(t->def->notified, row[2]);
  }
  mysql_free_result(result);
  return;
}

void set_pr_hist_notified(trx *t)
{
  MYSQL_RES *result;

  result = my_query(t->probe->db, 0,
                    "update pr_hist set notified = 'yes' "
                    "where  id = '%ull'", t->def->prevhistid);
  mysql_free_result(result);
}

/*
 * PSEUDO CODE:
 *
 * EXTRACT INFO FROM XML NODE
 * RETRIEVE PROBE DEFINITION RECORD FROM DATABASE
 *
 * returns:
 *  1 in case of success
 *  0 in case of database failure where trying again later might help
 * -1 in case of malformed input, never try again
 * -2 in case of a fatal error, just skip this batch
 */
int process(trx *t)
{
  int err = 1; /* default ok */

  if (!t->probe->db) {
    t->probe->db = open_database(OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME),
                              OPT_ARG(DBUSER), OPT_ARG(DBPASSWD),
                              OPT_VALUE_DBCOMPRESS);
  }

  if (debug > 3) fprintf(stderr, "accept_result\n");
  if (t->probe->accept_result) {
    t->probe->accept_result(t); // do some final calculations on the result
  }

  if (debug > 3) fprintf(stderr, "get_def\n");
  if (t->probe->get_def) {
    if (debug > 3) fprintf(stderr, "RETRIEVE PROBE DEFINITION RECORD FROM DATABASE\n");
    t->def = t->probe->get_def(t, FALSE); // RETRIEVE PROBE DEFINITION RECORD FROM DATABASE
  } else {
    t->def = get_def(t, FALSE);
  }
  if (!t->def) {  // Oops, def record not found. Skip this probe
    if (debug > 3) fprintf(stderr, "def NOT found\n");
    err = -1; /* malformed input FIXME should make distinction between db errors and def not found */
    goto exit_with_res;
  }

  // RETRIEVE HIST ENTRY RIGHT BEFORE THIS PROBE
  get_previous_pr_hist(t);

  // notify if needed
  if (strcmp(t->def->notified, "yes")) { // not already notified?
    if (notify(t)) {
      set_pr_hist_notified(t);
    }
  }

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

