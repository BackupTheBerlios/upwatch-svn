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
  gint (*find_domain)(trx *t);
#define NO_STORE_RESULTS NULL
  gint (*store_results)(trx *t);
#define NO_NOTIFY_MAIL_SUBJECT NULL
  gint (*notify_mail_subject)(trx *t, FILE *out, char *servername);
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

struct dbspec {
  char *domain;
  char *host;
  int port;
  char *db;
  char *user;
  char *password;
  char *srvrbyname; // query to retrieve the server id given the server name
  char *srvrbyid;   // query to retrieve the server name given the server id
  char *srvrbyip;   // query to retrieve the server id given the ipaddress
  MYSQL *mysql;
};
extern struct dbspec *dblist;
extern int dblist_cnt;

MYSQL *open_domain(char *domain);
int domain_server_by_name(char *domain, char *name);
char *domain_server_by_id(char *domain, int id);
int domain_server_by_ip(char *domain, char *ip);

int mail(char *to, char *subject, char *body, time_t date);

#endif /* __UW_PROCESS_GLOB_H */

