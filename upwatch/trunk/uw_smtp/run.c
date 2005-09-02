#include "config.h"
#include <db.h>
#include <generic.h>
#include <st.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "uw_smtp.h"
#define TIMEOUT	10000000L

struct probedef {
  int           id;             /* unique probe id */
  int           probeid;        /* server probe id */
  char          *realm;         /* database realm */
  int		seen;           /* seen */
#include "../common/common.h"
#include "probe.def_h"
#include "probe.res_h"
  char		*msg;           /* last error message */
};
GHashTable *cache;
int thread_count;
char myhostname[256];

void free_probe(void *probe)
{
  struct probedef *r = (struct probedef *)probe;

  if (r->ipaddress) g_free(r->ipaddress);
  if (r->realm) g_free(r->realm);
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

  if (probe->seen == 0) {
    LOG(LOG_INFO, "removed probe %s:%u from list", probe->realm, probe->probeid);
    return 1;
  }
  probe->seen = 0;
  return(0);
}

int init(void)
{
  if (!HAVE_OPT(OUTPUT)) {
    LOG(LOG_ERR, "missing output option");
    return 0;
  }
  daemonize = TRUE;
  every = EVERY_MINUTE;
  startsec = OPT_VALUE_BEGIN;
  if (gethostname(myhostname, sizeof(myhostname))) {
    strcpy(myhostname, "localhost");
  }
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
  
  LOG(LOG_INFO, "reading info from database");
  uw_setproctitle("reading info from database");
  mysql = open_database(OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME), 
			OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
  if (mysql) {
    refresh_database(mysql);
    close_database(mysql);
  }

  if (g_hash_table_size(cache) > 0) {
    LOG(LOG_INFO, "running %d probes", g_hash_table_size(cache));
    uw_setproctitle("running %d probes", g_hash_table_size(cache));
    run_actual_probes(); /* this runs the actual probes */

    LOG(LOG_INFO, "writing results");
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

  sprintf(qry,  "SELECT pr_smtp_def.id, pr_smtp_def.domid, pr_smtp_def.tblid, pr_realm.name, "
                "       pr_smtp_def.ipaddress, "
                "       pr_smtp_def.yellow,  pr_smtp_def.red "
                "FROM   pr_smtp_def, pr_realm "
                "WHERE  pr_smtp_def.id > 1 and pr_smtp_def.disable <> 'yes'"
                "       and pr_smtp_def.pgroup = '%d' and pr_realm.id = pr_smtp_def.domid",
                (unsigned)OPT_VALUE_GROUPID);

  result = my_query(mysql, 1, qry);
  if (!result) {
    return;
  }
    
  while ((row = mysql_fetch_row(result))) {
    int id;
    struct probedef *probe;

    id = atol(row[0]);
    probe = g_hash_table_lookup(cache, &id);
    if (!probe) {
      probe = g_malloc0(sizeof(struct probedef));
      if (atoi(row[1]) > 1) {
        probe->probeid = atoi(row[2]);
        probe->realm = strdup(row[3]);
      } else {
        probe->probeid = probe->id;
      }
      g_hash_table_insert(cache, guintdup(id), probe);
    }

    if (probe->ipaddress) g_free(probe->ipaddress);
    probe->ipaddress = strdup(row[4]);
    probe->yellow = atof(row[5]);
    probe->red = atof(row[6]);
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
    LOG(LOG_WARNING, "couldn't create thread");
  } else {
    thread_count++;
  }
}

void run_actual_probes(void)
{
  thread_count = 0;
  st_usleep(1); //force context switch so timers will work
  g_hash_table_foreach(cache, add_probe, NULL);
  while (thread_count) {
    st_sleep(1);
  }
}

void write_probe(gpointer key, gpointer value, gpointer user_data)
{
  xmlDocPtr doc = (xmlDocPtr) user_data;
  struct probedef *probe = (struct probedef *)value;

  xmlNodePtr subtree, smtp;
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

  smtp = xmlNewChild(xmlDocGetRootElement(doc), NULL, "smtp", NULL);
  if (probe->realm) {
    xmlSetProp(smtp, "realm", probe->realm);
  }
  sprintf(buffer, "%d", probe->probeid);      xmlSetProp(smtp, "id", buffer);
  sprintf(buffer, "%s", probe->ipaddress);    xmlSetProp(smtp, "ipaddress", buffer);
  sprintf(buffer, "%d", (int) now);           xmlSetProp(smtp, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+((unsigned)OPT_VALUE_EXPIRES*60));
    xmlSetProp(smtp, "expires", buffer);
  sprintf(buffer, "%d", color);               xmlSetProp(smtp, "color", buffer);
  sprintf(buffer, "%f", probe->connect);      subtree = xmlNewChild(smtp, NULL, "connect", buffer);
  sprintf(buffer, "%f", probe->total);        subtree = xmlNewChild(smtp, NULL, "total", buffer);
  if (probe->msg) {
    subtree = xmlNewTextChild(smtp, NULL, "info", probe->msg);
    free(probe->msg);
    probe->msg = NULL;
  }
}

void write_results(void)
{
  int ct  = STACKCT_OPT(OUTPUT);
  char **output = STACKLST_OPT(OUTPUT);
  int i;
  xmlDocPtr doc;

  doc = UpwatchXmlDoc("result", NULL);

  g_hash_table_foreach(cache, write_probe, doc);
  xmlSetDocCompressMode(doc, OPT_VALUE_COMPRESS);
  for (i=0; i < ct; i++) {
    spool_result(OPT_ARG(SPOOLDIR), output[i], doc, NULL);
  }
  xmlFreeDoc(doc);
}

void *probe(void *data) 
{ 
  int sock, len;
  struct sockaddr_in rmt;
  st_netfd_t rmt_nfd;
  struct probedef *probe = (struct probedef *)data;
  char buffer[1024];
  st_utime_t start;

  ST_INITIATE(25);

  start = st_utime();

  if (debug > 3) fprintf(stderr, "Connecting to: %s\n", probe->ipaddress);
  if (st_connect(rmt_nfd, (struct sockaddr *)&rmt, sizeof(rmt), TIMEOUT) < 0) {
    char buf[256];

    sprintf(buf, "%s(%d): %s", probe->ipaddress, __LINE__, strerror(errno));
    probe->connect = ((float) (st_utime() - start)) * 0.000001;
    probe->msg = strdup(buf);
    LOG(LOG_DEBUG, probe->msg);
    if (debug > 3) fprintf(stderr, "%s: %s\n", probe->ipaddress, probe->msg);
    goto err_close;
  }
  probe->connect = ((float) (st_utime() - start)) * 0.000001;

  // expect here: "220 n010080.nbs.netland.nl ESMTP Postfix"
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    ST_ERROR("read", TIMEOUT);
    goto err_close;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  if (strncmp(buffer, "220 ", 4) != 0) {
    probe->msg = strdup(buffer);
    goto err_close;
  }

  sprintf(buffer, "EHLO %s\n", myhostname);
  if (debug > 3) fprintf(stderr, "> %s", buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == -1) {
    ST_ERROR("write", TIMEOUT);
    goto err_close;
  }

  // expect here: 
  //  250-n010080.nbs.netland.nl
  //  250-PIPELINING
  //  250-SIZE 10240000
  //  250-VRFY
  //  250-ETRN
  //  250-XVERP
  //  250 8BITMIME
  // note the last line has a space on position 4
  //
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    ST_ERROR("read", TIMEOUT);
    goto err_close;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  if (strstr(buffer, "\r\n250 ") == NULL) {
    probe->msg = strdup(buffer);
    goto err_close;
  }

  sprintf(buffer, "MAIL FROM:\n");
  if (debug > 3) fprintf(stderr, "> %s", buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == -1) {
    ST_ERROR("write", TIMEOUT);
    goto err_close;
  }

  // expect here: "501 Syntax: MAIL FROM: <address>"
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    ST_ERROR("read", TIMEOUT);
    goto err_close;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  if (strncmp(buffer, "501 ", 4) != 0) {
    probe->msg = strdup(buffer);
    goto err_close;
  }

  sprintf(buffer, "QUIT\n");
  if (debug > 3) fprintf(stderr, "> %s", buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == -1) {
    ST_ERROR("write", TIMEOUT);
    goto err_close;
  }

  // expect here: 220 Sayonara
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    ST_ERROR("read", TIMEOUT);
    goto err_close;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  if (strncmp(buffer, "221 ", 4) != 0) {
    probe->msg = strdup(buffer);
    goto err_close;
  }

err_close:
  st_netfd_close(rmt_nfd);
  probe->total = ((float) (st_utime() - start)) * 0.000001;

done:
  thread_count--;
  return NULL;
}
