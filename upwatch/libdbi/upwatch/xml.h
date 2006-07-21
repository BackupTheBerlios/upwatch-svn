gfloat xmlNodeListGetFloat(xmlDocPtr doc, xmlNodePtr list, int inLine);
gint xmlNodeListGetInt(xmlDocPtr doc, xmlNodePtr list, int inLine);
glong xmlNodeListGetLong(xmlDocPtr doc, xmlNodePtr list, int inLine);

gint xmlGetPropInt(xmlNodePtr node, const xmlChar *name);
guint xmlGetPropUnsigned(xmlNodePtr node, const xmlChar *name);
glong xmlGetPropLong(xmlNodePtr node, const xmlChar *name);
gfloat xmlGetPropFloat(xmlNodePtr node, const xmlChar *name);

xmlDocPtr UpwatchXmlDoc(const char *root, char *fromhost);

void UpwatchXmlGenericErrorFunc(void *ctx, const char *fmt, ...);

int xmlNsEqual(xmlNs *ns1, xmlNs *ns2);

