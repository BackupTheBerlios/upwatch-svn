#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#include "uw_notify_glob.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

module errlog_module  = {
  STANDARD_MODULE_STUFF(errlog),
  NO_FREE_DEF,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  ct_get_from_xml,
  accept_result,
  get_def_by_servid,
  NO_ADJUST_RESULT,
  NO_END_RESULT,
  NO_END_RUN,
  NO_EXIT,
  notify
};

