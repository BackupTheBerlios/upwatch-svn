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
  s[i--] = 0;
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
  }
  return NULL;
}

#define TIMEOUT 10000000L

void handle_session(st_netfd_t rmt_nfd, char *remotehost)
{
  char user[64], passwd[18];
  char buffer[BUFSIZ];
  int filesize;
  char *filename;
  void *sp_info;
  char *targ;
  int length, len;

  sprintf(buffer, "+OK UpWatch Acceptor v" UW_ACCEPT_VERSION ". Please login\n");
login:
  uw_setproctitle("%s: %s", remotehost, buffer);
  if (debug > 3) fprintf(stderr, "%s[%u] > %s", remotehost, st_netfd_fileno(rmt_nfd), buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) LOG(LOG_WARNING, "timeout on greeting string");
    else LOG(LOG_WARNING, "%m");
    return;
  }

  // expect here: USER xxxxxxx
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) LOG(LOG_WARNING, "timeout reading USER string");
    else LOG(LOG_WARNING, "%m");
    return;
  }
  if (debug > 3) fprintf(stderr, "%s[%u] < %s", remotehost, st_netfd_fileno(rmt_nfd), buffer);
  chop(buffer, len);
  if (strncasecmp(buffer, "QUIT", 4) == 0) goto end;
  if (strncasecmp(buffer, "USER ", 5)) goto syntax; 
  chop(buffer, len);
  strncpy(user, buffer+5, sizeof(user)); user[sizeof(user)-1] = 0;

  sprintf(buffer, "+OK Please enter password\n");
  uw_setproctitle("%s: %s", remotehost, buffer);
  if (debug > 3) fprintf(stderr, "%s[%u] > %s", remotehost, st_netfd_fileno(rmt_nfd), buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) LOG(LOG_WARNING, "timeout writing password string");
    else LOG(LOG_WARNING, "%m");
    return;
  }

  // expect here: PASS xxxxxxx
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) LOG(LOG_WARNING, "timeout reading PASS response");
    else LOG(LOG_WARNING, "%m");
    return;
  }
  if (debug > 3) fprintf(stderr, "%s[%u] < %s", remotehost, st_netfd_fileno(rmt_nfd), buffer);
  chop(buffer, len);
  if (strncasecmp(buffer, "QUIT", 4) == 0) goto end;
  if (strncasecmp(buffer, "PASS ", 5)) goto syntax; 

  chop(buffer, len);
  strncpy(passwd, buffer+5, sizeof(passwd)); passwd[sizeof(passwd)-1] = 0;
  if (!uw_password_ok(user, passwd)) {
    LOG(LOG_WARNING, "Login error: %s/%s", user, passwd);
    st_sleep(2);
    sprintf(buffer, "-ERR Please login\n");
    goto login;
  }

  sprintf(buffer, "+OK logged in, enter command\n");
logged_in:
  uw_setproctitle("%s: %s", remotehost, buffer);
  if (debug > 3) fprintf(stderr, "%s[%u] > %s", remotehost, st_netfd_fileno(rmt_nfd), buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) LOG(LOG_WARNING, "timeout writing ENTER COMMAND");
    else LOG(LOG_WARNING, "%m");
    return;
  }

  // expect here: DATA size filename
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) LOG(LOG_WARNING, "timeout reading DATA statement");
    else LOG(LOG_WARNING, "%m");
    return;
  }
  if (debug > 3) fprintf(stderr, "%s[%u] < %s", remotehost, st_netfd_fileno(rmt_nfd), buffer);
  chop(buffer, len);
  if (strncasecmp(buffer, "QUIT", 4) == 0) goto end;
  if (strncasecmp(buffer, "DATA ", 5)) goto syntax; 
  filesize = atoi(buffer+5);
  for (filename = buffer+5; *filename && isdigit(*filename); filename++)
    ;
  for (; *filename && isspace(*filename); filename++)
    ;
  if (filename && strlen(filename) < 12) {
    filename = NULL;
  }

  sp_info = spool_open(OPT_ARG(SPOOLDIR), OPT_ARG(OUTPUT), filename);
  if (sp_info == NULL) {
    sprintf(buffer, "-ERR Sorry, error spooling file - enter command\n");
    goto logged_in;
  }

  sprintf(buffer, "+OK start sending your file\n");
  uw_setproctitle("%s: %s", remotehost, buffer);
  if (debug > 3) fprintf(stderr, "%s[%u] > %s", remotehost, st_netfd_fileno(rmt_nfd), buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) LOG(LOG_WARNING, "timeout writing %s", buffer);
    else LOG(LOG_WARNING, "%m");
    return;
  }

  while (filesize > 0) {
    memset(buffer, 0, sizeof(buffer));
    length = filesize > sizeof(buffer) ? sizeof(buffer) : filesize;
    //fprintf(stderr, "reading %u bytes", length);
    len = st_read(rmt_nfd, buffer, length, TIMEOUT);
    if (len == -1) {
      if (errno == ETIME) LOG(LOG_WARNING, "timeout reading fileblock");
      else LOG(LOG_WARNING, "%m");
      spool_close(sp_info, FALSE);
      goto end;
    }
    if (spool_write(sp_info, buffer, len) == -1) {
      LOG(LOG_NOTICE, "spoolfile write error");
      spool_close(sp_info, FALSE);
      goto end;
    }
    filesize -= len;
  }

  targ = strdup(spool_targfilename(sp_info));
  if (!spool_close(sp_info, TRUE)) {
    LOG(LOG_NOTICE, "spoolfile close error");
    goto end;
  }
  if (debug) LOG(LOG_DEBUG, "spooled to %s", targ);
  free(targ);

  sprintf(buffer, "+OK Thank you. Enter command\n");
  goto logged_in;

syntax:
  LOG(LOG_WARNING, buffer);
  sprintf(buffer, "-ERR unknown command\n");
  goto login;

end:
  sprintf(buffer, "+OK Arrivederci\n");
  uw_setproctitle("%s: %s", remotehost, buffer);
  if (debug > 3) fprintf(stderr, "%s[%u] > %s", remotehost, st_netfd_fileno(rmt_nfd), buffer);
  st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  uw_setproctitle("accepting connections");
}
