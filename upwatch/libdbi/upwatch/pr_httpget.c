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
void httpget_get_from_xml(trx *t)
{
  struct httpget_result *res = (struct httpget_result *)t->res;

  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "lookup")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->lookup = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "connect")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->connect = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "pretransfer")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->pretransfer = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "total")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->total = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
}

