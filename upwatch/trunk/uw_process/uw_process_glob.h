#ifndef __UW_PROCESS_GLOB_H
#define __UW_PROCESS_GLOB_H

extern int trust(char *name);

#include "uw_process.h"
#include <probe.h>

/* list of probes */
typedef enum
{
PROBE_EMPTY = 1,
#include "../probes.enum"
} probeidx;

struct _module {
  STANDARD_MODULE_STRUCT
#define NO_STORE_RESULTS NULL
  gint (*store_results)(trx *t);
#define NO_SUMMARIZE NULL
  void (*summarize)(trx *t, char *from, char *into, 
                    guint slot, guint slotlow, guint slothigh, gint ignoredupes);
};

MYSQL *open_domain(char *domain);

struct summ_spec {
  int period;   // 
  int perslot;  // # of recs per slot in the from table
  char *from;   // summarize from table
  char *to;     // summarize into this table
}; 

char *query_server_by_id(module *probe, int id);
void update_last_seen(module *probe);
int notify(trx *t);

void mod_ic_add(module *probe, const char *table, const char *str);
void mod_ic_flush(module *probe, const char *table);

/* generic functions */
extern gint ct_store_raw_result(trx *t);
extern void ct_summarize(trx *t, char *from, char *into, 
                         guint slot, guint slotlow, guint slothigh, gint resummarize);

#endif /* __UW_PROCESS_GLOB_H */

