#include "config.h"
#include <generic.h>
#include <sys/time.h>
#include <sys/types.h>
#include <probe.h>

#include "uw_purple.h"

struct dbspec {
  int domid;
  const char *realm;
  const char *host;
  int port;
  const char *db;
  const char *user;
  const char *password;
};

int init(void)
{
  if (!HAVE_OPT(OUTPUT)) {
    LOG(LOG_ERR, "missing output option");
    return 0;
  }
  daemonize = TRUE;
  every = EVERY_MINUTE;
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

int find_expired_probes(struct dbspec *dbspec)
{
  xmlDocPtr doc;
  xmlNodePtr probe;
  dbi_result result;
  dbi_conn conn;
  int count = 0;
  char buf[10];

  sprintf(buf, "%d", dbspec->port);

  conn = open_database(OPT_ARG(DBTYPE), dbspec->host, buf, dbspec->db,
                     dbspec->user, dbspec->password);
  if (!conn) return 0;

  doc = UpwatchXmlDoc("result", NULL);

  // find all expired probes, but skip those for which processing
  // has been stopped for some reason
  result = db_query(conn, 0, "select probe.name, pr_status.probe, " 
                             "       pr_status.server, pr_status.color, "
                             "       pr_status.expires "
                             "from   pr_status, probe "
                             "where  probe.id = pr_status.class and color <> 400 "
                             "       and expires < UNIX_TIMESTAMP()-30 "
                             "       and expires < probe.lastseen"
                             "       and probe.expiry = 'yes'");
  if (!result) goto errexit;

  while (dbi_result_next_row(result)) {
    char buffer[256];
    time_t now = time(NULL);

    probe = xmlNewChild(xmlDocGetRootElement(doc), NULL, dbi_result_get_string(result, "name"), NULL);
    xmlSetProp(probe, "realm", dbspec->realm);
    xmlSetProp(probe, "id", dbi_result_get_string(result, "probe"));
    xmlSetProp(probe, "server", dbi_result_get_string(result, "server"));
    sprintf(buffer, "%u", (int) now);	xmlSetProp(probe, "date", buffer);
    xmlSetProp(probe, "expires", dbi_result_get_string(result, "expires"));

    xmlNewChild(probe, NULL, "color", "400");  // PURPLE
    xmlNewChild(probe, NULL, "prevcolor", dbi_result_get_string(result, "color"));

    LOG(LOG_INFO, "%s: purpled %s %d", dbspec->realm, dbi_result_get_string(result, "name"), 
                                                      dbi_result_get_uint(result, "probe"));
    count++;
  }
  if (count) {
    xmlSetDocCompressMode(doc, OPT_VALUE_COMPRESS);
    spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(OUTPUT), doc, NULL);
    LOG(LOG_INFO, "%s: purpled %u probes", dbspec->realm, count);
  }

errexit:
  if (result) dbi_result_free(result);
  if (conn) close_database(conn);
  if (doc) xmlFreeDoc(doc);
  return count;
}

//************************************************************************
// read pr_status file for expired probes. Construct fake results
// for those.
//***********************************************************************
int run(void)
{
  dbi_result result;
  dbi_conn conn;
  int count = 0;

  conn = open_database(OPT_ARG(DBTYPE), OPT_ARG(DBHOST), OPT_ARG(DBPORT), OPT_ARG(DBNAME),
                        OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
  if (!conn) return 0;

  LOG(LOG_INFO, "processing ..");
  uw_setproctitle("reading info from database");
  result = db_query(conn, 0, "select pr_realm.id, pr_realm.name, pr_realm.host, "
                              "       pr_realm.port, pr_realm.db, pr_realm.user, "
                              "       pr_realm.password "
                              "from   pr_realm "
                              "where  pr_realm.id > 1");
  if (result) {
    while (dbi_result_next_row(result)) {
      struct dbspec db;

      db.domid = dbi_result_get_uint(result, "id");
      db.realm = dbi_result_get_string(result, "name");
      db.host = dbi_result_get_string(result, "host");
      db.port = dbi_result_get_uint(result, "port");
      db.db = dbi_result_get_string(result, "db");
      db.user = dbi_result_get_string(result, "user");
      db.password = dbi_result_get_string(result, "password");
      uw_setproctitle("checking %s", dbi_result_get_string(result, "name"));
      LOG(LOG_DEBUG, "checking %s", dbi_result_get_string(result, "name"));
      count += find_expired_probes(&db);
    }
    dbi_result_free(result);
  }
  close_database(conn);
  LOG(LOG_INFO, "sleeping");
  return count;
}

