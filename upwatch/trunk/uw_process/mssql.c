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

