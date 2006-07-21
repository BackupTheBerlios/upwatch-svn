#include "config.h"
#include <generic.h>
#include <cmd_options.h>
#include <unistd.h>

#include "uw_process.h"
#include "slot.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

int init(void)
{
  daemonize = FALSE;
  every = ONE_SHOT;
  return 1;
}

int run(void)
{
  trx t;
  module probe = { 2, "iptraf" } ;
  struct probe_def def = { 0, 74, { 0 } , 1, 1, 200, 0, 0, "raarts@office.netland.nl", 0, 0};
  struct probe_result res = { "iptraf", STAT_RED, 0, 1, 1 };
  struct probe_result prv = { "iptraf", STAT_GREEN, 0, 1, 1 };
  int now = (int) time(NULL);

  res.expires = now + 300;
  res.stattime = now;
  res.message = "Veel te veel kilobytes";

  probe.db = open_database(OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME),
                           OPT_ARG(DBUSER), OPT_ARG(DBPASSWD),
                           OPT_VALUE_DBCOMPRESS);

  t.def = &def;
  t.res = &res;
  notify(&probe, &t, &prv);
  return 0;
}

