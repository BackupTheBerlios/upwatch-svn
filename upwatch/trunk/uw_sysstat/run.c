#include "config.h"

#include <generic.h>
#include "cmd_options.h"
#include <statgrab.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <findsaddr.h>

char ipaddress[24];

typedef struct{
  cpu_percent_t *cpu;
  mem_stat_t *mem;
  swap_stat_t *swap;
  load_stat_t *load;
  process_stat_t *process;
  page_stat_t *paging;

  network_stat_t *network;
  int network_entries;

   diskio_stat_t *diskio;
   int diskio_entries;

   disk_stat_t *disk;
   int disk_entries;

   general_stat_t *general;
   user_stat_t *user;
} stats_t;
stats_t st;

int get_stats(void)
{
  if((st.cpu = cpu_percent_usage()) == NULL) return 0;
  if((st.mem = get_memory_stats()) == NULL) return 0;
  if((st.swap = get_swap_stats()) == NULL) return 0;
  if((st.load = get_load_stats()) == NULL) return 0;
  if((st.process = get_process_stats()) == NULL) return 0;
  if((st.paging = get_page_stats_diff()) == NULL) return 0;
  if((st.network = get_network_stats_diff(&(st.network_entries))) == NULL) return 0;
  if((st.diskio = get_diskio_stats_diff(&(st.diskio_entries))) == NULL) return 0;
  if((st.disk = get_disk_stats(&(st.disk_entries))) == NULL) return 0;
  if((st.general = get_general_stats()) == NULL) return 0;
  if((st.user = get_user_stats()) == NULL) return 0;

  return 1;
}

int init(void)
{
  struct sockaddr_in from, to;
  const char *msg;

  if (OPT_VALUE_SERVERID == 0) {
    fprintf(stderr, "Please set serverid first\n");
    return 0;
  }
  if (!get_stats()) {
    fprintf(stderr, "Failed to get statistics. Please check permissions\n");
    return 0;
  }

  // determine ip address for default gateway interface
  setsin(&to, inet_addr("1.1.1.1"));
  msg = findsaddr(&to, &from);
  if (msg) {
    LOG(LOG_NOTICE, msg);
    strcpy(ipaddress, "127.0.0.1");
  } else {
    strcpy(ipaddress, inet_ntoa(from.sin_addr)); 
  }
  if (debug) LOG(LOG_DEBUG, "using ipaddress %s", ipaddress);

  daemonize = TRUE;
  every = EVERY_SECOND;
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  sleep(2); // ensure we wait until the next minute
  return 1;
}

int run(void)
{
  xmlDocPtr doc;
  xmlNodePtr subtree, sysstat;
  int ret = 0;
  int color = STAT_GREEN;
static int prv_color = STAT_GREEN;
  time_t now;
  char buffer[1024];
  char info[32768];
  int systemp = 0;
  long long rt=0, wt=0;
  int ct  = STACKCT_OPT(OUTPUT);
  char **output = STACKLST_OPT(OUTPUT);
  int i;
extern int forever;

  for (i=0; i < OPT_VALUE_INTERVAL; i++) { // wait some minutes
    sleep(1);
    if (!forever)  {
      return(0);
    }
  }

  info[0] = 0;

  // compute sysstat
  //
  get_stats();
  { 
    diskio_stat_t *diskio_stat_ptr = st.diskio;
    int counter;

    for (counter=0; counter < st.diskio_entries; counter++) {
      long long  r, w;
      r = diskio_stat_ptr->read_bytes;
      rt += r;
      w = diskio_stat_ptr->write_bytes;
      wt += w;
      diskio_stat_ptr++;
    }
  }
  if (st.load->min1 > 5.0) color = STAT_RED;

  {
    char cmd[1024];
    FILE *in;

    sprintf(cmd, "%s > /tmp/.uw_sysstat.tmp", OPT_ARG(TOP_COMMAND));
    if (debug > 2) LOG(LOG_NOTICE, cmd);
    uw_setproctitle("running %s", OPT_ARG(TOP_COMMAND));
    system(cmd);
    in = fopen("/tmp/.uw_sysstat.tmp", "r");
    if (in) {
      signed char *s;

      fread(info, sizeof(info)-1, 1, in); 
      info[sizeof(info)-1] = 0;
      fclose(in);

      for (s = info; *s; s++) { // clean up strange characters
        if (*s < 0) *s = ' ';
      }
    } else {
      strcpy(info, strerror(errno));
    }
    unlink("/tmp/.uw_sysstat.tmp");
  }

  if (HAVE_OPT(SYSTEMP_COMMAND)) {
    char cmd[1024];
    FILE *in;

    sprintf(cmd, "%s > /tmp/.uw_sysstat.tmp", OPT_ARG(SYSTEMP_COMMAND));
    uw_setproctitle("running %s", OPT_ARG(SYSTEMP_COMMAND));
    system(cmd);
    in = fopen("/tmp/.uw_sysstat.tmp", "r");
    if (in) {
      fscanf(in, "%d", &systemp);
      fclose(in);
    }
    unlink("tmp/.uw_sysstat.tmp");
  }
  doc = UpwatchXmlDoc("result");
  xmlSetDocCompressMode(doc, OPT_VALUE_COMPRESS);
  now = time(NULL);

  sysstat = xmlNewChild(xmlDocGetRootElement(doc), NULL, "sysstat", NULL);
  sprintf(buffer, "%ld", OPT_VALUE_SERVERID);	xmlSetProp(sysstat, "server", buffer);
  xmlSetProp(sysstat, "ipaddress", ipaddress);
  sprintf(buffer, "%d", (int) now);		xmlSetProp(sysstat, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+((unsigned)OPT_VALUE_EXPIRES*60));
    xmlSetProp(sysstat, "expires", buffer);
  sprintf(buffer, "%d", color);				subtree = xmlNewChild(sysstat, NULL, "color", buffer);

  sprintf(buffer, "%.1f", st.load->min1);	
    subtree = xmlNewChild(sysstat, NULL, "loadavg", buffer);

  sprintf(buffer, "%u", (int) (st.cpu->user + st.cpu->nice));
    subtree = xmlNewChild(sysstat, NULL, "user", buffer);
  sprintf(buffer, "%u", (int) (st.cpu->kernel + st.cpu->iowait + st.cpu->swap));
    subtree = xmlNewChild(sysstat, NULL, "system", buffer);
  sprintf(buffer, "%u", (int) (st.cpu->idle));
    subtree = xmlNewChild(sysstat, NULL, "idle", buffer);

  sprintf(buffer, "%llu", st.paging->pages_pagein);
    subtree = xmlNewChild(sysstat, NULL, "swapin", buffer);
  sprintf(buffer, "%llu", st.paging->pages_pageout);
    subtree = xmlNewChild(sysstat, NULL, "swapout", buffer);

  sprintf(buffer, "%llu", rt/1024);	subtree = xmlNewChild(sysstat, NULL, "blockin", buffer);
  sprintf(buffer, "%llu", wt/1024);	subtree = xmlNewChild(sysstat, NULL, "blockout", buffer);

  sprintf(buffer, "%llu", st.swap->used/1024);	subtree = xmlNewChild(sysstat, NULL, "swapped", buffer);
  sprintf(buffer, "%llu", st.mem->free/1024);	subtree = xmlNewChild(sysstat, NULL, "free", buffer);
  sprintf(buffer, "%u", 0);			subtree = xmlNewChild(sysstat, NULL, "buffered", buffer);
  sprintf(buffer, "%llu", st.mem->cache/1024);	subtree = xmlNewChild(sysstat, NULL, "cached", buffer);
  sprintf(buffer, "%llu", st.mem->used/1024);	subtree = xmlNewChild(sysstat, NULL, "used", buffer);
  sprintf(buffer, "%d", systemp);		subtree = xmlNewChild(sysstat, NULL, "systemp", buffer);
  subtree = xmlNewTextChild(sysstat, NULL, "info", info);

  if (HAVE_OPT(HPQUEUE)) {
    if (color != prv_color) {
      // if status changed, it needs to be sent immediately. So drop into
      // the high priority queue. Else just drop in the normal queue where
      // uw_send in batched mode will pick it up later
      spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(HPQUEUE), doc, NULL);
    }
  }
  for (i=0; i < ct; i++) {
    spool_result(OPT_ARG(SPOOLDIR), output[i], doc, NULL);
  }
  prv_color = color; // remember for next time
  xmlFreeDoc(doc);
  return(ret);
}
