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
void ping_get_from_xml(trx *t)
{
  struct ping_result *res = (struct ping_result *)t->res;

  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "lowest")) && (t->cur->ns == t->ns)) {
    res->lowest = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "value")) && (t->cur->ns == t->ns)) {
    res->value = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "highest")) && (t->cur->ns == t->ns)) {
    res->highest = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
}