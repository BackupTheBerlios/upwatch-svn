#include "config.h"
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

static char *chop(char *s, int i)
{
  i--;
  while (i > 0 && isspace(s[i])) {
    s[i--] = 0;
  }
  return(s);
}

static int uw_password_ok(char *user, char *passwd) 
{
  MYSQL_RES *result;

  if (open_database() == 0) {
    gchar buffer[256];
    MYSQL_ROW row;

    sprintf(buffer, OPT_ARG(AUTHQUERY), user, passwd);
    if (debug > 1) LOG(LOG_DEBUG, buffer);
    if (mysql_query(mysql, buffer)) {
      LOG(LOG_ERR, "buffer: %s", mysql_error(mysql));
      close_database();
      return(FALSE);
    }
    result = mysql_store_result(mysql);
    if (!result || mysql_num_rows(result) < 1) {
      if (debug) LOG(LOG_DEBUG, "user %s, pwd %s not found", user, passwd);
      close_database();
      return(FALSE);
    }
    if ((row = mysql_fetch_row(result))) {
      int id;

      id = atoi(row[0]);
      if (debug) LOG(LOG_DEBUG, "user %s, pwd %s resulted in id %d", user, passwd, id);
    }
    mysql_free_result(result);
    close_database();
  } else {
    close_database();
    return(FALSE); // couldn't open database
  }
  return(TRUE);
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
  main_loop = g_main_new(FALSE);

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
  signal (SIGINT, ob_sig_term);

  /* Start the main loop */
  g_main_run(main_loop);

  return(1);
}

static void ob_sig_term (int signum)
{
  gnet_server_delete (ob_server);
  exit (1);
}

struct conn_stat {
#define STATE_INIT 0
#define STATE_USER 1
#define STATE_PWD  2
#define STATE_CMD  3
#define STATE_DATA 4
#define STATE_EXIT 5
#define STATE_ERROR 6	/* only for errors while spooling!! */
  int state;
  char user[16];
  char pwd[16];
  int length;
  void *sp_info;
  void *buffer;
};

static char *hello = "+OK UpWatch Acceptor v0.1. Please login\n";
static char *errhello = "+ERR Please login first\n";
static char *bye = "+OK Nice talking to you. Bye!\n";
static char *errbye = "+ERR Nice talking to you. Bye!\n";
static char *askpwd = "+OK Please enter password\n";
static char *erraskpwd = "+ERR Please enter password\n";
static char *erraskcmd = "+ERR unknown command\n";
static char *askcmd = "+OK logged in, enter command\n";
static char *askdata = "+OK start sending your file\n";
static char *datain = "+OK Thank you. Enter command\n";
static char *spoolerr = "-ERR Sorry, error spooling file - please try later\n";

static void ob_server_func (GServer* server, GServerStatus status, struct _GConn* conn, gpointer user_data)
{
  struct conn_stat *st;

  switch (status)
    {
    case GNET_SERVER_STATUS_CONNECT:
      {
        if (debug) {
          LOG(LOG_DEBUG, "New connection from %s", gnet_inetaddr_get_canonical_name(conn->inetaddr));
        }
        conn->func = ob_client_func;
        st = calloc(1, sizeof(struct conn_stat));
        st->buffer = malloc(4192);
        conn->user_data = st;
        gnet_conn_write (conn, strdup(hello), strlen(hello), 0);
        gnet_conn_readline (conn, st->buffer, 4192, 30000 /* 30 seconds */);
        break;
      }

    case GNET_SERVER_STATUS_ERROR:
      {
        gnet_server_delete (server);
        exit (1);
        break;
      }
    }
}

int ob_client_func (GConn* conn, GConnStatus status,
                gchar* buffer, gint length, gpointer user_data)
{
  struct conn_stat *cs = (struct conn_stat *)user_data;
  int i;
  char *targ;

  //LOG(LOG_NOTICE, "state=%d, got buffer(%d): %s", cs->state, length, buffer);
  switch (status) {
    case GNET_CONN_STATUS_READ:
      {
        //fprintf(stderr, "user_data: %d %s %s\n", cs->state, cs->user, cs->pwd);
        if (cs->state != STATE_DATA && !strncasecmp(buffer, "QUIT", 4)) {
          gnet_conn_write (conn, strdup(bye), strlen(bye), 0);
          cs->state = STATE_EXIT;
          break;
        }

        switch (cs->state) {
	case STATE_INIT:
          cs->state = STATE_USER;
        case STATE_USER:
          chop(buffer, length);
          if (strncasecmp(buffer, "USER ", 5)) {
            if (debug > 1) {
              LOG(LOG_DEBUG, buffer);
              LOG(LOG_DEBUG, errhello);
            }
            gnet_conn_write (conn, strdup(errhello), strlen(errhello), 0);
          } else {
            strncpy(cs->user, buffer+5, sizeof(cs->user));
            cs->state = STATE_PWD;
            gnet_conn_write (conn, strdup(askpwd), strlen(askpwd), 0);
          }
          break;
        case STATE_PWD: 
          chop(buffer, length);
          if (strncasecmp(buffer, "PASS ", 5)) {
            if (debug > 1) {
              LOG(LOG_DEBUG, buffer);
              LOG(LOG_DEBUG, erraskpwd);
            }
            gnet_conn_write (conn, strdup(erraskpwd), strlen(erraskpwd), 0);
            break;
          }
          strncpy(cs->pwd, buffer+5, sizeof(cs->pwd));
          /* check the password first */
          if (!uw_password_ok(cs->user, cs->pwd)) {
            LOG(LOG_NOTICE, "%s: password failure (%s/%s)", gnet_inetaddr_get_canonical_name(conn->inetaddr), cs->user, cs->pwd);
            gnet_conn_write (conn, strdup(errbye), strlen(errbye), 0);
            cs->state = STATE_EXIT;
            break;
          }
          gnet_conn_write (conn, strdup(askcmd), strlen(askcmd), 0);
          cs->state = STATE_CMD;
          break;
        case STATE_CMD:
          if (strncasecmp(buffer, "DATA ", 5)) {
            if (debug > 1) {
              LOG(LOG_DEBUG, buffer);
              LOG(LOG_DEBUG, erraskcmd);
            }
            gnet_conn_write (conn, strdup(erraskcmd), strlen(erraskcmd), 0);
            break;
          }
          cs->length = atoi(buffer + 5);
          if (cs->length < 1) {
            if (debug > 1) {
              LOG(LOG_DEBUG, buffer);
              LOG(LOG_DEBUG, erraskcmd);
            }
            gnet_conn_write (conn, strdup(erraskcmd), strlen(erraskcmd), 0);
            cs->length = 0;
            break;
          }
          cs->sp_info = spool_open(OPT_ARG(SPOOLDIR), OPT_ARG(OUTPUT));
          if (cs->sp_info == NULL) {
            if (debug > 1) {
              LOG(LOG_DEBUG, buffer);
              LOG(LOG_DEBUG, spoolerr);
            }
            gnet_conn_write (conn, strdup(spoolerr), strlen(spoolerr), 0);
            cs->state = STATE_EXIT;
            break;
          }
          gnet_conn_write (conn, strdup(askdata), strlen(askdata), 0);
          memset(cs->buffer, 0, 4192);
          gnet_conn_readany (conn, cs->buffer, cs->length > 4192 ? 4192 : cs->length, 10000 /* 10 seconds */);
          cs->state = STATE_DATA;
          return FALSE; // stop reading
          break;
        case STATE_DATA:
          if (spool_write(cs->sp_info, buffer, length) == -1) {
            if (debug > 1) {
              LOG(LOG_DEBUG, spoolerr);
            }
            gnet_conn_write (conn, strdup(spoolerr), strlen(spoolerr), 0);
            spool_close(cs->sp_info, FALSE);
            gnet_conn_write (conn, strdup(bye), strlen(bye), 0);
            cs->state = STATE_EXIT;
            break;
          }
          memset(cs->buffer, 0, length);
          cs->length -= length;
          if (cs->length > 0) break; // not totally ready yet?
          targ = strdup(spool_targfilename(cs->sp_info));
          if (!spool_close(cs->sp_info, TRUE)) {
            if (debug > 1) {
              LOG(LOG_DEBUG, spoolerr);
            }
            gnet_conn_write (conn, strdup(spoolerr), strlen(spoolerr), 0);
          } else {
            if (debug) LOG(LOG_DEBUG, "spooled to %s", targ);
          }
          free(targ);
          gnet_conn_write (conn, strdup(datain), strlen(datain), 0);
          cs->state = STATE_CMD;
          break;
        case STATE_ERROR:
          if (!strncasecmp(buffer, ".", 1)) {
            gnet_conn_write (conn, strdup(errbye), strlen(errbye), 0);
            cs->state = STATE_EXIT;
          }
        }
        break;
      }

    case GNET_CONN_STATUS_WRITE:
      {
        if (cs->state == STATE_EXIT) {
          if (debug) {
            LOG(LOG_DEBUG, "%s closed connection", gnet_inetaddr_get_canonical_name(conn->inetaddr));
          }
          g_free (buffer);
          free(conn->user_data);
          gnet_conn_delete (conn, TRUE);
        }
        break;
      }

    case GNET_CONN_STATUS_CLOSE:
    case GNET_CONN_STATUS_TIMEOUT:
    case GNET_CONN_STATUS_ERROR:
      {
        if (cs->state == STATE_DATA || cs->state == STATE_ERROR) {
          spool_close(cs->sp_info, FALSE);
          if (debug) {
            LOG(LOG_DEBUG, "%s unexpected end of input", gnet_inetaddr_get_canonical_name(conn->inetaddr));
          }
        } else {
          if (debug && cs->state != STATE_DATA) {
            LOG(LOG_DEBUG, "%s unexpected close", gnet_inetaddr_get_canonical_name(conn->inetaddr));
          }
        }
        if (buffer) g_free (buffer);
        free(conn->user_data);
        gnet_conn_delete (conn, TRUE);
        break;
      }

    default:
      g_assert_not_reached ();
    }

  return TRUE;  /* TRUE means read more if status was read, otherwise
                   its ignored */
}



