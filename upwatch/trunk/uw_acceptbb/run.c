#include "config.h"
#define _GNU_SOURCE
#include <time.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>

#include <generic.h>
#define GNET_EXPERIMENTAL
#include <gnet/gnet.h>
#include "cmd_options.h"

static GServer* ob_server = NULL;
static void ob_server_func(GServer* server, GServerStatus status,
                            GConn* conn, gpointer user_data);
static gboolean ob_client_func (GConn* conn, GConnStatus status,
                                gchar* buffer, gint length,
                                gpointer user_data);
static void ob_sig_term (int signum);

GStaticRecMutex mutex_doc = G_STATIC_REC_MUTEX_INIT;
xmlDocPtr doc = NULL;

gboolean writexmlfile(gpointer data)
{
  if (debug > 2) LOG(LOG_DEBUG, "timeout function");
  g_static_rec_mutex_lock(&mutex_doc);
  if (doc) {
    spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(OUTPUT), doc, NULL);
    xmlFreeDoc(doc);
    doc = NULL;
  }
  g_static_rec_mutex_unlock(&mutex_doc);
  return 1;
}

void add_to_xml_document(char *hostname, char *probename, char *colorstr, struct tm *probedate, char *message)
{
  xmlNodePtr subtree, probe, host;
  time_t date = mktime(probedate);
  char buffer[1024];
  int color = STAT_GREEN;

  g_static_rec_mutex_lock(&mutex_doc);
  if (doc == NULL) doc = UpwatchXmlDoc("result");
  if (!strcmp(colorstr, "red")) {
    color = STAT_RED;
  } else if (!strcmp(colorstr, "green")) {
    color = STAT_GREEN;
  } else if (!strcmp(colorstr, "yellow")) {
    color = STAT_YELLOW;
  }
  sprintf(buffer, "bb_%s", probename);
  probe = xmlNewChild(xmlDocGetRootElement(doc), NULL, buffer, NULL);
  sprintf(buffer, "%d", (int) date);          xmlSetProp(probe, "date", buffer);
  sprintf(buffer, "%d", ((int)date)+(2*60));  xmlSetProp(probe, "expires", buffer);
  host = xmlNewChild(probe, NULL, "host", NULL);
  sprintf(buffer, "%s", hostname);     subtree = xmlNewChild(host, NULL, "hostname", buffer);
  //sprintf(buffer, "%s", inet_ntoa(hosts[id]->saddr.sin_addr));
  //  subtree = xmlNewChild(host, NULL, "ipaddress", buffer);
  sprintf(buffer, "%d", color);               subtree = xmlNewChild(probe, NULL, "color", buffer);
  if (message && *message) {
    subtree = xmlNewChild(probe, NULL, "info", message);
  }
  g_static_rec_mutex_unlock(&mutex_doc);
}

int init(void)
{
  daemonize = TRUE;
  every = ONE_SHOT;
  g_thread_init(NULL);
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

int run(void)
{
  GMainLoop* main_loop;
  GInetAddr* addr;
  GServer* server;

  /* Create the main loop */
  main_loop = g_main_loop_new(NULL, FALSE);

  g_timeout_add(60000, writexmlfile, NULL);

  /* Create the interface */
  addr = gnet_inetaddr_new_any ();
  gnet_inetaddr_set_port (addr, OPT_VALUE_LISTEN);

  /* Create the server */
  server = gnet_server_new (addr, TRUE, ob_server_func, "hallo");
  if (!server)
    {
      fprintf (stderr, "Error: Could not start server\n");
      return(0);
    }

  ob_server = server;
  signal (SIGTERM, ob_sig_term);

  /* Start the main loop */
  g_main_run(main_loop);

  return(1);
}

static void ob_sig_term (int signum)
{
  gnet_server_delete (ob_server);
  exit (1);
}

static void ob_server_func (GServer* server, GServerStatus status, struct _GConn* conn, gpointer user_data)
{
  switch (status)
    {
    case GNET_SERVER_STATUS_CONNECT:
      {
        if (debug) {
          LOG(LOG_DEBUG, "New connection from %s", gnet_inetaddr_get_canonical_name(conn->inetaddr));
        }
        conn->func = ob_client_func;
        conn->user_data = g_malloc(16384);
        gnet_conn_readany(conn, conn->user_data, 16384, 30000);
        break;
      }

    case GNET_SERVER_STATUS_ERROR:
      {
        LOG(LOG_DEBUG, "%s error", gnet_inetaddr_get_canonical_name(conn->inetaddr));
        gnet_server_delete (server);
        exit (1);
        break;
      }
    }
}


int ob_client_func (GConn* conn, GConnStatus status,
                gchar* buffer, gint length, gpointer user_data)
{
void runbb(char *req);

  switch (status) {
    case GNET_CONN_STATUS_READ:
      {
        if (buffer) {
          int i = length-1;

          if (debug > 2) LOG(LOG_NOTICE, "GNET_CONN_STATUS_READ: %s", buffer);

          // remove trailing spaces. IS this really a sensible thing to do?
          while (i > 0 && isspace(buffer[i])) {
            buffer[i--] = 0;
          }
          if (i > 1024) {
            strcpy(&buffer[990], "\n\n... DATA TRUNCATED ...\n\n");
          } else {
            buffer[i+1] = 0;
          }
          for(;i>=0; i--) {
            if (buffer[i] & 0x80 || buffer[i] == '\r') {
              buffer[i] = ' ';
            }
          }
          runbb(buffer);
	}
	break;
      }

    case GNET_CONN_STATUS_WRITE:
      {
        if (buffer) {
          LOG(LOG_NOTICE, "GNET_CONN_STATUS_WRITE: %s", buffer);
        }
        if (debug > 1) {
          LOG(LOG_DEBUG, "%s closed connection", gnet_inetaddr_get_canonical_name(conn->inetaddr));
        }
        g_free (user_data);
        gnet_conn_delete (conn, TRUE);
        break;
      }

    case GNET_CONN_STATUS_CLOSE:
      {
        if (buffer) {
          LOG(LOG_NOTICE, "GNET_CONN_STATUS_CLOSE: %s", buffer);
        }
        //LOG(LOG_DEBUG, "%s unexpected close", gnet_inetaddr_get_canonical_name(conn->inetaddr));
        g_free (user_data);
        gnet_conn_delete (conn, TRUE);
        break;
      }
    case GNET_CONN_STATUS_TIMEOUT:
      {
        if (buffer) {
          LOG(LOG_NOTICE, "GNET_CONN_STATUS_TIMEOUT: %s", buffer);
        }
        LOG(LOG_DEBUG, "%s timeout", gnet_inetaddr_get_canonical_name(conn->inetaddr));
        g_free (user_data);
        gnet_conn_delete (conn, TRUE);
        break;
      }
    case GNET_CONN_STATUS_ERROR:
      {
        if (buffer) {
          LOG(LOG_NOTICE, "GNET_CONN_STATUS_ERROR: %s", buffer);
        }
        LOG(LOG_DEBUG, "%s error", gnet_inetaddr_get_canonical_name(conn->inetaddr));
        g_free (user_data);
        gnet_conn_delete (conn, TRUE);
        break;
      }

    default:
      g_assert_not_reached ();
    }

  return TRUE;  /* TRUE means read more if status was read, otherwise
                   its ignored */
}


void runbb(char *cmd)
{
 /*
  * IF THE REQUEST STARTS WITH "page" THEN CALL THE PAGING SCRIPT
  * IF THE REQUEST STARTS WITH "status" THEN LOG THE MESSAGE
  * IF THE REQUEST STARTS WITH "combo" THEN SET A FLAG AND PROCESS MSGS
  * IF THE REQUEST STARTS WITH "summary" THEN LOG THE MESSAGE
  * IF THE REQUEST STARTS WITH "ack" THEN PROCEED WITH AN ACKNOWLEDGEMENT
  * IF THE REQUEST STARTS WITH "enable" THEN ENABLE THE GIVEN HOSTS
  * IF THE REQUEST STARTS WITH "disable" THEN DISABLE THE GIVEN HOSTS
  * IF THE REQUEST STARTS WITH "offline" THEN SET UP THE OFFLINED HOST
  * IF THE REQUEST STARTS WITH "online" THEN SET UP THE ONLINED HOST
  * IF THE REQUEST STARTS WITH "data" THEN APPEND DATA TO THE GIVEN FILE NAME
  * IF THE REQUEST STARTS WITH "notes" THEN REPLACE THE NOTES FILE IN www/notes
  * IF THE REQUEST STARTS WITH "test" THEN DO NOTHING (MAYBE LATER DO SOMETHING)
  * OTHERWISE IGNORE IT (FOR SAFETY SAKE)
  */

  /* If received an agent message, skip that keyword */
  if (strncmp(cmd, "bbagent ", 8) == 0) {
    cmd += 8;
  }

  if (strncmp(cmd, "status", 6) == 0) {
    char *hostname;
    char *probename;
    char *color;
    struct tm probedate;
    char *message;
    char *p;

    memset(&probedate, 0, sizeof(struct tm));
    cmd += 6;
    if (*cmd == '+') {      // status+<delay> format - skip for now
      while (*cmd && *cmd != ' ') cmd++;
    }
    while (*cmd && *cmd == ' ') cmd++;
    if (!*cmd) {
      LOG(LOG_DEBUG, "Illegal status message format, dot not found");
      return;
    }
    //  status ntserver5,domain,nl.disk green Mon Oct 07 14:37:54 RDT 2002 [ntserver5.domain.nl]
    //
    //
    //	      Filesystem   1K-blocks     Used    Avail Capacity  Mounted
    //	      C              4192933  1357729  2835204    32%    /FIXED/C
    // 	      D              8883912  2171416  6712496    24%    /FIXED/D
    //

    // isolate hostname
    p = strchr(cmd, '.');
    if (!p) {
      LOG(LOG_DEBUG, "Illegal status message format, dot not found");
      return;
    }
    *p = 0;
    hostname = cmd;
    cmd = ++p;
    for (p=hostname; *p; p++) if (*p == ',') *p = '.'; // replace comma's with dots

    // isolate probe
    p = strchr(cmd, ' ');
    if (!p) {
      LOG(LOG_DEBUG, "Illegal status message format, probename not found");
      return;
    }
    *p = 0;
    probename = cmd;
    cmd = ++p;

    // isolate color
    p = strchr(cmd, ' ');
    if (!p) {
      LOG(LOG_DEBUG, "Illegal status message format, color not found");
      return;
    }
    *p = 0;
    color = cmd;
    cmd = ++p;
    while (*cmd == ' ') cmd++; // skip leading spaces

    // isolate date/time
    if (debug > 1) LOG(LOG_WARNING, "datestring: %s", cmd); 
    // Mon Oct 07 14:37:54 RDT 2002
    message = strptime(cmd, "%a %b %d %T ", &probedate);
    if (!message) {
      LOG(LOG_DEBUG, "Illegal status message format, illegal time format");
      return;
    }
    message = strptime(message+4, "%Y", &probedate); // lose the timezone info - it's unusable

    //fprintf(stderr, message);
    add_to_xml_document(hostname, probename, color, &probedate, message);
    if (debug > 1) LOG(LOG_WARNING, "host: %s, probe: %s, color: %s, msg: %s, date: %s", 
		    hostname, probename, color, message, asctime(&probedate));
  } else {
    if (debug > 1) LOG(LOG_WARNING, "unknown message: %s", cmd);
  }
}
