#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <options.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "generic.h"

#ifdef DMALLOC 
#include "dmalloc.h"
#endif

// publicly accessible vars
int debug;
int startsec;
int forever=30;
int sighup;
char *progname;
int every = ONE_SHOT;
int daemonize = TRUE;
int runcounter;

void sighup_handler (int signum)
{
  LOG(LOG_NOTICE, "signal %d received", signum); 
  sighup = 1;
}

void termination_handler (int signum)
{
  if (signum == SIGTRAP) return;

  if (forever == 0) {
    // been here before?
    _exit(0);
  }
  forever = 0;

  // Note: this function hangs when ElectricFence is used, don't know why
  LOG(LOG_NOTICE, "signal %d received - finishing up", signum); 

}

void exit_function(void)
{
  LOG(LOG_NOTICE, "exit");
}

/****************************
 main
 ***************************/
int main( int argc, char** argv, char **envp )
{
  int ret=0;
  int arg_ct;
  struct sigaction new_action;
  char** newArgv = malloc( (argc+1) * sizeof( char* ));
  int ix = 0;

  do { 
    newArgv[ ix ] = strdup( argv[ ix ] ); 
  } while (++ix < argc);
  newArgv[ ix ] = NULL;

  uw_initsetproctitle(argc, argv, envp);
  
  if ((progname = strrchr(newArgv[0], '/')) != NULL) {
    progname++;
  } else {
    progname = newArgv[0];
  }
  //fprintf(stderr, "%s\n", progname);

  umask(002); // all created files must be group-writable
  arg_ct = optionProcess( &progOptions, argc, newArgv );
  argc -= arg_ct;
  newArgv += arg_ct;

  debug = OPT_VALUE_DEBUG;

  LOG(LOG_NOTICE, "start");

  atexit(exit_function);
  if (!init()) exit(1);

  /* set up SIGTERM/SIGINT/SIGSEGV handler */
  new_action.sa_handler = termination_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction (SIGTERM, &new_action, NULL);

  new_action.sa_handler = termination_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction (SIGINT, &new_action, NULL);

  new_action.sa_handler = termination_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction (SIGSEGV, &new_action, NULL);
/*
  new_action.sa_handler = termination_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction (SIGTRAP, &new_action, NULL);
*/
  /* ignore SIGPIPE */
  new_action.sa_handler = SIG_IGN;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction (SIGPIPE, &new_action, NULL);

  if (daemonize) {
    new_action.sa_handler = sighup_handler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction (SIGHUP, &new_action, NULL);

    if (debug < 3) daemon(0, 0);
  }
  do {
    struct timeval start;
    int i, now;
    time_t now_t;
    struct tm *gmt;
    int sleep_seconds = 0;

    ++runcounter;
    gettimeofday(&start, NULL);
    uw_setproctitle("processing");
    ret = run();
    if (debug > 0 && ret) {
      struct timeval end;

      gettimeofday(&end, NULL);
      LOG(LOG_DEBUG, "run %d: processed %d items in %f seconds", runcounter, ret, 
                      ((float)timeval_diff(&end, &start))/1000000.0);
    }
    uw_setproctitle("sleeping");
    sleep(1);  // ensure second rolls over
    switch(every) {
    case EVERY_SECOND:
      // already slept 1 second
      break;
    case EVERY_5SECS:
      sleep_seconds = 5;
      if (debug > 3) { 
        LOG(LOG_DEBUG, "sleeping for %d seconds", sleep_seconds);
      }
      break;
    case EVERY_MINUTE:
      time(&now_t);
      gmt = gmtime(&now_t);
      now = gmt->tm_sec;
  
      if (debug > 3) { 
        sleep_seconds = 3;
      } else {
        if (now >= startsec) {
          sleep_seconds = startsec - now + 60;
        } else {
          sleep_seconds = startsec - now;
        }
      }
      if (debug > 3) { 
        LOG(LOG_DEBUG, "sleeping for %d seconds", sleep_seconds);
      }
      break;
    case ONE_SHOT:
    default:
      break;
    }
    for (i=0; i < sleep_seconds && forever; i++) {
      sleep(1);
    }
  } while (forever && (every != ONE_SHOT));
  exit(ret);
}

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

  if (OPT_VALUE_STDERR && (level-debug < 5)) {
    fprintf(stderr, "%s %s %s\n", timebuf, hostname, msg);
  }
  if (HAVE_OPT(LOGFILE) && (level-debug < 5)) {
    FILE *out;
    char *logfile = OPT_ARG(LOGFILE);

    out = fopen(logfile, "a");
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

