#include <stdio.h>

#include "generic.h"
#include "iptraf.h"

int forever=1;

int main(int argc, char *argv[])
{
  struct trace_info *ti = calloc(1, sizeof(struct trace_info));

  ti->type = 0;
  ti->hostname = strdup("lwn.net");
  ti->ipaddress = strdup("66.216.68.48");
  ti->port = 0;
  ti->workfilename = strdup("dinges");

  iptraf(ti, NULL);
}


#include <time.h>
#include <ctype.h>

pthread_mutex_t _logmutex = PTHREAD_MUTEX_INITIALIZER;
char *_logsrce;
int _logline;
char *_logfile;

char *progname = "test";

void _LOG(int level, const char *fmt, ...)
{
  char buffer[BUFSIZ];
  char msg[2*BUFSIZ];
  char *p, *file; 
  va_list arg;
  
  if (fmt == NULL) fmt = "(null)";

  va_start(arg, fmt);
  vsnprintf(buffer, BUFSIZ, fmt, arg);
  va_end(arg);

  // kill trailing blanks (xml errors have this a lot)
  for (p = &buffer[strlen(buffer) - 1]; isspace(*p); p--) {
    *p = 0;
  }

  if ((p = strrchr(_logsrce, '/')) != NULL) {
    file = ++p;
  } else {
    file = _logsrce;
  }
  sprintf(msg, "%s[%d] %s(%d): ", progname, getpid(), file, _logline);
  strcat(msg, buffer);

//  if (OPT_VALUE_STDERR) {
    fprintf(stderr, "%s\n", msg);
//  }

/*
  if (HAVE_OPT(LOGFILE)) {
    FILE *out;
    struct tm *tms;
    time_t now;
    char timebuf[30];
    char hostname[256];

    time(&now); 
    tms = gmtime(&now);
    strftime(timebuf, sizeof(timebuf), "%b %e %H:%M:%S", tms);
 
    gethostname(hostname, sizeof(hostname));
    strtok(hostname, "."); 

    out = fopen(OPT_ARG(LOGFILE), "a");
    if (out) {
      fprintf(out, "%s %s %s\n", timebuf, hostname, msg);
      fclose(out);
    } else {
      OPT_VALUE_SYSLOG = 1;
    }
  }

  if (OPT_VALUE_SYSLOG) {
    syslog(level, msg);
  }
*/
  pthread_mutex_unlock(&_logmutex);
}

