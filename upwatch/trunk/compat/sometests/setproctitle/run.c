#include "config.h"
#include <generic.h>
#include "cmd_options.h"

int init(void)
{
  daemonize = FALSE;
  every = ONE_SHOT;
  return(1);
}

int run(void)
{
  uw_setproctitle("Hallo %s", "Ron");
  sleep (10);
}

