#ifndef __UW_NOTIFY_GLOB_H
#define __UW_NOTIFY_GLOB_H

#ifdef TNOT
#include "uw_tnot.h"
#else
#include "uw_notify.h"
#endif

#include <probe.h>

/* list of probes */
typedef enum
{
PROBE_EMPTY = 1,
#include "../probes.enum"
} probeidx;

struct _module {
  STANDARD_MODULE_STRUCT
#define NO_NOTIFY NULL
  int (*notify)(trx *t);
};

char *query_server_by_id(module *probe, int id);

/* generic functions */
extern void ct_get_from_xml(trx *t);
extern gint ct_store_raw_result(trx *t);
extern void ct_summarize(trx *t, char *from, char *into, 
                         guint slot, guint slotlow, guint slothigh, gint resummarize);

int notify(trx *t);

#endif /* __UW_NOTIFY_GLOB_H */

