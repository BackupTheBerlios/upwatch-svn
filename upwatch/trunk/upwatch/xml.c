#include "generic.h"

xmlDocPtr UpwatchXmlDoc(const char *root)
{
  xmlDocPtr doc;
  xmlNodePtr tree;
  xmlNsPtr uwns;

  xmlKeepBlanksDefault(0);
  doc = xmlNewDoc("1.0");
  xmlCreateIntSubset(doc, root, NULL, PATH_RESULT_DTD);

  tree = xmlNewChild((xmlNodePtr)doc, NULL, root, NULL);
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

