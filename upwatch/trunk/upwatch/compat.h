#ifndef __COMPAT_H
#define __COMPAT_H

#ifndef HAVE_DAEMON
int daemon(int nochdir, int noclose);
#endif

// For Solaris
#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned long) -1)
#endif

#if defined(ENABLE_SERVER)
#ifndef HAVE_MYSQL_REAL_ESCAPE
unsigned long mysql_real_escape_string(MYSQL *mysql, char *to, const char *from, unsigned long length);
#endif
#endif

#endif

