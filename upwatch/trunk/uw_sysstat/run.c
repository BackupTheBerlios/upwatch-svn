#include "config.h"

#include <generic.h>
#include "cmd_options.h"

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

int init(void)
{
  daemonize = TRUE;
  every = EVERY_MINUTE;
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  getstat(cpu_use,cpu_nic,cpu_sys,cpu_idl, pgpgin,pgpgout,pswpin,pswpout, inter,ticks,ctxt);
  sleep(1);
  return(1);
}

int run(void)
{
  xmlDocPtr doc;
  xmlNodePtr subtree, cpuload;
  int ret = 0;
  int color = 200;
  unsigned int hz;
  time_t now;
  char buffer[1024];
  unsigned int duse,dsys,didl,div,divo2;
extern int sleep_seconds;

  hz = sysconf(_SC_CLK_TCK); /* get ticks/s from system */

  // compute cpuload
  //
  tog = !tog; // use alternating variables

  getstat(cpu_use+tog,cpu_nic+tog,cpu_sys+tog,cpu_idl+tog,
        pgpgin+tog,pgpgout+tog,pswpin+tog,pswpout+tog,
        inter+tog,ticks+tog,ctxt+tog);

  duse = *(cpu_use+tog)-*(cpu_use+!tog)+*(cpu_nic+tog)-*(cpu_nic+!tog);
  dsys = *(cpu_sys+tog)-*(cpu_sys+!tog);
  didl = (*(cpu_idl+tog)-*(cpu_idl+!tog))%UINT_MAX;
  div = (duse+dsys+didl);
  divo2 = div/2;
  duse = (100*duse+divo2)/div;
  dsys = (100*dsys+divo2)/div;
  didl = (100*didl+divo2)/div;
  //printf("%3u %3u %3u\n", duse,dsys,didl);

  doc = UpwatchXmlDoc("result");
  now = time(NULL);

  cpuload = xmlNewChild(xmlDocGetRootElement(doc), NULL, "cpuload", NULL);
  sprintf(buffer, "%d", OPT_VALUE_SERVERID);	xmlSetProp(cpuload, "id", buffer);
  sprintf(buffer, "%d", (int) now);		xmlSetProp(cpuload, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+(2*60));	xmlSetProp(cpuload, "expires", buffer);
  sprintf(buffer, "%d", color);			subtree = xmlNewChild(cpuload, NULL, "color", buffer);
  sprintf(buffer, "%u", duse);			subtree = xmlNewChild(cpuload, NULL, "user", buffer);
  sprintf(buffer, "%u", dsys);			subtree = xmlNewChild(cpuload, NULL, "system", buffer);
  sprintf(buffer, "%u", didl);			subtree = xmlNewChild(cpuload, NULL, "idle", buffer);
  spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(OUTPUT), doc, NULL);
  xmlFreeDoc(doc);
  return(ret);
}
