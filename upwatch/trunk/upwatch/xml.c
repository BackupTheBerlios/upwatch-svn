#include "config.h"
#include "generic.h"
#include <unistd.h>

#ifdef DMALLOC 
#include "dmalloc.h"
#endif

void UpwatchXmlGenericErrorFunc(void *ctx, const char *fmt, ...)
{
  char buffer[BUFSIZ];
  va_list arg;

  if (fmt == NULL) fmt = "(null)";

  va_start(arg, fmt);
  vsnprintf(buffer, BUFSIZ, fmt, arg);
  va_end(arg);

  LOG(LOG_WARNING, buffer);
}

xmlDocPtr UpwatchXmlDoc(const char *root, char *fromhost)
{
  xmlDocPtr doc;
  xmlNodePtr tree;
  xmlNsPtr uwns;
  char buf[256];
  time_t now = time(NULL);

  xmlKeepBlanksDefault(0);
  doc = xmlNewDoc((unsigned char *)"1.0");
  xmlCreateIntSubset(doc, root, NULL, PATH_RESULT_DTD);

  tree = xmlNewChild((xmlNodePtr)doc, NULL, root, NULL);
  if (fromhost && fromhost[0]) {
    xmlSetProp(tree, "fromhost", fromhost);
  } else {
    if (gethostname(buf, sizeof(buf))) {
      strcpy(buf, "localhost"); // we have to have something
    }
    xmlSetProp(tree, "fromhost", buf);
  }
  sprintf(buf, "%u", (unsigned) now);
  xmlSetProp(tree, "date", buf);
  uwns = xmlNewNs(tree, (xmlChar*) NAMESPACE_URL, (xmlChar*) NULL);
		    
  return doc;
}

gfloat xmlNodeListGetFloat(xmlDocPtr doc, xmlNodePtr list, int inLine)
{
  gfloat ret = 0;
  xmlChar *tmp = xmlNodeListGetString(doc, list, inLine);

  if (tmp) {
    ret = atof(tmp);
    xmlFree(tmp);
  }
  return(ret);
}

gint xmlNodeListGetInt(xmlDocPtr doc, xmlNodePtr list, int inLine)
{
  gint ret = 0;
  xmlChar *tmp = xmlNodeListGetString(doc, list, inLine);

  if (tmp) {
    ret = atoi(tmp);
    xmlFree(tmp);
  }
  return(ret);
}

glong xmlNodeListGetLong(xmlDocPtr doc, xmlNodePtr list, int inLine)
{
  glong ret = 0;
  xmlChar *tmp = xmlNodeListGetString(doc, list, inLine);

  if (tmp) {
    ret = atol(tmp);
    xmlFree(tmp);
  }
  return(ret);
}

guint xmlGetPropUnsigned(xmlNodePtr node, const xmlChar *name)
{
  guint ret = 0;
  xmlChar *tmp = xmlGetProp(node, name);

  if (tmp) {
    ret = strtoul(tmp, (char **)NULL, 10);;
    xmlFree(tmp);
  }
  return(ret);
}

gint xmlGetPropInt(xmlNodePtr node, const xmlChar *name)
{
  gint ret = 0;
  xmlChar *tmp = xmlGetProp(node, name);

  if (tmp) {
    ret = atoi(tmp);
    xmlFree(tmp);
  }
  return(ret);
}

glong xmlGetPropLong(xmlNodePtr node, const xmlChar *name)
{
  glong ret = 0;
  xmlChar *tmp = xmlGetProp(node, name);

  if (tmp) {
    ret = atol(tmp);
    xmlFree(tmp);
  }
  return(ret);
}

gfloat xmlGetPropFloat(xmlNodePtr node, const xmlChar *name)
{
  gfloat ret = 0;
  xmlChar *tmp = xmlGetProp(node, name);

  if (tmp) {
    ret = atof(tmp);
    xmlFree(tmp);
  }
  return(ret);
}

