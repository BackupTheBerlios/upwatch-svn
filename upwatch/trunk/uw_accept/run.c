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

static int uw_password_ok(char *user, char *passwd) 
{
  MYSQL_RES *result;

  if (open_database() == 0) {
    gchar buffer[256];
    MYSQL_ROW row;

    sprintf(buffer, OPT_ARG(AUTHQUERY), user, passwd);
    if (debug) LOG(LOG_DEBUG, buffer);
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
      printf("\n");
    }
    mysql_free_result(result);
    close_database();
  } else {
    return(FALSE); // couldn't open database
  }
  return(TRUE);
}

int init(void)
{
  daemonize = TRUE;
  every = ONE_SHOT;
  g_thread_init(NULL);
  return(1);
}

int run(void)
{
  int ret = 0;
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
#define STATE_DATA 3
#define STATE_EXIT 4
#define STATE_ERROR 5	/* only for errors while spooling!! */
  int state;
  char user[16];
  char pwd[16];
  void *sp_info;
};

static char *hello = "+OK UpWatch Acceptor v0.1. Please login\n";
static char *errhello = "+ERR Please login first\n";
static char *bye = "+OK Nice talking to you. Bye!\n";
static char *errbye = "+ERR Nice talking to you. Bye!\n";
static char *askpwd = "+OK Please enter password\n";
static char *erraskpwd = "+ERR Please enter password\n";
static char *askdata= "+OK start sending DATA, end with a single dot\n";
static char *spoolerr = "-ERR Sorry, error spooling file - please try later\n";

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
        conn->user_data = calloc(1, sizeof(struct conn_stat));
        gnet_conn_write (conn, strdup(hello), strlen(hello), 0);
        gnet_conn_readline (conn, NULL, 1024, 30000);
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
  int i = length-1;

  // remove trailing spaces. IS this really a sensible thing to do?
  while (i > 0 && isspace(buffer[i])) {
    buffer[i--] = 0;
  }

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
          if (strncasecmp(buffer, "USER ", 5)) {
            gnet_conn_write (conn, strdup(errhello), strlen(errhello), 0);
          } else {
            strncpy(cs->user, buffer+5, sizeof(cs->user));
            cs->state = STATE_PWD;
            gnet_conn_write (conn, strdup(askpwd), strlen(askpwd), 0);
          }
          break;
        case STATE_PWD: 
          if (strncasecmp(buffer, "PASS ", 5)) {
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
          cs->state = STATE_DATA;
          cs->sp_info = spool_open(OPT_ARG(SPOOLDIR), OPT_ARG(OUTPUT));
          if (cs->sp_info == NULL) {
            gnet_conn_write (conn, strdup(spoolerr), strlen(spoolerr), 0);
            cs->state = STATE_EXIT;
            break;
          }
          gnet_conn_write (conn, strdup(askdata), strlen(askdata), 0);
          break;
        case STATE_DATA:
          if (!strncasecmp(buffer, ".", 1)) {
            char *targfilename = strdup(spool_targfilename(cs->sp_info));
            if (!spool_close(cs->sp_info, TRUE)) {
              gnet_conn_write (conn, strdup(spoolerr), strlen(spoolerr), 0);
            } else {
              if (debug) LOG(LOG_DEBUG, "spooled to %s", targfilename);
            }
            free(targfilename);
            gnet_conn_write (conn, strdup(bye), strlen(bye), 0);
            cs->state = STATE_EXIT;
          } else {
            if (!spool_printf(cs->sp_info, "%s\n", buffer)) {
              spool_close(cs->sp_info, FALSE);
              cs->state = STATE_ERROR;
            }
          }
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
        g_free (buffer);
        if (cs->state == STATE_EXIT) {
          if (debug) {
            LOG(LOG_DEBUG, "%s closed connection", gnet_inetaddr_get_canonical_name(conn->inetaddr));
          }
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



