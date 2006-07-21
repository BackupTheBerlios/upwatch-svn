//#include <syslog.h>
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

