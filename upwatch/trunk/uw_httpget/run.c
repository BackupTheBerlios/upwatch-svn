#include "config.h"
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <curl/curl.h>

#include <generic.h>
#include "cmd_options.h"

struct hostinfo {
  int		id;             /* server probe id */
  char		*name;          /* server name */
#include "probe.def_h"
#include "../common/common.h"
#include "probe.res_h"
  int		ret;            /* return value */
  char		msg[CURL_ERROR_SIZE]; /* last error message */
  char		*info;           /* HTTP GET result */
  size_t	info_curlen;     /* HTTP GET result length */
  size_t	info_maxlen;     /* HTTP GET result length */
} hostinfo;

static struct hostinfo **hosts;
static int num_hosts;

void probe(gpointer data, gpointer user_data);
int init(void)
{
  daemonize = TRUE;
  every = EVERY_MINUTE;
  startsec = OPT_VALUE_BEGIN;
  g_thread_init(NULL);
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

int run(void)
{
int run_actual_probes(int count);
  xmlDocPtr doc, red;
  int id=0, redcount=0;
  time_t now;

  if (debug > 0) LOG(LOG_DEBUG, "reading info from database");
  uw_setproctitle("reading info from database");
  while (open_database() == 0) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    char qry[1024];

    sprintf(qry,  "SELECT pr_httpget_def.id, %s.%s, pr_httpget_def.hostname, " 
                  "       pr_httpget_def.uri, " 
                  "       pr_httpget_def.yellow,  pr_httpget_def.red "
                  "FROM   pr_httpget_def, server "
                  "WHERE  pr_httpget_def.server = %s.%s and pr_httpget_def.id > 1", 
            OPT_ARG(SERVER_TABLE_NAME), OPT_ARG(SERVER_TABLE_NAME_FIELD),
            OPT_ARG(SERVER_TABLE_NAME), OPT_ARG(SERVER_TABLE_ID_FIELD));

    if (mysql_query(mysql, qry)) {
      LOG(LOG_ERR, "%s: %s", qry, mysql_error(mysql)); // if we can't read info from the database, use cached info
      close_database();
      break;
    }

    result = mysql_store_result(mysql);
    if (result) {
      struct hostinfo **newh = calloc(mysql_num_rows(result)+1, sizeof(struct hostinfo));
      while ((row = mysql_fetch_row(result))) {
        newh[id] = calloc(1, sizeof(struct hostinfo));
        newh[id]->id = atol(row[0]);
        newh[id]->name = strdup(row[1]);
        if (debug > 2) LOG(LOG_DEBUG, "read %s", newh[id]->name);
        newh[id]->hostname = strdup(row[2]);
        newh[id]->uri = strdup(row[3]);
        newh[id]->yellow  = atof(row[4]);
        newh[id]->red = atof(row[5]);
        newh[id]->info = NULL;
        newh[id]->info_curlen = 0;
        newh[id]->info_maxlen = 0;
        id++;
      }
      mysql_free_result(result);
      newh[id] = NULL;
      num_hosts = id;

      if (hosts != NULL) {
        for (id=0; hosts[id]; id++) {
          printf("%s\n", hosts[id]->name);
          free(hosts[id]->name);
          free(hosts[id]->hostname);
          free(hosts[id]->uri);
          free(hosts[id]);
          if (hosts[id]->info) free(hosts[id]->info);
        }
        free(hosts);
      }
      hosts = newh; // replace with new structure
    }
    close_database();
    if (debug > 0) LOG(LOG_DEBUG, "%d probes read", num_hosts);
    break;
  }
  if (hosts == NULL) {
    LOG(LOG_ERR, "no database, no cached info - bailing out");
    close_database();
    return 0;
  }

  if (debug > 0) LOG(LOG_DEBUG, "running probes");
  uw_setproctitle("running %d probes", num_hosts);
  run_actual_probes(num_hosts); /* this runs the actual probes */
  if (debug > 0) LOG(LOG_DEBUG, "done running probes");

  uw_setproctitle("writing resultfile");
  doc = UpwatchXmlDoc("result");
  red = UpwatchXmlDoc("result");
  now = time(NULL);

  for (id=0; hosts[id]; id++) {
    xmlDocPtr cur = doc;
    xmlNodePtr subtree, httpget, host;
    int color, investigate = 0;
    char info[1024];
    char buffer[1024];

    info[0] = 0;
    if (hosts[id]->msg[0]) {
      color = STAT_RED;
      cur = red;
      investigate = 1;
      redcount++;
    } else {
      if (hosts[id]->total < hosts[id]->yellow) {
        color = STAT_GREEN;
      } else if (hosts[id]->total > hosts[id]->red) {
        color = STAT_RED;
        cur = red;
        redcount++;
      } else {
        color = STAT_YELLOW;
      }
    }
    httpget = xmlNewChild(xmlDocGetRootElement(cur), NULL, "httpget", NULL);
    sprintf(buffer, "%d", hosts[id]->id);       xmlSetProp(httpget, "id", buffer);
    sprintf(buffer, "%d", (int) now);           xmlSetProp(httpget, "date", buffer);
    sprintf(buffer, "%d", ((int)now)+(2*60));   xmlSetProp(httpget, "expires", buffer);
    if (investigate) { 
      xmlSetProp(httpget, "investigate", "tcptraceroute");
      xmlSetProp(httpget, "port", "80");
    } 
    host = xmlNewChild(httpget, NULL, "host", NULL);
    sprintf(buffer, "%s", hosts[id]->name);     subtree = xmlNewChild(host, NULL, "hostname", buffer);
    //sprintf(buffer, "%s", inet_ntoa(hosts[id]->saddr.sin_addr));
    //  subtree = xmlNewChild(host, NULL, "ipaddress", buffer);
    sprintf(buffer, "%d", color);               subtree = xmlNewChild(httpget, NULL, "color", buffer);
    sprintf(buffer, "%f", hosts[id]->lookup); subtree = xmlNewChild(httpget, NULL, "lookup", buffer);
    sprintf(buffer, "%f", hosts[id]->connect);    subtree = xmlNewChild(httpget, NULL, "connect", buffer);
    sprintf(buffer, "%f", hosts[id]->pretransfer); subtree = xmlNewChild(httpget, NULL, "pretransfer", buffer);
    sprintf(buffer, "%f", hosts[id]->total); subtree = xmlNewChild(httpget, NULL, "total", buffer);
    if (hosts[id]->info) {
      // remove non-ASCII characters as the xml library chokes on some
      signed char *s;

      for (s = hosts[id]->info; *s; s++) {
        if (*s < 0) *s = ' ';
      }
      subtree = xmlNewTextChild(httpget, NULL, "info", hosts[id]->info);
      free(hosts[id]->info);
      hosts[id]->info = NULL;
      hosts[id]->info_curlen = 0;
      hosts[id]->info_maxlen = 0;
    } else {
      subtree = xmlNewChild(httpget, NULL, "info", hosts[id]->msg);
    }
  }
  spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(OUTPUT), doc, NULL);
  if (redcount) {
    spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(INVESTIGATE), red, NULL);
  }
  xmlFreeDoc(doc);
  xmlFreeDoc(red);
  return num_hosts;
}


int run_actual_probes(int count)
{
  int id;
  GThreadPool *gtpool;

  gtpool = g_thread_pool_new(probe, NULL, 10, TRUE, NULL);

  if (gtpool == NULL) {
    //printf("%s\n", gerror->message);
    //LOG(LOG_ERR, gerror->message);
    fprintf(stderr, "could not create threadpool\n");
    return(0);
  }

  for (id=0; hosts[id]; id++) {
    g_thread_pool_push(gtpool, hosts[id], NULL);
  }
  g_thread_pool_free (gtpool, FALSE, TRUE);
  return(0);
}

static size_t write_function(void *ptr, size_t size, size_t nmemb, void *stream)
{
  struct hostinfo *host = (struct hostinfo *)stream;
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
  struct hostinfo *host = (struct hostinfo *)data;
  CURL *curl;
  char buffer[BUFSIZ];
  double result;

  if (!host) return;
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
  host->ret = curl_easy_perform(curl);

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

