#include "config.h"

#include <generic.h>
#include "uw_sysstat.h"
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
#include <regex.h>

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

struct regexspec {
  char *regex;  // regular expression
  regex_t preg; // compiled version
};
GSList *syslog_ignore;
GSList *syslog_red;

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

#define STATFILE "/var/run/upwatch/uw_sysstat.stat"
//
// check the system log for funny things. Set appropriate color
GString *check_log(char *syslogfile, int *color, int testmode)
{
  long offset = 0;
  long size = 0;
  FILE *in;
  char buffer[32768];
  GString* string;

  string = g_string_new("");

  // find previous last position
  if ((in = fopen(STATFILE, "r")) != NULL) {
    fscanf(in, "%lu", &offset);
    fclose(in);
  }
  
  // open logfile
  if ((in = fopen(syslogfile, "r")) == NULL) {
    LOG(LOG_ERR, "%s: %m", syslogfile);
    return(NULL);
  }
  fseek(in, 0, SEEK_END);
  size = ftell(in);
  if (offset > size) {
    //truncated? restart
    offset = 0;
  }
  if (testmode) offset = 0;
  fseek(in, offset, SEEK_SET);
  while (fgets(buffer, sizeof(buffer), in)) {
    int add = FALSE;
    int match = FALSE;

    // first check the red conditions
    GSList* list = g_slist_nth(syslog_red, 0);

    while (list) {
      struct regexspec *spec = list->data;
      if (spec && regexec(&spec->preg,  buffer, 0, 0, 0) == 0) {
        // match a red condition. Set color to red and add this logline
        if (*color < STAT_RED) *color = STAT_RED;
        add = TRUE;
        if (testmode) {
          printf("RED match: %s %s", spec->regex, buffer);
        }
        break;
      }
      list = g_slist_next(list);
    }

    list = g_slist_nth(syslog_ignore, 0);
    while (list && !add) {
      struct regexspec *spec = list->data;
      if (spec && regexec(&spec->preg,  buffer, 0, 0, 0) == 0) {
        match = TRUE; // for the ignore list. At least one needs to match
        break;
      }
      list = g_slist_next(list);
    }
    if (!match && *color < STAT_YELLOW) {
      *color = STAT_YELLOW;
      if (testmode) {
        printf("match YELLOW: %s", buffer);
      }
      add = TRUE;
    } 
    if (add) {
      string = g_string_append(string, buffer);
    }
  }
  offset = ftell(in);
  fclose(in);
  if (!testmode) {
    if ((in = fopen(STATFILE, "w")) != NULL) {
      fprintf(in, "%lu", offset);
      fclose(in);
    } else {
      LOG(LOG_WARNING, "%s: %m", STATFILE);
    }
  }
  return(string);
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

  // compile & store the regular expressions for the loglines to be ignored
  if (HAVE_OPT(SYSLOG_IGNORE)) {
    struct regexspec *spec = NULL;
    int     ct  = STACKCT_OPT( SYSLOG_IGNORE );
    char**  pn = STACKLST_OPT( SYSLOG_IGNORE );

    while (ct--) {
      int err;

      if (!spec) {
        spec = malloc(sizeof(struct regexspec));
      }
      err = regcomp(&spec->preg, pn[ct], REG_EXTENDED|REG_NOSUB);
      if (err) {
        char buffer[256];

        regerror(err, &spec->preg, buffer, sizeof(buffer));
        LOG(LOG_ERR, buffer);
        continue;
      }
      spec->regex = pn[ct];
      syslog_ignore = g_slist_append(syslog_ignore, spec);
      spec = NULL;
    }
  }

  // compile & store the regular expressions for the loglines that are code red
  if (HAVE_OPT(SYSLOG_RED)) {
    struct regexspec *spec = NULL;
    int     ct  = STACKCT_OPT( SYSLOG_RED );
    char**  pn = STACKLST_OPT( SYSLOG_RED );

    while (ct--) {
      int err;

      if (!spec) {
        spec = malloc(sizeof(struct regexspec));
      }
      err = regcomp(&spec->preg, pn[ct], REG_EXTENDED|REG_NOSUB);
      if (err) {
        char buffer[256];

        regerror(err, &spec->preg, buffer, sizeof(buffer));
        LOG(LOG_ERR, buffer);
        continue;
      }
      spec->regex = pn[ct];
      syslog_red = g_slist_append(syslog_red, spec);
      spec = NULL;
    }
  }

  if (OPT_VALUE_SYSLOG_TEST) {
    int logcolor = STAT_GREEN;
    check_log(OPT_ARG(SYSLOG_FILE), &logcolor, TRUE);
    printf("color = %d\n", logcolor);
    exit(0);
  }

  // determine ip address for default gateway interface
  setsin(&to, inet_addr("1.1.1.1"));
  msg = findsaddr(&to, &from);
  if (msg) {
    LOG(LOG_INFO, (char *)msg);
    strcpy(ipaddress, OPT_ARG(IPADDRESS));
  } else {
    strcpy(ipaddress, inet_ntoa(from.sin_addr)); 
  }
  LOG(LOG_INFO, "using ipaddress %s", ipaddress);

  daemonize = TRUE;
  every = EVERY_SECOND;
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  sleep(2); // ensure we wait until the next minute
  return 1;
}

static int prv_color = STAT_GREEN;

int run(void)
{
  xmlDocPtr doc;
  xmlNodePtr subtree, sysstat, errlog, diskfree;
  int ret = 0;
  int color = STAT_GREEN;
  time_t now;
  char buffer[1024];
  float fullest = 0.0;
  char info[32768];
  int systemp = 0;
  long long rt=0, wt=0;
  int ct  = STACKCT_OPT(OUTPUT);
  char **output = STACKLST_OPT(OUTPUT);
  GString *log;
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
  if (st.diskio != NULL) { 
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

  if (st.load->min1 >= atof(OPT_ARG(LOADAVG_YELLOW))) { 
    char cmd[1024];
    FILE *in;

    if (st.load->min1 >= atof(OPT_ARG(LOADAVG_YELLOW))) color = STAT_YELLOW;
    if (st.load->min1 >= atof(OPT_ARG(LOADAVG_RED))) color = STAT_RED;

    sprintf(cmd, "%s > /tmp/.uw_sysstat.tmp", OPT_ARG(TOP_COMMAND));
    LOG(LOG_INFO, cmd);
    uw_setproctitle("running %s", OPT_ARG(TOP_COMMAND));
    system(cmd);
    in = fopen("/tmp/.uw_sysstat.tmp", "r");
    if (in) {
      signed char *s = info;
      size_t len;

      for (len=0; len < sizeof(info)-1; len++) {
        char c;
        if ((c = fgetc(in)) != EOF) {
          if (c < 0) c = ' ';
          *s++ = c;
        }
      }
      *s = 0;
      fclose(in);
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
  if (HAVE_OPT(DOMAIN)) {
    xmlSetProp(sysstat, "domain", OPT_ARG(DOMAIN));
  }
  sprintf(buffer, "%ld", OPT_VALUE_SERVERID);	xmlSetProp(sysstat, "server", buffer);
  xmlSetProp(sysstat, "ipaddress", ipaddress);
  sprintf(buffer, "%d", (int) now);		xmlSetProp(sysstat, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+((unsigned)OPT_VALUE_EXPIRES*60));
    xmlSetProp(sysstat, "expires", buffer);
  sprintf(buffer, "%d", color);  		xmlSetProp(sysstat, "color", buffer);

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

  color = STAT_GREEN;
  log = check_log(OPT_ARG(SYSLOG_FILE), &color, FALSE);
  errlog = xmlNewChild(xmlDocGetRootElement(doc), NULL, "errlog", NULL);
  if (HAVE_OPT(DOMAIN)) {
    xmlSetProp(sysstat, "domain", OPT_ARG(DOMAIN));
  }
  sprintf(buffer, "%ld", OPT_VALUE_SERVERID);	xmlSetProp(errlog, "server", buffer);
  xmlSetProp(errlog, "ipaddress", ipaddress);
  sprintf(buffer, "%d", (int) now);		xmlSetProp(errlog, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+((unsigned)OPT_VALUE_EXPIRES*60));
    xmlSetProp(errlog, "expires", buffer);
  sprintf(buffer, "%d", color);			subtree = xmlNewChild(errlog, NULL, "color", buffer);
  if (log && log->str && strlen(log->str) > 0) {
    subtree = xmlNewTextChild(errlog, NULL, "info", log->str);
  }

  // see which filesystems are full
  color = STAT_GREEN;
  if (st.disk != NULL) {
    disk_stat_t *disk_stat_ptr = st.disk;
    int counter;

    for (counter=0; counter < st.disk_entries; counter++){
      float use;
      use = 100.00 * ((float) disk_stat_ptr->used / (float) (disk_stat_ptr->used + disk_stat_ptr->avail));
      if (use > fullest) fullest = use;
      disk_stat_ptr++;
    }
  }

  info[0] = 0;
  if (fullest > OPT_VALUE_DISKFREE_YELLOW) { // if some disk is more then 80% full give `df` listing
    char cmd[1024];
    FILE *in;

    if (fullest > OPT_VALUE_DISKFREE_YELLOW) color = STAT_YELLOW;
    if (fullest > OPT_VALUE_DISKFREE_RED) color = STAT_RED;

    sprintf(cmd, "%s > /tmp/.uw_sysstat.tmp", OPT_ARG(DF_COMMAND));
    LOG(LOG_INFO, cmd);
    uw_setproctitle("running %s", OPT_ARG(DF_COMMAND));
    system(cmd);
    in = fopen("/tmp/.uw_sysstat.tmp", "r");
    if (in) {
      signed char *s = info;
      size_t len;

      for (len=0; len < sizeof(info)-1; len++) {
        char c;
        if ((c = fgetc(in)) != EOF) {
          if (c < 0) c = ' ';
          *s++ = c;
        }
      }
      *s = 0;
      fclose(in);
    } else {
      strcpy(info, strerror(errno));
    }
    unlink("/tmp/.uw_sysstat.tmp");
  }

  diskfree = xmlNewChild(xmlDocGetRootElement(doc), NULL, "diskfree", NULL);
  if (HAVE_OPT(DOMAIN)) {
    xmlSetProp(sysstat, "domain", OPT_ARG(DOMAIN));
  }
  sprintf(buffer, "%ld", OPT_VALUE_SERVERID);	xmlSetProp(diskfree, "server", buffer);
  xmlSetProp(diskfree, "ipaddress", ipaddress);
  sprintf(buffer, "%d", (int) now);		xmlSetProp(diskfree, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+((unsigned)OPT_VALUE_EXPIRES*60));
    xmlSetProp(diskfree, "expires", buffer);
  sprintf(buffer, "%d", color);			subtree = xmlNewChild(diskfree, NULL, "color", buffer);
  subtree = xmlNewTextChild(diskfree, NULL, "info", info);

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
