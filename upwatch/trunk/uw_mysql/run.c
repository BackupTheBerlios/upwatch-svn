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

  if (r->dbname) g_free(r->dbname);
  if (r->dbuser) g_free(r->dbuser);
  if (r->dbpasswd) g_free(r->dbpasswd);
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

void refresh_database(void);
int run_actual_probes(void);

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

  if (debug > 0) LOG(LOG_DEBUG, "running probes");
  uw_setproctitle("running %d probes", g_hash_table_size(cache));
  run_actual_probes(); /* this runs the actual probes */
  if (debug > 0) LOG(LOG_DEBUG, "done running probes");
  return(g_hash_table_size(cache));
}

void refresh_database(void)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char qry[1024];

  sprintf(qry,  "SELECT pr_mysql_def.id, %s.%s, "
                "       pr_mysql_def.ipaddress, pr_mysql_def.dbname, "
                "       pr_mysql_def.dbuser, pr_mysql_def.dbpasswd,"
                "       pr_mysql_def.query, "
                "       pr_mysql_def.yellow,  pr_mysql_def.red "
                "FROM   pr_mysql_def, server "
                "WHERE  pr_mysql_def.server = %s.%s and pr_mysql_def.id > 1",
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
    if (probe->dbname) g_free(probe->dbname);
    probe->dbname = strdup(row[3]);
    if (probe->dbuser) g_free(probe->dbuser);
    probe->dbuser = strdup(row[4]);
    if (probe->dbpasswd) g_free(probe->dbpasswd);
    probe->dbpasswd = strdup(row[5]);
    if (probe->query) g_free(probe->query);
    probe->query = strdup(row[6]);
    probe->yellow = atof(row[7]);
    probe->red = atof(row[8]);
    if (probe->msg) g_free(probe->msg);
    probe->msg = NULL;
    probe->seen = 1;
  }
  mysql_free_result(result);
  if (mysql_errno(mysql)) {
    g_hash_table_foreach(cache, reset_seen, NULL);
    return;
  }
  g_hash_table_foreach_remove(cache, return_seen, NULL);
}

void add_probe(gpointer key, gpointer value, gpointer user_data)
{
  GThreadPool *gtpool = (GThreadPool *) user_data;

  g_thread_pool_push(gtpool, value, NULL);
}

void probe(gpointer data, gpointer user_data);
int run_actual_probes(void)
{
  GThreadPool *gtpool;

  gtpool = g_thread_pool_new(probe, NULL, 10, TRUE, NULL);

  if (gtpool == NULL) {
    //printf("%s\n", gerror->message);
    //LOG(LOG_ERR, gerror->message);
    fprintf(stderr, "could not create threadpool\n");
    return(0);
  }
  g_hash_table_foreach(cache, add_probe, gtpool);
  g_thread_pool_free (gtpool, FALSE, TRUE);
  return(0);
}

void probe(gpointer data, gpointer user_data) 
{ 
  char *dbhost, *dbuser, *dbpasswd, *dbname;
  struct probedef *probe = (struct probedef *)data;
  MYSQL_RES *result;
  struct timeval start, now;

  if ((mysql = (MYSQL *)malloc(sizeof(MYSQL))) == NULL) {
    LOG(LOG_ERR, "malloc: %m");
    return;
  }

  dbhost = probe->ipaddress;
  dbname = probe->dbname;
  dbuser = probe->dbuser;
  dbpasswd = probe->dbpasswd;

  mysql_init(mysql);
  mysql_options(mysql,MYSQL_OPT_COMPRESS,0);
  gettimeofday(&start, NULL);
  if (!mysql_real_connect(mysql, dbhost, dbuser, dbpasswd, dbname, 0, NULL, 0)) {
    probe->msg = strdup(mysql_error(mysql));
    return;
  }
  gettimeofday(&now, NULL);
  probe->connect = (float) timeval_diff(&now, &start);
  if (mysql_query(mysql, probe->query)) {
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
  probe->total = (float) timeval_diff(&now, &start);
  return;
}
