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
#include <logregex.h>
#include "mbmon.h"

char ipaddress[24];

struct _errlogspec {
  char *style;
  char *path;
  long long offset;
} errlogspec[256];

typedef struct {
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

typedef struct {
  float temp1;
  float temp2;
  float temp3;
  float vc0;
  float vc1;
  float v33;
  float v50p;
  float v12p;
  float v12n;
  float v50n;
  int rot1;
  int rot2;
  int rot3;
} hwstats_t;
hwstats_t hw;

int get_stats(void)
{
  st.cpu = cpu_percent_usage();
  if (!st.cpu) { LOG(LOG_INFO, "could not get cpu_percent_usage"); }
  st.mem = get_memory_stats();
  if (!st.cpu) { LOG(LOG_INFO, "could not get_memory_stats"); }
  st.swap = get_swap_stats();
  if (!st.cpu) { LOG(LOG_INFO, "could not get_swap_stats"); }
  st.load = get_load_stats();
  if (!st.cpu) { LOG(LOG_INFO, "could not get_load_stats"); }
  st.process = get_process_stats();
  if (!st.cpu) { LOG(LOG_INFO, "could not get_process_stats"); }
  st.paging = get_page_stats_diff();
  if (!st.cpu) { LOG(LOG_INFO, "could not get_page_stats_diff"); }
  st.network = get_network_stats_diff(&(st.network_entries));
  if (!st.cpu) { LOG(LOG_INFO, "could not get_network_stats_diff"); }
  st.diskio = get_diskio_stats_diff(&(st.diskio_entries));
  if (!st.cpu) { LOG(LOG_INFO, "could not get_diskio_stats_diff"); }
  st.disk = get_disk_stats(&(st.disk_entries));
  if (!st.cpu) { LOG(LOG_INFO, "could not get_disk_stats"); }
  st.general = get_general_stats();
  if (!st.cpu) { LOG(LOG_INFO, "could not get_general_stats"); }
  st.user = get_user_stats();
  if (!st.cpu) { LOG(LOG_INFO, "could not get_user_stats"); }

  return 1;
}

int get_hwstats(void)
{
  getTemp(&hw.temp1, &hw.temp2, &hw.temp3);
  //printf("Temp.= %4.1f, %4.1f, %4.1f;",hw.temp1, hw.temp2, hw.temp3);
  getFanSp(&hw.rot1, &hw.rot2, &hw.rot3);
  //printf(" Rot.= %4d, %4d, %4d\n", hw.rot1, hw.rot2, hw.rot3);
  getVolt(&hw.vc0, &hw.vc1, &hw.v33, &hw.v50p, &hw.v50n, &hw.v12p, &hw.v12n);
  //printf(" Vcore = %4.2f, %4.2f; Volt. = %4.2f, %4.2f, %5.2f, %6.2f, %5.2f\n", 
  //       hw.vc0, hw.vc1, hw.v33, hw.v50p, hw.v12p, hw.v12n, hw.v50n);
}

#define STATFILE "/var/run/upwatch/uw_sysstat.stat"
int check_log(GString *string, int idx, int *color)
{
  FILE *in;
  struct stat st;
  char buffer[8192];
  int firstmatch = TRUE;
  int logcolor = STAT_GREEN;

  in = fopen(errlogspec[idx].path, "r");
  if (!in) {
    errlogspec[idx].offset = 0;
    return STAT_GREEN;
  }
  if (fstat(fileno(in), &st)) {
    char buf2[PATH_MAX+4];

    sprintf(buf2, "%s: %m\n", errlogspec[idx].path);
    g_string_append(string, buf2);
    LOG(LOG_WARNING, buf2);
    fclose(in);
    errlogspec[idx].offset = 0;
    return STAT_YELLOW;
  }
  if (st.st_size < errlogspec[idx].offset) {
    errlogspec[idx].offset = 0;
  }
  fseek(in, errlogspec[idx].offset, SEEK_SET);
  while (fgets(buffer, sizeof(buffer), in)) {
    int color;

    if (logregex_matchline(errlogspec[idx].style, buffer, &color)) {
      if (firstmatch) {
        char buf2[PATH_MAX+4];

        sprintf(buf2, "%s:\n", errlogspec[idx].path);
        g_string_append(string, buf2);
        firstmatch = FALSE;
      }
      g_string_append(string, buffer);
    }
    if (color > logcolor) logcolor = color;
  }
  errlogspec[idx].offset = st.st_size;
  fclose(in);
  return logcolor;
}

//
// check the system log for funny things. Set appropriate color
GString *check_logs(int *color)
{
  GString* string;
  FILE *out;
  int i;

  string = g_string_new("");
  logregex_refresh("/etc/upwatch.d/uw_sysstat.d");

  for (i=0; errlogspec[i].path; i++) {
    int logcolor = check_log(string, i, color);
    if (logcolor > *color) *color = logcolor;
  }
  out = fopen(STATFILE, "w");
  if (out) {
    for (i=0; errlogspec[i].path; i++) {
      fprintf(out, "%s %Ld\n", errlogspec[i].path, errlogspec[i].offset);
    }
    fclose(out);
  }
  return(string);
}

int init(void)
{
  struct sockaddr_in from, to;
  const char *msg;
  int i, idx;
  FILE *in;

  if (OPT_VALUE_SERVERID == 0) {
    fprintf(stderr, "Please set serverid first\n");
    return 0;
  }

  if (HAVE_OPT(ERRLOG)) {
    int ct  = STACKCT_OPT(ERRLOG);
    char **errlog = STACKLST_OPT(ERRLOG);
    for (idx=0, i=0; i < ct && idx < 255; i++) {
      char *start, *end;
  
      start = end = errlog[i];
      while (*end && !isspace(*end)) {
        end++;
      }
      if (!*end) {
        LOG(LOG_WARNING, "Illegal format: %s", errlog[i]);
        continue;
      }
      *end++ = 0;
      errlogspec[idx].style = start;

      while (*end && isspace(*end)) {
        end++;
      }
      if (!*end) {
        LOG(LOG_WARNING, "Illegal format: %s", errlog[i]);
        continue;
      }
      start = end;
      while (*end && !isspace(*end)) {
        end++;
      }
      *end = 0;
      errlogspec[idx++].path = start;
    }
    in = fopen(STATFILE, "r");
    if (in) {
      while (!feof(in)) {
        char path[PATH_MAX];
        long long offset;
        int i;

        if (fscanf(in, "%s %Ld", path, &offset) != 2) continue;
        for (i=0; errlogspec[i].path; i++) {
          if (strcmp(errlogspec[i].path, path)) continue;
          errlogspec[i].offset = offset;
          break;
        }
      }
      fclose(in);
    }
  }

  if (OPT_VALUE_HWSTATS) {
    if ((i = InitMBInfo(' ')) != 0) {
      LOG(LOG_ERR, "InitMBInfo: %m");
      if (i < 0) {
        LOG(LOG_ERR,"This program needs setuid root");
      }
    }
  } else {
    LOG(LOG_INFO,"Hardware stats will not be generated");
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

xmlNodePtr newnode(xmlDocPtr doc, char *name)
{
  char buffer[24];
  xmlNodePtr node;
  time_t now = time(NULL);

  node = xmlNewChild(xmlDocGetRootElement(doc), NULL, name, NULL);
  if (HAVE_OPT(DOMAIN)) {
    xmlSetProp(node, "domain", OPT_ARG(DOMAIN));
  }
  sprintf(buffer, "%ld", OPT_VALUE_SERVERID);	xmlSetProp(node, "server", buffer);
  xmlSetProp(node, "ipaddress", ipaddress);
  sprintf(buffer, "%d", (int) now);		xmlSetProp(node, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+((unsigned)OPT_VALUE_EXPIRES*60));
    xmlSetProp(node, "expires", buffer);
  xmlSetProp(node, "color", "200");
  sprintf(buffer, "%ld", OPT_VALUE_INTERVAL);	xmlSetProp(node, "interval", buffer);
  return node;
}

void add_hwstat(xmlNodePtr node)
{
  char buffer[24];

  sprintf(buffer, "%.1f", hw.temp1);    xmlNewChild(node, NULL, "temp1", buffer);
  sprintf(buffer, "%.1f", hw.temp2);    xmlNewChild(node, NULL, "temp2", buffer);
  sprintf(buffer, "%.1f", hw.temp3);    xmlNewChild(node, NULL, "temp3", buffer);

  sprintf(buffer, "%d", hw.rot1);       xmlNewChild(node, NULL, "rot1", buffer);
  sprintf(buffer, "%d", hw.rot2);       xmlNewChild(node, NULL, "rot2", buffer);
  sprintf(buffer, "%d", hw.rot3);       xmlNewChild(node, NULL, "rot3", buffer);

  sprintf(buffer, "%.2f", hw.vc0);       xmlNewChild(node, NULL, "vc0", buffer);
  sprintf(buffer, "%.2f", hw.vc1);       xmlNewChild(node, NULL, "vc1", buffer);
  sprintf(buffer, "%.2f", hw.v33);       xmlNewChild(node, NULL, "v33", buffer);
  sprintf(buffer, "%.2f", hw.v50p);      xmlNewChild(node, NULL, "v50p", buffer);
  sprintf(buffer, "%.2f", hw.v50n);      xmlNewChild(node, NULL, "v50n", buffer);
  sprintf(buffer, "%.2f", hw.v12p);      xmlNewChild(node, NULL, "v12p", buffer);
  sprintf(buffer, "%.2f", hw.v12n);      xmlNewChild(node, NULL, "v12n", buffer);

  if (hw.temp1 > 60.0 || hw.temp2 > 60.0 || hw.rot1 < 100 || hw.rot2 < 100) {
    xmlSetProp(node, "color", "500");
  }
}

void add_loadavg(xmlNodePtr node)
{
  char buffer[24];

  if (st.load) {
    sprintf(buffer, "%.1f", st.load->min1);	
    xmlNewChild(node, NULL, "loadavg", buffer);
  }
}

void add_cpu(xmlNodePtr node)
{
  char buffer[24];

  if (st.cpu) {
    sprintf(buffer, "%u", (int) (st.cpu->user + st.cpu->nice));
    xmlNewChild(node, NULL, "user", buffer);
    sprintf(buffer, "%u", (int) (st.cpu->kernel + st.cpu->iowait + st.cpu->swap));
    xmlNewChild(node, NULL, "system", buffer);
    sprintf(buffer, "%u", (int) (st.cpu->idle));
    xmlNewChild(node, NULL, "idle", buffer);
  }
}

void add_paging(xmlNodePtr node)
{
  char buffer[24];

  if (st.paging) {
    sprintf(buffer, "%llu", st.paging->pages_pagein);
    xmlNewChild(node, NULL, "swapin", buffer);
    sprintf(buffer, "%llu", st.paging->pages_pageout);
    xmlNewChild(node, NULL, "swapout", buffer);
  }
}

void add_blockio(xmlNodePtr node)
{
  char buffer[24];
  long long rt=0, wt=0;

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

  sprintf(buffer, "%llu", rt/1024);
  xmlNewChild(node, NULL, "blockin", buffer);
  sprintf(buffer, "%llu", wt/1024);
  xmlNewChild(node, NULL, "blockout", buffer);
}

void add_swap(xmlNodePtr node)
{
  char buffer[24];

  if (st.swap) {
    sprintf(buffer, "%llu", st.swap->used/1024);
    xmlNewChild(node, NULL, "swapped", buffer);
  }
}

void add_memory(xmlNodePtr node)
{
  char buffer[24];

  if (st.mem) {
    sprintf(buffer, "%llu", st.mem->free/1024);
    xmlNewChild(node, NULL, "free", buffer);
    sprintf(buffer, "%u", 0);
    xmlNewChild(node, NULL, "buffered", buffer);
    sprintf(buffer, "%llu", st.mem->cache/1024);
    xmlNewChild(node, NULL, "cached", buffer);
    sprintf(buffer, "%llu", st.mem->used/1024);
    xmlNewChild(node, NULL, "used", buffer);
  }
}

void add_systemp(xmlNodePtr node)
{
  char buffer[24];
  int systemp = 0;

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
  sprintf(buffer, "%d", systemp);
  xmlNewChild(node, NULL, "systemp", buffer);
}

void add_sysstat_info(xmlNodePtr node)
{
  char info[32768];

  if (st.load) {
    if (st.load->min1 >= atof(OPT_ARG(LOADAVG_YELLOW))) { 
      char cmd[1024];
      char buffer[24];
      FILE *in;

      if (st.load->min1 >= atof(OPT_ARG(LOADAVG_YELLOW))) sprintf(buffer, "%d", STAT_YELLOW);
      if (st.load->min1 >= atof(OPT_ARG(LOADAVG_RED))) sprintf(buffer, "%d", STAT_RED);
      xmlSetProp(node, "color", buffer);

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
      xmlNewTextChild(node, NULL, "info", info);
    }
  }
}

void add_diskfree_info(xmlNodePtr node)
{
  float fullest = 0.0;
  char info[32768];

  if (st.disk) {
    disk_stat_t *disk_stat_ptr = st.disk;
    int counter;

    for (counter=0; counter < st.disk_entries; counter++){
      float use;
      use = 100.00 * ((float) disk_stat_ptr->used / (float) (disk_stat_ptr->used + disk_stat_ptr->avail));
      if (use > fullest) fullest = use;
      disk_stat_ptr++;
    }
  }

  if (fullest > OPT_VALUE_DISKFREE_YELLOW) { // if some disk is more then 80% full give `df` listing
    char cmd[1024];
    char buffer[24];
    FILE *in;

    if (fullest > OPT_VALUE_DISKFREE_YELLOW) sprintf(buffer, "%d", STAT_YELLOW);
    if (fullest > OPT_VALUE_DISKFREE_RED) sprintf(buffer, "%d", STAT_RED);
    xmlSetProp(node, "color", buffer);

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
    xmlNewTextChild(node, NULL, "info", info);
  }

}

static int prv_highest_color = STAT_GREEN;

int run(void)
{
  xmlDocPtr doc;
  xmlNodePtr node;
  int ct  = STACKCT_OPT(OUTPUT);
  char **output = STACKLST_OPT(OUTPUT);
  GString *log;
  int i;
  int color;
  int highest_color = STAT_GREEN;
  char buf[24];
extern int forever;

  for (i=0; i < OPT_VALUE_INTERVAL; i++) { // wait some minutes
    sleep(1);
    if (!forever)  {
      return(0);
    }
  }

  get_stats();
  doc = UpwatchXmlDoc("result", NULL);
  xmlSetDocCompressMode(doc, OPT_VALUE_COMPRESS);

  // do the sysstat
  node = newnode(doc, "sysstat");
  add_loadavg(node);
  add_cpu(node);
  add_paging(node);
  add_blockio(node);
  add_swap(node);
  add_memory(node);
  add_systemp(node);
  add_sysstat_info(node);
  color = xmlGetPropInt(node, "color");
  if (color > highest_color) highest_color = color;

  if (OPT_VALUE_HWSTATS) {
    // do the hwstat
    get_hwstats();
    node = newnode(doc, "hwstat");
    add_hwstat(node);
    color = xmlGetPropInt(node, "color");
    if (color > highest_color) highest_color = color;
  }

  // do the errlog
  node = newnode(doc, "errlog");
  log = check_logs(&color);
  if (color > highest_color) highest_color = color;
  sprintf(buf, "%u", color);
  xmlSetProp(node, "color", buf);
  if (log) {
    if (log->str && strlen(log->str) > 0) {
      xmlNewTextChild(node, NULL, "info", log->str);
    }
    g_string_free(log, TRUE);
  }

  // do the diskfree
  node = newnode(doc, "diskfree");
  add_diskfree_info(node);
  color = xmlGetPropInt(node, "color");
  if (color > highest_color) highest_color = color;

  if (HAVE_OPT(HPQUEUE)) {
    if (highest_color != prv_highest_color) {
      // if status changed, it needs to be sent immediately. So drop into
      // the high priority queue. Else just drop in the normal queue where
      // uw_send in batched mode will pick it up later
      spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(HPQUEUE), doc, NULL);
    }
  }
  for (i=0; i < ct; i++) {
    spool_result(OPT_ARG(SPOOLDIR), output[i], doc, NULL);
  }
  prv_highest_color = highest_color; // remember for next time
  xmlFreeDoc(doc);
  return 0;
}
