#include <generic.h>

#ifdef DMALLOC 
#include "dmalloc.h"
#endif

guint *guintdup(guint val)
{
  guint *iptr = g_malloc(sizeof(guint));
  *iptr = val;
  return(iptr);
}

