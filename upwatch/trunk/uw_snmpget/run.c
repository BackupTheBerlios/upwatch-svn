#include "config.h"
#include <generic.h>
#include <sys/time.h>

#include "cmd_options.h"
#include <ucd-snmp/ucd-snmp-config.h>
#include <ucd-snmp/ucd-snmp-includes.h>
#include <ucd-snmp/mib.h>
#define RECEIVED_MESSAGE   1

struct probedef {
  int		id;             /* server probe id */
  int		seen;           /* seen */
  struct snmp_session *sess;    /* snmp_session */
  char		*ipaddress;     /* server name */
#include "probe.def_h"
#include "../common/common.h"
#include "probe.res_h"
  char		*msg;           /* last error message */
};
GHashTable *cache;

void free_probe(void *probe)
{
  struct probedef *r = (struct probedef *)probe;

  if (r->OID) g_free(r->OID);
  if (r->community) g_free(r->community);
  g_free(r);
}

void reset_seen(gpointer key, gpointer value, gpointer user_data)
{
  struct probedef *probe = (struct probedef *)value;

  probe->seen = 0;
}

gboolean return_seen(gpointer key, gpointer value, gpointer user_data)
{
  struct probedef *probe = (struct probedef *)value;

  return(probe->seen == 0);
}

int init(void)
{
  daemonize = TRUE;
  every = EVERY_MINUTE;
  startsec = OPT_VALUE_BEGIN;
  g_thread_init(NULL);
  init_snmp(progname);
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

void refresh_database(void);
void run_actual_probes(void);
void write_results(void);

int run(void)
{
  if (!cache) {
    cache = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, free_probe);
  }
  
  if (debug > 0) LOG(LOG_DEBUG, "reading info from database");
  uw_setproctitle("reading info from database");
  if (open_database() == 0) {
    refresh_database();
  }

  if (debug > 0) LOG(LOG_DEBUG, "running %d probes", g_hash_table_size(cache));
  uw_setproctitle("running %d probes", g_hash_table_size(cache));
  run_actual_probes(); /* this runs the actual probes */

  if (debug > 0) LOG(LOG_DEBUG, "writing results");
  uw_setproctitle("writing results");
  write_results();

  return(g_hash_table_size(cache));
}

void refresh_database(void)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char qry[1024];

  sprintf(qry,  "SELECT pr_snmpget_def.id, %s.%s, "
                "       pr_snmpget_def.ipaddress, pr_snmpget_def.community, "
                "       pr_snmpget_def.passwd, pr_snmpget_def.OID, "
                "       pr_snmpget_def.yellow,  pr_snmpget_def.red "
                "FROM   pr_snmpget_def, server "
                "WHERE  pr_snmpget_def.server = %s.%s and pr_snmpget_def.id > 1",
          OPT_ARG(SERVER_TABLE_NAME), OPT_ARG(SERVER_TABLE_NAME_FIELD),
          OPT_ARG(SERVER_TABLE_NAME), OPT_ARG(SERVER_TABLE_ID_FIELD));

  if (mysql_query(mysql, qry)) {
    LOG(LOG_ERR, "%s: %s", qry, mysql_error(mysql)); 
    close_database();
    return;
  }
  result = mysql_store_result(mysql);
  if (!result) {
    LOG(LOG_ERR, "%s: %s", qry, mysql_error(mysql)); 
    close_database();
    return;
  }
    
  while ((row = mysql_fetch_row(result))) {
    int id;
    struct probedef *probe;

    id = atol(row[0]);
    probe = g_hash_table_lookup(cache, &id);
    if (!probe) {
      probe = g_malloc0(sizeof(struct probedef));
      probe->id = id;
      g_hash_table_insert(cache, guintdup(id), probe);
    }

    if (probe->ipaddress) g_free(probe->ipaddress);
    probe->ipaddress = strdup(row[2]);
    if (probe->community) g_free(probe->community);
    probe->community = strdup(row[3]);
    if (probe->passwd) g_free(probe->passwd);
    probe->passwd = strdup(row[4]);
    if (probe->OID) g_free(probe->OID);
    probe->OID = strdup(row[5]);
    probe->yellow = atof(row[6]);
    probe->red = atof(row[7]);
    if (probe->msg) g_free(probe->msg);
    probe->msg = NULL;
    probe->seen = 1;
  }
  mysql_free_result(result);
  if (mysql_errno(mysql)) {
    g_hash_table_foreach(cache, reset_seen, NULL);
  } else {
    g_hash_table_foreach_remove(cache, return_seen, NULL);
  }
}

static int active_hosts;

/*
 * response handler
 */
int asynch_response(int operation, struct snmp_session *sp, int reqid,
		    struct snmp_pdu *pdu, void *magic)
{
  struct probedef *probe = (struct probedef *)magic;

  if (operation == RECEIVED_MESSAGE) {
    struct variable_list *vp = pdu->variables;

    if (pdu->errstat == SNMP_ERR_NOERROR) {
      char buf[128];

      snprint_value(buf, sizeof(buf), vp->name, vp->name_length, vp);
      switch (vp->type) {
      case ASN_INTEGER:
 	probe->value = (float) *vp->val.integer; 
        break;
      case ASN_FLOAT:
 	probe->value = (float) *vp->val.floatVal; 
        break;
      }
    } else {
      int ix;
      char buf[128], err[256];

      for (ix = 1; vp && ix != pdu->errindex; vp = vp->next_variable, ix++)
        ;
      if (vp) {
        snprint_objid(buf, sizeof(buf), vp->name, vp->name_length);
      } else {
        strcpy(buf, "(none)");
      }
      sprintf(err, "%s: %s: %s", sp->peername, buf, snmp_errstring(pdu->errstat));
      probe->msg = strdup(err);
    }
  } else {
    probe->msg = strdup("Timeout");
  }

  /* something went wrong (or end of variables) 
   * this host not active any more
   */
  active_hosts--;
  return 1;
}

void start_session(gpointer key, gpointer value, gpointer user_data)
{
  struct probedef *probe = (struct probedef *) value; 
  struct snmp_pdu *req;
  struct snmp_session sess;
struct oid {
  const char *Name;
  oid Oid[MAX_OID_LEN];
  int OidLen;
} s_oid;

  s_oid.OidLen = sizeof(s_oid.Oid)/sizeof(s_oid.Oid[0]);
  s_oid.Name = probe->OID;
  if (!snmp_parse_oid(s_oid.Name, s_oid.Oid, &s_oid.OidLen)) {
    probe->msg = strdup(snmp_api_errstring(snmp_errno));
    return;
  }

  snmp_sess_init(&sess);			/* initialize session */
  sess.version = SNMP_VERSION_1;
  sess.peername = probe->ipaddress;
  sess.community = probe->community;
  sess.community_len = strlen(sess.community);
  sess.callback = asynch_response;		/* default callback */
  sess.callback_magic = probe;
  if (!(probe->sess = snmp_open(&sess))) {
    probe->msg = strdup(snmp_api_errstring(snmp_errno));
    return;
  }
  req = snmp_pdu_create(SNMP_MSG_GET);	/* send the first GET */
  snmp_add_null_var(req, s_oid.Oid, s_oid.OidLen);
  if (snmp_send(probe->sess, req)) {
    active_hosts++;
  } else {
    probe->msg = strdup(snmp_api_errstring(snmp_errno));
    snmp_free_pdu(req);
  }
}

void cleanup_session(gpointer key, gpointer value, gpointer user_data)
{
  struct probedef *probe = (struct probedef *) value; 

  if (probe->sess) snmp_close(probe->sess);
}

void run_actual_probes(void)
{
  g_hash_table_foreach(cache, start_session, NULL);
  while (active_hosts) {
    int fds = 0, block = 1;
    fd_set fdset;
    struct timeval timeout;

    FD_ZERO(&fdset);
    snmp_select_info(&fds, &fdset, &timeout, &block);
    fds = select(fds, &fdset, NULL, NULL, block ? NULL : &timeout);
    if (fds) snmp_read(&fdset);
    else snmp_timeout();
  }
  g_hash_table_foreach(cache, cleanup_session, NULL);
}

void write_probe(gpointer key, gpointer value, gpointer user_data)
{
  xmlDocPtr doc = (xmlDocPtr) user_data;
  struct probedef *probe = (struct probedef *)value;

  xmlNodePtr subtree, snmpget;
  int color;
  char info[1024];
  char buffer[1024];
  time_t now = time(NULL);

  info[0] = 0;
  if (probe->msg) {
    color = STAT_RED;
  } else {
    if (probe->value < probe->yellow) {
      color = STAT_GREEN;
    } else if (probe->value > probe->red) {
      color = STAT_RED;
    } else {
      color = STAT_YELLOW;
    }
  }

  snmpget = xmlNewChild(xmlDocGetRootElement(doc), NULL, "snmpget", NULL);
  sprintf(buffer, "%d", probe->id);           xmlSetProp(snmpget, "id", buffer);
  sprintf(buffer, "%d", (int) now);           xmlSetProp(snmpget, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+(2*60));   xmlSetProp(snmpget, "expires", buffer);
  sprintf(buffer, "%d", color);               subtree = xmlNewChild(snmpget, NULL, "color", buffer);
  sprintf(buffer, "%f", probe->value);        subtree = xmlNewChild(snmpget, NULL, "value", buffer);
  if (probe->msg) {
    subtree = xmlNewTextChild(snmpget, NULL, "info", probe->msg);
    free(probe->msg);
    probe->msg = NULL;
  }
}

void write_results(void)
{
  xmlDocPtr doc = UpwatchXmlDoc("result");
  g_hash_table_foreach(cache, write_probe, doc);
  spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(OUTPUT), doc, NULL);
  xmlFreeDoc(doc);
}
