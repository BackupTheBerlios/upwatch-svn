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

