#include "config.h"

#include <generic.h>
#include "cmd_options.h"
#include "traceroute.h"

void process(gpointer data, gpointer user_data);
GThreadPool *pool;

int init(void)
{
  GError *error;

  daemonize = TRUE;
  every = EVERY_5SECS;
  g_thread_init(NULL);
  pool = g_thread_pool_new(traceroute, NULL, 10, 0, &error);
  if (pool == NULL) {
    LOG(LOG_NOTICE, error->message);
    g_error_free(error);
    return(0);
  }
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return 1;
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
  return(count);
}

void Unlink(char *file) {}
#define unlink Unlink

void process(gpointer data, gpointer user_data)
{
  char *filename = (char *)data;
  xmlDocPtr doc, workq;
  xmlNsPtr ns;
  xmlNodePtr cur;
  int probe_count = 0;

  if (debug) LOG(LOG_DEBUG, "Processing %s", filename);

  doc = xmlParseFile(filename);
  if (doc == NULL) {
    LOG(LOG_NOTICE, "%s: %m", filename);
    return;
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
  for (;cur != NULL; cur = cur->next) {
    GError *error = NULL;
    char *inv;
    int type, port;
    struct trace_info *ti;
    xmlNodePtr host;

    if (xmlIsBlankNode(cur)) continue;
    inv = xmlGetProp(cur, (const xmlChar *) "investigate");
    if (inv == NULL) continue;
    if (!strcmp(inv, "icmptraceroute")) {
      type = TR_ICMP;
    } else if (!strcmp(inv, "tcptraceroute")) {
      type = TR_TCP;
      port = xmlGetPropInt(cur, (const xmlChar *) "port");
    } else if (!strcmp(inv, "udptraceroute")) {
      type = TR_UDP;
      port = xmlGetPropInt(cur, (const xmlChar *) "port");
    } else {
      xmlFree(inv);  
      continue; // unknown type
    }
    // this one needs investigation, so remove from this document tree, 
    // save it in the workqueue, and start an investigate thread 
    ti = calloc(1, sizeof(struct trace_info));
    ti->type = type;
    ti->cur = cur;

    // first find the hostname and/or ipaddress
    for (host = cur->xmlChildrenNode; host != NULL; host = host->next) {
      if ((!xmlStrcmp(host->name, (const xmlChar *) "host")) && (host->ns == ns)) {
        xmlNodePtr hname;
        xmlChar *p;

        for (hname = host->xmlChildrenNode; hname != NULL; hname = hname->next) {
          if (xmlIsBlankNode(hname)) continue;
          if ((!xmlStrcmp(hname->name, (const xmlChar *) "hostname")) && (hname->ns == ns)) {
            p = xmlNodeListGetString(doc, hname->xmlChildrenNode, 1);
            ti->hostname = strdup(p);
            xmlFree(p);
          }
          if ((!xmlStrcmp(hname->name, (const xmlChar *) "ipaddress")) && (hname->ns == ns)) {
            p = xmlNodeListGetString(doc, hname->xmlChildrenNode, 1);
            ti->ipaddress = strdup(p);
            xmlFree(p);
          }
        }
      }
    }

    xmlUnlinkNode(cur);
    workq = UpwatchXmlDoc("result"); // create new document
    xmlAddPrevSibling(workq->children, ti->cur);  // link in the current node
    spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(WORKQUEUE), workq, &ti->workfilename); // save in workqueue
    xmlUnlinkNode(ti->cur);  // unlink again
    xmlFreeDoc(workq); // and free work document
     
    g_thread_pool_push(pool, ti, &error);
    xmlFree(inv);  
    if (error != NULL) {
      LOG(LOG_NOTICE, error->message);
      g_error_free(error);
      xmlFreeDoc(doc);
      return;
    }
  }
  spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(OUTPUT), doc, NULL); // these don't need to be investigated
  xmlFreeDoc(doc);
  if (debug > 1) LOG(LOG_DEBUG, "Processed %d probes", probe_count);
  if (debug > 1) LOG(LOG_DEBUG, "unlink(%s)", filename);
  unlink(filename);
  return;
}

