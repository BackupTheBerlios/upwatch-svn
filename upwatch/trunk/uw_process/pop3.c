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
  NO_FREE_DEF,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  ct_get_from_xml,
  NO_FIX_RESULT,
  NO_GET_DEF,
  ct_store_raw_result,
  ct_summarize,
  NO_END_PROBE,
  NO_END_RUN
};

