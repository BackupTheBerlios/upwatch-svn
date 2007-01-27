#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#include "uw_process_glob.h"

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
  dbi_result result;

  if (debug > 3) fprintf(stderr, "%s %u %s (was %s). %s\n", t->probe->module_name, t->def->probeid, 
                  color2string(t->res->color), color2string(t->res->prevhistcolor), t->res->hostname);

  // First check if this color has stabilized long enough
  if (t->res->stattime - t->res->changed < (t->def->delay * 60)) {
    if (debug > 3) fprintf(stderr, "Not %s long enough\n", color2string(t->res->color));
    return(notified); // NOT YET
  }

  // now go back to the last time the user received a warning, and
  // retrieve the color we had at that time
  result = db_query(t->probe->db, 0,
                    "select   color "
                    "from     pr_hist  "
                    "where    probe = '%u' and class = '%u' and stattime <= '%u' "
                    "         and notified = 'yes' "
                    "order by stattime desc limit 1",
                    t->def->probeid, t->probe->class, t->res->stattime);
  if (!result) return(notified);
  if (dbi_result_next_row(result)) {
    t->res->prevhistcolor = dbi_result_get_uint(result, "color");
  } else {
    t->res->prevhistcolor = 0;
  }
  dbi_result_free(result);

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

//*******************************************************************
// Low level Email functions
//*******************************************************************
#if HAVE_LIBESMTP
#include <libesmtp.h>
#endif

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

int mail(const char *to, const char *subject, const char *body, time_t date)
{
  int notified = FALSE;

#if HAVE_LIBESMTP
  smtp_session_t session;
  smtp_message_t message;
  char buf[128];
  FILE *fp;
  time_t now = time(NULL);
  char rcpt[1024], *s;

  session = smtp_create_session ();
  sprintf(buf, "%s:%lu", OPT_ARG(SMTPSERVER), OPT_VALUE_SMTPSERVERPORT);
  smtp_set_server(session, buf);

  message = smtp_add_message (session);
  smtp_set_reverse_path (message, OPT_ARG(FROM_EMAIL));
  smtp_set_header (message, "Date", date ? &date : &now);
  strncpy(rcpt, to, sizeof(rcpt));
  s = strtok(rcpt, ", ");
  if (s) {
    if (strlen(s) > 5) smtp_add_recipient (message, s);
    if (strlen(s) > 5) smtp_add_recipient (message, s);
    s = strtok(NULL, ", ");
  }

  fp = tmpfile();
  if (!fp) {
    LOG(LOG_ERR, "tmpfile: %m");
    return(notified);
  }
  fprintf(fp, "From: %s <%s>\n", OPT_ARG(FROM_NAME), OPT_ARG(FROM_EMAIL));
  fprintf(fp, "To: %s\n", to);
  fprintf(fp, "Subject: %s\n", subject);
  fprintf(fp, "\n");
  fprintf(fp, "%s", body);
  smtp_set_messagecb(message, libesmtp_messagefp_cb, fp);

  // Initiate a connection to the SMTP server and transfer the message.
  if (!smtp_start_session (session)) {
    char buf[128];

    smtp_strerror (smtp_errno (), buf, sizeof(buf));
    LOG(LOG_WARNING, "%s:%u: %s", OPT_ARG(SMTPSERVER), OPT_VALUE_SMTPSERVERPORT, buf);
  } else {
    const smtp_status_t *status;

    // Report on the success or otherwise of the mail transfer.
    status = smtp_message_transfer_status (message);
    LOG(LOG_NOTICE, "%s", subject);
    LOG(LOG_NOTICE, "MAILTO: %s %d %s", to, status->code, status->text ? status->text : "");
    if (debug > 3) fprintf(stderr, "MAILTO: %s %d %s", to, status->code, status->text ? status->text : "");
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

//*******************************************************************
// Low level SMS functions
//*******************************************************************
#if HAVE_LIBGNOKII
#include <gnokii.h>

static char *lockfile = NULL;
static struct gn_statemachine state;
static gn_data data;

static void  gnokii_error_logger(const char *fmt, va_list ap)
{
  char buf[512];
  vsnprintf(buf, sizeof(buf), fmt, ap);
  LOG(LOG_NOTICE, buf);
}

static void busterminate(void)
{
  gn_sm_functions(GN_OP_Terminate, NULL, &state);
  if (lockfile) gn_device_unlock(lockfile);
}

static int businit(void)
{
  gn_error error;
  char *aux;
  static atexit_registered = 0;

  gn_data_clear(&data);

  aux = gn_cfg_get(gn_cfg_info, "global", "use_locking");
  /* Defaults to 'no' */
  if (aux && !strcmp(aux, "yes")) {
    lockfile = gn_device_lock(state.config.port_device);
    if (lockfile == NULL) {
      LOG(LOG_NOTICE, "Lock file error. Cannot send SMS messages\n");
      return 0;
    }
  }

  /* register cleanup function */
  if (!atexit_registered) {
    atexit_registered = 1;
    atexit(busterminate);
  }
  /* signal(SIGINT, bussignal); */

  /* Initialise the code for the GSM interface. */
  error = gn_gsm_initialise(&state);
  if (error != GN_ERR_NONE) {
    LOG(LOG_NOTICE, "Telephone interface init failed: %s\n", gn_error_print(error));
    return 0;
  }
  return 1;
}
#endif

int sms(char *to, char *msg)
{
static int firsttime = 1;
#if HAVE_LIBGNOKII
  gn_sms sms;
  gn_error error;
  int input_len, i, curpos = 0;

  if (firsttime) {
    gn_elog_handler = gnokii_error_logger;

    /* Read config file */
    if (gn_cfg_read_default() < 0)
      return 0;

    if (!gn_cfg_phone_load("", &state))
      return 0;

    if (businit()) {
      firsttime = 0;
    }
  }
  input_len = GN_SMS_MAX_LENGTH;

  /* The memory is zeroed here */
  gn_sms_default_submit(&sms);

  memset(&sms.remote.number, 0, sizeof(sms.remote.number));
  strncpy(sms.remote.number, to, sizeof(sms.remote.number) - 1);
  if (sms.remote.number[0] == '+') {
    sms.remote.type = GN_GSM_NUMBER_International;
  } else {
    sms.remote.type = GN_GSM_NUMBER_Unknown;
  }

  if (!sms.smsc.number[0]) {
    data.message_center = calloc(1, sizeof(gn_sms_message_center));
    data.message_center->id = 1;
    if (gn_sm_functions(GN_OP_GetSMSCenter, &data, &state) == GN_ERR_NONE) {
      strcpy(sms.smsc.number, data.message_center->smsc.number);
      sms.smsc.type = data.message_center->smsc.type;
    } else {
      LOG(LOG_WARNING, "Cannot read the SMSC number from your phone.");
    }
    free(data.message_center);
  }

  if (!sms.smsc.type) sms.smsc.type = GN_GSM_NUMBER_Unknown;

  if (curpos != -1) {
    strcpy(sms.user_data[curpos].u.text, msg);
    sms.user_data[curpos].type = GN_SMS_DATA_Text;
    if (!gn_char_def_alphabet(sms.user_data[curpos].u.text))
      sms.dcs.u.general.alphabet = GN_SMS_DCS_UCS2;
    sms.user_data[++curpos].type = GN_SMS_DATA_None;
  }

  data.sms = &sms;

  error = gn_sms_send(&data, &state); /* send it */

  if (error == GN_ERR_NONE) {
    LOG(LOG_NOTICE, "SMS: %s", msg);
    return 1;
  }
  LOG(LOG_NOTICE, "SMS to %s FAILED: %s", to, gn_error_print(error));
  if (debug > 3) fprintf(stderr, "SMS to %s FAILED: %s", to, gn_error_print(error));
#endif
  return 0;
}


//*******************************************************************
// Notify user if necessary
//*******************************************************************
static int do_notification(trx *t)
{
  int notified = FALSE;
  char *servername;
  char subject[256];
  char subject_extra[256];
  char body[8192];
  char body_probe_def[8192];
  char msg[8192];

  servername = realm_server_by_id(t->res->realm, t->def->server);

  if (t->probe->notify_mail_subject_extra) {
    t->probe->notify_mail_subject_extra(t, subject_extra, sizeof(subject_extra));
  } else {
    subject_extra[0] = 0;
  }
  snprintf(subject, sizeof(subject), "%s: %s %s (was %s) %s", servername,
                   t->probe->module_name, color2string(t->res->color),
                   color2string(t->res->prevhistcolor), subject_extra);

  if (t->probe->notify_mail_body_probe_def) {
    t->probe->notify_mail_body_probe_def(t, body_probe_def, sizeof(body_probe_def));
  } else {
    body_probe_def[0] = 0;
  }
  if (t->res->message) {
    sprintf(msg, "\nFollowing response has been received:\n%s\n", t->res->message);
  } else {
    msg[0] = 0;
  }
  snprintf(body, sizeof(body), "Dear customer,\n\n"
                "Moments ago, at %s"
                "the status of probe %s\n"
                "at server %s\n"
                "has changed from %s to %s\n"
                "%s:\n%s\n"
                "%s"
                "\n"
                "regards,\n"
                "%s\n", 
                   ctime((time_t *)&t->res->stattime), 
                   t->probe->module_name,
                   servername, 
                   color2string(t->res->prevhistcolor), color2string(t->res->color),
                   body_probe_def[0] ? "\nProbe details" : "", body_probe_def,
                   msg,
                   OPT_ARG(FROM_NAME)
  );
  free(servername);
  if (t->def->email[0]) {
    notified |= mail(t->def->email, subject, body, t->res->stattime);
  }
  if (t->def->sms[0]) {
    char *p;

    strcpy(body, t->def->sms);
    p = strtok(body, " ,/");
    do {
      notified |= sms(p, subject);
    } while ((p = strtok(NULL, " ,/")) != NULL);
  }
  if (notified) {
    strcpy(t->res->notified, "yes");
  }
  return(notified);
}

