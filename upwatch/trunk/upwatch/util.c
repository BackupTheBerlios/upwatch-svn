#include <generic.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#ifdef DMALLOC 
#include "dmalloc.h"
#endif

guint *guintdup(guint val)
{
  guint *iptr = g_malloc(sizeof(guint));
  *iptr = val;
  return(iptr);
}

int uw_rand(float maxval)
{
  return 1+(int) (maxval*rand()/(RAND_MAX+1.0));
}

void
setsin(register struct sockaddr_in *sin, register u_int32_t addr)
{

        memset(sin, 0, sizeof(*sin));
#ifdef HAVE_SOCKADDR_SA_LEN
        sin->sin_len = sizeof(*sin);
#endif
        sin->sin_family = AF_INET;
        sin->sin_addr.s_addr = addr;
}

