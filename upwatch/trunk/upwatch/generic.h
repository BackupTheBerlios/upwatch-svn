#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <glib.h>
#include <db.h>

#include <options.h>

#include <syslog.h>
#define LOG pthread_mutex_lock(&_logmutex),_logsrce=__FILE__,_logline=__LINE__,_LOG
void _LOG(int level, const char *fmt, ...);
extern pthread_mutex_t _logmutex;
extern char *_logsrce;
extern char *progname;
extern int _logline;

extern int run(void);
extern int init(void);

#include "spool.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "xml.h"

#define PATH_RESULT_DTD "/usr/lib/upwatch/dtd/result.dtd"
#define NAMESPACE_URL	"http://www.upwatch.com/schemas/1.0/"

char *uw_gmtime(time_t *now);
long timeval_diff(struct timeval *a,struct timeval *b);

extern int debug;
extern int startsec;
extern int every;
extern int daemonize;

#define ONE_SHOT 0
#define EVERY_SECOND 1
#define EVERY_5SECS 2
#define EVERY_MINUTE 3

#define STAT_NONE 0
#define STAT_BLUE 100
#define STAT_GREEN 200
#define STAT_YELLOW 300
#define STAT_PURPLE 400
#define STAT_RED 500

/* list of probes */
#define PROBE_PING 1
#define PROBE_HTTPGET 2

