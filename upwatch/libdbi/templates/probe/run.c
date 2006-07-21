#include "config.h"
#include <generic.h>
#include "cmd_options.h"

int init(void)
{
  daemonize = TRUE;
  every = EVERY_5SECS;
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

int run(void)
{
  return 0;
}

