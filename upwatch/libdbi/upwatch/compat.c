#include "config.h"
#include "generic.h"

#ifndef HAVE_DAEMON
/* daemon() - fall into background {{{ */
int daemon(int nochdir,int noclose) {
    pid_t ret;
#ifdef TIOCNOTTY
    int fd;
#endif

    ret=fork();
    if (ret) exit(0);
    if (ret<0) return (int)ret;
    if (noclose==0) {
        freopen("/dev/null", "r", stdin);
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
#ifdef TIOCNOTTY
        /* disconnect from the controlling tty */
        fd=open("/dev/tty",O_RDWR|O_NOCTTY);
        if (fd>=0) {
        (void)ioctl(fd,TIOCNOTTY,NULL);
        close(fd);
    }
#endif
    }
#ifdef HAVE_SETSID
    setsid();
#endif
    if (nochdir==0) chdir("/");
    return 0;
} /* }}} */
#endif


