#include "config.h"
#include <string.h>
#include <generic.h>
#include "cmd_options.h"
#include "slot.h"
#include "uw_process.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct mssql_result {
  STANDARD_PROBE_RESULT;
#include "../uw_mssql/probe.res_h"
};

module mssql_module  = {
  STANDARD_MODULE_STUFF(mssql),
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

