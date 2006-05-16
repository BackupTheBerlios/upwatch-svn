#include "config.h"

#include <generic.h>
#include "uw_sysstat.h"
#include <statgrab.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <findsaddr.h>
#if HAVE_LIBPCRE
#include <logregex.h>
#endif
#if USE_XMBMON
#include "mbmon.h"
#endif
#ifdef __OpenBSD__
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/sensors.h>
#endif

char ipaddress[24];

struct _errlogspec {
  char *style;
  char *path;
  long long offset;
} errlogspec[256];

typedef struct {
  sg_cpu_percents *cpu;
  sg_mem_stats *mem;
  sg_swap_stats *swap;
  sg_load_stats *load;
  sg_process_stats *process;
  int process_entries;
  sg_page_stats *paging;

  sg_network_io_stats *network;
  int network_entries;

  sg_disk_io_stats *diskio;
  int diskio_entries;

  sg_fs_stats *disk;
  int disk_entries;

  sg_host_info *hostinfo;
  sg_user_stats *user;
} stats_t;
stats_t st;

#if USE_XMBMON|| defined (__OpenBSD__)
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
#endif

int get_stats(void)
{
  st.cpu = sg_get_cpu_percents();
  if (!st.cpu) { LOG(LOG_INFO, "could not sg_get_cpu_stats"); }
  st.mem = sg_get_mem_stats();
  if (!st.mem) { LOG(LOG_INFO, "could not sg_get_mem_stats"); }
  st.swap = sg_get_swap_stats();
  if (!st.swap) { LOG(LOG_INFO, "could not get_swap_stats"); }
  st.load = sg_get_load_stats();
  if (!st.load) { LOG(LOG_INFO, "could not get_load_stats"); }
  st.process = sg_get_process_stats(&st.process_entries);
  if (!st.process) { LOG(LOG_INFO, "could not get_process_stats"); }
  st.paging = sg_get_page_stats_diff();
  if (!st.paging) { LOG(LOG_INFO, "could not get_page_stats_diff"); }
  st.network = sg_get_network_io_stats_diff(&(st.network_entries));
  if (!st.network) { LOG(LOG_INFO, "could not get_network_stats_diff"); }
  st.diskio = sg_get_disk_io_stats_diff(&(st.diskio_entries));
  if (!st.diskio) { LOG(LOG_INFO, "could not get_diskio_stats_diff"); }
  st.disk = sg_get_fs_stats(&(st.disk_entries));
  if (!st.disk) { LOG(LOG_INFO, "could not get_disk_stats"); }
  st.hostinfo = sg_get_host_info();
  if (!st.hostinfo) { LOG(LOG_INFO, "could not get_host_info"); }
  st.user = sg_get_user_stats();
  if (!st.user) { LOG(LOG_INFO, "could not get get_user_stats"); }

  return 1;
}

#if USE_XMBMON
void get_hwstats(void)
{
  getTemp(&hw.temp1, &hw.temp2, &hw.temp3);
  //printf("Temp.= %4.1f, %4.1f, %4.1f;",hw.temp1, hw.temp2, hw.temp3);
  getFanSp(&hw.rot1, &hw.rot2, &hw.rot3);
  //printf(" Rot.= %4d, %4d, %4d\n", hw.rot1, hw.rot2, hw.rot3);
  getVolt(&hw.vc0, &hw.vc1, &hw.v33, &hw.v50p, &hw.v50n, &hw.v12p, &hw.v12n);
  //printf(" Vcore = %4.2f, %4.2f; Volt. = %4.2f, %4.2f, %5.2f, %6.2f, %5.2f\n", 
  //       hw.vc0, hw.vc1, hw.v33, hw.v50p, hw.v12p, hw.v12n, hw.v50n);
}
#endif
#ifdef __OpenBSD__
void get_hwstats(void)
{
  int i;
  struct sensor s; 
  size_t slen = sizeof(s); 
  float value;
  for (i=0;i<256;i++)
  {
    int mib[] = { CTL_HW, HW_SENSORS, i }; 
    if ( sysctl(mib, sizeof(mib)/sizeof(mib[0]), &s, &slen, NULL, 0) == -1 ) continue;
    if (s.flags & SENSOR_FINVALID) continue;

    /* Ok, we have a valid sensor now, now check the type */
    switch (s.type) {
      case SENSOR_TEMP:
              if ( debug > 5 ) printf("Sensor %d is a temparature sensor\n", s.num);
              value = (s.value - 273150000) / 1000000.0;
              if(strcmp(s.desc,"Temp1")==0)  hw.temp1 = value; 
              if(strcmp(s.desc,"Temp2")==0)  hw.temp2 = value;
              if(strcmp(s.desc,"Temp3")==0)  hw.temp3 = value;
              break;
      case SENSOR_FANRPM:
              if ( debug > 4 ) printf("Sensor %d is a fan speed sensor\n", s.num);
              if(strcmp(s.desc,"Fan1")==0)  hw.rot1 = (int) s.value;
              if(strcmp(s.desc,"Fan2")==0)  hw.rot2 = (int) s.value;
              if(strcmp(s.desc,"Fan3")==0)  hw.rot3 = (int) s.value;
              break;
      case SENSOR_VOLTS_DC:
              if ( debug > 5 ) printf("Sensor %d is a voltage sensor\n", s.num);
              value = s.value / 1000000.0;
              if(strcmp(s.desc,"VCore A")==0)  hw.vc0 = value;
              if(strcmp(s.desc,"VCore B")==0)  hw.vc1 = value;
              if(strcmp(s.desc,"+3.3V")==0)  hw.v33 = value;
              if(strcmp(s.desc,"+5V")==0)  hw.v50p = value;
              if(strcmp(s.desc,"+12V")==0)  hw.v12p = value;
              if(strcmp(s.desc,"-12V")==0)  hw.v12n = value;
              if(strcmp(s.desc,"-5V")==0)  hw.v50n = value;
              break;
      default:
              if ( debug > 5 ) printf("Sensor hw.sensors.%d is of a unknown type! It describes itselve as: %s\n", s.num, s.desc);
              break;
    }
  }
  if ( debug > 4 ) printf("Temp = %4.1f, %4.1f, %4.1f;",hw.temp1, hw.temp2, hw.temp3);
  if ( debug > 4 ) printf("Rot = %4d, %4d, %4d\n", hw.rot1, hw.rot2, hw.rot3);
  if ( debug > 4 ) printf("Vcore = %4.2f, %4.2f\nVolt = %4.2f, %4.2f, %5.2f, %6.2f, %5.2f\n", 
         hw.vc0, hw.vc1, hw.v33, hw.v50p, hw.v12p, hw.v12n, hw.v50n);
}
#endif

#if HAVE_LIBPCRE
#define STATFILE "/var/run/upwatch/uw_sysstat.stat"
int check_log(GString *string, int idx)
{
  FILE *in;
  struct stat st;
  char buffer[8192];
  int firstmatch = TRUE;
  int logcolor = STAT_GREEN;
  int warnlines = 0;
  int readlines = 0;
  long long filesize = 0;
extern int forever;

  uw_setproctitle("scanning %s", errlogspec[idx].path);
  in = fopen(errlogspec[idx].path, "r");
  if (!in) {
    errlogspec[idx].offset = 0;
    return STAT_GREEN;
  }
  if (stat(errlogspec[idx].path, &st)) {
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
  filesize = st.st_size;
  fseek(in, errlogspec[idx].offset, SEEK_SET);
  while (fgets(buffer, sizeof(buffer), in)) {
    int color;

    if (!forever) break;
    if (++readlines % 5 == 0) {
      float perc;
      long long pos = ftell(in);

      perc = (float)(pos - errlogspec[idx].offset) / (float) (filesize - errlogspec[idx].offset);
      perc *= 100;
      uw_setproctitle("scanning %s, %lu of %lu (%3.f%%)", errlogspec[idx].path, (long) pos, (long) filesize, perc);
    }
    if (logregex_matchline(errlogspec[idx].style, buffer, &color)) {
      if (firstmatch) {
        char buf2[PATH_MAX+4];

        sprintf(buf2, "%s:\n", errlogspec[idx].path);
        g_string_append(string, buf2);
        firstmatch = FALSE;
      }
      if (++warnlines < 50) g_string_append(string, buffer);
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

  *color = STAT_GREEN;
  for (i=0; errlogspec[i].path; i++) {
    int logcolor;

    logregex_refresh_type("/etc/upwatch.d/uw_sysstat.d", errlogspec[i].style);
    logcolor = check_log(string, i);
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
#endif

int init(void)
{
  struct sockaddr_in from, to;
  const char *msg;
  int i, idx;
  FILE *in;

  if (!HAVE_OPT(OUTPUT)) {
    LOG(LOG_ERR, "missing output option");
    return 0;
  }
  if (OPT_VALUE_SERVERID == 0) {
    fprintf(stderr, "Please set serverid first\n");
    return 0;
  }

  if (HAVE_OPT(ERRLOG)) {
    int ct  = STACKCT_OPT(ERRLOG);
    char **errlog = (char **) &STACKLST_OPT(ERRLOG);
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
#if HAVE_LIBPCRE
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
#endif
  }

#if USE_XMBMON
  if (OPT_VALUE_HWSTATS) {
    if (OPT_VALUE_TYAN_TIGER_MP) {
      TyanTigerMP_flag = 1;
    }
    if ((i = InitMBInfo(' ')) != 0) {
      LOG(LOG_ERR, "InitMBInfo: %m");
      if (i < 0) {
        LOG(LOG_ERR,"This program needs setuid root");
      }
    }
  } else {
    LOG(LOG_INFO,"Hardware stats will not be generated");
  }
#endif

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

  setpriority(PRIO_PROCESS, 0, 5); // we are not very important - be nice to other processes
  return 1;
}

xmlNodePtr newnode(xmlDocPtr doc, char *name)
{
  char buffer[24];
  xmlNodePtr node;
  time_t now = time(NULL);

  node = xmlNewChild(xmlDocGetRootElement(doc), NULL, name, NULL);
  if (HAVE_OPT(REALM)) {
    xmlSetProp(node, "realm", OPT_ARG(REALM));
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

#if USE_XMBMON || defined (__OpenBSD__)
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
}
#endif

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
    sprintf(buffer, "%u", (int) (st.cpu->kernel /* + st.cpu->iowait + st.cpu->swap */));
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
    sg_disk_io_stats *diskio_stat_ptr = st.diskio;
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
          int c;
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
  int i, ignore;

  if (st.disk) {
    sg_fs_stats *disk_stat_ptr = st.disk;
    int counter;

    for (counter=0, ignore=0; counter < st.disk_entries; counter++){
      float use;

    if (HAVE_OPT( IGNOREDISKFREE )) {
        int     ct = STACKCT_OPT(  IGNOREDISKFREE );
        char**  pp = (char **) &STACKLST_OPT( IGNOREDISKFREE );

        do  {
            char* p = *pp++;
            if (strcmp(disk_stat_ptr->device_name, p) ==0 ) ignore=1;
        } while (--ct > 0);
    }

     if (ignore == 0 ) {
        use = 100.00 * ((float) disk_stat_ptr->used / (float) (disk_stat_ptr->used + disk_stat_ptr->avail));
        if (use > fullest) fullest = use;
      }
      ignore=0; /* Reset the ignore flag */
      disk_stat_ptr++; /* next partition please */
    }
  }

  if (fullest > OPT_VALUE_DISKFREE_YELLOW) { // if some disk is more then the yellow treshold full give `df` listing

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
        int c;
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
  char **output = (char **) &STACKLST_OPT(OUTPUT);
  GString *log;
  int i;
  int color;
  int highest_color = STAT_GREEN;
  char buf[24];
extern int forever;

  uw_setproctitle("sleeping");
  for (i=0; i < OPT_VALUE_INTERVAL; i++) { // wait some minutes
    sleep(1);
    if (!forever)  {
      return(0);
    }
  }

  uw_setproctitle("getting system statistics");
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
#if USE_XMBMON|| defined (__OpenBSD__)
  if (OPT_VALUE_HWSTATS ) { 
    // do the hwstat
    get_hwstats();
    node = newnode(doc, "hwstat");
    add_hwstat(node);
    color = xmlGetPropInt(node, "color");
    if (color > highest_color) highest_color = color;
  }
#endif

#if HAVE_LIBPCRE
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
#endif

  // do the diskfree
  uw_setproctitle("checking diskspace");
  node = newnode(doc, "diskfree");
  add_diskfree_info(node);
  color = xmlGetPropInt(node, "color");

  if (color > highest_color) highest_color = color;

  if (HAVE_OPT(HPQUEUE) && (highest_color != prv_highest_color)) {
    // if status changed, it needs to be sent immediately. So drop into
    // the high priority queue. Else just drop in the normal queue where
    // uw_send in batched mode will pick it up later
    spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(HPQUEUE), doc, NULL);
  } else {
    for (i=0; i < ct; i++) {
      spool_result(OPT_ARG(SPOOLDIR), output[i], doc, NULL);
    }
  }
  prv_highest_color = highest_color; // remember for next time
  xmlFreeDoc(doc);
  return 0;
}
