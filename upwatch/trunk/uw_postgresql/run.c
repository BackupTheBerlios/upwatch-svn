#include "config.h"
#include <generic.h>
#include <sys/time.h>

#include "cmd_options.h"

struct probedef {
  int		id;             /* server probe id */
  int		seen;           /* seen */
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

  if (r->ipaddress) g_free(r->ipaddress);
  if (r->dbname) g_free(r->dbname);
  if (r->dbuser) g_free(r->dbuser);
  if (r->dbpasswd) g_free(r->dbpasswd);
  if (r->query) g_free(r->query);
  if (r->msg) g_free(r->msg);
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
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

void refresh_database(MYSQL *mysql);
void run_actual_probes(void);
void write_results(void);

int run(void)
{
  MYSQL *mysql;

  if (!cache) {
    cache = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, free_probe);
  }
  
  if (debug > 0) LOG(LOG_DEBUG, "reading info from database");
  uw_setproctitle("reading info from database");
  mysql = open_database(OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME), 
			OPT_ARG(DBUSER), OPT_ARG(DBPASSWD), OPT_VALUE_DBCOMPRESS);
  if (mysql) {
    refresh_database(mysql);
    close_database(mysql);
  }

  if (debug > 0) LOG(LOG_DEBUG, "running %d probes", g_hash_table_size(cache));
  uw_setproctitle("running %d probes", g_hash_table_size(cache));
  run_actual_probes(); /* this runs the actual probes */

  if (debug > 0) LOG(LOG_DEBUG, "writing results");
  uw_setproctitle("writing results");
  write_results();

  return(g_hash_table_size(cache));
}

void refresh_database(MYSQL *mysql)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char qry[1024];

  sprintf(qry,  "SELECT pr_postgresql_def.id, "
                "       pr_postgresql_def.ipaddress, pr_postgresql_def.dbname, "
                "       pr_postgresql_def.dbuser, pr_postgresql_def.dbpasswd,"
                "       pr_postgresql_def.query, "
                "       pr_postgresql_def.yellow,  pr_postgresql_def.red "
                "FROM   pr_postgresql_def "
                "WHERE  pr_postgresql_def.id > 1 and pr_postgresql_def.pgroup = '%d'",
                OPT_VALUE_GROUPID);

  result = my_query(mysql, 1, qry);
  if (!result) {
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
    probe->ipaddress = strdup(row[1]);
    if (probe->dbname) g_free(probe->dbname);
    probe->dbname = strdup(row[2]);
    if (probe->dbuser) g_free(probe->dbuser);
    probe->dbuser = strdup(row[3]);
    if (probe->dbpasswd) g_free(probe->dbpasswd);
    probe->dbpasswd = strdup(row[4]);
    if (probe->query) g_free(probe->query);
    probe->query = strdup(row[5]);
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
    //printf("%s\n", gerror->message);
    //LOG(LOG_ERR, gerror->message);
    fprintf(stderr, "could not create threadpool\n");
    return;
  }

  g_hash_table_foreach(cache, add_probe, gtpool);
  g_thread_pool_free (gtpool, FALSE, TRUE);
}

void write_probe(gpointer key, gpointer value, gpointer user_data)
{
  xmlDocPtr doc = (xmlDocPtr) user_data;
  struct probedef *probe = (struct probedef *)value;

  xmlNodePtr subtree, postgresql;
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

  postgresql = xmlNewChild(xmlDocGetRootElement(doc), NULL, "postgresql", NULL);
  sprintf(buffer, "%d", probe->id);           xmlSetProp(postgresql, "id", buffer);
  sprintf(buffer, "%s", probe->ipaddress);    xmlSetProp(postgresql, "ipaddress", buffer);
  sprintf(buffer, "%d", (int) now);           xmlSetProp(postgresql, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+(2*60));   xmlSetProp(postgresql, "expires", buffer);
  sprintf(buffer, "%d", color);               subtree = xmlNewChild(postgresql, NULL, "color", buffer);
  sprintf(buffer, "%f", probe->connect);      subtree = xmlNewChild(postgresql, NULL, "connect", buffer);
  sprintf(buffer, "%f", probe->total);        subtree = xmlNewChild(postgresql, NULL, "total", buffer);
  if (probe->msg) {
    subtree = xmlNewTextChild(postgresql, NULL, "info", probe->msg);
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

#include <libpq-fe.h>

void probe(gpointer data, gpointer user_data) 
{ 
  char *dbhost, *dbuser, *dbpasswd, *dbname;
  struct probedef *probe = (struct probedef *)data;
  PGconn *conn;
  PGresult *res;
  struct timeval start, now;

  dbhost = probe->ipaddress;
  dbname = probe->dbname;
  dbuser = probe->dbuser;
  dbpasswd = probe->dbpasswd;

  gettimeofday(&start, NULL);

  conn = PQsetdbLogin(dbhost, NULL, NULL, NULL, dbname, dbuser, dbpasswd);
  if (PQstatus(conn) == CONNECTION_BAD) {
    probe->msg = strdup(PQerrorMessage(conn));
    return;
  }
  gettimeofday(&now, NULL);
  probe->connect = ((float) timeval_diff(&now, &start)) * 0.000001;
  res = PQexec(conn, probe->query);
  if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
    probe->msg = strdup(PQerrorMessage(conn));
    goto exit;
  }
  if (PQntuples(res) == 0) {
    probe->msg = strdup("Empty set");
  }

exit:
  PQclear(res);
  PQfinish(conn);
  gettimeofday(&now, NULL);
  probe->total = ((float) timeval_diff(&now, &start)) * 0.000001;
  return;
}
