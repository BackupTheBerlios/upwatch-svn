/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape Portable Runtime library.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):  Silicon Graphics, Inc.
 * 
 * Portions created by SGI are Copyright (C) 2000-2001 Silicon
 * Graphics, Inc.  All Rights Reserved.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#ifndef __ST_THREAD_H__
#define __ST_THREAD_H__

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <time.h>
#include <errno.h>

#if defined(__MACH__) && defined(__APPLE__)
struct pollfd
{
	int fd;
	short events;
	short revents;
};

#define POLLIN          0x0001          /* fd is readable */
#define POLLPRI         0x0002          /* high priority info at fd */
#define POLLOUT         0x0004          /* fd is writeable (won't block) */
#define POLLRDNORM      0x0040          /* normal data is readable */
#define POLLWRNORM      POLLOUT
#define POLLRDBAND      0x0080          /* out-of-band data is readable */
#define POLLWRBAND      0x0100          /* out-of-band data is writeable */

#define POLLNORM        POLLRDNORM

#define POLLERR         0x0008          /* fd has error condition */
#define POLLHUP         0x0010          /* fd has been hung up on */
#define POLLNVAL        0x0020          /* invalid pollfd entry */

#define POLLREMOVE      0x0800  /* remove a cached poll fd from /dev/poll */

#else
#include <poll.h>
#endif

#ifndef ETIME
#define ETIME ETIMEDOUT
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long long  st_utime_t;
typedef struct _st_thread * st_thread_t;
typedef struct _st_cond *   st_cond_t;
typedef struct _st_mutex *  st_mutex_t;
typedef struct _st_netfd *  st_netfd_t;

extern int st_init(void);
extern int st_getfdlimit(void);

extern st_thread_t st_thread_self(void);
extern void st_thread_exit(void *retval);
extern int st_thread_join(st_thread_t thread, void **retvalp);
extern void st_thread_interrupt(st_thread_t thread);
extern st_thread_t st_thread_create(void *(*start)(void *arg), void *arg,
				    int joinable, int stack_size);
extern int st_randomize_stacks(int on);

extern st_utime_t st_utime(void);
extern st_utime_t st_utime_last_clock(void);
extern int st_timecache_set(int on);
extern time_t st_time(void);
extern int st_usleep(st_utime_t usecs);
extern int st_sleep(int secs);
extern st_cond_t st_cond_new(void);
extern int st_cond_destroy(st_cond_t cvar);
extern int st_cond_timedwait(st_cond_t cvar, st_utime_t timeout);
extern int st_cond_wait(st_cond_t cvar);
extern int st_cond_signal(st_cond_t cvar);
extern int st_cond_broadcast(st_cond_t cvar);
extern st_mutex_t st_mutex_new(void);
extern int st_mutex_destroy(st_mutex_t lock);
extern int st_mutex_lock(st_mutex_t lock);
extern int st_mutex_unlock(st_mutex_t lock);
extern int st_mutex_trylock(st_mutex_t lock);

extern int st_key_create(int *keyp, void (*destructor)(void *));
extern int st_key_getlimit(void);
extern int st_thread_setspecific(int key, void *value);
extern void *st_thread_getspecific(int key);

extern st_netfd_t st_netfd_open(int osfd);
extern st_netfd_t st_netfd_open_socket(int osfd);
extern void st_netfd_free(st_netfd_t fd);
extern int st_netfd_close(st_netfd_t fd);
extern int st_netfd_fileno(st_netfd_t fd);
extern void st_netfd_setspecific(st_netfd_t fd, void *value,
				 void (*destructor)(void *));
extern void *st_netfd_getspecific(st_netfd_t fd);
extern int st_netfd_serialize_accept(st_netfd_t fd);
extern int st_netfd_poll(st_netfd_t fd, int how, st_utime_t timeout);

extern int st_poll(struct pollfd *pds, int npds, st_utime_t timeout);
extern st_netfd_t st_accept(st_netfd_t fd, struct sockaddr *addr, int *addrlen,
			    st_utime_t timeout);
extern int st_connect(st_netfd_t fd, const struct sockaddr *addr, int addrlen,
		      st_utime_t timeout);
extern ssize_t st_read(st_netfd_t fd, void *buf, size_t nbyte,
		       st_utime_t timeout);
extern ssize_t st_read_fully(st_netfd_t fd, void *buf, size_t nbyte,
			     st_utime_t timeout);
extern int st_read_resid(st_netfd_t fd, void *buf, size_t *resid,
			 st_utime_t timeout);
extern ssize_t st_write(st_netfd_t fd, const void *buf, size_t nbyte,
			st_utime_t timeout);
extern int st_write_resid(st_netfd_t fd, const void *buf, size_t *resid,
			  st_utime_t timeout);
extern ssize_t st_writev(st_netfd_t fd, const struct iovec *iov, int iov_size,
			 st_utime_t timeout);
extern int st_recvfrom(st_netfd_t fd, void *buf, int len,
		       struct sockaddr *from, int *fromlen,
		       st_utime_t timeout);
extern int st_sendto(st_netfd_t fd, const void *msg, int len,
		     const struct sockaddr *to, int tolen, st_utime_t timeout);
extern st_netfd_t st_open(const char *path, int oflags, mode_t mode);

#ifdef DEBUG
extern void _st_show_thread_stack(st_thread_t thread, const char *messg);
extern void _st_iterate_threads(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* !__ST_THREAD_H__ */

