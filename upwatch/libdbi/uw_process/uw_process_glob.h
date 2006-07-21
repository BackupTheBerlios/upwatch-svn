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
#define NO_FIND_DOMAIN NULL
  gint (*find_realm)(trx *t);
#define NO_STORE_RESULTS NULL
  gint (*store_results)(trx *t);
#define NO_NOTIFY_MAIL_SUBJECT_EXTRA NULL
  void (*notify_mail_subject_extra)(trx *t, char *buf, size_t buflen);
#define NO_NOTIFY_MAIL_BODY_PROBE_DEF NULL
  void (*notify_mail_body_probe_def)(trx *t, char *buf, size_t buflen);
#define NO_SUMMARIZE NULL
  void (*summarize)(trx *t, char *from, char *into, 
                    guint slot, guint slotlow, guint slothigh, gint ignoredupes);
};

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

struct dbspec {
  const char *realm;
  const char *host;
  const char *port;
  const char *db;
  const char *user;
  const char *password;
  const char *srvrbyname; // query to retrieve the server id given the server name
  const char *srvrbyid;   // query to retrieve the server name given the server id
  const char *srvrbyip;   // query to retrieve the server id given the ipaddress
  dbi_conn conn;
};
extern struct dbspec *dblist;
extern int dblist_cnt;

int realm_exists(const char *realm);
dbi_conn open_realm(const char *realm);
int realm_server_by_name(const char *realm, const char *name);
char *realm_server_by_id(const char *realm, int id);
int realm_server_by_ip(const char *realm, char *ip);

int mail(const char *to, const char *subject, const char *body, time_t date);

#endif /* __UW_PROCESS_GLOB_H */

