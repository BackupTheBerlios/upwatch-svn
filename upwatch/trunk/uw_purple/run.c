#include "config.h"
#include <generic.h>
#include <sys/time.h>
#include <sys/types.h>
#include <probe.h>

#include "uw_purple.h"

struct dbspec {
  int domid;
  char *domain;
  char *host;
  int port;
  char *db;
  char *user;
  char *password;
};

int init(void)
{
  daemonize = TRUE;
  every = EVERY_MINUTE;
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

int find_expired_probes(struct dbspec *dbspec)
{
  xmlDocPtr doc;
  xmlNodePtr probe;
  MYSQL_RES *result;
  MYSQL_ROW row;
  MYSQL *db;
  int count = 0;

  db = open_database(dbspec->host, dbspec->port, dbspec->db,
                     dbspec->user, dbspec->password);
  if (!db) return 0;

  doc = UpwatchXmlDoc("result", NULL);

  // find all expired probes, but skip those for which processing
  // has been stopped for some reason
  result = my_query(db, 0, "select probe.name, pr_status.probe, " 
                           "       pr_status.server, pr_status.color, "
                           "       pr_status.expires "
                           "from   pr_status, probe "
                           "where  probe.id = pr_status.class and color <> 400 "
                           "       and expires < UNIX_TIMESTAMP()-30 "
                           "       and expires < probe.lastseen"
                           "       and probe.expiry = 'yes'");
  if (!result) goto errexit;

  while ((row = mysql_fetch_row(result)) != NULL) {
    char buffer[256];
    time_t now = time(NULL);

    probe = xmlNewChild(xmlDocGetRootElement(doc), NULL, row[0], NULL);
    xmlSetProp(probe, "domain", dbspec->domain);
    xmlSetProp(probe, "id", row[1]);
    xmlSetProp(probe, "server", row[2]);
    sprintf(buffer, "%u", (int) now);	xmlSetProp(probe, "date", buffer);
    xmlSetProp(probe, "expires", row[4]);

    xmlNewChild(probe, NULL, "color", "400");  // PURPLE
    xmlNewChild(probe, NULL, "prevcolor", row[3]);

    LOG(LOG_NOTICE, "%s: purpled %s %s", dbspec->domain, row[0], row[1]);
    count++;
  }
  if (count) {
    xmlSetDocCompressMode(doc, OPT_VALUE_COMPRESS);
    spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(OUTPUT), doc, NULL);
    LOG(LOG_NOTICE, "%s: purpled %u probes", dbspec->domain, count);
  }

errexit:
  if (result) mysql_free_result(result);
  if (db) close_database(db);
  if (doc) xmlFreeDoc(doc);
  return count;
}

//************************************************************************
// read pr_status file for expired probes. Construct fake results
// for those.
//***********************************************************************
int run(void)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  MYSQL *upwatch;
  int count = 0;

  upwatch = open_database(OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME),
                        OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
  if (!upwatch) return 0;

  LOG(LOG_INFO, "syncing..");
  uw_setproctitle("reading info from database");
  result = my_query(upwatch, 0, "select pr_domain.id, pr_domain.name, pr_domain.host, "
                                "       pr_domain.port, pr_domain.db, pr_domain.user, "
                                "       pr_domain.password "
                                "from   pr_domain "
                                "where  pr_domain.id > 1");
  if (result) {
    while ((row = mysql_fetch_row(result)) != NULL) {
      struct dbspec db;

      db.domid = atoi(row[0]);
      db.domain = row[1];
      db.host = row[2];
      db.port = atoi(row[3]);
      db.db = row[4];
      db.user = row[5];
      db.password = row[6];
      uw_setproctitle("purpling %s", row[1]);
      LOG(LOG_DEBUG, "purpling %s", row[1]);
      count += find_expired_probes(&db);
    }
    mysql_free_result(result);
  }
  close_database(upwatch);
  LOG(LOG_INFO, "sleeping");
  return count;
}

