#ifndef __UW_PROCESS_H
#define __UW_PROCESS_H

/* list of probes */
typedef enum
{
PROBE_EMPTY = 1,
#include "../probes.enum"
} probeidx;


#define STANDARD_PROBE_RESULT   \
  guint color; \
  guint stattime; \
  guint probeid; \
  guint server; \
  guint expires; \
  char *message; 

struct probe_result {
  STANDARD_PROBE_RESULT;
#include "../common/common.h"
};

#define STANDARD_PROBE_DEF   \
  guint stamp; \
  guint server; \
  guint probeid; \
  guint color; \
  guint newest; \

struct probe_def{
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};

typedef struct _module {
  int class;
  char *name;
  GHashTable *cache;
  void (*free_def)(void *def);
  void (*free_res)(void *res);
  void *(*extract_from_xml)(struct _module *probe, xmlDocPtr, xmlNodePtr, xmlNsPtr);
  void *(*get_def)(struct _module *probe, void *res);
  gint (*store_results)(struct _module *probe, void *def, void *res);
  void (*summarize)(void *def, void *res, char *from, char *into, guint slotlow, guint slothigh);
} module;

typedef struct transaction {
  module *mod;		// point to the module processing this transaction
  xmlDocPtr doc; 	// the Xml doc structure
  xmlNodePtr node;	// current node in the Xml doc
  xmlNsPtr ns;		// current namespace
  void *res;		// probe result pointer
  void *def;		// probe definition data
} trx;

#define STANDARD_MODULE_STUFF(a, b) PROBE_##a, b, NULL
#endif /* __UW_PROCESS_H */

