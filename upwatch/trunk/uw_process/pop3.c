#include "config.h"
#include <string.h>
#include <generic.h>
#include "cmd_options.h"
#include "slot.h"
#include "uw_process.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct pop3_result {
  STANDARD_PROBE_RESULT;
#include "../uw_pop3/probe.res_h"
};

module pop3_module  = {
  STANDARD_MODULE_STUFF(pop3),
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

