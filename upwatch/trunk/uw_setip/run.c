#include "config.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <signal.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>

#include <generic.h>
#include <st.h>
#include "cmd_options.h"

int thread_count;

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
  MYSQL *mysql;
  MYSQL_RES *result;

  mysql = open_database(OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME), 
			OPT_ARG(DBUSER), OPT_ARG(DBPASSWD), OPT_VALUE_DBCOMPRESS);
  if (mysql) {
    gchar buffer[256];
    MYSQL_ROW row;

    sprintf(buffer, OPT_ARG(AUTHQUERY), user, passwd);
    if (debug > 1) LOG(LOG_DEBUG, buffer);
    if (mysql_query(mysql, buffer)) {
      LOG(LOG_ERR, "buffer: %s", mysql_error(mysql));
      close_database(mysql);
      return(FALSE);
    }
    result = mysql_store_result(mysql);
    if (!result || mysql_num_rows(result) < 1) {
      if (debug) LOG(LOG_DEBUG, "user %s, pwd %s not found", user, passwd);
      close_database(mysql);
      return(FALSE);
    }
    if ((row = mysql_fetch_row(result))) {
      int id;

      id = atoi(row[0]);
      if (debug>1) LOG(LOG_DEBUG, "user %s, pwd %s resulted in id %d", user, passwd, id);
    }
    mysql_free_result(result);
    close_database(mysql);
  } else {
    close_database(mysql);
    return(FALSE); // couldn't open database
  }
  return(TRUE);
}

static int uw_set_ip(char *user, char *ip) 
{
  MYSQL *mysql;

  mysql = open_database(OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME), 
			OPT_ARG(DBUSER), OPT_ARG(DBPASSWD), OPT_VALUE_DBCOMPRESS);
  if (mysql) {
    gchar buffer[256];

    sprintf(buffer, OPT_ARG(SETIPQUERY), ip, user);
    if (debug > 1) LOG(LOG_DEBUG, buffer);
    if (mysql_query(mysql, buffer)) {
      LOG(LOG_ERR, "buffer: %s", mysql_error(mysql));
      close_database(mysql);
      return(FALSE);
    }
    close_database(mysql);
  } else {
    close_database(mysql);
    return(FALSE); // couldn't open database
  }
  return(TRUE);
}

int init(void)
{
  daemonize = TRUE;
  every = ONE_SHOT;
  st_init();
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

void *handle_connections(void *arg);

int run(void)
{
  int n, sock;
  struct sockaddr_in serv_addr;
  st_netfd_t nfd;

  /* Start the main loop */
  uw_setproctitle("accepting connections");
  
  /* Create server socket */
  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    LOG(LOG_NOTICE, "socket: %m");
    return 0;
  }
  n = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&n, sizeof(n)) < 0) {
    LOG(LOG_NOTICE, "setsockopt(REUSEADDR): %m");
    close(sock);
    return 0;
  }
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(OPT_VALUE_LISTEN);
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
    struct hostent *hp;
    /* not dotted-decimal */
    if ((hp = gethostbyname("0.0.0.0")) == NULL) {
      LOG(LOG_NOTICE, "0.0.0.0: %m");
      close(sock);
      return 0;
    }
    memcpy(&serv_addr.sin_addr, hp->h_addr, hp->h_length);
  }

  /* Do bind and listen */
  if (bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    LOG(LOG_NOTICE, "bind: %m");
    close(sock);
    return 0;
  }
  if (listen(sock, 10) < 0) {
    LOG(LOG_NOTICE, "listen: %m");
    close(sock);
    return 0;
  }

  /* Create file descriptor object from OS socket */
  if ((nfd = st_netfd_open_socket(sock)) == NULL) {
    LOG(LOG_NOTICE, "st_netfd_open_socket: %m");
    close(sock);
    return 0;
  }

  for (n = 0; n < 10; n++) {
    if (st_thread_create(handle_connections, (void *)&nfd, 0, 0) != NULL) {
      thread_count++;
    } else {
      LOG(LOG_NOTICE, "st_thread_create: %m");
    }
  }

  while (thread_count) {
    st_usleep(10000);
  }
  return(1);
}

void handle_session(st_netfd_t cli_nfd, char *remotehost);

void *handle_connections(void *arg)
{
extern int forever;
  st_netfd_t srv_nfd, cli_nfd;
  struct sockaddr_in from;
  int fromlen = sizeof(from);

  srv_nfd = *(st_netfd_t *) arg;

  while (forever) {
    char *remote;
    cli_nfd = st_accept(srv_nfd, (struct sockaddr *)&from, &fromlen, -1);
    if (cli_nfd == NULL) {
      LOG(LOG_NOTICE, "st_accept: %m");
      continue;
    }
    remote = strdup(inet_ntoa(from.sin_addr));
    LOG(LOG_NOTICE, "New connection from: %s", remote);
    handle_session(cli_nfd, remote);
    free(remote);
    st_netfd_close(cli_nfd);
    uw_setproctitle("accepting connections");
  }
  return NULL;
}

#define TIMEOUT 10000000L

void handle_session(st_netfd_t rmt_nfd, char *remotehost)
{
  char user[64], passwd[18], ip[40];
  char buffer[BUFSIZ];
  int len;

  sprintf(buffer, "+OK UpWatch set IP v" UW_SETIP_VERSION ".\n");
  uw_setproctitle("%s: %s", remotehost, buffer);
  if (debug > 3) fprintf(stderr, "> %s", buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == ETIME) {
    LOG(LOG_WARNING, "timeout on greeting string");
    return;
  }
  if (len == -1) {
    LOG(LOG_WARNING, "%m");
    return;
  }

  // expect here: USER PASSWORD NEW-IP
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == ETIME) {
    LOG(LOG_WARNING, "timeout on response");
    return;
  }
  if (len == -1) {
    LOG(LOG_WARNING, "%m");
    return;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  chop(buffer, len);
  sscanf(buffer, "%s %s %s", user, passwd, ip);
  if (!uw_password_ok(user, passwd)) {
    LOG(LOG_WARNING, "Login error: %s/%s", user, passwd);
    return;
  }
  uw_set_ip(user, ip);

  sprintf(buffer, "+OK Arrivederci\n");
  uw_setproctitle("%s: %s", remotehost, buffer);
  if (debug > 3) fprintf(stderr, "> %s", buffer);
  st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
}
