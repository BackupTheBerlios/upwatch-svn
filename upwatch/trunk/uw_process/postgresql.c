#include "config.h"
#include <string.h>
#include <generic.h>
#include "cmd_options.h"
#include "slot.h"
#include "uw_process.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct postgresql_result {
  STANDARD_PROBE_RESULT;
#include "../uw_postgresql/probe.res_h"
};

module postgresql_module  = {
  STANDARD_MODULE_STUFF(postgresql),
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  ct_get_from_xml,
  NULL,
  NULL,
  ct_store_raw_result,
  ct_summarize,
  NULL
};

