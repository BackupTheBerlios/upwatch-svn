#include "config.h"
#include <string.h>
#include <generic.h>
#include "cmd_options.h"
#include "slot.h"
#include "uw_process.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static void do_notification(module *probe, struct probe_def *def, struct probe_result *res, struct probe_result *prv);

//*******************************************************************
// Notify user if necessary
//*******************************************************************
void notify(module *probe, struct probe_def *def, struct probe_result *res, struct probe_result *prv)
{
  unsigned int now = (unsigned int) time(NULL);
  int color = res->color;
//  int prevwasblue;
//  int expiretime = res->expires - res->stattime;

  if (now > res->expires) color = STAT_BLUE;

  if (debug > 3) fprintf(stderr, "%s %u %s (was %s). ", probe->module_name, def->probeid, 
                  color2string(res->color), color2string(prv->color));
  switch (color) {
  case STAT_RED:
    if (prv->color == STAT_RED || def->redmins == 0) {
      if (strcmp(def->notified, "yes") == 0) {
        if (debug > 3) fprintf(stderr, "Already notified\n");
        break; // already notified
      }
      if (def->email[0] == 0) {
        if (debug > 3) fprintf(stderr, "No email address\n");
        break; // nowhere to email to
      }
      if (res->stattime - def->changed < (def->redmins * 60)) {
        if (debug > 3) fprintf(stderr, "Not RED long enough\n");
        break; // NOT YET
      }
      do_notification(probe, def, res, prv);
      break;
    }
    break;

  case STAT_BLUE:
    if (strcmp(def->notified, "yes") == 0) {
      if (debug > 3) fprintf(stderr, "Already notified\n");
      break; // already notified
    }
    if (def->email[0] == 0) {
      if (debug > 3) fprintf(stderr, "No email address\n");
      break; // nowhere to email to
    }
    if (res->stattime - def->changed < (def->redmins * 60)) {
        if (debug > 3) fprintf(stderr, "Not BLUE long enough\n");
      break; // NOT YET
    }
    do_notification(probe, def, res, prv);
    break;

  case STAT_YELLOW:
    if (prv->color == STAT_RED || prv->color == STAT_GREEN) {
      if (def->email[0] == 0) {
        if (debug > 3) fprintf(stderr, "No email address\n");
        break; // nowhere to email to
      }
      do_notification(probe, def, res, prv);
      break;
    }
    if (prv->color == STAT_BLUE) {
      if (def->email[0] == 0) {
        if (debug > 3) fprintf(stderr, "No email address\n");
        break; // nowhere to email to
      }
      do_notification(probe, def, res, prv);
      break;
    }
    break;

  case STAT_GREEN:
    if (prv->color == STAT_RED || prv->color == STAT_YELLOW) {
      if (def->email[0] == 0) {
        if (debug > 3) fprintf(stderr, "No email address\n");
        break; // nowhere to email to
      }
      do_notification(probe, def, res, prv);
      break;
    }
    if (prv->color == STAT_BLUE) {
      if (def->email[0] == 0) {
        if (debug > 3) fprintf(stderr, "No email address\n");
        break; // nowhere to email to
      }
      do_notification(probe, def, res, prv);
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

#include <libesmtp.h>

#define BUFLEN 8192

static const char*
libesmtp_messagefp_cb(void **buf, int *len, void *arg)
{
  int octets;

  if (*buf == NULL)
    *buf = malloc(BUFLEN);

  if (len == NULL) {
    rewind((FILE*) arg);
    return NULL;
  }

  if (fgets(*buf, BUFLEN - 2, (FILE*) arg) == NULL) {
    octets = 0;
  } else {
    char* p = strchr(*buf, '\0');

    if (p[-1] == '\n' && p[-2] != '\r') {
      strcpy(p - 1, "\r\n");
      p++;
    }
    octets = p - (char*) *buf;
  }

  *len = octets;
  return *buf;
}

static void do_notification(module *probe, struct probe_def *def, struct probe_result *res, struct probe_result *prv)
{
#if HAVE_LIBESMTP
  smtp_session_t session;
  smtp_message_t message;
  char buf[256];
  FILE *fp;

  session = smtp_create_session ();
  sprintf(buf, "%s:%lu", OPT_ARG(SMTPSERVER), OPT_VALUE_SMTPSERVERPORT);
  smtp_set_server(session, buf);

  message = smtp_add_message (session);
  smtp_set_reverse_path (message, OPT_ARG(FROM_EMAIL));
  smtp_set_header (message, "Date", &res->stattime);
  smtp_add_recipient (message, def->email);

  fp = tmpfile();
  if (!fp) {
    LOG(LOG_NOTICE, "tmpfile: %m");
    return;
  }
  fprintf(fp, "From: %s <%s>\n", OPT_ARG(FROM_NAME), OPT_ARG(FROM_EMAIL)); 
  fprintf(fp, "To: %s\n", def->email); 
  fprintf(fp, "Subject: %s %s -> %s op %s\n", probe->module_name, color2string(prv->color), 
                   color2string(res->color), query_server_by_id(probe, def->server));
  fprintf(fp, "\n");
  fprintf(fp, "Geachte klant,\n\n"
              "zojuist, om %s"
              "is de status van de probe %s\n"
              "op server %s\n"
              "overgegaan van %s in %s\n", ctime((time_t *)&res->stattime), probe->module_name, 
                   query_server_by_id(probe, def->server), color2string(prv->color), color2string(res->color));
  if (res->message) {
    fprintf(fp, "\nDe volgende melding werd hierbij gegeven:\n%s\n", res->message);
  }
  fprintf(fp, "\n"
              "vriendelijke groet\n"
              "%s\n", OPT_ARG(FROM_NAME));
  smtp_set_messagecb(message, libesmtp_messagefp_cb, fp);

  // Initiate a connection to the SMTP server and transfer the message.
  if (!smtp_start_session (session)) {
    char buf[128];

    smtp_strerror (smtp_errno (), buf, sizeof(buf));
    LOG(LOG_NOTICE, "%s: %s", "localhost:25", buf);
  } else {
    const smtp_status_t *status;

    strcpy(def->notified, "yes");
    if (probe->db) {
      MYSQL_RES *result;

      result = my_query(probe->db, 0,
                        "update pr_status set notified = 'yes' "
                        "where  probe = '%u' and class = '%u'",
                        def->probeid, probe->class);

      if (result) mysql_free_result(result);
    }

    // Report on the success or otherwise of the mail transfer. 
    status = smtp_message_transfer_status (message);
    LOG(LOG_INFO, "MAILTO: %s %d %s", def->email, status->code, status->text ? status->text : "");
    if (debug > 3) fprintf(stderr, "MAILTO: %s %d %s", def->email, status->code, status->text ? status->text : "");
  }
  smtp_destroy_session (session);
  fclose(fp);
#endif /* HAVE_LIBESMTP */
}


