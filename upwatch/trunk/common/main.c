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

static int logexit = TRUE;
void exit_function(void)
{
  if (logexit) {
    LOG(LOG_NOTICE, "exit");
  }
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

#ifdef USE_ST
  st_init();
#endif
  
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

  if (OPT_VALUE_STDERR)  _log2stderr  = TRUE;
  if (OPT_VALUE_SYSLOG)  _log2syslog  = TRUE;
  if (HAVE_OPT(LOGFILE)) _logfilename = OPT_ARG(LOGFILE);

  LOG(LOG_NOTICE, "start (Version %s-%s, date %s %s)", VERSION, RELEASE, __DATE__, __TIME__);
  LOG(LOG_INFO, "using GCC %s", __VERSION__);

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

    if (debug < 3) {
      logexit = FALSE;
      daemon(0, 0);
      logexit = TRUE;
    }
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

