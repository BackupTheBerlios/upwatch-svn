#include "config.h"
#include <generic.h>
#include <sys/time.h>
#include "tds.h"

#include "uw_mssql.h"

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
  if (r->realm) g_free(r->realm);
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

void refresh_database(MYSQL *mysql);
void run_actual_probes(void);
void write_results(void);

int run(void)
{
  MYSQL *mysql;

  if (!cache) {
    cache = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, free_probe);
  }
  
  LOG(LOG_INFO, "reading info from database");
  uw_setproctitle("reading info from database");
  mysql = open_database(OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME), 
			OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
  if (mysql) {
    refresh_database(mysql);
    close_database(mysql);
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

void refresh_database(MYSQL *mysql)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char qry[1024];

  sprintf(qry,  "SELECT pr_mssql_def.id, pr_mssql_def.domid, pr_mssql_def.tblid, pr_realm.name, "
                "       pr_mssql_def.ipaddress, pr_mssql_def.dbname, "
                "       pr_mssql_def.dbuser, pr_mssql_def.dbpasswd,"
                "       pr_mssql_def.query, "
                "       pr_mssql_def.yellow,  pr_mssql_def.red "
                "FROM   pr_mssql_def, pr_realm "
                "WHERE  pr_mssql_def.id > 1 and pr_mssql_def.disable <> 'yes'"
                "       and pr_mssql_def.pgroup = '%d' and pr_realm.id = pr_mssql_def.domid ",
                (unsigned)OPT_VALUE_GROUPID);

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
      if (atoi(row[1]) > 1) {
        probe->probeid = atoi(row[2]);
        probe->realm = strdup(row[3]);
      } else {
        probe->probeid = probe->id;
      }
      g_hash_table_insert(cache, guintdup(id), probe);
    }

    if (probe->ipaddress) g_free(probe->ipaddress);
    probe->ipaddress = strdup(row[4]);
    if (probe->dbname) g_free(probe->dbname);
    probe->dbname = strdup(row[5]);
    if (probe->dbuser) g_free(probe->dbuser);
    probe->dbuser = strdup(row[6]);
    if (probe->dbpasswd) g_free(probe->dbpasswd);
    probe->dbpasswd = strdup(row[7]);
    if (probe->query) g_free(probe->query);
    probe->query = strdup(row[8]);
    probe->yellow = atof(row[9]);
    probe->red = atof(row[10]);
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
  GError *gerror = NULL;

  gtpool = g_thread_pool_new(probe, NULL, 10, TRUE, &gerror);

  if (gtpool == NULL) {
    LOG(LOG_ERR, gerror->message);
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

  xmlNodePtr subtree, mssql;
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

  mssql = xmlNewChild(xmlDocGetRootElement(doc), NULL, "mssql", NULL);
  if (probe->realm) {
    xmlSetProp(mssql, "realm", probe->realm);
  }
  sprintf(buffer, "%d", probe->probeid);      xmlSetProp(mssql, "id", buffer);
  sprintf(buffer, "%s", probe->ipaddress);    xmlSetProp(mssql, "ipaddress", buffer);
  sprintf(buffer, "%d", (int) now);           xmlSetProp(mssql, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+((unsigned)OPT_VALUE_EXPIRES*60));
    xmlSetProp(mssql, "expires", buffer);
  sprintf(buffer, "%d", color);               xmlSetProp(mssql, "color", buffer);
  sprintf(buffer, "%f", probe->connect);      subtree = xmlNewChild(mssql, NULL, "connect", buffer);
  sprintf(buffer, "%f", probe->total);        subtree = xmlNewChild(mssql, NULL, "total", buffer);
  if (probe->msg) {
    subtree = xmlNewTextChild(mssql, NULL, "info", probe->msg);
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

void probe(gpointer data, gpointer user_data) 
{ 
  struct probedef *probe = (struct probedef *)data;
  struct timeval start, now;
  TDSSOCKET *tds;
  TDSLOGIN *login;
  TDSCONTEXT *context;
  TDSCONNECTINFO *connect_info;
  int rc;

  /* grab a login structure */
  login = tds_alloc_login();

  context = tds_alloc_context();
  if( context->locale && !context->locale->date_fmt ) {
    /* set default in case there's no locale file */
    context->locale->date_fmt = strdup("%b %e %Y %l:%M%p");
  }

  /*
  context->msg_handler = tsql_handle_message;
  context->err_handler = tsql_handle_message;
  */

  tds_set_user(login, probe->dbuser);
  tds_set_app(login, "TSQL");
  tds_set_library(login, "TDS-Library");
  tds_set_server(login, probe->ipaddress);
  tds_set_port(login, 1433);
  tds_set_client_charset(login, "ISO-8859-1");
  tds_set_language(login, "us_english");
  tds_set_passwd(login, probe->dbpasswd);

  gettimeofday(&start, NULL);

  /* open a connection*/
  
  tds = tds_alloc_socket(context, 512);
  tds_set_parent(tds, NULL);
  connect_info = tds_read_config_info(NULL, login, context->locale);
  if (!connect_info || tds_connect(tds, connect_info) == TDS_FAIL) {
    gettimeofday(&now, NULL);
    probe->connect = ((float) timeval_diff(&now, &start)) * 0.000001;
    tds_free_connect(connect_info);
    probe->msg = strdup("There was a problem connecting to the server");
    goto end;
  }
  tds_free_connect(connect_info);

  gettimeofday(&now, NULL);
  probe->connect = ((float) timeval_diff(&now, &start)) * 0.000001;

  rc = tds_submit_query(tds, probe->query);
  if (rc != TDS_SUCCEED) {
    probe->msg = strdup("tds_submit_query() failed");
  }

end:
  tds_free_socket(tds);
  tds_free_login(login);
  tds_free_context(context);

  gettimeofday(&now, NULL);
  probe->total = ((float) timeval_diff(&now, &start)) * 0.000001;
  return;
}
