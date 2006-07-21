#include "config.h"
#include "uw_accept.h"
#include "db.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <generic.h>
#include <st.h>

struct dbspec {
  char realm[25];
  char host[65];
  int port;
  char db[64];
  char user[25];
  char password[25];
  dbi_conn conn;
} *dblist;
int dblist_cnt;

int thread_count;
int spooldir_strlen; // strlen of spooldir

static char *chop(char *s, int i)
{
  s[i--] = 0;
  while (i > 0 && isspace(s[(char)i])) {
    s[i--] = 0;
  }
  return(s);
}

void init_dblist(void)
{
  dbi_conn conn;

  conn = open_database(OPT_ARG(DBTYPE), OPT_ARG(DBHOST), OPT_ARG(DBPORT), OPT_ARG(DBNAME),
                     OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
  if (conn) {
    dbi_result result;

    if (dblist) free(dblist);
    dblist = calloc(100, sizeof(struct dbspec));

    result = db_query(conn, 0, "select pr_realm.name, pr_realm.host, "
                             "       pr_realm.port, pr_realm.db, pr_realm.user, "
                             "       pr_realm.password "
                             "from   pr_realm "
                             "where  pr_realm.id > 1");
    if (result) {
      dblist_cnt = 0;
      while (dbi_result_next_row(result)) {
        strcpy(dblist[dblist_cnt].realm, dbi_result_get_string(result, "name"));
        strcpy(dblist[dblist_cnt].host, dbi_result_get_string(result, "host"));
        dblist[dblist_cnt].port = dbi_result_get_uint(result, "port");
        strcpy(dblist[dblist_cnt].db, dbi_result_get_string(result, "db"));
        strcpy(dblist[dblist_cnt].user, dbi_result_get_string(result, "user"));
        strcpy(dblist[dblist_cnt].password, dbi_result_get_string(result, "password"));
        dblist_cnt++;
      }
      dbi_result_free(result);
    }
    close_database(conn);
    LOG(LOG_INFO, "read %u realms", dblist_cnt);
  } else {
    LOG(LOG_NOTICE, "could not open %s database %s@%s as user %s", OPT_ARG(DBTYPE), OPT_ARG(DBNAME), OPT_ARG(DBHOST), OPT_ARG(DBUSER));
  } 
}

dbi_conn open_realm(char *realm)
{
  int i;
  dbi_conn conn;
static int call_cnt = 0;

  if (!dblist || ++call_cnt == 100) {
    call_cnt = 0;
    init_dblist();
    if (!dblist) {
      LOG(LOG_ERR, "open_realm but no dblist found");
      return NULL;
    }
  }
  if (realm == NULL || realm[0] == 0) {
    char buf[10];

    sprintf(buf, "%d", dblist[0].port);
    conn = open_database(OPT_ARG(DBTYPE), dblist[0].host, buf,
            dblist[0].db, dblist[0].user, dblist[0].password);
    return(conn);
  }

  for (i=0; i < dblist_cnt; i++) {
    if (strcmp(dblist[i].realm, realm) == 0) {
      char buf[10];

      sprintf(buf, "%d", dblist[0].port);
      conn = open_database(OPT_ARG(DBTYPE), dblist[i].host, buf,
              dblist[i].db, dblist[i].user, dblist[i].password);
      return(conn);
    }
  }
  return(NULL);
}

static int uw_password_ok(char *user, char *passwd) 
{
  dbi_conn conn;
  dbi_result result;
  char user_realm[256];
  char *realm;

  strncpy(user_realm, user, sizeof(user_realm));
  realm = strrchr(user, '@');
  if (realm) { 
    *realm++ = 0; 
  }
  conn = open_realm(realm);
  if (conn) {
    gchar buffer[256];

    sprintf(buffer, OPT_ARG(AUTHQUERY), user, passwd);
    LOG(LOG_DEBUG, buffer);
    result = dbi_conn_query(conn, buffer);
    if (!result) {
      close_database(conn);
      return(FALSE);
    }
    if (dbi_result_next_row(result)) {
      int id = dbi_result_get_uint_idx(result, 0);
      LOG(LOG_DEBUG, "user %s, pwd %s resulted in id %d", user_realm, passwd, id);
    }
    dbi_result_free(result);
    close_database(conn);
  } else {
    close_database(conn);
    return(FALSE); // couldn't open database
  }
  return(TRUE);
}


int init(void)
{
  if (!HAVE_OPT(OUTPUT)) {
    LOG(LOG_ERR, "missing output option");
    return 0;
  }

  spooldir_strlen = strlen(OPT_ARG(SPOOLDIR))+1;
  daemonize = TRUE;
  every = ONE_SHOT;
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
    LOG(LOG_ERR, "socket: %m");
    return 0;
  }
  n = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&n, sizeof(n)) < 0) {
    LOG(LOG_ERR, "setsockopt(REUSEADDR): %m");
    close(sock);
    return 0;
  }
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(OPT_VALUE_LISTEN);
  if (HAVE_OPT(BIND)) {
    if (strcmp(OPT_ARG(BIND), "*") == 0) {
      serv_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
      inet_aton(OPT_ARG(BIND), &serv_addr.sin_addr);
    }
  } else {
    serv_addr.sin_addr.s_addr = INADDR_ANY;
  }
  if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
    struct hostent *hp;
    /* not dotted-decimal */
    if ((hp = gethostbyname("0.0.0.0")) == NULL) {
      LOG(LOG_ERR, "0.0.0.0: %m");
      close(sock);
      return 0;
    }
    memcpy(&serv_addr.sin_addr, hp->h_addr, hp->h_length);
  }

  /* Do bind and listen */
  if (bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    LOG(LOG_ERR, "bind: %m");
    close(sock);
    return 0;
  }
  if (listen(sock, 10) < 0) {
    LOG(LOG_ERR, "listen: %m");
    close(sock);
    return 0;
  }

  /* Create file descriptor object from OS socket */
  if ((nfd = st_netfd_open_socket(sock)) == NULL) {
    LOG(LOG_ERR, "st_netfd_open_socket: %m");
    close(sock);
    return 0;
  }

  for (n = 0; n < 10; n++) {
    if (st_thread_create(handle_connections, (void *)&nfd, 0, 0) != NULL) {
      thread_count++;
    } else {
      LOG(LOG_WARNING, "st_thread_create: %m");
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
  struct hostent *host;
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
    host = gethostbyaddr((char*)&from.sin_addr.s_addr, sizeof(from.sin_addr.s_addr), AF_INET);
    LOG(LOG_INFO, "%s: new connection from %s", remote, host? host->h_name: remote);
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
  int filesize, givensize;
  char *filename;
  void *sp_info;
  char *targ;
  int len;
  int errors = 0;

  sprintf(buffer, "+OK UpWatch Acceptor v" UW_ACCEPT_VERSION ". Please login\n");
login:
  uw_setproctitle("%s: %s", remotehost, buffer);
  if (debug > 3) fprintf(stderr, "%s[%u] > %s", remotehost, st_netfd_fileno(rmt_nfd), buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) { 
      LOG(LOG_WARNING, "%s: timeout sending greeting string", remotehost);
    } else {
      LOG(LOG_WARNING, "%s: %m", remotehost);
    }
    return;
  }

  // expect here: USER xxxxxxx or USER xxxxx@realm
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) {
      LOG(LOG_WARNING, "%s: timeout reading USER string", remotehost);
    } else {
      LOG(LOG_WARNING, "%s: %m", remotehost);
    }
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
    if (errno == ETIME) {
      LOG(LOG_WARNING, "%s: timeout writing password string", remotehost);
    } else {
      LOG(LOG_WARNING, "%s: %m", remotehost);
    }
    return;
  }

  // expect here: PASS xxxxxxx
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) {
      LOG(LOG_WARNING, "%s: timeout reading PASS response", remotehost);
    } else { 
      LOG(LOG_WARNING, "%s: %m", remotehost);
    }
    return;
  }
  if (debug > 3) fprintf(stderr, "%s[%u] < %s", remotehost, st_netfd_fileno(rmt_nfd), buffer);
  chop(buffer, len);
  if (strncasecmp(buffer, "QUIT", 4) == 0) goto end;
  if (strncasecmp(buffer, "PASS ", 5)) goto syntax; 

  chop(buffer, len);
  strncpy(passwd, buffer+5, sizeof(passwd)); passwd[sizeof(passwd)-1] = 0;
  if (!uw_password_ok(user, passwd)) {
    LOG(LOG_WARNING, "%s: Login error: -%s/%s-", remotehost, user, passwd);
    st_sleep(1);
    sprintf(buffer, "-ERR Please login\n");
    if (++errors < 4) goto login;
    goto end;
  }

  sprintf(buffer, "+OK logged in, enter command\n");
logged_in:
  uw_setproctitle("%s: %s", remotehost, buffer);
  if (debug > 3) fprintf(stderr, "%s[%u] > %s", remotehost, st_netfd_fileno(rmt_nfd), buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) { 
      LOG(LOG_WARNING, "%s: timeout writing ENTER COMMAND", remotehost);
    } else { 
      LOG(LOG_WARNING, "%s: %m", remotehost);
    }
    return;
  }

  // expect here: DATA size filename
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) {
      LOG(LOG_WARNING, "%s: timeout reading DATA statement", remotehost);
    } else { 
      LOG(LOG_WARNING, "%s: %m", remotehost);
    }
    return;
  }
  if (debug > 3) fprintf(stderr, "%s[%u] < %s", remotehost, st_netfd_fileno(rmt_nfd), buffer);
  chop(buffer, len);
  if (strncasecmp(buffer, "QUIT", 4) == 0) goto end;
  if (strncasecmp(buffer, "DATA ", 5)) goto syntax; 
  filesize = atoi(buffer+5);
  if (filesize < 174 /* empty result file */ || filesize > 100000) {
    LOG(LOG_WARNING, "%u: Illegal filesize", filesize);
    goto syntax;
  }
  givensize = filesize; // remember for logging
  for (filename = buffer+5; *filename && isdigit(*filename); filename++)
    ;
  for (; *filename && isspace(*filename); filename++)
    ;
  if (filename) {
    if (strlen(filename) < 12) {
      filename = NULL;
    } else {
      char *s;
      
      if (strstr(filename, "..")) {
        LOG(LOG_WARNING, "%s: Illegal filename", filename);
        goto syntax;
      }
      s = strrchr(filename, '/'); // strip a path, if present
      if (s) filename = ++s;
      s = strrchr(filename, '\\'); // DOS format paths
      if (s) filename = ++s;
    }
  }
  sp_info = spool_open(OPT_ARG(SPOOLDIR), OPT_ARG(OUTPUT), filename);
  if (sp_info == NULL) {
    sprintf(buffer, "-ERR Sorry, error spooling file - enter command\n");
    if (++errors < 4) goto logged_in;
    goto end;
  }

  sprintf(buffer, "+OK start sending your file\n");
  uw_setproctitle("%s: %s", remotehost, buffer);
  if (debug > 3) fprintf(stderr, "%s[%u] > %s", remotehost, st_netfd_fileno(rmt_nfd), buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == -1) {
    if (errno == ETIME) {
      LOG(LOG_WARNING, "%s: timeout writing %s", remotehost, buffer);
    } else {
      LOG(LOG_WARNING, "%s: %m", remotehost);
    }
    return;
  }

  targ = strdup(spool_tmpfilename(sp_info));
  while (filesize > 0) {
    int length;

    memset(buffer, 0, sizeof(buffer));
    length = filesize > sizeof(buffer) ? sizeof(buffer) : filesize;
    //fprintf(stderr, "reading %u bytes", length);
    len = st_read(rmt_nfd, buffer, length, TIMEOUT);
    if (len == -1) {
      if (errno == ETIME) {
        LOG(LOG_WARNING, "%s: timeout reading fileblock", remotehost);
      } else { 
        LOG(LOG_WARNING, "%s: %m", remotehost);
      }
      spool_close(sp_info, FALSE);
      goto end;
    }
    if (debug > 3) {
      fprintf(stderr, "%s[%u] > %.*s", remotehost, st_netfd_fileno(rmt_nfd), len, buffer);
    }
    if (spool_write(sp_info, buffer, len) == -1) {
      LOG(LOG_ERR, "%s: %s write error writing %u bytes, %u bytes to go", 
                      targ+spooldir_strlen, remotehost, len, filesize);
      spool_close(sp_info, FALSE);
      goto end;
    }
    filesize -= len;
    if (debug > 3) {
      fprintf(stderr, "%s[%u] still %u bytes to receive", remotehost, st_netfd_fileno(rmt_nfd), filesize);
    }
  }
  free(targ);

  targ = strdup(spool_targfilename(sp_info));
  if (!spool_close(sp_info, TRUE)) {
    LOG(LOG_ERR, "%s: %s close error", remotehost, targ+spooldir_strlen);
    goto end;
  }
  LOG(LOG_INFO, "%s: spooled %u bytes to %s", remotehost, givensize, targ+spooldir_strlen);
  free(targ);

  sprintf(buffer, "+OK Thank you. Enter command\n");
  goto logged_in;

syntax:
  LOG(LOG_WARNING, "%s: %s", remotehost, buffer);
  sprintf(buffer, "-ERR syntax error\n");
  if (++errors < 4) goto login;

end:
  sprintf(buffer, "+OK Arrivederci\n");
  uw_setproctitle("%s: %s", remotehost, buffer);
  if (debug > 3) fprintf(stderr, "%s[%u] > %s", remotehost, st_netfd_fileno(rmt_nfd), buffer);
  st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  uw_setproctitle("accepting connections");
}
