#include "config.h"
#include <generic.h>
#include "bbhimport.h"

/* list of probes */
typedef enum
{
PROBE_EMPTY = 1,
#include "../../../probes.enum"
} probeidx;

void process(dbi_conn conn, char *ip, char *hostname, char *args);

int init(void)
{
  daemonize = FALSE;
  every = ONE_SHOT;
  return(1);
}

int run(void)
{
  FILE *in;
  char buffer[4096];
  int count = 0;
  dbi_conn conn;

  if (!HAVE_OPT(INPUT)) {
    fprintf(stderr, "parameter -I is required\n");
    return 0;
  }

  conn = open_database(OPT_ARG(DBTYPE), OPT_ARG(DBHOST), OPT_ARG(DBPORT), OPT_ARG(DBNAME), 
			OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
  if (!conn) {
    printf("Can't open database\n");
    return 0;
  }
  in = fopen(OPT_ARG(INPUT), "r");
  if (!in) {
    perror(OPT_ARG(INPUT));
    return 0;
  }
  while (fgets(buffer, sizeof(buffer), in)) {
    char *ip, *hostname, *arg;

    if (buffer[0] == '\n' || buffer[0] == '#' || buffer[0] == ' ') {
      continue;
    }
    buffer[strlen(buffer)-1] = 0; // chop \n
    if (strstr(buffer, "group-compress")) continue;
    ip = strtok(buffer, " \t");
    hostname = strtok(NULL, " \t");
    arg = strtok(NULL, " \t");
    if (arg) {
      arg = strtok(NULL, " \t");
    }
    count++;
    process(conn, ip, hostname, arg ? arg : "");
  }
  fclose(in);
  close_database(conn);
  return count;
}

void process(dbi_conn conn, char *ip, char *hostname, char *args)
{
  dbi_result result;
  int serverid;
  char *p;

  result = db_query(conn, 0, "select %s from %s where %s = '%s'", 
       OPT_ARG(SERVER_TABLE_ID_FIELD), OPT_ARG(SERVER_TABLE_NAME), 
       OPT_ARG(SERVER_TABLE_NAME_FIELD), hostname);
  if (!result) {
    printf("internal error. Stop.\n");
    exit(1);
  }
  if (!dbi_result_next_row(result)) {
    dbi_result_free(result);
    return;
  }
  serverid = dbi_result_get_uint_idx(result, 0);
  dbi_result_free(result);
  if (!serverid) return;

  if (strstr(args, "noping") == NULL) {
    int rows;

    result = db_query(conn, 0, 
                      "select id from pr_ping_def where server = '%d' and ipaddress = '%s'",
                      serverid, ip);
    if (!result) {
      printf("internal error. Stop.\n");
      exit(1);
    }
    rows = dbi_result_get_numrows(result);
    dbi_result_free(result);

    if (rows == 0) { 
      printf("INSERT ping %s %s\n", ip, hostname);
      result = db_query(conn, 0, 
               "insert into pr_ping_def set server = '%d', ipaddress = '%s', description = '%s'",
               serverid, ip, hostname);
      dbi_result_free(result);
    } else {
      printf("ALREADY THERE: ping %s %s\n", ip, hostname);
    }
  }

  if ((p = strstr(args, "http://")) != NULL) {
    char HostName[256];
    char URI[2048];
    char *s;
    int i;
    int rows;

    s = p + 7;
    for (i=0; *s && *s != '/'; i++, s++) {
      HostName[i] = *s;
    }
    HostName[i] = 0;
    for (i=0; *s; i++, s++) {
      URI[i] = *s;
    }
    if (i == 0) {
      URI[i++] = '/';
    }
    URI[i] = 0;

    result = db_query(conn, 0,
                      "select id from pr_httpget_def where server = '%d' and ipaddress = '%s' and "
                      "hostname = '%s' and uri = '%s'", serverid, ip, HostName, URI);
    if (!result) {
      printf("internal error. Stop.\n");
      exit(1);
    }
    rows = dbi_result_get_numrows(result);
    dbi_result_free(result);

    if (rows == 0) { 
      printf("INSERT httpget %s %s http://%s%s\n", ip, hostname, HostName, URI);
      result = db_query(conn, 0,
               "insert into pr_httpget_def set server = '%d', ipaddress = '%s', description = '%s', "
               "hostname = '%s', uri = '%s'", serverid, ip, hostname, HostName, URI);
      dbi_result_free(result);
    } else {
      printf("ALREADY THERE: httpget %s %s http://%s%s\n", ip, hostname, HostName, URI);
    }
  }

  if ((p = strstr(args, "pop-3")) != NULL || (p = strstr(args, "pop3")) != NULL) {
    int rows;

    result = db_query(conn, 0,
                      "select id from pr_pop3_def where server = '%d' and ipaddress = '%s'",
                      serverid, ip);
    if (!result) {
      printf("internal error. Stop.\n");
      exit(1);
    }
    rows = dbi_result_get_numrows(result);
    dbi_result_free(result);

    if (rows == 0) { 
      printf("INSERT pop-3 %s %s\n", ip, hostname);
      result = db_query(conn, 0,
               "insert into pr_pop3_def set server = '%d', ipaddress = '%s', description = '%s'",
               serverid, ip, hostname);
      dbi_result_free(result);
    } else {
      printf("ALREADY THERE: pop3 %s %s \n", ip, hostname);
    }
  }

  if ((p = strstr(args, "imap")) != NULL) {
    int rows;

    result = db_query(conn, 0,
                      "select id from pr_imap_def where server = '%d' and ipaddress = '%s'",
                      serverid, ip);
    if (!result) {
      printf("internal error. Stop.\n");
      exit(1);
    }
    rows = dbi_result_get_numrows(result);
    dbi_result_free(result);

    if (rows == 0) { 
      printf("INSERT imap %s %s\n", ip, hostname);
      result = db_query(conn, 0,
               "insert into pr_imap_def set server = '%d', ipaddress = '%s', description = '%s'",
               serverid, ip, hostname);
      dbi_result_free(result);
    } else {
      printf("ALREADY THERE: imap %s %s \n", ip, hostname);
    }
  }
}

