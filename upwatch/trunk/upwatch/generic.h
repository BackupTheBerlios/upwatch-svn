#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <glib.h>
#if defined(ENABLE_SERVER) || defined(ENABLE_MONITORS)
#include <db.h>
#endif

#include <options.h>

#include <syslog.h>
#ifdef WITH_THREADS
#define LOG pthread_mutex_lock(&_logmutex),_logsrce=__FILE__,_logline=__LINE__,_LOG
#define LOGRAW pthread_mutex_lock(&_logmutex),_logsrce=__FILE__,_logline=__LINE__,_LOGRAW
extern pthread_mutex_t _logmutex;
#else
#define LOG _logsrce=__FILE__,_logline=__LINE__,_LOG
#define LOGRAW _logsrce=__FILE__,_logline=__LINE__,_LOGRAW
#endif
void _LOG(int level, char *fmt, ...);
void _LOGRAW(int level, char *fmt);
extern char *_logsrce;
extern char *progname;
extern int _logline;
extern int _log2stderr;
extern int _log2syslog;
extern char *_logfilename;

extern int run(void);
extern int init(void);

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "setproctitle.h"

#include "xml.h"

#define PATH_RESULT_DTD "/usr/lib/upwatch/dtd/result.dtd"
#define NAMESPACE_URL	"http://www.upwatch.com/schemas/1.0/"

#include "spool.h"

#ifndef HAVE_U_INT32_T
#if HAVE_UINT32_T
#define u_int32_t uint32_t
#endif
#endif

char *uw_gmtime(time_t *now);
long timeval_diff(struct timeval *a,struct timeval *b);
guint *guintdup(guint val);
int uw_rand(float maxval);
char *strcat_realloc(char *here, char *str);
void setsin(struct sockaddr_in *, u_int32_t);

#include <compat.h>

extern int debug;
extern int startsec;
extern int every;
extern int daemonize;
extern int forever;
extern int runcounter;

#define ONE_SHOT 0
#define EVERY_SECOND 1
#define EVERY_5SECS 2
#define EVERY_MINUTE 3

char *color2string(int color);

#define STAT_NONE 0
#define STAT_BLUE 100
#define STAT_GREEN 200
#define STAT_YELLOW 300
#define STAT_PURPLE 400
#define STAT_RED 500

