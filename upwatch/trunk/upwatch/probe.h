#ifndef __UW_PROBE_H
#define __UW_PROBE_H

#define STANDARD_PROBE_RESULT   \
  char *name; \
  guint color; \
  guint prevcolor; \
  guint stattime; \
  guint probeid; \
  guint server; \
  char *hostname; \
  char *ipaddress; \
  guint expires; \
  guint received; \
  guint interval; \
  guint changed; \
  char *proto; \
  char *target; \
  long long prevhistid; \
  guint prevhistcolor; \
  char notified[4]; \
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
  guint newest; \
  char email[65]; \
  guint delay; \

struct probe_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};

typedef struct _module module;

typedef struct transaction {
  module *probe;		// point to the module processing this transaction
  int doc_count;
  xmlDocPtr doc;		// points to current XML document
  xmlNsPtr ns;			// points to namespace
  xmlNodePtr cur;		// points to current result node in document
  int failed_count;
  xmlDocPtr failed;		// points to current XML document holding failed nodes
  struct probe_def *def;	// probe definition data
  struct probe_result *res;	// probe result pointer
  guint seen_before;		// true if this is a dupe result
  int (*process)(struct transaction *t);
} trx;

#define STANDARD_MODULE_STUFF(a) PROBE_##a, #a, NULL, NULL, \
  NULL, sizeof(struct a##_result), sizeof(struct a##_def), 0, 0, 0

#define STANDARD_MODULE_STRUCT \
  int class;					/* numberic probe class (id of record in probe table) */ \
  char *module_name; \
  MYSQL *db;					/* database handle the methods should use */ \
  GHashTable *cache;				/* cached definition records */ \
  GPtrArray *insertc;				/* cache for doing multi value insert statements */ \
  int res_size;					/* size of a result record */ \
  int def_size;					/* size of a definition record */ \
  int fuse;					/* be a fuse, (once red is always red, until the user resets it) */ \
  int count;					/* stats: total handles in this run */ \
  int errors;					/* stats: total errors in this run */ \
  void (*free_def)(void *def);			/* free the probe_def record for this probe */ \
  void (*free_res)(void *res);			/* free the probe_result record for this probe */ \
  int (*init)(void); 				/* called once at program startup */ \
  void (*start_run)(module *probe);		/* called once before every run */ \
  int (*accept_probe)(trx *t, const char *name);/* module wants to process this probe? */  \
  void (*xml_result_node)(trx *t);		/* process properties of the "result" node  */ \
  void (*get_from_xml)(trx *t);			/* process one child of the "result" node */ \
  int (*accept_result)(trx *t);			/* accept (and maybe convert) result */ \
  void *(*get_def)(trx *t, int create);		/* retrieve probe definition */ \
  void (*adjust_result)(trx *t);		/* adjust result: usually compute our own colors */ \
  int (*end_result)(trx *t);                    /* maybe do some cleanup for this result */  \
  void (*end_run)(module *probe);		/* called once at end of every run */ \
  void (*exit)(void);				/* called once at program exit */ 

#define NO_FREE_DEF NULL
#define NO_FREE_RES NULL
#define NO_INIT NULL
#define NO_START_RUN NULL
#define NO_ACCEPT_PROBE NULL
#define NO_XML_RESULT_NODE NULL
#define NO_GET_FROM_XML NULL
#define NO_ACCEPT_RESULT NULL
#define NO_GET_DEF NULL
#define NO_ADJUST_RESULT NULL
#define NO_END_RESULT NULL
#define NO_END_RUN NULL
#define NO_EXIT NULL

#ifdef _UPWATCH
struct _module {
  STANDARD_MODULE_STRUCT
};
#endif

extern module *modules[];

int set_result_value(trx *t, char *name, char *value);
int extract_info_from_xml(trx *t);
int accept_result(trx *t);
void *get_def(trx *t, int create);
void *get_def_by_servid(trx *t, int create);
int handle_result_file(gpointer data, gpointer user_data);

void ct_get_from_xml(trx *t);

/****************************** probe bb ************************/
struct bb_result {
  STANDARD_PROBE_RESULT;
  char *bbname;
};
struct bb_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};

extern char *query_server_by_name;
void bb_free_res(void *res);
void bb_xml_result_node(trx *t);
int bb_accept_result(module *probe, void *probe_res);
void *bb_get_def(trx *t, int create);

/****************************** probe bb_cpu ************************/
struct bb_cpu_result {
  STANDARD_PROBE_RESULT;
#include "../uw_acceptbb/probe.res_h"
};
struct bb_cpu_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};
extern module bb_cpu_module;

int bb_cpu_accept_result(trx *t);
void *bb_cpu_get_def(trx *t, int create);

/****************************** probe diskfree ************************/
struct diskfree_result {
  STANDARD_PROBE_RESULT;
};
struct diskfree_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};

/****************************** probe errlog ************************/
struct errlog_result {
  STANDARD_PROBE_RESULT;
};
struct errlog_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};

/****************************** probe httpget ************************/
struct httpget_result {
  STANDARD_PROBE_RESULT;
#include "../uw_httpget/probe.res_h"
};
struct httpget_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};

void httpget_get_from_xml(trx *t);

/****************************** probe imap ************************/
struct imap_result {
  STANDARD_PROBE_RESULT;
#include "../uw_imap/probe.res_h"
};
struct imap_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};

/****************************** probe iptraf ************************/
struct iptraf_result {
  STANDARD_PROBE_RESULT;
  struct in_addr ipaddr;
#include "../uw_iptraf/probe.res_h"
};

struct iptraf_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
  float slotday_in;   // in-memory counters for the current slot in the
  float slotday_out;  // pr_iptraf_day table. To speed things up a bit.
  guint slotday_max_color;  // same with color
  float slotday_avg_yellow; //
  float slotday_avg_red;    //
};

extern char *query_server_by_ip;
void iptraf_xml_result_node(trx *t);
void iptraf_get_from_xml(trx *t);
void *iptraf_get_def(trx *t, int create);
void iptraf_adjust_result(trx *t);

/****************************** probe mssql ************************/
struct mssql_result {
  STANDARD_PROBE_RESULT;
#include "../uw_mssql/probe.res_h"
};
struct mssql_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};

/****************************** probe mysql ************************/
struct mysql_result {
  STANDARD_PROBE_RESULT;
#include "../uw_mysql/probe.res_h"
};
struct mysql_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};

/****************************** probe ping ************************/
struct ping_result {
  STANDARD_PROBE_RESULT;
#include "../uw_ping/probe.res_h"
};
struct ping_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};

void ping_get_from_xml(trx *t);

/****************************** probe pop3 ************************/
struct pop3_result {
  STANDARD_PROBE_RESULT;
#include "../uw_pop3/probe.res_h"
};
struct pop3_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};

/****************************** probe postgresql ************************/
struct postgresql_result {
  STANDARD_PROBE_RESULT;
#include "../uw_postgresql/probe.res_h"
};
struct postgresql_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};

/****************************** probe snmpget ************************/
struct snmpget_result {
  STANDARD_PROBE_RESULT;
#include "../uw_snmpget/probe.res_h"
};
struct snmpget_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};

void snmpget_get_from_xml(trx *t);

/****************************** probe sysstat ************************/
struct sysstat_result {
  STANDARD_PROBE_RESULT;
#include "../uw_sysstat/probe.res_h"
};
struct sysstat_def {
  STANDARD_PROBE_DEF;
#include "../common/common.h"
};

void sysstat_get_from_xml(trx *t);
void *sysstat_get_def(trx *t, int create);
void sysstat_adjust_result(trx *t);


#endif /* __UW_PROBE_H */

