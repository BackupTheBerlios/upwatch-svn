#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#ifdef UW_PROCESS
#include "uw_process_glob.h"
#endif
#ifdef UW_NOTIFY
#include "uw_notify_glob.h"
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static void do_notification(module *probe, trx *t, struct probe_result *prv);

//*******************************************************************
// Notify user if necessary
//*******************************************************************
void notify(module *probe, trx *t, struct probe_result *prv)
{
  int color = t->res->color;
//  int prevwasblue;
//  int expiretime = t->res->expires - t->res->stattime;

  if (t->res->received > t->res->expires) color = STAT_BLUE;

  if (debug > 2) fprintf(stderr, "%s %u %s (was %s). %s\n", probe->module_name, t->def->probeid, 
                  color2string(t->res->color), color2string(prv->color), t->res->hostname);
  switch (color) {
  case STAT_RED:
    if (prv->color == STAT_RED || t->def->redmins == 0) {
      if (strcmp(t->def->notified, "yes") == 0) {
        if (debug > 3) fprintf(stderr, "Already notified\n");
        break; // already notified
      }
      if (t->def->email[0] == 0) {
        if (debug > 3) fprintf(stderr, "No email address\n");
        break; // nowhere to email to
      }
      if (t->res->stattime - t->def->changed < (t->def->redmins * 60)) {
        if (debug > 3) fprintf(stderr, "Not RED long enough\n");
        break; // NOT YET
      }
      do_notification(probe, t, prv);
      break;
    }
    break;

  case STAT_BLUE:
    if (strcmp(t->def->notified, "yes") == 0) {
      if (debug > 3) fprintf(stderr, "Already notified\n");
      break; // already notified
    }
    if (t->def->email[0] == 0) {
      if (debug > 3) fprintf(stderr, "No email address\n");
      break; // nowhere to email to
    }
    do_notification(probe, t, prv);
    break;

  case STAT_YELLOW:
    if (prv->color == STAT_RED || prv->color == STAT_GREEN) {
      if (t->def->email[0] == 0) {
        if (debug > 3) fprintf(stderr, "No email address\n");
        break; // nowhere to email to
      }
      do_notification(probe, t, prv);
      break;
    }
    if (prv->color == STAT_BLUE) {
      if (t->def->email[0] == 0) {
        if (debug > 3) fprintf(stderr, "No email address\n");
        break; // nowhere to email to
      }
      do_notification(probe, t, prv);
      break;
    }
    break;

  case STAT_GREEN:
    if (prv->color == STAT_RED || prv->color == STAT_YELLOW) {
      if (t->def->email[0] == 0) {
        if (debug > 3) fprintf(stderr, "No email address\n");
        break; // nowhere to email to
      }
      do_notification(probe, t, prv);
      break;
    }
    if (prv->color == STAT_BLUE) {
      if (t->def->email[0] == 0) {
        if (debug > 3) fprintf(stderr, "No email address\n");
        break; // nowhere to email to
      }
      do_notification(probe, t, prv);
      break;
    }
    break;

  default:
    break;
  }
}

char *query_server_by_id(module *probe, int id)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
static char name[256];

  if (probe->db == NULL) return("unknown");
  result = my_query(probe->db, 0,
                    OPT_ARG(QUERY_SERVER_BY_ID), id, id, id, id, id);
  if (!result) return("");
  row = mysql_fetch_row(result);
  if (row && row[0]) {
    strcpy(name, row[0]);
  } else {
    LOG(LOG_NOTICE, "name for server %u not found", id);
    mysql_free_result(result);
    return("");
  }
  mysql_free_result(result);
  return(name);
}

static void do_notification(module *probe, trx *t, struct probe_result *prv)
{
  xmlNodePtr notify;

  notify = xmlNewChild(t->node, NULL, "notify", NULL);
  xmlSetProp(notify, "proto", "smtp");
  xmlSetProp(notify, "target", t->def->email);
  t->notify = TRUE;
}

