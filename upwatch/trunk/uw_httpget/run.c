#include <stdio.h>
#include <generic.h>
#include <db.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "st.h"
#include "uw_httpget.h"

#define TIMEOUT 50000000L

struct probedef {
  int           id;             /* unique probe id */
  int           probeid;        /* server probe id */
  char          *realm;        /* database realm */
  int           seen;           /* seen */
#include "../common/common.h"
#include "probe.def_h"
#include "probe.res_h"
  char          *msg;           /* last error message */
  char          *info;          /* HTTP GET result */
};

GHashTable *cache;
static void *probe(void *user_data); 
static void write_results(void);
void run_actual_probes(void);
void refresh_database(database *db);

int thread_count = 0;

void free_probe(void *probe)
{
  struct probedef *r = (struct probedef *)probe;

  //fprintf(stderr, "Freeing %u\n", r->id);
  if (r->ipaddress) {
    //fprintf(stderr, "%s ", r->ipaddress);
    g_free(r->ipaddress);
  }
  if (r->realm) g_free(r->realm);
  if (r->uri) {
    //fprintf(stderr, "%s ", r->uri);
    g_free(r->uri);
  }
  if (r->hostname) {
    //fprintf(stderr, "%s ", r->hostname);
    g_free(r->hostname);
  }
  if (r->msg) g_free(r->msg);
  if (r->info) g_free(r->info);
  g_free(r);
  //fprintf(stderr, "\n");
}

static void reset_seen(gpointer key, gpointer value, gpointer user_data)
{
  struct probedef *probe = (struct probedef *)value;

  probe->seen = 0;
}

static gboolean return_seen(gpointer key, gpointer value, gpointer user_data)
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
  return 1;
}

int run()
{
  database *db;

  if (!cache) {
    cache = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, free_probe);
  }

  LOG(LOG_INFO, "reading info from database");
  uw_setproctitle("reading info from database");
  db = open_database(OPT_ARG(DBTYPE), OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME),
                        OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
  if (db) {
    refresh_database(db);
    close_database(db);
  }

  if (g_hash_table_size(cache) > 0) {
    LOG(LOG_INFO, "running %d probes in group %u", g_hash_table_size(cache), (unsigned) OPT_VALUE_GROUPID);
    uw_setproctitle("running %d probes in group %u", g_hash_table_size(cache), (unsigned) OPT_VALUE_GROUPID);
    run_actual_probes(); /* this runs the actual probes */

    LOG(LOG_INFO, "writing results");
    uw_setproctitle("writing results");
    write_results();
  }

  return(g_hash_table_size(cache));
}

void refresh_database(database *db)
{
  dbi_result result;
  char qry[1024];
  char *error;

  //g_hash_table_foreach_remove(cache, delete_probe, NULL);
  sprintf(qry,  "SELECT pr_httpget_def.id, pr_httpget_def.domid, pr_httpget_def.tblid, pr_realm.name,"
                "       pr_httpget_def.ipaddress, pr_httpget_def.uri, "
                "       pr_httpget_def.hostname, pr_httpget_def.port, "
                "       pr_httpget_def.yellow,  pr_httpget_def.red "
                "FROM   pr_httpget_def, pr_realm "
                "WHERE  pr_httpget_def.id > 1 and pr_httpget_def.disable <> 'yes'"
                "       and pr_httpget_def.pgroup = '%u' and pr_realm.id = pr_httpget_def.domid",
                (unsigned) OPT_VALUE_GROUPID);

  result = db_query(db, 1, qry);
  if (!result) {
    return;
  }
  while (dbi_result_next_row(result)) {
    int id;
    struct probedef *probe;

    id = dbi_result_get_int(result, "id");
    probe = g_hash_table_lookup(cache, &id);
    if (!probe) {
      probe = g_malloc0(sizeof(struct probedef));
      probe->id = id;
      if (dbi_result_get_int(result, "domid") > 1) {
        probe->probeid = dbi_result_get_int(result, "tblid");
        probe->realm = dbi_result_get_string_copy(result, "name");
      } else {
        probe->probeid = probe->id;
      }
      //fprintf(stderr, "Adding %u\n", id);
      g_hash_table_insert(cache, guintdup(id), probe);
    }

    if (probe->ipaddress) g_free(probe->ipaddress);
    probe->ipaddress = dbi_result_get_string_copy(result, "ipaddress");
    if (probe->uri) g_free(probe->uri);
    probe->uri = dbi_result_get_string_copy(result, "uri");
    if (probe->hostname) g_free(probe->hostname);
    probe->hostname = dbi_result_get_string_copy(result, "hostname");
    probe->port = dbi_result_get_int(result, "port");
    probe->yellow = dbi_result_get_float(result, "yellow");
    probe->red = dbi_result_get_float(result, "red");
    if (probe->msg) g_free(probe->msg);
    probe->msg = NULL;
    if (probe->info) g_free(probe->info);
    probe->info = NULL;
    probe->seen = 1;
  }
  dbi_result_free(result);
  if (dbi_conn_error(db, &error) == DBI_ERROR_NONE) { 
    g_hash_table_foreach_remove(cache, return_seen, NULL);
  } else {
    LOG(LOG_ERR, "%s", error);
    g_hash_table_foreach(cache, reset_seen, NULL);
  }
}

void add_probe(gpointer key, gpointer value, gpointer user_data)
{
  if (st_thread_create(probe, value, 0, 256*1024) == NULL) {
    //fprintf(stderr, "couldn't create thread");
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

static void write_probe(gpointer key, gpointer value, gpointer user_data)
{
  xmlDocPtr doc = (xmlDocPtr) user_data;
  struct probedef *probe = (struct probedef *)value;

  xmlNodePtr httpget;
  int color;
  char buffer[1024];
  time_t now = time(NULL);

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
  httpget = xmlNewChild(xmlDocGetRootElement(doc), NULL, "httpget", NULL);
  if (probe->realm) {
    xmlSetProp(httpget, "realm", probe->realm);
  }
  sprintf(buffer, "%u", probe->probeid);      xmlSetProp(httpget, "id", buffer);
  sprintf(buffer, "%s", probe->ipaddress);    xmlSetProp(httpget, "ipaddress", buffer);
  sprintf(buffer, "%u", (int) now);           xmlSetProp(httpget, "date", buffer);
  sprintf(buffer, "%u", ((int)now)+((unsigned)OPT_VALUE_EXPIRES*60));
    xmlSetProp(httpget, "expires", buffer);
  sprintf(buffer, "%u", color);               xmlSetProp(httpget, "color", buffer);
  sprintf(buffer, "%f", probe->lookup);       xmlNewChild(httpget, NULL, "lookup", buffer);
  sprintf(buffer, "%f", probe->connect);      xmlNewChild(httpget, NULL, "connect", buffer);
  sprintf(buffer, "%f", probe->pretransfer);  xmlNewChild(httpget, NULL, "pretransfer", buffer);
  sprintf(buffer, "%f", probe->total);        xmlNewChild(httpget, NULL, "total", buffer);
  probe->lookup = 0.0;
  probe->connect = 0.0;
  probe->pretransfer = 0.0;
  probe->total = 0.0;
  if (probe->info) {
    xmlNewTextChild(httpget, NULL, "info", probe->info);
    free(probe->info);
    probe->info = NULL;
  }
  if (probe->msg) {
    xmlNewTextChild(httpget, NULL, "info", probe->msg);
    free(probe->msg);
    probe->msg = NULL;
  }
}

static void write_results(void)
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

static void *probe(void *data)
{
  int sock, len;
  struct sockaddr_in rmt;
  st_netfd_t rmt_nfd;
  struct probedef *probe = (struct probedef *)data;
  char buffer[1024];
  st_utime_t start;

  ST_INITIATE(probe->port);

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

  sprintf(buffer, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", probe->uri, probe->hostname);
  if (debug > 3) fprintf(stderr, "> %s", buffer);
  len = st_write(rmt_nfd, buffer, strlen(buffer), TIMEOUT);
  if (len == -1) {
    ST_ERROR("write", TIMEOUT);
    goto err_close;
  }
  probe->pretransfer = ((float) (st_utime() - start)) * 0.000001;

  // Now read the response
  memset(buffer, 0, sizeof(buffer));
  len = st_read(rmt_nfd, buffer, 512, TIMEOUT);
  if (len == -1) {
    ST_ERROR("read", TIMEOUT);
    goto err_close;
  }
  if (debug > 3) fprintf(stderr, "< %s", buffer);
  probe->info = strdup(buffer);

err_close:
  st_netfd_close(rmt_nfd);
  probe->total = ((float) (st_utime() - start)) * 0.000001;
  if (probe->pretransfer == 0)  probe->pretransfer = probe->total;

done:
  thread_count--;
  return NULL;
}

