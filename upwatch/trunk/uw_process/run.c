#include "config.h"
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <curl/curl.h>
#include <netdb.h>

#include <generic.h>
#include "cmd_options.h"

extern int process_ping(xmlDocPtr, xmlNodePtr, xmlNsPtr);
extern int process_httpget(xmlDocPtr, xmlNodePtr, xmlNsPtr);

struct _probe_proc {
  char *name;
  int (*process)(xmlDocPtr, xmlNodePtr, xmlNsPtr);
} prob_proc[] = {
  { "ping",         process_ping },
  { "httpget",      process_httpget },
  { NULL,  NULL }
};

void process(gpointer data, gpointer user_data);

int init(void)
{
  daemonize = TRUE;
  every = EVERY_5SECS;
  g_thread_init(NULL);
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

int mystrcmp(char **a, char **b)
{
  int ret = strcmp(*a, *b);

  if (ret < 0) return(-1);
  if (ret > 0) return(1);
  return(0);
 
}

int run(void)
{
  int count = 0;
  char path[PATH_MAX];
  G_CONST_RETURN gchar *filename;
  GDir *dir;
  GPtrArray *arr = g_ptr_array_new();
  int i;
  int files = 0;

  if (debug > 3) LOG(LOG_DEBUG, "run()");
  sprintf(path, "%s/%s/new", OPT_ARG(SPOOLDIR), progname);
  dir = g_dir_open (path, 0, NULL);
  while ((filename = g_dir_read_name(dir)) != NULL) {
    char buffer[PATH_MAX];

    if (open_database() != 0) { // will do nothing if already open
      break;
    }
    if (filename[0] == '.') continue;  // skip hidden files
    sprintf(buffer, "%s/%s", path, filename);
    g_ptr_array_add(arr, strdup(buffer));
    files++;
  }
  g_dir_close(dir);
  if (files) {
    g_ptr_array_sort(arr, mystrcmp);
  }

  for (i=0; i < arr->len; i++) {
    //printf("%s\n", g_ptr_array_index(arr,i));
    process(g_ptr_array_index(arr,i), NULL);
    free(g_ptr_array_index(arr,i));
    count++;
  }

  g_ptr_array_free(arr, TRUE);
  close_database();
  return(count);
}

void process(gpointer data, gpointer user_data)
{
  char *filename = (char *)data;
  xmlDocPtr doc; 
  xmlNsPtr ns;
  xmlNodePtr cur;
  int found=0, failures=0;
  struct _probe_proc *probe;
  int probe_count = 0;

  if (debug) LOG(LOG_DEBUG, "Processing %s", filename);

  doc = xmlParseFile(filename);
  if (doc == NULL) {
    LOG(LOG_NOTICE, "%s: %m", filename);
    return;
  }

  if (HAVE_OPT(COPY)) {
    spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(COPY), doc, NULL);
  }

  cur = xmlDocGetRootElement(doc);
  if (cur == NULL) {
    LOG(LOG_NOTICE, "%s: empty document", filename);
    xmlFreeDoc(doc);
    unlink(filename);
    return;
  }
  ns = xmlSearchNsByHref(doc, cur, (const xmlChar *) NAMESPACE_URL);
  if (ns == NULL) {
    LOG(LOG_NOTICE, "%s: wrong type, result namespace not found", filename);
    xmlFreeDoc(doc);
    unlink(filename);
    return;
  }
  if (xmlStrcmp(cur->name, (const xmlChar *) "result")) {
    LOG(LOG_NOTICE, "%s: wrong type, root node is not 'result'", filename);
    xmlFreeDoc(doc);
    unlink(filename);
    return;
  }
  /*
   * Now, walk the tree.
   */
  /* First level we expect just result */
  cur = cur->xmlChildrenNode;
  while (cur && xmlIsBlankNode(cur)) {
    cur = cur->next;
  }
  if (cur == 0) {
    LOG(LOG_NOTICE, "%s: wrong type, empty file'", filename);
    xmlFreeDoc(doc);
    unlink(filename);
    return;
  }
  /* Second level is a list of probes, but be laxist */
  for (failures = 0; cur != NULL; cur = cur->next) {
    if (xmlIsBlankNode(cur)) continue;
    for (found = 0, probe = prob_proc; probe->name; probe++) {
      if (!xmlStrcmp(cur->name, (const xmlChar *) probe->name)) {
	if (cur->ns != ns) {
          LOG(LOG_ERR, "method found, but namespace incorrect on %s", cur->name);
	  continue;
	}
        found = 1;
        probe_count++;
        if ((*probe->process)(doc, cur, ns) == 0) {
          failures++;
        }
        break;
      }
    }
    if (!found) {
      LOG(LOG_ERR, "can't find method: %s", cur->name);
      failures++;
    }
  }
  if (failures) {
    xmlSaveFormatFile(OPT_ARG(FAILURES), doc, 1);
  }
  xmlFreeDoc(doc);
  if (debug > 1) LOG(LOG_DEBUG, "Processed %d probes", probe_count);
  if (debug > 1) LOG(LOG_DEBUG, "unlink(%s)", filename);
  unlink(filename);
  return;
}


