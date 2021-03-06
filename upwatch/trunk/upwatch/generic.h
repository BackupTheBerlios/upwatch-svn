#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <glib.h>
#include <dbi/dbi.h>
#include <netdb.h>
#if defined(ENABLE_SERVER) || defined(ENABLE_MONITORS)
#include <db.h>
#endif

#ifndef ETIME
#define ETIME ETIMEDOUT                         /* For OpenBSD and possibly more platforms */
#endif

#ifdef USE_ST
#include <st.h>
#define sleep st_sleep
#endif

#include <autoopts/options.h>

#define max(x,y) ((x) > (y) ? (x) : (y))
#define min(x,y) ((x) < (y) ? (x) : (y))

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
extern const char *_logsrce;
extern char *progname;
extern int _logline;
extern int _log2stderr;
extern int _log2syslog;
extern const char *_logfilename;

extern int run(void);
extern int init(void);

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "setproctitle.h"

#include "xml.h"

#define PATH_RESULT_DTD "/usr/share/upwatch/dtd/result.dtd"
#define NAMESPACE_URL	"http://www.upwatch.com/schemas/1.0/"

#include "spool.h"

#ifndef HAVE_U_INT32_T
#if HAVE_UINT32_T
#define u_int32_t uint32_t
#endif
#endif

struct dbspec {
  int domid;
  char realm[25];
  char host[65];
  int port;
  char dbtype[64];
  char dbname[64];
  char dbuser[25];
  char dbpassword[25];
  char *srvrbyname; // query to retrieve the server id given the server name
  char *srvrbyid;   // query to retrieve the server name given the server id
  char *srvrbyip;   // query to retrieve the server id given the ipaddress
  database *db;
};
extern struct dbspec *dblist;
extern int dblist_cnt;

char *uw_gmtime(time_t *now);
long timeval_diff(struct timeval *a,struct timeval *b);
guint *guintdup(guint val);
int uw_rand(float maxval);
char *strcat_realloc(char *here, char *str);
void setsin(struct sockaddr_in *, u_int32_t);
database *open_realm(char *realm, const char *dbtype, const char *dbhost, int dbport, const char *dbname, const char *dbuser, const char *dbpasswd);
int uw_password_ok(char *user, char *passwd, const char *authquery, const char *dbtype, const char *dbhost, int dbport, const char *dbname, const char *dbuser, const char *dbpasswd);
int realm_exists(char *realm);
char *realm_server_by_id(char *realm, int id);
int realm_server_by_ip(char *realm, char *ip);
int realm_server_by_name(char *realm, char *name);

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

// not exactly the right place for these defines but i'm lazy
#define ST_INITIATE(a) \
  struct hostent *h; \
  memset(&rmt, 0, sizeof(struct sockaddr_in)); \
  rmt.sin_family = AF_INET; \
  rmt.sin_port = htons((uint16_t)a); \
  /* try to resolve, one shot attempt ! */ \
  h = gethostbyname (probe->ipaddress); \
  rmt.sin_addr.s_addr = *(in_addr_t *) (h->h_addr_list[0]); \
  if (rmt.sin_addr.s_addr == INADDR_NONE) /* Nothing valid is returned, so assume a ipaddress was given */ { \
  rmt.sin_addr.s_addr = inet_addr(probe->ipaddress); \
  } \
  if (rmt.sin_addr.s_addr == INADDR_NONE) /* still no valid ip */{ \
    char buf[50]; \
\
    sprintf(buf, "Illegal IP address '%s'", probe->ipaddress); \
    probe->msg = strdup(buf); \
    LOG(LOG_DEBUG, probe->msg); \
    goto done; \
  } \
 \
  /* Connect to remote host */ \
  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) { \
    char buf[256]; \
\
    sprintf(buf, "%s: %s", probe->ipaddress, strerror(errno)); \
    probe->msg = strdup(buf); \
    LOG(LOG_DEBUG, probe->msg);  \
    if (debug > 3) fprintf(stderr, "%s: %s\n", probe->ipaddress, probe->msg); \
    goto done; \
  } \
  if ((rmt_nfd = st_netfd_open_socket(sock)) == NULL) { \
    char buf[256]; \
\
    sprintf(buf, "%s: %s", probe->ipaddress, strerror(errno)); \
    probe->msg = strdup(buf); \
    LOG(LOG_DEBUG, probe->msg); \
    if (debug > 3) fprintf(stderr, "%s: %s\n", probe->ipaddress, probe->msg); \
    close(sock); \
    goto done; \
  } 

#define ST_ERROR(a, b) \
  { char buf[256]; \
  if (errno == ETIME) { \
    sprintf(buf, "%s(%d): %s timeout after %u seconds", \
            probe->ipaddress, __LINE__, a, (unsigned) (b / 1000000L)); \
  } else { \
    sprintf(buf, "%s(%d): %s", probe->ipaddress, __LINE__, strerror(errno)); \
  } \
  probe->msg = strdup(buf); \
  LOG(LOG_DEBUG, probe->msg); \
  if (debug > 3) fprintf(stderr, "%s: %s\n", probe->ipaddress, probe->msg); }

