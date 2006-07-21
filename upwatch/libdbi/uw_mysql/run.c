#include "config.h"
#include <db.h>
#include <generic.h>
#include <sys/time.h>
#include <mysql.h>

#include "uw_mysql.h"

struct probedef {
  int           id;             /* unique probe id */
  int           probeid;        /* server probe id */
  char          *realm;        /* database realm */
  int		seen;           /* seen */
#include "../common/common.h"
#include "probe.def_h"
#include "probe.res_h"
  char		*msg;           /* last error message */
};
GHashTable *cache;

void free_probe(void *probe)
{
  struct probedef *r = (struct probedef *)probe;

  if (r->ipaddress) g_free(r->ipaddress);
  if (r->dbname) g_free(r->dbname);
  if (r->dbuser) g_free(r->dbuser);
  if (r->dbpasswd) g_free(r->dbpasswd);
  if (r->query) g_free(r->query);
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

  if (probe->seen == 0) {
    LOG(LOG_INFO, "removed probe %s:%u from list", probe->realm, probe->probeid);
    return 1;
  }
  probe->seen = 0;
  return(0);
}

int init(void)
{
  if (!HAVE_OPT(OUTPUT)) {
    LOG(LOG_ERR, "missing output option");
    return 0;
  }
  daemonize = TRUE;
  every = EVERY_MINUTE;
  startsec = OPT_VALUE_BEGIN;
  g_thread_init(NULL);
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

void refresh_database(dbi_conn conn);
void run_actual_probes(void);
void write_results(void);

int run(void)
{
  dbi_conn conn;

  if (!cache) {
    cache = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, free_probe);
  }
  
  LOG(LOG_INFO, "reading info from database"); 
  uw_setproctitle("reading info from database");
  conn = open_database(OPT_ARG(DBTYPE), OPT_ARG(DBHOST), OPT_ARG(DBPORT), OPT_ARG(DBNAME), 
			OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
  if (conn) {
    refresh_database(conn);
    close_database(conn);
  }

  if (g_hash_table_size(cache) > 0) {
    LOG(LOG_INFO, "running %d probes", g_hash_table_size(cache));
    uw_setproctitle("running %d probes", g_hash_table_size(cache));
    run_actual_probes(); /* this runs the actual probes */

    LOG(LOG_INFO, "writing results"); 
    uw_setproctitle("writing results");
    write_results();
  }

  return(g_hash_table_size(cache));
}

void refresh_database(dbi_conn conn)
{
  dbi_result result;
  char qry[1024];

  sprintf(qry,  "SELECT pr_mysql_def.id, pr_mysql_def.domid, pr_mysql_def.tblid, pr_realm.name, "
                "       pr_mysql_def.ipaddress, pr_mysql_def.dbname, "
                "       pr_mysql_def.dbuser, pr_mysql_def.dbpasswd,"
                "       pr_mysql_def.query, "
                "       pr_mysql_def.yellow,  pr_mysql_def.red "
                "FROM   pr_mysql_def, pr_realm "
                "WHERE  pr_mysql_def.id > 1 and pr_mysql_def.disable <> 'yes'"
                "       and pr_mysql_def.pgroup = '%d' and pr_realm.id = pr_mysql_def.domid",
                (unsigned)OPT_VALUE_GROUPID);

  result = db_query(conn, 1, qry);
  if (!result) {
    return;
  }
    
  while (dbi_result_next_row(result)) {
    int id;
    struct probedef *probe;

    id = dbi_result_get_uint(result, "id");
    probe = g_hash_table_lookup(cache, &id);
    if (!probe) {
      probe = g_malloc0(sizeof(struct probedef));
      if (dbi_result_get_uint(result, "domid") > 1) {
        probe->probeid = dbi_result_get_uint(result, "tblid");
        strcpy(probe->realm, dbi_result_get_string(result, "name"));
      } else {
        probe->probeid = probe->id;
      }
      g_hash_table_insert(cache, guintdup(id), probe);
    }

    if (probe->ipaddress) g_free(probe->ipaddress);
    probe->ipaddress = dbi_result_get_string_copy(result, "ipaddress");
    if (probe->dbname) g_free(probe->dbname);
    probe->dbname = dbi_result_get_string_copy(result, "dbname");
    if (probe->dbuser) g_free(probe->dbuser);
    probe->dbuser = dbi_result_get_string_copy(result, "dbuser");
    if (probe->dbpasswd) g_free(probe->dbpasswd);
    probe->dbpasswd = dbi_result_get_string_copy(result, "dbpasswd");
    if (probe->query) g_free(probe->query);
    probe->query = dbi_result_get_string_copy(result, "query");
    probe->yellow = dbi_result_get_float(result, "yellow");
    probe->red = dbi_result_get_float(result, "red");
    if (probe->msg) g_free(probe->msg);
    probe->msg = NULL;
    probe->seen = 1;
  }
  if (dbi_conn_error_flag(conn)) {
    const char *errmsg;
    dbi_conn_error(conn, &errmsg);
    LOG(LOG_ERR, "%s", errmsg);
    g_hash_table_foreach(cache, reset_seen, NULL);
  } else {
    g_hash_table_foreach_remove(cache, return_seen, NULL);
  }
  dbi_result_free(result);
}

void probe(gpointer data, gpointer user_data);
void add_probe(gpointer key, gpointer value, gpointer user_data)
{
  GThreadPool *gtpool = (GThreadPool *) user_data;

  g_thread_pool_push(gtpool, value, NULL);
//probe(value, user_data);
}

void run_actual_probes(void)
{
  GThreadPool *gtpool = NULL;

  gtpool = g_thread_pool_new(probe, NULL, 10, TRUE, NULL);

  if (gtpool == NULL) {
    LOG(LOG_ERR, "could not create threadpool");
    return;
  }

  g_hash_table_foreach(cache, add_probe, gtpool);
  g_thread_pool_free (gtpool, FALSE, TRUE);
}

void write_probe(gpointer key, gpointer value, gpointer user_data)
{
  xmlDocPtr doc = (xmlDocPtr) user_data;
  struct probedef *probe = (struct probedef *)value;

  xmlNodePtr subtree, mysql;
  int color;
  char info[1024];
  char buffer[1024];
  time_t now = time(NULL);

  info[0] = 0;
  if (probe->msg) {
    color = STAT_RED;
  } else {
    if (probe->total < probe->yellow) {
      color = STAT_GREEN;
    } else if (probe->total > probe->red) {
      color = STAT_RED;
    } else {
      color = STAT_YELLOW;
    }
  }

  mysql = xmlNewChild(xmlDocGetRootElement(doc), NULL, "mysql", NULL);
  if (probe->realm) {
    xmlSetProp(mysql, "realm", probe->realm);
  }
  sprintf(buffer, "%d", probe->probeid);      xmlSetProp(mysql, "id", buffer);
  sprintf(buffer, "%s", probe->ipaddress);    xmlSetProp(mysql, "ipaddress", buffer);
  sprintf(buffer, "%d", (int) now);           xmlSetProp(mysql, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+((unsigned)OPT_VALUE_EXPIRES*60)); 
    xmlSetProp(mysql, "expires", buffer);
  sprintf(buffer, "%d", color);               xmlSetProp(mysql, "color", buffer);
  sprintf(buffer, "%f", probe->connect);      subtree = xmlNewChild(mysql, NULL, "connect", buffer);
  sprintf(buffer, "%f", probe->total);        subtree = xmlNewChild(mysql, NULL, "total", buffer);
  if (probe->msg) {
    subtree = xmlNewTextChild(mysql, NULL, "info", probe->msg);
    free(probe->msg);
    probe->msg = NULL;
  }
}

void write_results(void)
{
  int ct  = STACKCT_OPT(OUTPUT);
  const char **output = STACKLST_OPT(OUTPUT);
  int i;
  xmlDocPtr doc;

  doc = UpwatchXmlDoc("result", NULL);

  g_hash_table_foreach(cache, write_probe, doc);
  xmlSetDocCompressMode(doc, OPT_VALUE_COMPRESS);
  for (i=0; i < ct; i++) {
    spool_result(OPT_ARG(SPOOLDIR), output[i], doc, NULL);
  }
  xmlFreeDoc(doc);
}

void probe(gpointer data, gpointer user_data) 
{ 
  char *dbhost, *dbuser, *dbpasswd, *dbname;
  struct probedef *probe = (struct probedef *)data;
  MYSQL *mysql = mysql_init(NULL);
  MYSQL_RES *result;
  struct timeval start, now;
  unsigned int option;

  dbhost = probe->ipaddress;
  dbname = probe->dbname;
  dbuser = probe->dbuser;
  dbpasswd = probe->dbpasswd;

  option = 0 ; mysql_options(mysql, MYSQL_OPT_COMPRESS, (const char *)&option);
  option = 50; mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&option);
  gettimeofday(&start, NULL);
  if (!mysql_real_connect(mysql, dbhost, dbuser, dbpasswd, dbname, 0, NULL, 0)) {
    probe->msg = strdup(mysql_error(mysql));
    return;
  }
  gettimeofday(&now, NULL);
  probe->connect = ((float) timeval_diff(&now, &start)) * 0.000001;
  if (mysql_query(mysql, probe->query)) {
    gettimeofday(&now, NULL);
    probe->total = ((float) timeval_diff(&now, &start)) * 0.000001;
    probe->msg = strdup(mysql_error(mysql));
    return;
  }
  result = mysql_store_result(mysql);
  if (mysql_errno(mysql)) {
    probe->msg = strdup(mysql_error(mysql));
  }
  if (mysql_num_rows(result) == 0) {
    probe->msg = strdup("Empty set");
  }
  mysql_free_result(result);
  mysql_close(mysql);
  gettimeofday(&now, NULL);
  probe->total = ((float) timeval_diff(&now, &start)) * 0.000001;
  return;
}
