#include "config.h"

#include <generic.h>
#include "cmd_options.h"
#include "sysinfo.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>

#define BUFFSIZE 8192
static char buff[BUFFSIZE]; /* used in the procedures */

unsigned int cpu_use[2], cpu_nic[2], cpu_sys[2];
unsigned long cpu_idl[2];
unsigned int pgpgin[2], pgpgout[2], pswpin[2], pswpout[2];
unsigned int inter[2],ticks[2],ctxt[2];
int tog = 0;

void getstat(unsigned *cuse, unsigned *cice, unsigned *csys, unsigned long *cide,
             unsigned *pin, unsigned *pout, unsigned *sin, unsigned *sout,
             unsigned *itot, unsigned *i1, unsigned *ct) {
  static int stat;
  
  if ((stat=open("/proc/stat", O_RDONLY, 0)) != -1) {
    char* b;
    buff[BUFFSIZE-1] = 0;  /* ensure null termination in buffer */
    read(stat,buff,BUFFSIZE-1);
    close(stat);
    *itot = 0; 
    *i1 = 1;   /* ensure assert below will fail if the sscanf bombs */
    b = strstr(buff, "cpu ");
    sscanf(b, "cpu  %u %u %u %lu", cuse, cice, csys, cide);
    b = strstr(buff, "page ");
    sscanf(b, "page %u %u", pin, pout);
    b = strstr(buff, "swap ");
    sscanf(b, "swap %u %u", sin, sout);
    b = strstr(buff, "intr ");
    sscanf(b, "intr %u %u", itot, i1);
    b = strstr(buff, "ctxt ");
    sscanf(b, "ctxt %u", ct);
  } else {
    LOG(LOG_NOTICE, "/proc/stat: %m");
  }
}

void getrunners(unsigned int *running, unsigned int *blocked,
                unsigned int *swapped) {
  static struct dirent *ent;
  static char filename[80];
  static int fd;
  static unsigned size;
  static char c;
  DIR *proc;
  
  *running=0;
  *blocked=0;
  *swapped=0;
    
  if ((proc=opendir("/proc"))==NULL) {
    LOG(LOG_NOTICE, "/proc: %m");
    return;
  }
    
  while((ent=readdir(proc))) {
    if (isdigit(ent->d_name[0])) {  /*just to weed out the obvious junk*/
      sprintf(filename, "/proc/%s/stat", ent->d_name);
      if ((fd = open(filename, O_RDONLY, 0)) != -1) { /*this weeds the rest*/
        read(fd,buff,BUFFSIZE-1);
        sscanf(buff, "%*d %*s %c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*d %*d %*d %*d %*d %*d %*u %*u %*d %*u %u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u\n",&c,&size);
        close(fd);
    
        if (c=='R') {
          if (size>0) (*running)++;
          else (*swapped)++;
        }
        else if (c=='D') {
          if (size>0) (*blocked)++;
          else (*swapped)++;
        }
      }
    }
  }
  closedir(proc);
  
#if 1
  /* is this next line a good idea?  It removes this thing which
     uses (hopefully) little time, from the count of running processes */
  (*running)--;
#endif
}

void getmeminfo(unsigned *memfree, unsigned *membuff, unsigned *swapused, unsigned *memcache, unsigned *memused) {
  unsigned long long** mem;
  if (!(mem = meminfo())) return;
  *memfree  = mem[meminfo_main][meminfo_free]    >> 10; /* bytes to k */
  *membuff  = mem[meminfo_main][meminfo_buffers] >> 10;
  *swapused = mem[meminfo_swap][meminfo_used]    >> 10;
  *memcache = mem[meminfo_main][meminfo_cached]  >> 10;
  *memused  = mem[meminfo_main][meminfo_used]    >> 10;
}

int init(void)
{
  if (OPT_VALUE_SERVERID == 0) {
    fprintf(stderr, "Please set serverid first\n");
    return 0;
  }
  daemonize = TRUE;
  every = EVERY_MINUTE;
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  getstat(cpu_use,cpu_nic,cpu_sys,cpu_idl, pgpgin,pgpgout,pswpin,pswpout, inter,ticks,ctxt);
  sleep(2); // ensure we wait until the next minute
  return 1;
}

int run(void)
{
  xmlDocPtr doc;
  xmlNodePtr subtree, sysstat;
  int ret = 0;
  int color = 200;
  double lavg, dummy;
  time_t now;
  FILE *in;
  char buffer[1024];
  char info[32768];
  unsigned int duse,dsys,didl,div,divo2;
  unsigned int memfree,membuff,swapused, memcache, memused;
  unsigned int kb_per_page = sysconf(_SC_PAGESIZE) / 1024;
  unsigned int hz = sysconf(_SC_CLK_TCK); /* get ticks/s from system */
  int systemp = 0;

  // compute sysstat
  //
  tog = !tog; // use alternating variables

  getstat(cpu_use+tog,cpu_nic+tog,cpu_sys+tog,cpu_idl+tog,
        pgpgin+tog,pgpgout+tog,pswpin+tog,pswpout+tog,
        inter+tog,ticks+tog,ctxt+tog);
  getmeminfo(&memfree,&membuff,&swapused,&memcache,&memused);
  loadavg(&lavg, &dummy, &dummy);

  duse = *(cpu_use+tog)-*(cpu_use+!tog)+*(cpu_nic+tog)-*(cpu_nic+!tog);
  dsys = *(cpu_sys+tog)-*(cpu_sys+!tog);
  didl = (*(cpu_idl+tog)-*(cpu_idl+!tog))%UINT_MAX;
  div = (duse+dsys+didl);
  divo2 = div/2;
  duse = (100*duse+divo2)/div;
  dsys = (100*dsys+divo2)/div;
  didl = (100*didl+divo2)/div;
  //printf("%3u %3u %3u\n", duse,dsys,didl);

  if (lavg > 5.0) color = STAT_RED;

  uw_setproctitle("running top");
  system("top -b -n 1 | head -30 > /tmp/.uw_sysstat.tmp");
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
  unlink("tmp/.uw_sysstat.tmp");
  if (HAVE_OPT(SYSTEMP_COMMAND)) {
    char cmd[1024];

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
  now = time(NULL);

  sysstat = xmlNewChild(xmlDocGetRootElement(doc), NULL, "sysstat", NULL);
  sprintf(buffer, "%ld", OPT_VALUE_SERVERID);	xmlSetProp(sysstat, "server", buffer);
  sprintf(buffer, "%s", "127.0.0.1");		xmlSetProp(sysstat, "ipaddress", buffer);
  sprintf(buffer, "%d", (int) now);		xmlSetProp(sysstat, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+(OPT_VALUE_EXPIRES*60));	xmlSetProp(sysstat, "expires", buffer);
  sprintf(buffer, "%d", color);			subtree = xmlNewChild(sysstat, NULL, "color", buffer);
  sprintf(buffer, "%.1f", lavg);		subtree = xmlNewChild(sysstat, NULL, "loadavg", buffer);
  sprintf(buffer, "%u", duse);			subtree = xmlNewChild(sysstat, NULL, "user", buffer);
  sprintf(buffer, "%u", dsys);			subtree = xmlNewChild(sysstat, NULL, "system", buffer);
  sprintf(buffer, "%u", didl);			subtree = xmlNewChild(sysstat, NULL, "idle", buffer);
  sprintf(buffer, "%u", ((*(pswpin+tog)-*(pswpin+(!tog)))*kb_per_page+30)/60);
    subtree = xmlNewChild(sysstat, NULL, "swapin", buffer);
  sprintf(buffer, "%u", ((*(pswpout+tog)-*(pswpout+(!tog)))*kb_per_page+30)/60);
    subtree = xmlNewChild(sysstat, NULL, "swapout", buffer);
  sprintf(buffer, "%u", (*(pgpgin+tog)-*(pgpgin+(!tog))+30)/60);
    subtree = xmlNewChild(sysstat, NULL, "blockin", buffer);
  sprintf(buffer, "%u", (*(pgpgout+tog)-*(pgpgout+(!tog))+30)/60);
    subtree = xmlNewChild(sysstat, NULL, "blockout", buffer);
  sprintf(buffer, "%u", swapused);		subtree = xmlNewChild(sysstat, NULL, "swapped", buffer);
  sprintf(buffer, "%u", memfree);		subtree = xmlNewChild(sysstat, NULL, "free", buffer);
  sprintf(buffer, "%u", membuff);		subtree = xmlNewChild(sysstat, NULL, "buffered", buffer);
  sprintf(buffer, "%u", memcache);		subtree = xmlNewChild(sysstat, NULL, "cached", buffer);
  sprintf(buffer, "%u", memused);		subtree = xmlNewChild(sysstat, NULL, "used", buffer);
  sprintf(buffer, "%d", systemp);		subtree = xmlNewChild(sysstat, NULL, "systemp", buffer);
    subtree = xmlNewChild(sysstat, NULL, "info", info);

  spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(OUTPUT), doc, NULL);
  xmlFreeDoc(doc);
  return(ret);
}
