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
#include "cmd_options.h"
#include "generic.h"

static void wait_to_start(void);

int debug;
int startsec;
int forever=1;
static int runcounter;
char *progname;

void termination_handler (int signum)
{
  // Note: this function hangs when ElectricFence is used, don't know why
  LOG(LOG_NOTICE, "signal %d received - finishing up", signum); 

  forever = 0;
}

void exit_function(void)
{
  LOG(LOG_NOTICE, "exit");
}

int every = ONE_SHOT;
int daemonize = TRUE;

static void one_shot(void)
{
  int ret;
  struct timeval start;

  gettimeofday(&start, NULL);
  ret = run(); 	/* this will run once */
  if (debug > 0 && ret) {
    struct timeval end;

    gettimeofday(&end, NULL);
    LOG(LOG_DEBUG, "run lasted %f seconds", ((float)timeval_diff(&end, &start))/1000000.0);
  }     
  return;
}

static void every_second(void)
{
  int ret;

  while (forever) {
    struct timeval start;

    sleep(1);
    if (!forever) break; // kill -TERM ?
    gettimeofday(&start, NULL);
    ++runcounter;
    ret = run(); 	/* this will run every second */
    if (debug > 0 && ret) {
      struct timeval end;

      gettimeofday(&end, NULL);
      LOG(LOG_DEBUG, "run %d: processed %d items in %f seconds", runcounter, ret, ((float)timeval_diff(&end, &start))/1000000.0);
    }     
  }
  return;
}

static void every_5secs(void)
{
  int ret;

  while (forever) {
    struct timeval start;

    sleep(5);
    if (!forever) break; // kill -TERM ?
    gettimeofday(&start, NULL);
    ++runcounter;
    ret = run(); 	/* this will run every 5 seconds */
    if (debug > 0 && ret) {
      struct timeval end;

      gettimeofday(&end, NULL);
      LOG(LOG_DEBUG, "run %d: processed %d items in %f seconds", runcounter, ret, ((float)timeval_diff(&end, &start))/1000000.0);
    }     
  }
  return;
}

static void every_minute(void)
{
  int ret;

  while (forever) {
    struct timeval start;

    wait_to_start();
    if (!forever) break; // kill -TERM ?
    ++runcounter;
    gettimeofday(&start, NULL);
    ret = run(); 	/* run every minute - returns amount of items processed */
    if (debug > 0 && ret) {
      struct timeval end;

      gettimeofday(&end, NULL);
      LOG(LOG_DEBUG, "run %d: processed %d items in %f seconds", runcounter, ret, ((float)timeval_diff(&end, &start))/1000000.0);
    }     
  }
  return;
}

/****************************
 main
 ***************************/
int main( int argc, char** argv, char **envp )
{
  int ret=0;
  int arg_ct;
  struct sigaction new_action;

  if ((progname = strrchr(argv[0], '/')) != NULL) {
    progname++;
  } else {
    progname = argv[0];
  }
  fprintf(stderr, "%s\n", progname);

  umask(002); // all created files must be group-writable
  arg_ct = optionProcess( &progOptions, argc, argv );
  argc -= arg_ct;
  argv += arg_ct;

  debug = OPT_VALUE_DEBUG;

  LOG(LOG_NOTICE, "start");
  atexit(exit_function);
  if (!init()) exit(1);

  /* set up SIGTERM/SIGINT handler */
  new_action.sa_handler = termination_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction (SIGTERM, &new_action, NULL);
  new_action.sa_handler = termination_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction (SIGINT, &new_action, NULL);

  /* ignore SIGHUP/SIGPIPE */
  new_action.sa_handler = SIG_IGN;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction (SIGHUP, &new_action, NULL);
  new_action.sa_handler = SIG_IGN;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction (SIGPIPE, &new_action, NULL);

  if (debug < 3 && daemonize) daemon(0, 0);
  switch(every) {
    case EVERY_MINUTE:  every_minute(); break;
    case EVERY_5SECS:   every_5secs(); break;
    case EVERY_SECOND:  every_second(); break;
    case ONE_SHOT:      one_shot(); break;
  }
  exit(ret);
}

/****************************
 time functions
 ***************************/
static int cur_second(void)
{
  time_t now;
  struct tm *gmt;

  time(&now);
  gmt = gmtime(&now);
  return(gmt->tm_sec);
}

int sleep_seconds = 0;

static void wait_to_start(void)
{
  int now = cur_second();
  
  if (debug > 2) { 
    sleep_seconds = 3;
    fprintf(stderr, "waiting %d seconds..\n", sleep_seconds);
  } else {
    if (now >= startsec) {
      sleep_seconds = startsec - now + 60;
    } else {
      sleep_seconds = startsec - now;
    }
    if (debug > 1) LOG(LOG_DEBUG, "sleeping for %d seconds", sleep_seconds);
  }
  sleep(sleep_seconds);
}

/* compute time diff in microsecs */
long timeval_diff(struct timeval *a,struct timeval *b)
{
  double temp;

  temp = ((a->tv_sec*1000000) + a->tv_usec) -
           ((b->tv_sec*1000000) + b->tv_usec);

  return (long) temp;
}

/****************************
 logging utility function
 Mar 31 22:17:53 ts probe[20492]: main.c(63): startup ron
 ***************************/

#include <time.h>
#include <ctype.h>

pthread_mutex_t _logmutex = PTHREAD_MUTEX_INITIALIZER;
char *_logsrce;
int _logline;
char *_logfile;

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

  if (OPT_VALUE_STDERR) {
    fprintf(stderr, "%s\n", msg);
  }
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
  pthread_mutex_unlock(&_logmutex);
}

// return an ascii format timestamp in UTC format
char *uw_gmtime(time_t *now)
{
    struct tm *tnow;
    char *datetime;

    tnow = gmtime(now);
    datetime = asctime(tnow);
    datetime[strlen(datetime)-1] = 0;
    return(datetime);
}

