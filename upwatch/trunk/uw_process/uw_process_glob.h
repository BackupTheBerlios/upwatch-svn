#ifndef __UW_PROCESS_H
#define __UW_PROCESS_H

#define STANDARD_PROBE_RESULT   \
  guint stamp; \
  guint color; \
  guint stattime; \
  guint probeid; \
  guint server; \
  guint expires; \
  char *message;

struct probe_result {
  STANDARD_PROBE_RESULT;
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

#define STANDARD_MODULE_STUFF(a, b) PROBE_##a, b, NULL
#endif /* __UW_PROCESS_H */

