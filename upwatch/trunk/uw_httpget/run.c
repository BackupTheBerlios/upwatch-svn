#include "config.h"
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <curl/curl.h>

#include <generic.h>
#include "cmd_options.h"

struct hostinfo {
  int                   id;             /* server probe id */
  char                  *name;          /* server name */
  struct sockaddr_in    saddr;          /* internet address */
  int                   yellowtime;     /* if lower then this value: green */
  int                   redtime;        /* if higher then this value: red */
  int                   ret;            /* return value */
  double                total_time;     /* total retrieval time (sec) */
  double                namelookup_time;/* time in sec for DNS lookup  */
  double                connect_time;   /* time for connect (sec) */
  double                pretransfer_time;/* time to first byte (sec) */
  char                  msg[CURL_ERROR_SIZE]; /* last error message */
} hostinfo;

static struct hostinfo **hosts;
static int num_hosts;

static void exit_probes(void)
{
  close_database();
}

void probe(gpointer data, gpointer user_data);
int init(void)
{
  daemonize = TRUE;
  every = EVERY_MINUTE;
  startsec = OPT_VALUE_BEGIN;
  g_thread_init(NULL);
  return(1);
}

int run(void)
{
  void *spool;
  int id;

  if (debug > 0) LOG(LOG_DEBUG, "reading info from database");
  if (open_database() == 0) {
    MYSQL_RES *result;
    MYSQL_ROW row;

    char *qry = // no real http probes yet
      "SELECT server.id, name, ipaddress.ip "
      "FROM   server, ipaddress "
      "WHERE  ipaddress.id = server.ipaddress and "
      "       server.status = 'enabled' "
      "ORDER  BY name";

    if (mysql_query(mysql, qry)) {
      LOG(LOG_ERR, "%s: %s", qry, mysql_error(mysql));
      close_database();
      return(1);
    }

    result = mysql_store_result(mysql);
    if (result) {
      struct hostinfo **newh = calloc(mysql_num_rows(result)+1, sizeof(struct hostinfo));
      while ((row = mysql_fetch_row(result))) {
        u_int ip;
        struct in_addr *ipa = (struct in_addr *)&ip;
        time_t now = time(NULL);
        int probeid = atol(row[0]);
        char *name = row[1];
        char *ipaddress = row[2];

        if ((ip = inet_addr(ipaddress)) == -1) {
          LOG(LOG_NOTICE, "illegal ip address for %s: %s", name, ipaddress);
          continue;
        }

        //if (!strstr(name, "amstel02.netland.nl")) continue;
        newh[id] = calloc(1, sizeof(struct hostinfo));
        newh[id]->id = probeid;
        newh[id]->name = strdup(name);
        newh[id]->saddr.sin_family = AF_INET;
        newh[id]->saddr.sin_addr = *ipa;;
        id++;
      }
      mysql_free_result(result);
      newh[id] = NULL;
      num_hosts = id;

      if (hosts != NULL) {
        for (id=0; hosts[id]; id++) {
          printf("%s\n", hosts[id]->name);
          free(hosts[id]->name);
          free(hosts[id]);
        }
        free(hosts);
      }
      hosts = newh; // replace with new structure
    } else if (hosts == NULL) {
      LOG(LOG_ERR, mysql_error(mysql));
      LOG(LOG_ERR, "no database, no cached info - bailing out");
      close_database();
      return(1);
    }
    close_database();
    if (debug > 0) LOG(LOG_DEBUG, "done reading");
  } else if (hosts == NULL) {
    LOG(LOG_ERR, "no database, no cached info - bailing out");
    return(1);
  }

  if (debug > 0) LOG(LOG_DEBUG, "running probes", "new");
  run_actual_probes(num_hosts); /* this runs the actual probes */
  if (debug > 0) LOG(LOG_DEBUG, "done running probes");

  spool = spool_open(OPT_ARG(SPOOLDIR), OPT_ARG(OUTPUT));
  if (!spool) {
    LOG(LOG_ERR, "can't open %s/%s", OPT_ARG(SPOOLDIR), OPT_ARG(OUTPUT));
  } else {
    time_t now = time(NULL);
  /*
   * output format:
   *  <method><lines><probeid><password><date><expires><ipaddress><color>
   *  <minpingtime><avgpingtime><maxpingtime><hostname>
   * <result line 1>
   * <result line 2>
   * <result line 3>
   */
    for (id=0; hosts[id]; id++) {
      int color;
      int lines;
      char buffer[BUFSIZ];

      if (hosts[id]->msg[0]) {
        strcpy(buffer, "\n");
        strcat(buffer, hosts[id]->msg);
        lines = 2;
        color = STAT_RED;
      } else {
        buffer[0] = 0;
        lines = 1;
        if (hosts[id]->total_time < hosts[id]->yellowtime) {
          color = STAT_GREEN;
        } else if (hosts[id]->total_time > hosts[id]->redtime) {
          color = STAT_RED;
        } else {
          color = STAT_YELLOW;
        }
      }
      if (!spool_printf(spool, "%s %d %d %s %s %d %d %s %d %.3f %.3f %.3f %.3f %s%s\n", 
          "httpget", lines,  hosts[id]->id, OPT_ARG(UWUSER), OPT_ARG(UWPASSWD), (int) now,
          ((int)now)+(2*60), inet_ntoa(hosts[id]->saddr.sin_addr), color, 
          hosts[id]->namelookup_time, hosts[id]->connect_time, 
          hosts[id]->pretransfer_time, hosts[id]->total_time,
          hosts[id]->name, buffer)) {
        LOG(LOG_ERR, "can't write to spoolfile");
        break;
      }
    }
    if (!spool_close(spool, TRUE)) {
      LOG(LOG_ERR, "couldn't close spoolfile");
    }
  }
  return(0);
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
  return(size * nmemb);
}

void probe(gpointer data, gpointer user_data)
{
  struct hostinfo *host = (struct hostinfo *)data;
  CURL *curl;
  char buffer[BUFSIZ];

  sprintf(buffer, "http://%s/", host->name);
  curl = curl_easy_init();

  curl_easy_setopt(curl, CURLOPT_FILE, host);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
  curl_easy_setopt(curl, CURLOPT_URL, buffer);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, host->msg);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, TRUE);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 50L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
  host->ret = curl_easy_perform(curl);

  // Pass a pointer to a double to receive the total transaction time in seconds for the
  // previous transfer.
  curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &host->total_time);
  host->total_time *= 1000.0; // convert to millesecs

  // Pass a pointer to a double to receive the time, in seconds, it took from the start
  //until the name resolving was completed.
  curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME, &host->namelookup_time);
  host->namelookup_time *= 1000.0; // convert to millesecs

  // Pass a pointer to a double to receive the time, in seconds, it took from the start
  // until the connect to the remote host (or proxy) was completed.
  curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &host->connect_time);
  host->connect_time *= 1000.0; // convert to millesecs

  // Pass a pointer to a double to receive the time, in seconds, it  took from the start
  // until the file transfer is just about to begin. This includes all pre-transfer com­
  // mands and negotiations that are specific to the particular protocol(s) involved.
  curl_easy_getinfo(curl, CURLINFO_PRETRANSFER_TIME, &host->pretransfer_time);
  host->pretransfer_time *= 1000.0; // convert to millesecs

  curl_easy_cleanup(curl);
}

