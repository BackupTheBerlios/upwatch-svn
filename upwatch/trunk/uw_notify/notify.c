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
// if we enter this function, we know
// the user has not been warned yet.
// 
//*******************************************************************
int notify(trx *t)
{
  int notified = FALSE;
  MYSQL_RES *result;
  MYSQL_ROW row;

  if (debug > 3) fprintf(stderr, "%s %u %s (was %s). %s\n", t->probe->module_name, t->def->probeid, 
                  color2string(t->res->color), color2string(t->res->prevhistcolor), t->res->hostname);

  // First check if this color has stabilized long enough
  if (t->res->stattime - t->res->changed < (t->def->delay * 60)) {
    if (debug > 3) fprintf(stderr, "Not %s long enough\n", color2string(t->res->color));
    return(notified); // NOT YET
  }

  // now go back to the last time the user received a warning, and
  // retrieve the color we had at that time
  result = my_query(t->probe->db, 0,
                    "select   color "
                    "from     pr_hist  "
                    "where    probe = '%u' and class = '%u' and stattime <= '%u' "
                    "         and notified = 'yes' "
                    "order by stattime desc limit 1",
                    t->def->probeid, t->probe->class, t->res->stattime);
  if (!result) return(notified);
  row = mysql_fetch_row(result);
  if (row) {
    t->res->prevhistcolor = atoi(row[0]);
  } else {
    t->res->prevhistcolor = 0;
  }
  mysql_free_result(result);

  // if it's the same, we don't really need to warn right?
  if (t->res->prevhistcolor == t->res->color) {
    return(notified);
  }
  // and if it's a fuse, we only warn if the fuse blows
  if (t->probe->fuse && (t->res->color < STAT_PURPLE)) {
    return(notified);
  }
  notified = do_notification(t);
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
    LOG(LOG_ERR, "tmpfile: %m");
    return(notified);
  }
  fprintf(fp, "From: %s <%s>\n", OPT_ARG(FROM_NAME), OPT_ARG(FROM_EMAIL));
  fprintf(fp, "To: %s\n", t->def->email);
  fprintf(fp, "Subject: %s: %s %s (was %s)\n", servername,
                   t->probe->module_name, color2string(t->res->color),
                   color2string(t->res->prevhistcolor));
  fprintf(fp, "\n");
  fprintf(fp, "Geachte klant,\n\n"
              "zojuist, om %s"
              "is de status van de probe %s\n"
              "op server %s\n"
              "overgegaan van %s in %s\n", ctime((time_t *)&t->res->stattime), t->probe->module_name,
                   servername, color2string(t->res->prevhistcolor), color2string(t->res->color));
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
    LOG(LOG_WARNING, "%s:%u: %s", OPT_ARG(SMTPSERVER), OPT_VALUE_SMTPSERVERPORT, buf);
  } else {
    const smtp_status_t *status;

    strcpy(t->res->notified, "yes");

    // Report on the success or otherwise of the mail transfer.
    status = smtp_message_transfer_status (message);
    LOG(LOG_INFO, "%s: %s %u %s (was %s)", servername, t->res->name, t->res->probeid, 
                   color2string(t->res->color), color2string(t->res->prevhistcolor));
    LOG(LOG_INFO, "MAILTO: %s %d %s", t->def->email, status->code, status->text ? status->text : "");
    if (debug > 3) fprintf(stderr, "MAILTO: %s %d %s", t->def->email, status->code, status->text ? status->text : "");
    if (status->code == 250) {
      notified = TRUE;
    }
  }
  smtp_destroy_session (session);
  fclose(fp);
#else /* HAVE_LIBESMTP */
  LOG(LOG_WARNING, "STMP support not compiled in");
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


