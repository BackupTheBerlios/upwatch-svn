#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#include "uw_process_glob.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

module bb_module  = {
  STANDARD_MODULE_STUFF(bb),
  NO_FREE_DEF,
  bb_free_res,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  bb_xml_result_node,
  NO_GET_FROM_XML,
  accept_result,
  bb_get_def,
  NO_ADJUST_RESULT,
  NO_END_RESULT,
  NO_END_RUN,
  NO_EXIT,
  NO_STORE_RESULTS,
  NO_SUMMARIZE
};

