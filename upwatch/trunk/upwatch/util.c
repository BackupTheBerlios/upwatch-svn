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

char *color2string(int color) {
static char buf[10];

  switch (color) {
  case STAT_NONE:	return("NONE");
  case STAT_BLUE:	return("BLUE");
  case STAT_GREEN:	return("GREEN");
  case STAT_YELLOW:	return("YELLOW"); 
  case STAT_PURPLE:	return("PURPLE");
  case STAT_RED:	return("RED");
  default:	sprintf(buf, "%u", color); return(buf);
  }
}

char *strcat_realloc(char *old, char *str)
{
  char *new;
  int len;

  if (old == NULL) {
    return strdup(str);
  }
  
  len = strlen(old);
  new = realloc(old, len+strlen(str)+2);
  strcpy(&new[len], str);
  return(new);
}

