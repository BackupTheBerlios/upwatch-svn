#include "config.h"
#include <generic.h>
#include <sys/time.h>
#include "tds.h"

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

  if (g_hash_table_size(cache) > 0) {
    if (debug > 0) LOG(LOG_DEBUG, "running %d probes", g_hash_table_size(cache));
    uw_setproctitle("running %d probes", g_hash_table_size(cache));
    run_actual_probes(); /* this runs the actual probes */

    if (debug > 0) LOG(LOG_DEBUG, "writing results");
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

  sprintf(qry,  "SELECT pr_mssql_def.id, "
                "       pr_mssql_def.ipaddress, pr_mssql_def.dbname, "
                "       pr_mssql_def.dbuser, pr_mssql_def.dbpasswd,"
                "       pr_mssql_def.query, "
                "       pr_mssql_def.yellow,  pr_mssql_def.red "
                "FROM   pr_mssql_def "
                "WHERE  pr_mssql_def.id > 1 and pr_mssql_def.disable <> 'yes'"
                "       and pr_mssql_def.pgroup = '%d'",
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
  sprintf(buffer, "%d", probe->id);           xmlSetProp(mssql, "id", buffer);
  sprintf(buffer, "%s", probe->ipaddress);    xmlSetProp(mssql, "ipaddress", buffer);
  sprintf(buffer, "%d", (int) now);           xmlSetProp(mssql, "date", buffer);
  sprintf(buffer, "%d", ((int)now)+((unsigned)OPT_VALUE_EXPIRES*60));
    xmlSetProp(mssql, "expires", buffer);
  sprintf(buffer, "%d", color);               subtree = xmlNewChild(mssql, NULL, "color", buffer);
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
  char **output = STACKLST_OPT(OUTPUT);
  int i;

  xmlDocPtr doc = UpwatchXmlDoc("result");
  g_hash_table_foreach(cache, write_probe, doc);
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
  int rc;

  /* grab a login structure */
  login = (void *) tds_alloc_login();

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
  tds_set_host(login, "myhost");
  tds_set_library(login, "TDS-Library");
  tds_set_server(login, probe->ipaddress);
  tds_set_port(login, 1433);
  tds_set_charset(login, "iso_1");
  tds_set_language(login, "us_english");
  tds_set_packet(login, 512);
  tds_set_passwd(login, probe->dbpasswd);

  gettimeofday(&start, NULL);

  /* open a connection*/
  tds = tds_connect(login, context, NULL);

  gettimeofday(&now, NULL);
  probe->connect = ((float) timeval_diff(&now, &start)) * 0.000001;

  if (!tds) {
    /* FIX ME -- need to hook up message/error handlers */
    probe->msg = strdup("There was a problem connecting to the server");
    goto end;
  }

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
