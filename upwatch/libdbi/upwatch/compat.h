#ifndef __COMPAT_H
#define __COMPAT_H

#ifndef HAVE_DAEMON
int daemon(int nochdir, int noclose);
#endif

// For Solaris
#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned long) -1)
#endif

#endif

