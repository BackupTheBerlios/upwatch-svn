#ifndef __UW_PROCESS_H
#define __UW_PROCESS_H

extern int trust(char *name);

/* list of probes */
typedef enum
{
PROBE_EMPTY = 1,
#include "../probes.enum"
} probeidx;

#define STANDARD_PROBE_RESULT   \
  char *name; \
  guint color; \
  guint stattime; \
  guint probeid; \
  guint server; \
  char *hostname; \
  char *ipaddress; \
  guint expires; \
  char *message; 

struct probe_result {
  STANDARD_PROBE_RESULT;
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
  char *module_name;
  GHashTable *cache;
  int res_size;
  void (*free_def)(void *def);
  void (*free_res)(void *res);
  int (*init)(void);
  void (*start_run)(void);
  int (*accept_probe)(const char *name);
  void (*xml_result_node)(struct _module *probe, xmlDocPtr, xmlNodePtr, xmlNsPtr, void *res);
  void (*get_from_xml)(struct _module *probe, xmlDocPtr, xmlNodePtr, xmlNsPtr, void *res);
  int  (*fix_result)(struct _module *probe, void *res);
  void *(*get_def)(struct _module *probe, void *res);
  gint (*store_results)(struct _module *probe, void *def, void *res);
  void (*summarize)(void *def, void *res, char *from, char *into, guint slot, guint slotlow, guint slothigh);
} module;

extern module *modules[];

typedef struct transaction {
  module *mod;		// point to the module processing this transaction
  xmlDocPtr doc; 	// the Xml doc structure
  xmlNodePtr node;	// current node in the Xml doc
  xmlNsPtr ns;		// current namespace
  void *res;		// probe result pointer
  void *def;		// probe definition data
} trx;

#define STANDARD_MODULE_STUFF(a) PROBE_##a, #a, NULL, sizeof(struct a##_result)
#endif /* __UW_PROCESS_H */

