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
  xmlChar *tmp = xmlNodeListGetString(doc, list, inLine);
  gfloat ret = atof(tmp);
  xmlFree(tmp);
  return(ret);
}

gint xmlNodeListGetInt(xmlDocPtr doc, xmlNodePtr list, int inLine)
{
  xmlChar *tmp = xmlNodeListGetString(doc, list, inLine);
  gint ret = atoi(tmp);
  xmlFree(tmp);
  return(ret);
}

glong xmlNodeListGetLong(xmlDocPtr doc, xmlNodePtr list, int inLine)
{
  xmlChar *tmp = xmlNodeListGetString(doc, list, inLine);
  glong ret = atol(tmp);
  xmlFree(tmp);
  return(ret);
}

gint xmlGetPropInt(xmlNodePtr node, const xmlChar *name)
{
  xmlChar *tmp = xmlGetProp(node, name);
  gint ret = atoi(tmp);
  xmlFree(tmp);
  return(ret);
}

glong xmlGetPropLong(xmlNodePtr node, const xmlChar *name)
{
  xmlChar *tmp = xmlGetProp(node, name);
  glong ret = atol(tmp);
  xmlFree(tmp);
  return(ret);
}

gfloat xmlGetPropFloat(xmlNodePtr node, const xmlChar *name)
{
  xmlChar *tmp = xmlGetProp(node, name);
  gfloat ret = atof(tmp);
  xmlFree(tmp);
  return(ret);
}

