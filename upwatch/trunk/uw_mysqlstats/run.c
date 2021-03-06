#include "config.h"
#include <generic.h>
#include <db.h>
#include <sys/time.h>
#include <mysql.h>
#include <mysqld_error.h>

#include "uw_mysqlstats.h"

struct probedef {
  int           id;             /* unique probe id */
  int           probeid;        /* server probe id */
  char          *realm;        /* database realm */
  int		seen;           /* seen */
#include "../common/common.h"
#include "probe.def_h"
#include "probe.res_h"
  char		*msg;           /* last error message */
  long long      abs_selectq;
  long long      abs_insertq;
  long long      abs_updateq;
  long long      abs_deleteq;
};
GHashTable *cache;

void free_probe(void *probe)
{
  struct probedef *r = (struct probedef *)probe;

  if (r->ipaddress) g_free(r->ipaddress);
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

void refresh_database(database *db);
void run_actual_probes(void);
void write_results(void);

int run(void)
{
  database *db;

  if (!cache) {
    cache = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, free_probe);
  }
  
  LOG(LOG_INFO, "reading info from database (group %u)", (unsigned)OPT_VALUE_GROUPID); 
  uw_setproctitle("reading info from database (group %u)", (unsigned)OPT_VALUE_GROUPID);
  db = open_database(OPT_ARG(DBTYPE), OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME), 
			OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
  if (db) {
    refresh_database(db);
    close_database(db);
  }

  if (g_hash_table_size(cache) > 0) {
    LOG(LOG_INFO, "running %d probes from group %u", g_hash_table_size(cache), (unsigned)OPT_VALUE_GROUPID);
    uw_setproctitle("running %d probes from group %u", g_hash_table_size(cache), (unsigned)OPT_VALUE_GROUPID);
    run_actual_probes(); /* this runs the actual probes */

    LOG(LOG_INFO, "writing results"); 
    uw_setproctitle("writing results");
    write_results();
  }

  return(g_hash_table_size(cache));
}

void refresh_database(database *db)
{
  dbi_result result;
  char qry[1024];
  char *error;

  sprintf(qry,  "SELECT pr_mysqlstats_def.id, pr_mysqlstats_def.domid, pr_mysqlstats_def.tblid, pr_realm.name, "
                "       pr_mysqlstats_def.ipaddress, pr_mysqlstats_def.dbname, "
                "       pr_mysqlstats_def.dbuser, pr_mysqlstats_def.dbpasswd,"
                "       pr_mysqlstats_def.yellow,  pr_mysqlstats_def.red "
                "FROM   pr_mysqlstats_def, pr_realm "
                "WHERE  pr_mysqlstats_def.id > 1 and pr_mysqlstats_def.disable <> 'yes'"
                "       and pr_mysqlstats_def.pgroup = '%d' and pr_realm.id = pr_mysqlstats_def.domid",
                (unsigned)OPT_VALUE_GROUPID);

  result = db_query(db, 1, qry);
  if (!result) {
    return;
  }
    
  while (dbi_result_next_row(result)) {
    int id;
    struct probedef *probe;
    
    id = dbi_result_get_int(result, "id");
    probe = g_hash_table_lookup(cache, &id);
    if (!probe) {
      probe = g_malloc0(sizeof(struct probedef));
      probe->id = id;
      if (dbi_result_get_int(result, "domid") > 1) {
        probe->probeid = dbi_result_get_int(result, "tblid");
        probe->realm = dbi_result_get_string_copy(result, "name");
      } else {
        probe->probeid = probe->id;
      }
      //fprintf(stderr, "Adding %u\n", id);
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
    probe->yellow = dbi_result_get_float(result, "yellow");
    probe->red = dbi_result_get_float(result, "yellow");
    if (probe->msg) g_free(probe->msg);
    probe->msg = NULL;
    probe->seen = 1;
  }
  dbi_result_free(result);
  if (dbi_conn_error(db, &error) == DBI_ERROR_NONE) {
    g_hash_table_foreach_remove(cache, return_seen, NULL);
  } else {
    LOG(LOG_ERR, "%s", error);
    g_hash_table_foreach(cache, reset_seen, NULL);
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

  xmlNodePtr subtree, mysqlstats;
  int color;
  char info[1024];
  char buffer[1024];
  time_t now = time(NULL);

  info[0] = 0;
  if (probe->msg) {
    color = STAT_RED;
  } else {
    color = STAT_GREEN;
  }

  mysqlstats = xmlNewChild(xmlDocGetRootElement(doc), NULL, "mysqlstats", NULL);
  if (probe->realm) {
    xmlSetProp(mysqlstats, "realm", probe->realm);
  }
  sprintf(buffer, "%d", probe->probeid);      xmlSetProp(mysqlstats, "id", buffer);
  sprintf(buffer, "%s", probe->ipaddress);    xmlSetProp(mysqlstats, "ipaddress", buffer);
  sprintf(buffer, "%d", (int) now);           xmlSetProp(mysqlstats, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+((unsigned)OPT_VALUE_EXPIRES*60)); 
    xmlSetProp(mysqlstats, "expires", buffer);
  sprintf(buffer, "%d", color);               xmlSetProp(mysqlstats, "color", buffer);
  sprintf(buffer, "%llu", probe->selectq);      subtree = xmlNewChild(mysqlstats, NULL, "selectq", buffer);
  sprintf(buffer, "%llu", probe->insertq);      subtree = xmlNewChild(mysqlstats, NULL, "insertq", buffer);
  sprintf(buffer, "%llu", probe->updateq);      subtree = xmlNewChild(mysqlstats, NULL, "updateq", buffer);
  sprintf(buffer, "%llu", probe->deleteq);      subtree = xmlNewChild(mysqlstats, NULL, "deleteq", buffer);
  if (probe->msg) {
    subtree = xmlNewTextChild(mysqlstats, NULL, "info", probe->msg);
    free(probe->msg);
    probe->msg = NULL;
  }
}

void write_results(void)
{
  int ct  = STACKCT_OPT(OUTPUT);
  char **output = (char **) &STACKLST_OPT(OUTPUT);
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

long long qdiff(long long *previous, long long new)
{
  long long retval = 0;

  if (*previous > 0) { // not the first time?
    retval = new - *previous; // we assume that long long values do not wrap - ever.
  }
  *previous = new;
  return retval;
}

void probe(gpointer data, gpointer user_data) 
{ 
  char *dbhost, *dbuser, *dbpasswd, *dbname;
  struct probedef *probe = (struct probedef *)data;
  MYSQL *mysql = mysql_init(NULL);
  MYSQL_RES *result;
  unsigned int timeout = 50;

  dbhost = probe->ipaddress;
  dbname = probe->dbname;
  dbuser = probe->dbuser;
  dbpasswd = probe->dbpasswd;

  mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, (char * ) &timeout);

  LOG(LOG_DEBUG, "%s %s %s %s", dbhost, dbuser, dbpasswd, dbname);
  if (!mysql_real_connect(mysql, dbhost, dbuser, dbpasswd, dbname, 0, NULL, 0)) {
    probe->msg = strdup(mysql_error(mysql));
    return;
  }
  if (mysql_query(mysql, "show status")) {
    probe->msg = strdup(mysql_error(mysql));
    return;
  }
  if (mysql_errno(mysql)) {
    probe->msg = strdup(mysql_error(mysql));
    goto err_close;
  }
  result = mysql_store_result(mysql);
  if (mysql_num_rows(result) == 0) {
    probe->msg = strdup("Empty set");
  } else {
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(result))) {
      if (strcmp(row[0], "Com_select") == 0) {
        probe->selectq = qdiff(&probe->abs_selectq, atoll(row[1]));
      } else if (strcmp(row[0], "Com_update") == 0) {
        probe->updateq = qdiff(&probe->abs_updateq, atoll(row[1]));
      } else if (strcmp(row[0], "Com_insert") == 0) {
        probe->insertq = qdiff(&probe->abs_insertq, atoll(row[1]));
      } else if (strcmp(row[0], "Com_delete") == 0) {
        probe->deleteq = qdiff(&probe->abs_deleteq, atoll(row[1]));
      }
    }
  }
  mysql_free_result(result);
err_close:
  mysql_close(mysql);
  return;
}
