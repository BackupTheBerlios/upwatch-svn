#include "config.h"
#include <stdio.h>
// #include <stdarg.h>
// #include <string.h>
// #include <signal.h>
// #include <limits.h>
// #include <options.h>
// #include <time.h>
// #include <sys/time.h>
// #include <sys/types.h>
// #include <sys/stat.h>
#include "generic.h"

#ifdef DMALLOC 
#include "dmalloc.h"
#endif

// publicly accessible vars
char *progname;
int _log2stderr;
int _log2syslog;
char *_logfilename;

/****************************
 logging utility function
 Mar 31 22:17:53 ts probe[20492]: main.c(63): startup ron
 ***************************/

#include <time.h>
#include <ctype.h>

#ifdef WITH_THREADS
pthread_mutex_t _logmutex = PTHREAD_MUTEX_INITIALIZER;
#endif
char *_logsrce;
int _logline;
char *_logfile;
static void _ll_lograw(int level, const char *msg);

void _LOGRAW(int level, char *buffer)
{
  char *p, *file;
  char msg[2*16384];
  unsigned now = (unsigned) time(NULL);
static char prvmsg[512];
static int rpt;
static unsigned prv;

  if ((p = strrchr(_logsrce, '/')) != NULL) {
    file = ++p;
  } else {
    file = _logsrce;
  }
  sprintf(msg, "%s[%lu] %s(%d): ", progname, (unsigned long)getpid(), file, _logline);
  strcat(msg, buffer);

  //fprintf(stderr, "compare: rpt=%u, msg=%s, prv=%s\n", rpt, msg, prvmsg);
  // test if the same error happens again and again
  if (strncmp(prvmsg, msg, sizeof(prvmsg)) == 0) { 
    //fprintf(stderr, "they are the same\n");
    if (now - prv < 300) { // but not more then 5 minutes ago
      //fprintf(stderr, "and not longer than 5 min ago\n");
      rpt++;
      return;
    }
  }
  // at this point it either isn't the same message, or 5 minutes have passed
  if (rpt) {
    char buf[256];

    sprintf(buf, "%s[%lu] last message repeated %u times", progname, (unsigned long)getpid(), rpt);
    _ll_lograw(level, buf);
    rpt = 0;
  }
  prv = now;
  strncpy(prvmsg, msg, sizeof(prvmsg));
  _ll_lograw(level, msg);
}

static void _ll_lograw(int level, const char *msg)
{
  struct tm *tms;
  time_t now;
  char timebuf[30];
  char hostname[256];
  char *p;

  time(&now);
  tms = gmtime(&now);
  strftime(timebuf, sizeof(timebuf), "%b %e %H:%M:%S", tms);
 
  gethostname(hostname, sizeof(hostname));
  p = strchr(hostname, '.');
  if (p) *p = 0;

  if (_log2stderr && (level-debug < 5)) {
    fprintf(stderr, "%s %s %s\n", timebuf, hostname, msg);
  }
  if (_logfilename && (level-debug < 5)) {
    FILE *out;

    out = fopen(_logfilename, "a");
    if (out) {
      fprintf(out, "%s %s %s\n", timebuf, hostname, msg);
      fclose(out);
    } else {
      _log2syslog = 1;
    }
  }
  if (_log2syslog) {
    syslog(level, msg);
  }
#ifdef WITH_THREADS
  pthread_mutex_unlock(&_logmutex);
#endif
}

void _LOG(int level, char *fmt, ...)
{
  char buffer[16384];
  char newfmt[16585];
  char *p, *q;
  va_list arg;

static int firsttime = 1;
static int snprintf_does_errno;  // set to true if snprintf does %m conversions

  if (firsttime) {
    char tmp[256];

    firsttime = 0;
    snprintf(tmp, sizeof(tmp), "%m");
    if (strcmp(tmp, strerror(errno))) {
      snprintf_does_errno = 0;
    } else {
      snprintf_does_errno = 1;
    }
  }

  if (fmt == NULL) fmt = "(null)";

  // expand %m if needed
  if (!snprintf_does_errno) {
    for (p = fmt, q = &newfmt[0]; *p;) {
      switch(*p) {
      case '\\': 
        *q++ = *p++; 
        if (*p) *q++ = *p++; /* escape a character */
        break; 
      case '%':   
        if (*(p+1) == 'm') {
          strcpy(q, strerror(errno));
          p += 2;
          q += strlen(strerror(errno));
          break;
        }
      default:
        *q++ = *p++;
        break;
      }
    }
    *q = 0;
    fmt = newfmt;
  }

  va_start(arg, fmt);
  vsnprintf(buffer, sizeof(buffer)-256 /* extra space for %m expansions */, fmt, arg);
  va_end(arg);

      
  // kill trailing blanks (xml errors have this a lot)
  for (p = &buffer[(unsigned char)(strlen(buffer) - 1)]; isspace(*p); p--) {
    *p = 0;
  }

  _LOGRAW(level, buffer);
}

