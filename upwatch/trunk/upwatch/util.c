#include "config.h"
#include "generic.h"
#include <sys/types.h>

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

