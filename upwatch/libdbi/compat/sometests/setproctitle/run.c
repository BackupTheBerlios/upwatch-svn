#include "config.h"
#include <generic.h>
#include "setproctitle.h"

int init(void)
{
  daemonize = FALSE;
  every = ONE_SHOT;
  return(1);
}

int run(void)
{
//  LOG(LOG_NOTICE, "perror: %d \%m %m %s", 45, "test string");
  uw_setproctitle("Hallo %s", "Ron");
  fprintf(stderr, "Hallo!\n");
  every = EVERY_SECOND;
}

