#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#include "uw_notify_glob.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static int do_notification(trx *t);

//*******************************************************************
// Notify user if necessary - if we enter this function, we know
// that the color changed, and the user has not been warned yet.
//*******************************************************************
int notify(trx *t)
{
  int notified = FALSE;
  int color = t->res->color;

//  if (t->res->received > t->res->expires) color = STAT_BLUE;

  if (debug > 3) fprintf(stderr, "%s %u %s (was %s). %s\n", t->probe->module_name, t->def->probeid, 
                  color2string(t->res->color), color2string(t->res->prevcolor), t->res->hostname);

  if (t->res->stattime - t->def->changed < (t->def->delay * 60)) {
    if (debug > 3) fprintf(stderr, "Not RED long enough\n");
    return(notified); // NOT YET
  }
  switch (color) {
  case STAT_RED:
    switch (t->res->prevcolor) {
    case STAT_RED:
      notified = do_notification(t);
      break;
    case STAT_YELLOW:
      notified = do_notification(t);
      break;
    case STAT_GREEN:
      notified = do_notification(t);
      break;
    }
    break;

  case STAT_YELLOW:
    switch (t->res->prevcolor) {
    case STAT_RED:
      notified = do_notification(t);
      break;
    case STAT_YELLOW:
      notified = do_notification(t);
      break;
    case STAT_GREEN:
      notified = do_notification(t);
      break;
    }
    break;

  case STAT_GREEN:
    switch (t->res->prevcolor) {
    case STAT_RED:
      notified = do_notification(t);
      break;
    case STAT_YELLOW:
      notified = do_notification(t);
      break;
    case STAT_GREEN:
      notified = do_notification(t);
      break;
    }
    break;

  default:
    break;
  }
  return(notified);
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

//*******************************************************************
// Notify user if necessary
//*******************************************************************
static int do_notification(trx *t)
{
  int notified = FALSE;
#if HAVE_LIBESMTP
  smtp_session_t session;
  smtp_message_t message;
  char buf[256];
  FILE *fp;
  char *servername;

  if (t->def->email[0] == 0) {
    return(notified);
  }
  servername = query_server_by_id(t->probe, t->def->server);

  session = smtp_create_session ();
  sprintf(buf, "%s:%lu", OPT_ARG(SMTPSERVER), OPT_VALUE_SMTPSERVERPORT);
  smtp_set_server(session, buf);

  message = smtp_add_message (session);
  smtp_set_reverse_path (message, OPT_ARG(FROM_EMAIL));
  smtp_set_header (message, "Date", &t->res->stattime);
  smtp_add_recipient (message, t->def->email);

  fp = tmpfile();
  if (!fp) {
    LOG(LOG_NOTICE, "tmpfile: %m");
    return(notified);
  }
  fprintf(fp, "From: %s <%s>\n", OPT_ARG(FROM_NAME), OPT_ARG(FROM_EMAIL));
  fprintf(fp, "To: %s\n", t->def->email);
  fprintf(fp, "Subject: %s: %s %s (was %s)\n", servername,
                   t->probe->module_name, color2string(t->res->color),
                   color2string(t->res->prevcolor));
  fprintf(fp, "\n");
  fprintf(fp, "Geachte klant,\n\n"
              "zojuist, om %s"
              "is de status van de probe %s\n"
              "op server %s\n"
              "overgegaan van %s in %s\n", ctime((time_t *)&t->res->stattime), t->probe->module_name,
                   servername, color2string(t->res->prevcolor), color2string(t->res->color));
  if (t->res->message) {
    fprintf(fp, "\nDe volgende melding werd hierbij gegeven:\n%s\n", t->res->message);
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

    strcpy(t->def->notified, "yes");
    if (t->probe->db) {
      MYSQL_RES *result;

      result = my_query(t->probe->db, 0,
                        "update pr_status set notified = 'yes' "
                        "where  probe = '%u' and class = '%u'",
                        t->def->probeid, t->probe->class);

      if (result) mysql_free_result(result);
    }

    // Report on the success or otherwise of the mail transfer.
    status = smtp_message_transfer_status (message);
    LOG(LOG_INFO, "%s: %s %u %s (was %s)", servername, t->res->name, t->res->probeid, 
                   color2string(t->res->color), color2string(t->res->prevcolor));
    LOG(LOG_INFO, "MAILTO: %s %d %s", t->def->email, status->code, status->text ? status->text : "");
    if (debug > 3) fprintf(stderr, "MAILTO: %s %d %s", t->def->email, status->code, status->text ? status->text : "");
    if (status->code == 250) {
      notified = TRUE;
    }
  }
  smtp_destroy_session (session);
  fclose(fp);
#else /* HAVE_LIBESMTP */
  LOG(LOG_NOTICE, "STMP support not compiled in");
#endif /* HAVE_LIBESMTP */
  return(notified);
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


