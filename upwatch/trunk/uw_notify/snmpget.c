#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#include "uw_notify_glob.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

module snmpget_module  = {
  STANDARD_MODULE_STUFF(snmpget),
  NO_FREE_DEF,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  snmpget_get_from_xml,
  NO_ACCEPT_RESULT,
  NO_GET_DEF,
  NO_ADJUST_RESULT,
  NO_END_RESULT,
  NO_END_RUN,
  NO_EXIT,
  notify
};

