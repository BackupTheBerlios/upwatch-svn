#include "config.h"
#include <math.h>
#include <generic.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "curl/curl.h"

#include "cmd_options.h"

struct probedef {
  int		id;             /* server probe id */
  int		seen;           /* seen */
  char		*ipaddress;     /* server name */
#include "probe.def_h"
#include "../common/common.h"
#include "probe.res_h"
  char		*info;          /* HTTP GET result */
  size_t        info_curlen;    /* HTTP GET result length */
  size_t        info_maxlen;    /* HTTP GET result length */
  char		*msg;           /* last error message */
};
GHashTable *cache;

void free_probe(void *probe)
{
  struct probedef *r = (struct probedef *)probe;

  if (r->hostname) g_free(r->hostname);
  if (r->uri) g_free(r->uri);
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
  g_thread_init(NULL);
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

void refresh_database(MYSQL *mysql);
void run_actual_probes(int probecount);
void write_results(void);

int run(void)
{
  MYSQL *mysql;

  if (!cache) {
    cache = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, free_probe);
  }
  
  if (debug > 0) LOG(LOG_DEBUG, "reading info from database");
  uw_setproctitle("reading info from database");
  mysql = open_database(OPT_ARG(DBHOST), OPT_ARG(DBNAME), OPT_ARG(DBUSER), OPT_ARG(DBPASSWD),
                        OPT_VALUE_DBCOMPRESS);
  if (mysql) {
    refresh_database(mysql);
    close_database(mysql);
  }

  if (debug > 0) LOG(LOG_DEBUG, "running %d probes", g_hash_table_size(cache));
  uw_setproctitle("running %d probes", g_hash_table_size(cache));
  run_actual_probes(g_hash_table_size(cache)); /* this runs the actual probes */

  if (debug > 0) LOG(LOG_DEBUG, "writing results");
  uw_setproctitle("writing results");
  write_results();

  return(g_hash_table_size(cache));
}

void refresh_database(MYSQL *mysql)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char qry[1024];

  sprintf(qry,  "SELECT pr_httpget_def.id, pr_httpget_def.ipaddress, "
                "       pr_httpget_def.hostname, pr_httpget_def.uri, "
                "       pr_httpget_def.yellow,  pr_httpget_def.red "
                "FROM   pr_httpget_def "
                "WHERE  pr_httpget_def.id > 1");

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
      probe->id = id;
      g_hash_table_insert(cache, guintdup(id), probe);
    }

    if (probe->ipaddress) g_free(probe->ipaddress);
    probe->ipaddress = strdup(row[1]);
    if (probe->hostname) g_free(probe->hostname);
    probe->hostname = strdup(row[2]);
    if (probe->uri) g_free(probe->uri);
    probe->uri = strdup(row[3]);
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

void probe(gpointer data, gpointer user_data);
void add_probe(gpointer key, gpointer value, gpointer user_data)
{
  GThreadPool *gtpool = (GThreadPool *) user_data;

  g_thread_pool_push(gtpool, value, NULL);
//probe(value, user_data);
}

void run_actual_probes(int probecount)
{
  GThreadPool *gtpool = NULL;
  GError *gerror;

  gtpool = g_thread_pool_new(probe, NULL, probecount < 30 ? 10 : (10 * (int) log(probecount)), 
                            TRUE, &gerror);
  if (gtpool == NULL) {
    LOG(LOG_ERR, gerror->message);
    fprintf(stderr, "could not create threadpool\n");
    return;
  }

  g_hash_table_foreach(cache, add_probe, gtpool);
  g_thread_pool_free (gtpool, FALSE, TRUE);
}

void write_probe(gpointer key, gpointer value, gpointer user_data)
{
  xmlDocPtr doc = (xmlDocPtr) user_data;
  struct probedef *probe = (struct probedef *)value;

  xmlNodePtr subtree, httpget;
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

  httpget = xmlNewChild(xmlDocGetRootElement(doc), NULL, "httpget", NULL);
  sprintf(buffer, "%d", probe->id);           xmlSetProp(httpget, "id", buffer);
  sprintf(buffer, "%s", probe->ipaddress);    xmlSetProp(httpget, "ipaddress", buffer);
  sprintf(buffer, "%d", (int) now);           xmlSetProp(httpget, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+(2*60));   xmlSetProp(httpget, "expires", buffer);
  sprintf(buffer, "%d", color);               subtree = xmlNewChild(httpget, NULL, "color", buffer);
  sprintf(buffer, "%f", probe->lookup);       subtree = xmlNewChild(httpget, NULL, "lookup", buffer);
  sprintf(buffer, "%f", probe->connect);      subtree = xmlNewChild(httpget, NULL, "connect", buffer);
  sprintf(buffer, "%f", probe->pretransfer);  subtree = xmlNewChild(httpget, NULL, "pretransfer", buffer);
  sprintf(buffer, "%f", probe->total);        subtree = xmlNewChild(httpget, NULL, "total", buffer);
  if (probe->msg) {
    subtree = xmlNewTextChild(httpget, NULL, "info", probe->msg);
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

static size_t write_function(void *ptr, size_t size, size_t nmemb, void *stream)
{
  struct probedef *host = (struct probedef *)stream;
  int len = size * nmemb;

  if (host->info_curlen + len > host->info_maxlen) {
    host->info = realloc(host->info, host->info_maxlen + len + 512);
    host->info_maxlen += (len + 512);
  }
  memcpy(host->info + host->info_curlen, ptr, len);
  host->info_curlen += len;
  *(host->info + host->info_curlen) = 0;
  return(len);
}

void probe(gpointer data, gpointer user_data)
{
  struct probedef *host = (struct probedef *)data;
  CURL *curl;
  char buffer[BUFSIZ];
  double result;

  if (!host) return;

  if (host->ipaddress == NULL) {
    struct hostent ret, *result;
    int h_errno;

    if (gethostbyname_r (host->hostname, &ret, buffer, sizeof(buffer), &result, &h_errno) == 0) {
      int **addresslist = (int **)ret.h_addr_list;
      struct in_addr myaddr;

      myaddr.s_addr = **addresslist;
      host->ipaddress = strdup(inet_ntoa(myaddr));
    } else {
      host->info = strdup(buffer);
      return;
    }
  }
  sprintf(buffer, "http://%s", host->hostname);
  if (host->uri[0] != '/') {
    strcat(buffer, "/");
  }
  strcat(buffer, host->uri);

  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_FILE, host);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
  curl_easy_setopt(curl, CURLOPT_URL, buffer);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, host->msg);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, TRUE);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 50L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
  //host->ret = curl_easy_perform(curl);

  // Pass a pointer to a double to receive the time, in seconds, it took from the start
  // until the name resolving was completed.
  curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME, &result);
  host->lookup = (float) result;

  // Pass a pointer to a double to receive the time, in seconds, it took from the start
  // until the connect to the remote host (or proxy) was completed.
  curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &result);
  host->connect = (float) result;

  // Pass a pointer to a double to receive the time, in seconds, it  took from the start
  // until the file transfer is just about to begin. This includes all pre-transfer com­
  // mands and negotiations that are specific to the particular protocol(s) involved.
  curl_easy_getinfo(curl, CURLINFO_PRETRANSFER_TIME, &result);
  host->pretransfer = (float) result;

  // Pass a pointer to a double to receive the total transaction time in seconds for the
  // previous transfer.
  curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &result);
  host->total = (float) result;

  if (host->info) host->info[512] = 0; // limit amount of data

  curl_easy_cleanup(curl);
}

