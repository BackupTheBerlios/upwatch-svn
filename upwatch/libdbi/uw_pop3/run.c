#include "config.h"
#include <db.h>
#include <generic.h>
#include <st.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "uw_pop3.h"
#define TIMEOUT	10000000L

struct probedef {
  int           id;             /* unique probe id */
  int           probeid;        /* server probe id */
  char          *realm;        /* database realm */
  int		seen;           /* seen */
#include "../common/common.h"
#include "probe.def_h"
#include "probe.res_h"
  char		*msg;           /* last error message */
};
GHashTable *cache;
int thread_count;

void free_probe(void *probe)
{
  struct probedef *r = (struct probedef *)probe;

  if (r->ipaddress) g_free(r->ipaddress);
  if (r->realm) g_free(r->realm);
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
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

void refresh_database(dbi_conn conn);
void run_actual_probes(void);
void write_results(void);

int run(void)
{
  dbi_conn conn;

  if (!cache) {
    cache = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, free_probe);
  }
  
  LOG(LOG_INFO, "reading info from database");
  uw_setproctitle("reading info from database");
  conn = open_database(OPT_ARG(DBTYPE), OPT_ARG(DBHOST), OPT_ARG(DBPORT), OPT_ARG(DBNAME), 
			OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
  if (conn) {
    refresh_database(conn);
    close_database(conn);
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

void refresh_database(dbi_conn conn)
{
  dbi_result result;
  char qry[1024];

  sprintf(qry,  "SELECT pr_pop3_def.id, pr_pop3_def.domid, pr_pop3_def.tblid, pr_realm.name, "
                "       pr_pop3_def.ipaddress, pr_pop3_def.username, "
                "       pr_pop3_def.password, "
                "       pr_pop3_def.yellow,  pr_pop3_def.red "
                "FROM   pr_pop3_def, pr_realm "
                "WHERE  pr_pop3_def.id > 1 and pr_pop3_def.disable <> 'yes'"
                "       and pr_pop3_def.pgroup = '%d' and pr_realm.id = pr_pop3_def.domid",
                (unsigned)OPT_VALUE_GROUPID);

  result = db_query(conn, 1, qry);
  if (!result) {
    return;
  }
    
  while (dbi_result_next_row(result)) {
    int id;
    struct probedef *probe;

    id = dbi_result_get_uint(result, "id");
    probe = g_hash_table_lookup(cache, &id);
    if (!probe) {
      probe = g_malloc0(sizeof(struct probedef));
      if (dbi_result_get_uint(result, "domid") > 1) {
        probe->probeid = dbi_result_get_uint(result, "tblid");
        probe->realm=strdup(dbi_result_get_string(result, "name"));
      } else {
        probe->probeid = probe->id;
      }
      g_hash_table_insert(cache, guintdup(id), probe);
    }

    if (probe->ipaddress) g_free(probe->ipaddress);
    probe->ipaddress = dbi_result_get_string_copy(result, "ipaddress");
    if (probe->username) g_free(probe->username);
    probe->username = dbi_result_get_string_copy(result, "username");
    if (probe->password) g_free(probe->password);
    probe->password = dbi_result_get_string_copy(result, "password");
    probe->yellow = dbi_result_get_float(result, "yellow");
    probe->red = dbi_result_get_float(result, "red");
    if (probe->msg) g_free(probe->msg);
    probe->msg = NULL;
    probe->seen = 1;
  }
  if (dbi_conn_error_flag(conn)) {
    const char *errmsg;
    dbi_conn_error(conn, &errmsg);
    LOG(LOG_ERR, "%s", errmsg);
    g_hash_table_foreach(cache, reset_seen, NULL);
  } else {
    g_hash_table_foreach_remove(cache, return_seen, NULL);
  }
  dbi_result_free(result);
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
  if (probe->realm) {
    xmlSetProp(pop3, "realm", probe->realm);
  }
  sprintf(buffer, "%d", probe->probeid);      xmlSetProp(pop3, "id", buffer);
  sprintf(buffer, "%s", probe->ipaddress);    xmlSetProp(pop3, "ipaddress", buffer);
  sprintf(buffer, "%d", (int) now);           xmlSetProp(pop3, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+((unsigned)OPT_VALUE_EXPIRES*60));
    xmlSetProp(pop3, "expires", buffer);
  sprintf(buffer, "%d", color);               xmlSetProp(pop3, "color", buffer);
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
  int ct  = STACKCT_OPT(OUTPUT);
  char **output = (char **) &STACKLST_OPT(OUTPUT);
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

  ST_INITIATE(110);

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

  // expect here: +OK POP3 xxx.xxxxxxx.xx v2000.70rh server ready
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    ST_ERROR("read", TIMEOUT);
    goto err_close;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  if (buffer[0] != '+') {
    probe->msg = strdup(buffer);
    goto err_close;
  }
  if (probe->username == NULL || probe->username[0] == 0) {
    probe->msg = strdup("missing username");
    goto err_close;
  }

  sprintf(buffer, "USER %s\n", probe->username);
  if (debug > 3) fprintf(stderr, "> %s", buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == -1) {
    ST_ERROR("write", TIMEOUT);
    goto err_close;
  }

  // expect here: +OK User name accepted, password please
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    ST_ERROR("read", TIMEOUT);
    goto err_close;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  if (buffer[0] != '+') {
    probe->msg = strdup(buffer);
    goto err_close;
  }

  sprintf(buffer, "PASS %s\n", probe->password);
  if (debug > 3) fprintf(stderr, "> %s", buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == -1) {
    ST_ERROR("write", TIMEOUT);
    goto err_close;
  }

  // expect here: +OK Mailbox open, 62 messages
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    ST_ERROR("read", TIMEOUT);
    goto err_close;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  if (buffer[0] != '+') {
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

  // expect here: +OK Sayonara
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, sizeof(buffer), TIMEOUT);
  if (len == -1) {
    ST_ERROR("read", TIMEOUT);
    goto err_close;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  if (buffer[0] != '+') {
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
