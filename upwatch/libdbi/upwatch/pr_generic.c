#include "config.h"
#include <string.h>
#include <generic.h>
#include "slot.h"
#define _UPWATCH
#include "probe.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct ct_result {
  STANDARD_PROBE_RESULT;
  float connect;	/* time for connection to complete */
  float total;		/* total time needed */
};

//*******************************************************************
// GET THE INFO FROM THE XML FILE
// Caller must free the pointer it returns
//*******************************************************************
void ct_get_from_xml(trx *t)
{
  struct ct_result *res = (struct ct_result *)t->res;

  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "connect")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->connect = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
  if ((!xmlStrcmp(t->cur->name, (const xmlChar *) "total")) && (xmlNsEqual(t->cur->ns, t->ns))) {
    res->total = xmlNodeListGetFloat(t->doc, t->cur->xmlChildrenNode, 1);
    return;
  }
}

