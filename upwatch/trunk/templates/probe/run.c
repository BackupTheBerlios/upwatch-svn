#include "config.h"

#include <generic.h>
#include "cmd_options.h"

int init(void)
{
  daemonize = TRUE;
  every = EVERY_5SECS;
  g_thread_init(NULL);
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

int run(void)
{
  int ret = 0;

  return(ret);
}


