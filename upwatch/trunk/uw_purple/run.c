#include "config.h"
#include <generic.h>
#include <sys/time.h>
#include <sys/types.h>
#include <probe.h>

#include "uw_purple.h"

int init(void)
{
  daemonize = TRUE;
  every = EVERY_MINUTE;
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

//************************************************************************
// read pr_status file for expired probes. Construct fake results
// for those.
//***********************************************************************
int run(void)
{
  xmlDocPtr doc;
  xmlNodePtr probe;
  MYSQL_RES *result;
  MYSQL_ROW row;
  MYSQL *db;
  int count = 0;

  doc = UpwatchXmlDoc("result");
  xmlSetDocCompressMode(doc, OPT_VALUE_COMPRESS);

  db = open_database(OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME),
                     OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
  if (!db) goto errexit;

  // find all expired probes, but skip those for which processing
  // has been stopped for some reason
  result = my_query(db, 0, "select probe.name, pr_status.probe, " 
                           "       pr_status.server, pr_status.color "
                           "from   pr_status, probe "
                           "where  probe.id = pr_status.class and color <> 400 "
                           "       and expires < UNIX_TIMESTAMP()-30 "
                           "       and expires < probe.lastseen");
  if (!result) goto errexit;

  while ((row = mysql_fetch_row(result)) != NULL) {
    char buffer[256];
    time_t now = time(NULL);

    probe = xmlNewChild(xmlDocGetRootElement(doc), NULL, row[0], NULL);
    xmlSetProp(probe, "id", row[1]);
    xmlSetProp(probe, "server", row[2]);
    sprintf(buffer, "%u", (int) now);	xmlSetProp(probe, "date", buffer);
    sprintf(buffer, "%u", INT_MAX); 	xmlSetProp(probe, "expires", buffer); // never (well.. in 2038)

    xmlNewChild(probe, NULL, "color", "400");  // PURPLE
    xmlNewChild(probe, NULL, "prevcolor", row[3]);

    count++;
  }
  if (count) spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(OUTPUT), doc, NULL);

errexit:
  if (result) mysql_free_result(result);
  if (db) close_database(db);
  if (doc) xmlFreeDoc(doc);
  return count;

}

