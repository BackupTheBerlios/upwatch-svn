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
  guint interval; \
  char *message; 

struct probe_result {
  STANDARD_PROBE_RESULT;
};

#define STANDARD_PROBE_DEF   \
  guint stamp; \
  guint server; \
  char hide[4]; \
  guint contact; \
  guint probeid; \
  guint color; \
  guint changed; \
  guint newest; \
  char email[65]; \
  guint redmins; \
  char notified[4]; \

struct probe_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};

#define STANDARD_MODULE_STUFF(a) PROBE_##a, #a, NULL, NULL, G_STATIC_MUTEX_INIT, \
  NULL, NULL, sizeof(struct a##_result), sizeof(struct a##_def), 0, -1, 0, 0

typedef struct _module {
  int class; 		// numberic probe class (id of record in probe table)
  char *module_name;
  MYSQL *db;		// database handle the methods should use
  GHashTable *cache;	// cached definition records
  GStaticMutex queue_mutex;
  GQueue *queue;	// queued result record 
  GPtrArray *insertc;	// cache for doing multi value insert statements 
  int res_size;		// size of a result record
  int def_size;		// size of a definition record
  int fuse;		// act like a fuse, i.e. once red is always red, until the user resets it.
  int sep;		// group number for separate thread
  int count;		// stats: total handles in this run
  int errors;		// stats: total errors in this run
#define NO_FREE_DEF NULL
  void (*free_def)(void *def);
#define NO_FREE_RES NULL
  void (*free_res)(void *res);
#define NO_INIT NULL
  int (*init)(void);
#define NO_START_RUN NULL
  void (*start_run)(struct _module *probe);
#define NO_ACCEPT_PROBE NULL
  int (*accept_probe)(struct _module *probe, const char *name);
#define NO_XML_RESULT_NODE NULL
  void (*xml_result_node)(struct _module *probe, xmlDocPtr, xmlNodePtr, xmlNsPtr, void *res);
#define NO_GET_FROM_XML NULL
  void (*get_from_xml)(struct _module *probe, xmlDocPtr, xmlNodePtr, xmlNsPtr, void *res);
#define NO_FIX_RESULT NULL
  int  (*fix_result)(struct _module *probe, void *res);
#define NO_GET_DEF NULL
  void *(*get_def)(struct _module *probe, void *res);
#define NO_STORE_RESULTS NULL
  gint (*store_results)(struct _module *probe, void *def, void *res, guint *seen_before);
#define NO_SUMMARIZE NULL
  void (*summarize)(struct _module *probe, void *def, void *res, char *from, char *into, 
                    guint slot, guint slotlow, guint slothigh, gint ignoredupes);
#define NO_END_PROBE NULL
  void (*end_probe)(struct _module *probe, void *def, void *res);
#define NO_END_RUN NULL
  void (*end_run)(struct _module *probe);
} module;

extern module *modules[];

struct summ_spec {
  int period;   // 
  int perslot;  // # of recs per slot in the from table
  char *from;   // summarize from table
  char *to;     // summarize into this table
}; 

typedef struct transaction {
  module *probe;		// point to the module processing this transaction
  void *def;			// probe definition data
  void *loc;			// local data ptr
  struct probe_result *res;	// probe result pointer
} trx;

void notify(module *probe, struct probe_def *def, struct probe_result *res, struct probe_result *prv);
char *query_server_by_id(module *probe, int id);


void mod_ic_add(module *probe, const char *table, const char *str);
void mod_ic_flush(module *probe, const char *table);

/* generic functions */
extern void ct_get_from_xml(module *probe, xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns, void *probe_res);
extern gint ct_store_raw_result(struct _module *probe, void *probe_def, void *probe_res, guint *seen_before);
extern void ct_summarize(module *probe, void *probe_def, void *probe_res, char *from, char *into, 
                         guint slot, guint slotlow, guint slothigh, gint resummarize);

#endif /* __UW_PROCESS_H */

