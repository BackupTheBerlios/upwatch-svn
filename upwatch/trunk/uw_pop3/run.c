#include "config.h"
#include <generic.h>
#include <st.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cmd_options.h"
#define TIMEOUT	10000000L

struct probedef {
  int		id;             /* server probe id */
  int		seen;           /* seen */
  char		*ipaddress;     /* server name */
#include "probe.def_h"
#include "../common/common.h"
#include "probe.res_h"
  char		*msg;           /* last error message */
};
GHashTable *cache;
int thread_count;

void free_probe(void *probe)
{
  struct probedef *r = (struct probedef *)probe;

  if (r->ipaddress) g_free(r->ipaddress);
  if (r->username) g_free(r->username);
  if (r->password) g_free(r->password);
  if (r->msg) g_free(r->msg);
  g_free(r);
}

void reset_seen(gpointer key, gpointer value, gpointer user_data)
{
  struct probedef *probe = (struct probedef *)value;

  probe->seen = 0;
}

gboolean return_seen(gpointer key, gpointer value, gpointer user_data)
{
  struct probedef *probe = (struct probedef *)value;

  return(probe->seen == 0);
}

int init(void)
{
  daemonize = TRUE;
  every = EVERY_MINUTE;
  startsec = OPT_VALUE_BEGIN;
  st_init(); 
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

void refresh_database(MYSQL *mysql);
void run_actual_probes(void);
void write_results(void);

int run(void)
{
  MYSQL *mysql;

  if (!cache) {
    cache = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, free_probe);
  }
  
  if (debug > 0) LOG(LOG_DEBUG, "reading info from database");
  uw_setproctitle("reading info from database");
  mysql = open_database(OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME), 
			OPT_ARG(DBUSER), OPT_ARG(DBPASSWD), OPT_VALUE_DBCOMPRESS);
  if (mysql) {
    refresh_database(mysql);
    close_database(mysql);
  }

  if (g_hash_table_size(cache) > 0) {
    if (debug > 0) LOG(LOG_DEBUG, "running %d probes", g_hash_table_size(cache));
    uw_setproctitle("running %d probes", g_hash_table_size(cache));
    run_actual_probes(); /* this runs the actual probes */

    if (debug > 0) LOG(LOG_DEBUG, "writing results");
    uw_setproctitle("writing results");
    write_results();
  }

  return(g_hash_table_size(cache));
}

void refresh_database(MYSQL *mysql)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char qry[1024];

  sprintf(qry,  "SELECT pr_pop3_def.id, "
                "       pr_pop3_def.ipaddress, pr_pop3_def.username, "
                "       pr_pop3_def.password, "
                "       pr_pop3_def.yellow,  pr_pop3_def.red "
                "FROM   pr_pop3_def "
                "WHERE  pr_pop3_def.id > 1 and pr_pop3_def.pgroup = '%d'",
                OPT_VALUE_GROUPID);

  result = my_query(mysql, 1, qry);
  if (!result) {
    LOG(LOG_ERR, "%s: %s", qry, mysql_error(mysql)); 
    return;
  }
    
  while ((row = mysql_fetch_row(result))) {
    int id;
    struct probedef *probe;

    id = atol(row[0]);
    probe = g_hash_table_lookup(cache, &id);
    if (!probe) {
      probe = g_malloc0(sizeof(struct probedef));
      probe->id = id;
      g_hash_table_insert(cache, guintdup(id), probe);
    }

    if (probe->ipaddress) g_free(probe->ipaddress);
    probe->ipaddress = strdup(row[1]);
    if (probe->username) g_free(probe->username);
    probe->username = strdup(row[2]);
    if (probe->password) g_free(probe->password);
    probe->password = strdup(row[3]);
    probe->yellow = atof(row[4]);
    probe->red = atof(row[5]);
    if (probe->msg) g_free(probe->msg);
    probe->msg = NULL;
    probe->seen = 1;
  }
  mysql_free_result(result);
  if (mysql_errno(mysql)) {
    g_hash_table_foreach(cache, reset_seen, NULL);
  } else {
    g_hash_table_foreach_remove(cache, return_seen, NULL);
  }
}

void *probe(void *user_data); 
void add_probe(gpointer key, gpointer value, gpointer user_data)
{
  if (st_thread_create(probe, value, 0, 0) == NULL) {
    LOG(LOG_DEBUG, "couldn't create thread");
  } else {
    thread_count++;
  }
}

void run_actual_probes(void)
{
  g_hash_table_foreach(cache, add_probe, NULL);
  while (thread_count) {
    st_sleep(1);
  }
}

void write_probe(gpointer key, gpointer value, gpointer user_data)
{
  xmlDocPtr doc = (xmlDocPtr) user_data;
  struct probedef *probe = (struct probedef *)value;

  xmlNodePtr subtree, pop3;
  int color;
  char info[1024];
  char buffer[1024];
  time_t now = time(NULL);

  info[0] = 0;
  if (probe->msg) {
    color = STAT_RED;
  } else {
    if (probe->total < probe->yellow) {
      color = STAT_GREEN;
    } else if (probe->total > probe->red) {
      color = STAT_RED;
    } else {
      color = STAT_YELLOW;
    }
  }

  pop3 = xmlNewChild(xmlDocGetRootElement(doc), NULL, "pop3", NULL);
  sprintf(buffer, "%d", probe->id);           xmlSetProp(pop3, "id", buffer);
  sprintf(buffer, "%s", probe->ipaddress);    xmlSetProp(pop3, "ipaddress", buffer);
  sprintf(buffer, "%d", (int) now);           xmlSetProp(pop3, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+(2*60));   xmlSetProp(pop3, "expires", buffer);
  sprintf(buffer, "%d", color);               subtree = xmlNewChild(pop3, NULL, "color", buffer);
  sprintf(buffer, "%f", probe->connect);      subtree = xmlNewChild(pop3, NULL, "connect", buffer);
  sprintf(buffer, "%f", probe->total);        subtree = xmlNewChild(pop3, NULL, "total", buffer);
  if (probe->msg) {
    subtree = xmlNewTextChild(pop3, NULL, "info", probe->msg);
    free(probe->msg);
    probe->msg = NULL;
  }
}

void write_results(void)
{
  xmlDocPtr doc = UpwatchXmlDoc("result");
  g_hash_table_foreach(cache, write_probe, doc);
  spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(OUTPUT), doc, NULL);
  xmlFreeDoc(doc);
}

void *probe(void *data) 
{ 
  int sock, len;
  struct sockaddr_in rmt;
  st_netfd_t rmt_nfd;
  struct probedef *probe = (struct probedef *)data;
  char buffer[1024];
  st_utime_t start, now;


  memset(&rmt, 0, sizeof(struct sockaddr_in));
  rmt.sin_family = AF_INET;
  rmt.sin_port = htons(110);
  rmt.sin_addr.s_addr = inet_addr(probe->ipaddress);
  if (rmt.sin_addr.s_addr == INADDR_NONE) {
    char buf[50];

    sprintf(buf, "Illegal IP address '%s'", probe->ipaddress);
    probe->msg = strdup(buf);
    goto done;
  }

  /* Connect to remote host */
  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    probe->msg = strdup(strerror(errno));
    goto done;
  }
  if ((rmt_nfd = st_netfd_open_socket(sock)) == NULL) {
    probe->msg = strdup(strerror(errno));
    close(sock);
    goto done;
  }
  start = st_utime();
  if (st_connect(rmt_nfd, (struct sockaddr *)&rmt, sizeof(rmt), -1) < 0) {
    probe->msg = strdup(strerror(errno));
    st_netfd_close(rmt_nfd);
    goto done;
  }
  now = st_utime();
  probe->connect = ((float) (now - start)) * 0.000001;

  // expect here: +OK POP3 xxx.xxxxxxx.xx v2000.70rh server ready
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT); 
  if (len == ETIME) {
    probe->msg = strdup("Timeout");
    goto close;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  if (buffer[0] != '+') {
    probe->msg = strdup(buffer);
    goto close;
  }
  if (probe->username == NULL || probe->username[0] == 0) goto close;

  sprintf(buffer, "USER %s\n", probe->username);
  if (debug > 3) fprintf(stderr, "> %s", buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == ETIME) {
    probe->msg = strdup("Timeout");
    goto close;
  }
  if (len == -1) {
    probe->msg = strdup(strerror(errno));
    goto close;
  }

  // expect here: +OK User name accepted, password please
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT); 
  if (len == ETIME) {
    probe->msg = strdup("Timeout");
    goto close;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  if (buffer[0] != '+') {
    probe->msg = strdup(buffer);
    goto close;
  }

  sprintf(buffer, "PASS %s\n", probe->password);
  if (debug > 3) fprintf(stderr, "> %s", buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == ETIME) {
    probe->msg = strdup("Timeout");
    goto close;
  }
  if (len == -1) {
    probe->msg = strdup(strerror(errno));
    goto close;
  }

  // expect here: +OK Mailbox open, 62 messages
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT); 
  if (len == ETIME) {
    probe->msg = strdup("Timeout");
    goto close;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  if (buffer[0] != '+') {
    probe->msg = strdup(buffer);
    goto close;
  }

  sprintf(buffer, "QUIT\n");
  if (debug > 3) fprintf(stderr, "> %s", buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == ETIME) {
    probe->msg = strdup("Timeout");
    goto close;
  }
  if (len == -1) {
    probe->msg = strdup(strerror(errno));
    goto close;
  }

  // expect here: +OK Sayonara
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT); 
  if (len == ETIME) {
    probe->msg = strdup("Timeout");
    goto close;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  if (buffer[0] != '+') {
    probe->msg = strdup(buffer);
    goto close;
  }

close:
  now = st_utime();
  probe->total = ((float) (now - start)) * 0.000001;
  st_netfd_close(rmt_nfd);

done:
  thread_count--;
  return NULL;
}
