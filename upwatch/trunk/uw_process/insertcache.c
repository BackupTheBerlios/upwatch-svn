#include "config.h"
#include <generic.h>
#include <unistd.h>
#include "cmd_options.h"
#include "uw_process.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

void mod_ic_add(module *probe, const char *table, const char *str)
{
  if (probe->insertc->len >= OPT_VALUE_MULTI_VALUE_CACHE_SIZE) {
    mod_ic_flush(probe, table);
  }
  g_ptr_array_add(probe->insertc, (void *)str);
}

void mod_ic_flush(module *probe, const char *table)
{
  int i, len=0;
  char *sql, *start;
  MYSQL_RES *result;

  if (probe->insertc->len == 0) return;
  for (i=0; i < probe->insertc->len; i++) {
    len += strlen(g_ptr_array_index(probe->insertc, i)) + 2;
  }
  sql = malloc(len + 256);
  start = sql + sprintf(sql, "insert into %s values %s", 
                             (char *) table, (char *) g_ptr_array_index(probe->insertc, 0));
  g_free(g_ptr_array_index(probe->insertc, 0));
  for (i=1; i < probe->insertc->len; i++) {
    start += sprintf(start, ", %s", g_ptr_array_index(probe->insertc, i));
    g_free(g_ptr_array_index(probe->insertc, i));
  }
  g_ptr_array_free(probe->insertc, TRUE); 
  probe->insertc = g_ptr_array_new();
  result = my_query(probe->db, 0, sql);
  free(sql);
  if (result) mysql_free_result(result);
  // note we ignore dupes/errors for the sake of speed.
}
