#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#ifdef UW_PROCESS
#include "uw_process_glob.h"
#endif
#ifdef UW_NOTIFY
#include "uw_notify_glob.h"
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct imap_result {
  STANDARD_PROBE_RESULT;
#include "../uw_imap/probe.res_h"
};
struct imap_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};

module imap_module  = {
  STANDARD_MODULE_STUFF(imap),
  NO_FREE_DEF,
  NO_FREE_RES,
  NO_INIT,
  NO_START_RUN,
  NO_ACCEPT_PROBE,
  NO_XML_RESULT_NODE,
  ct_get_from_xml,
  NO_FIX_RESULT,
  NO_GET_DEF,
#ifdef UW_PROCESS
  ct_store_raw_result,
  ct_summarize,
#endif
  NO_END_PROBE,
  NO_END_RUN
};

