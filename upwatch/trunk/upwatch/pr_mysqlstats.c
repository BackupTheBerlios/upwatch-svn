#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#define _UPWATCH
#include "probe.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//*******************************************************************
void mysqlstats_get_from_xml(trx *t)
{
  struct mysqlstats_result *res = (struct mysqlstats_result *)t->res;

  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "selectq")) && (t->cur->ns == t->ns)) {
    res->selectq = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "insertq")) && (t->cur->ns == t->ns)) {
    res->insertq = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "updateq")) && (t->cur->ns == t->ns)) {
    res->updateq = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "deleteq")) && (t->cur->ns == t->ns)) {
    res->deleteq = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
}
