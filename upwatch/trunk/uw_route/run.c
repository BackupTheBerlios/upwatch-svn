#include "config.h"
#include <generic.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <generic.h>
#include "uw_route.h"
#include "slot.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static int handle_file(gpointer data, gpointer user_data);

struct _router {
  char *name;
  char *queue;
  xmlDocPtr doc;  
} route[256];

int init(void)
{
  daemonize = TRUE;
  if (HAVE_OPT(RUN_QUEUE)) {
    every = ONE_SHOT;
  } else {
    every = EVERY_5SECS;
  }

  if (HAVE_OPT(ROUTE)) {
    int i=0;
    int     ct  = STACKCT_OPT( ROUTE );
    char**  pn = STACKLST_OPT( ROUTE );

    while (ct--) {
      char probe[256], queue[256];

      int cnt = sscanf(pn[ct], "%s %s", probe, queue);
      if (cnt != 2) {
        fprintf(stderr, "syntax error: route %s\n", pn[ct]);
        return 0;
      }
      route[i].name = strdup(probe);
      route[i].queue= strdup(queue);
      i++;
    }
  }
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

static int mystrcmp(char **a, char **b)
{
  int ret = strcmp(*a, *b);

  if (ret < 0) return(-1);
  if (ret > 0) return(1);
  return(0);
 
}

//************************************************************************
// read all files in the directory, sort and process them
//***********************************************************************
int read_input_files(char *path)
{
  int count = 0;
  G_CONST_RETURN gchar *filename;
  GError *error=NULL;
  GDir *dir;
  GPtrArray *arr = g_ptr_array_new();
  int i;
  int files = 0;
extern int forever;
  dir = g_dir_open (path, 0, &error);
  if (dir == NULL) {
    LOG(LOG_NOTICE, "g_dir_open: %s", error);
    g_ptr_array_free(arr, TRUE);
    return 0;
  }
  while ((filename = g_dir_read_name(dir)) != NULL) {
    char buffer[PATH_MAX];

    if (filename[0] == '.') continue;  // skip hidden files
    sprintf(buffer, "%s/%s", path, filename);
    g_ptr_array_add(arr, strdup(buffer));
    files++;
  }
  g_dir_close(dir);

  if (!files) {
    g_ptr_array_free(arr, TRUE);
    return 0;
  }
  g_ptr_array_sort(arr, mystrcmp);
  if (debug > 3) { fprintf(stderr, "%u files in directory\n", files); sleep(3); }

  // now we have a sorted list of files 
  // 
  for (i=0; i < arr->len && forever; i++) {
    if (debug > 3) { fprintf(stderr, "%u: %s\n", i, (char *)g_ptr_array_index(arr,i)); }
    uw_setproctitle("processing %s", g_ptr_array_index(arr,i));
    handle_file(g_ptr_array_index(arr,i), NULL); // extract probe results and queue them
    count++;
  }
  g_ptr_array_free(arr, TRUE);
  return(count);
}

int run(void)
{
  char path[PATH_MAX];
  if (debug > 3) { 
    int i;

    LOG(LOG_DEBUG, "run()"); 
    for (i = 0; route[i].name; i++) {
      fprintf(stderr, "%s -> %s\n", route[i].name, route[i].queue);
    }
  }

  sprintf(path, "%s/%s/new", OPT_ARG(SPOOLDIR), OPT_ARG(INPUT));
  uw_setproctitle("listing %s", path);
  return(read_input_files(path));
}

/*
 * Reads a results file
*/
static int handle_file(gpointer data, gpointer user_data)
{
  char *filename = (char *)data;
  xmlDocPtr doc; 
  xmlNsPtr ns;
  xmlNodePtr cur;
  int failures=0;
  struct stat st;
  int filesize;
  time_t filetime;
  int i;

  if (debug) { LOG(LOG_DEBUG, "Processing %s", filename); }

  if (stat(filename, &st)) {
    LOG(LOG_WARNING, "%s: %m", filename);
    unlink(filename);
    free(filename);
    return 0;
  }
  filesize = (int) st.st_size;
  filetime = st.st_mtime;
  if (filesize == 0) {
    LOG(LOG_WARNING, "%s: size 0, removed", filename);
    unlink(filename);
    free(filename);
    return 0;
  }

  doc = xmlParseFile(filename);
  if (doc == NULL) {
    char cmd[2048];

    LOG(LOG_NOTICE, "%s: %m", filename);
    sprintf(cmd, "cat %s >> %s", filename, OPT_ARG(FAILURES));
    system(cmd);
    unlink(filename);
    free(filename);
    return 0;
  }
  //xmlDocFormatDump(stderr, doc, TRUE);
  if (HAVE_OPT(COPY) && strcmp(OPT_ARG(COPY), "none")) {
    spool_result(OPT_ARG(SPOOLDIR), OPT_ARG(COPY), doc, NULL);
  }

  cur = xmlDocGetRootElement(doc);
  if (cur == NULL) {
    LOG(LOG_NOTICE, "%s: empty document", filename);
    unlink(filename);
    free(filename);
    xmlFreeDoc(doc);
    return 0;
  }
  ns = xmlSearchNsByHref(doc, cur, (const xmlChar *) NAMESPACE_URL);
  if (ns == NULL) {
    LOG(LOG_NOTICE, "%s: wrong type, result namespace not found", filename);
    unlink(filename);
    free(filename);
    xmlFreeDoc(doc);
    return 0;
  }
  if (xmlStrcmp(cur->name, (const xmlChar *) "result")) {
    LOG(LOG_NOTICE, "%s: wrong type, root node is not 'result'", filename);
    unlink(filename);
    free(filename);
    xmlFreeDoc(doc);
    return 0;
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
    LOG(LOG_NOTICE, "%s: empty file", filename);
    unlink(filename);
    free(filename);
    xmlFreeDoc(doc);
    return 0;
  }
  /* Second level is a list of probes, but be laxist */
  for (failures = 0; cur != NULL;) {
    int found = 0;

    if (xmlIsBlankNode(cur)) {
      cur = cur->next;
      continue;
    }
    for (i = 0; route[i].name; i++) {
      xmlNodePtr node, new;
      char buffer[20];

      if (strcmp(cur->name, route[i].name)) {
        continue;
      } 

      if (cur->ns != ns) {
        LOG(LOG_ERR, "method found, but namespace incorrect on %s", cur->name);
        continue;
      }
      found = 1;
      if (route[i].doc == NULL) {
        route[i].doc = UpwatchXmlDoc("result");
        xmlSetDocCompressMode(route[i].doc, OPT_VALUE_COMPRESS);
      }
      node = cur;
      cur = cur->next;
      xmlUnlinkNode(node);
      new = xmlCopyNode(node, 1);
      xmlFreeNode(node);
      sprintf(buffer, "%u", (int) filetime);
      xmlSetProp(new, "received", buffer); 
      xmlAddChild(xmlDocGetRootElement(route[i].doc), new);
      //xmlDocFormatDump(stderr, route[i].doc, TRUE);
      break;
    }
    if (!found) {
      LOG(LOG_ERR, "can't find method: %s, saved to %s", cur->name, OPT_ARG(FAILURES));
      failures++;
      cur = cur->next;
    }
  }
  if (failures) {
    xmlSaveFormatFile(OPT_ARG(FAILURES), doc, 1);
  }
  xmlFreeDoc(doc);

  for (i = 0; route[i].name; i++) {
    if (route[i].doc) {
      spool_result(OPT_ARG(SPOOLDIR), route[i].queue, route[i].doc, NULL);
      xmlFreeDoc(route[i].doc);
      route[i].doc = NULL;
    }
  }
  unlink(filename);
  free(filename);
  return 0;
}

