#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "generic.h"
#include "logregex.h"

#define MACROS "macros.txt"
#define RMACROS "rmacros.txt"

struct regexspec {
  char *file;   // basefilename this regex is originating from
  char *regex;  // regular expression
  regex_t preg; // compiled version
  unsigned match; // number of times this regex matched
};

struct macrodef {
  char *key;
  int keylen;
  char *value;
};

struct typespec {
  char *name;
  int runs;                 // total amount of matching runs
  int pondered;             // total amount of regexes pondered
  GPtrArray *macros;        // points to list of macrodefs
  GPtrArray *rmacros;       // points to list of macrodefs
  GPtrArray *green;         // points to list of regexspec structures
  int gmatch;               // how many times is the green matching done so far
  GPtrArray *yellow;        // same here
  GPtrArray *red;           // same here
};

static GHashTable *files; // table of all files and the modification date
static GHashTable *types; // table of all logfile type. point to struct typespecs

// return TRUE if the given file was modified since
// we last read it.
static int logregex_file_newer(char *fullname)
{
  struct stat st;
  time_t *previous;

  if (files == NULL) {
    files = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
  }
  if (stat(fullname, &st)) {
    LOG(LOG_WARNING, "%s: %m", fullname);
    return 0;                  // problem accessing file
  }
  previous = (time_t *)g_hash_table_lookup(files, fullname);
  if (!previous) { // not yet in table: insert it
    g_hash_table_insert(files, strdup(fullname), guintdup(st.st_mtime));
    return 1;
  }
  if (st.st_mtime > *previous) { // current file newer?
    *previous = st.st_mtime;
    return 1;
  } 
  return 0;
}

// (re)read macro-definitions if necessary
//
static void logregex_read_macros(GPtrArray *macs, char *filename)
{
  FILE *in;
  char buffer[4096];

  //printf("read macros %s\n", filename);
  in = fopen(filename, "r");
  if (in == NULL) {
    LOG(LOG_WARNING, "%s: %m", filename);
    return;
  }

  while (fgets(buffer, sizeof(buffer), in)) {
    char *p;
    struct macrodef *macro;

    if (buffer[0] == 0) continue;
    if (buffer[0] == '\n') continue;
    if (buffer[0] == ' ') continue;
    if (buffer[0] == '#') continue;
    if (buffer[0] == ';') continue;

    buffer[strlen(buffer)-1] = 0;
    for (p=buffer; *p; p++) {
      if (*p == ' ' || *p == '\t') break;
    }
    if (!*p) continue;
    for (*p++ = 0; *p; p++) {
      if (*p != ' ' && *p != '\t') {
        break;
      }
    }
    macro = malloc(sizeof(struct macrodef));
    macro->key = strdup(buffer);
    macro->keylen = strlen(buffer);
    macro->value = strdup(p);
    g_ptr_array_add(macs, macro);
  }
  fclose(in);
}

static void logregex_refresh_macros(struct typespec *ts, char *path)
{
  char filename[PATH_MAX];

  sprintf(filename, "%s/%s", path, MACROS);
  if (!logregex_file_newer(filename)) return;
  LOG(LOG_INFO, "reloading %s", filename);
  if (ts->macros) {
    while (ts->macros->len) {
      struct macrodef *macro;

      macro = g_ptr_array_remove_index_fast(ts->macros, 0);
      free(macro->key);
      free(macro->value);
      free(macro);
    }
    g_ptr_array_free(ts->macros, 0);
  }
  ts->macros = g_ptr_array_new();
  logregex_read_macros(ts->macros, filename);
}

static char *logregex_macro_find(GPtrArray *macros, char *key)
{
  int i;

  for (i=0; i < macros->len; i++) {
    struct macrodef *macro;

    macro =  g_ptr_array_index(macros, i);
    if (strcmp(macro->key, key) == 0) {
      return(macro->value);
    }
  }
  return NULL;
}

static int logregex_is_macro(GPtrArray *macros, char *key)
{
  int i;

  for (i=0; i < macros->len; i++) {
    struct macrodef *macro;

    macro = g_ptr_array_index(macros, i);
    if (strncmp(key, macro->key, macro->keylen) == 0) {
      return(macro->keylen);
    }
  }
  return 0;
}

static void logregex_replace_macros(GPtrArray *macros, char *in, char *out)
{
  char *p;

  for (p = in; *p; p++) {
    if (*p == '[') {
      char *head, *tail;
      char *replace;
      char var[4096];

      for (head = p+1, tail = var; *head;) {
        if (*head == ']') {
          *tail = 0;
          break;
        }
        *tail++ = *head++;
      }
      if (!*head) continue;
      replace = logregex_macro_find(macros, var);
      if (replace) {
        strcpy(out, replace);
        out += strlen(replace);
        p = head;
        continue;
      }
    }
    *out++ = *p;
  }
  *out = 0;
}

static void logregex_refresh_rmacros(struct typespec *ts, char *path)
{
  char filename[PATH_MAX];

  sprintf(filename, "%s/%s", path, RMACROS);
  if (!logregex_file_newer(filename)) return;
  LOG(LOG_INFO, "reloading %s", filename);
  if (ts->rmacros) {
    while (ts->rmacros->len) {
      struct macrodef *macro;

      macro = g_ptr_array_remove_index_fast(ts->rmacros, 0);
      free(macro->key);
      free(macro->value);
      free(macro);
    }
    g_ptr_array_free(ts->rmacros, 0);
  }
  ts->rmacros = g_ptr_array_new();
  logregex_read_macros(ts->rmacros, filename);
}

static void free_regexspec(gpointer data)
{
  struct regexspec *rs = (struct regexspec *) data;

  free(rs->file);
  free(rs->regex);
  regfree(&rs->preg);
  free(rs);
}

static void logregex_remove_file(struct typespec *ts, char *file)
{
  int i;

  //printf("remove %s:%s\n", ts->name, file);
  if (ts->green) {
    for (i=0; i < ts->green->len; i++) {
      struct regexspec *rs = g_ptr_array_index(ts->green, i);
      if (strcmp(file, rs->file) == 0) {
        g_ptr_array_remove_index(ts->green, i);
        free_regexspec(rs);
        i--;
      }
    }
  }
  if (ts->yellow) {
    for (i=0; i < ts->yellow->len; i++) {
      struct regexspec *rs = g_ptr_array_index(ts->yellow, i);
      if (strcmp(file, rs->file) == 0) {
        g_ptr_array_remove_index(ts->yellow, i);
        free_regexspec(rs);
        i--;
      }
    }
  }
  if (ts->red) {
    for (i=0; i < ts->red->len; i++) {
      struct regexspec *rs = g_ptr_array_index(ts->red, i);
      if (strcmp(file, rs->file) == 0) {
        g_ptr_array_remove_index(ts->red, i);
        free_regexspec(rs);
        i--;
      }
    }
  }
}

static void free_typespec(gpointer data)
{
  struct typespec *ts = (struct typespec *) data;

  g_free(ts->name);
  g_ptr_array_free(ts->macros, TRUE);
  g_ptr_array_free(ts->green, TRUE);
  g_ptr_array_free(ts->yellow, TRUE);
  g_ptr_array_free(ts->red, TRUE);
  g_free(ts);
}

// add the regular expressions in a file to our internal database
// 
static void logregex_add_file(char *fullname, struct typespec *ts, char *file)
{
  FILE *in;
  char buffer[4096];
  char buffer2[8192];
  int line = 0;

  //printf("add %s as %s:%s\n", fullname, ts->name, file);

  LOG(LOG_INFO, "loading %s", fullname);
  in = fopen(fullname, "r");
  if (in == NULL) {
    LOG(LOG_WARNING, "%s: %m", fullname);
    return;
  }
  while (fgets(buffer, sizeof(buffer), in)) {
    char *p;
    int err;
    struct regexspec *spec = NULL;

    line++;
    if (buffer[0] == 0) continue;
    if (buffer[0] == '\n') continue;
    if (buffer[0] == '\t') continue;
    if (buffer[0] == ' ') continue;
    if (buffer[0] == '#') continue;
    if (buffer[0] == ';') continue;
    if (buffer[0] == '/') continue;

    buffer[strlen(buffer)-1] = 0;          // strip trailing \n
    for (p=buffer; *p; p++) {
      if (*p == ' ' || *p == '\t') break;  // find the end of the first word (probably red or green)
    }
    if (!*p) continue;
    for (*p++ = 0; *p; p++) {
      if (*p != ' ' && *p != '\t') { // skip to the beginning of next word
        break;
      }
    }
    if (!*p) continue;
    logregex_replace_macros(ts->macros, p, buffer2);
    //strcpy(buffer2, p);

    if (!spec) {
      spec = malloc(sizeof(struct regexspec));
    }
    memset(&spec->preg, 0, sizeof(spec->preg));
    err = regcomp(&spec->preg, buffer2, REG_EXTENDED|REG_ICASE);
    if (err) {
      char buffer[256];

      regerror(err, &spec->preg, buffer, sizeof(buffer));
      LOG(LOG_ERR, "%s(%u): %s: %s", fullname, line, buffer2, buffer);
      fprintf(stderr, "%s(%u): %s: %s\n", fullname, line, buffer2, buffer);
      continue;
    }
    spec->file = strdup(file);
    spec->regex = strdup(buffer2);
    if (strncmp("green", buffer, 6) == 0) {
      g_ptr_array_add(ts->green, spec);
    }
    if (strncmp("yellow", buffer, 6) == 0) {
      g_ptr_array_add(ts->yellow, spec);
    }
    if (strncmp("red", buffer, 3) == 0) {
      g_ptr_array_add(ts->red, spec);
    }
    spec = NULL;
  }
  fclose(in);
}

// refresh the data for a specific filetype (for example syslog)
// 
static void logregex_refresh_type(char *path, char *type)
{
  struct typespec *ts;
  GDir *dir;
  GError *error=NULL;
  G_CONST_RETURN gchar *filename;

  if (types == NULL) {
    types = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, free_typespec);
  }
  ts = g_hash_table_lookup(types, type);
  if (!ts) {
    ts = g_malloc0(sizeof(struct typespec));
    ts->name = strdup(type);
    ts->green = g_ptr_array_new();
    ts->yellow = g_ptr_array_new();
    ts->red = g_ptr_array_new();
    g_hash_table_insert(types, strdup(type), ts);
  }

  logregex_refresh_macros(ts, path); // refresh the macros first
  logregex_refresh_rmacros(ts, path); // and the reverse macros
  dir = g_dir_open (path, 0, &error);
  if (dir == NULL) {
    perror(path);
    return; 
  }
  while ((filename = g_dir_read_name(dir)) != NULL) {
    char buffer[PATH_MAX];

    if (filename[0] == '.') continue;  // skip '.', '..' and hidden files
    if (strcmp(filename, MACROS) == 0) continue; // skip macrofiles
    if (strcmp(filename, RMACROS) == 0) continue; // .. and this one too
    sprintf(buffer, "%s/%s", path, filename);
    if (!g_file_test(buffer, G_FILE_TEST_IS_REGULAR)) {
      //fprintf(stderr, "%s: not a regular file\n", buffer);
      continue;
    }
    if (logregex_file_newer(buffer)) {
      logregex_remove_file(ts, (char *)filename);
      logregex_add_file(buffer, ts, (char *)filename);
    }
  }
  g_dir_close(dir);
}

static void logregex_print_1stat(gpointer key, gpointer value, gpointer user_data)
{
  char *type = (char *)user_data;
  char *name = (char *)key;
  struct typespec *ts = (struct typespec *) value;
  if (type && strcmp(key, type)) return;

  printf("%s: runs %u, avg pondered/run %u\n", name, ts->runs, ts->pondered/(ts->runs?ts->runs:1));
}

// print stats for all 
// 
void logregex_print_stats(char *type)
{
  if (types == NULL) return;
  g_hash_table_foreach(types, logregex_print_1stat, type);
}

void logregex_expand_macros(char *type, char *in, char *out)
{
  struct typespec *ts;

  if (!types) {
    strcpy(out, in);
    return;
  }
  ts = g_hash_table_lookup(types, type);
  if (ts == NULL) {
    strcpy(out, in);
    return;
  }
  logregex_replace_macros(ts->macros, in, out);
}

// read all directories in the path
// the name of these directories denote the filetype
// for each filetype read regular expressions and macros
int logregex_refresh(char *path)
{
  GDir *dir;
  GError *error=NULL;
  G_CONST_RETURN gchar *filename;

  dir = g_dir_open (path, 0, &error);
  if (dir == NULL) {
    perror(path);
    return 0;
  }
  while ((filename = g_dir_read_name(dir)) != NULL) {
    char buffer[PATH_MAX];

    if (filename[0] == '.') continue;  // skip '.', '..' and hidden files
    sprintf(buffer, "%s/%s", path, filename);
    if (!g_file_test(buffer, G_FILE_TEST_IS_DIR)) {
      fprintf(stderr, "%s: not a regular file\n", buffer);
      continue;
    }
    logregex_refresh_type(buffer, (char *)filename);
  }
  g_dir_close(dir);
  return 1;
}

int logregex_rmatchline(char *type, char *line)
{
  struct typespec *ts;
  ts = g_hash_table_lookup(types, type);
  regmatch_t mt;
  regex_t spec;
  int i, j;
  int replaced = 0;
  char buffer[4096];
  char *out = buffer;

  if (!ts) return 0;

  for (j=0; j < ts->rmacros->len; j++) {
    struct macrodef *macro;
    int err;
    int matching = 1;

    macro = g_ptr_array_index(ts->rmacros, j);

    memset(&spec, 0, sizeof(spec));
    err = regcomp(&spec, macro->value, REG_EXTENDED);
    if (err) {
      char buffer[256];

      regerror(err, &spec, buffer, sizeof(buffer));
      LOG(LOG_ERR, buffer);
      continue;
    }
    while (matching) {
      matching = 0;
      if (regexec(&spec,  line, 1, &mt, 0) == 0) {
        char buffer[4096];
        char *out = buffer;
        int i;

        for (i=0; line[i];) {
          if (i == mt.rm_so) {
            matching = 1;
            *out ++ = '[';
            strcpy(out, macro->key);
            out += strlen(macro->key);
            *out++ = ']';
            i = mt.rm_eo;
          } else {
            *out++ = line[i++];
          }
        }
        *out = 0;
        strcpy(line, buffer);
        replaced = 1;
      }
    }
    regfree(&spec);
  }

  // change '[[' to '\[['
  // change ']]' to ']\]'
  // escape '?', '*', '(' and ')'
  for (i=0; line[i];) {
    int c = line[i];

    if (c == '[') {
      int len;

      len = logregex_is_macro(ts->rmacros, &line[i+1]);
      if (len && line[i+len+1] == ']') { // and ends with ']'
        // this is start of macro, don't escape it but just copy it
        *out++ = line[i++];  // copy the starting '[' 
        while (len--) { 
          *out++ = line[i++]; // copy the macroname
        }
      } else {
        *out++ = '\\'; // just escape it
      }
    } else if (c == ']') {
      *out++ = '\\';
    } else // other metacharacter?   {}()^$.|*+\?  
      if (c == '{' || c == '}' || c == '(' || c == ')' || c == '^' || c == '$' ||
          c == '.' || c == '|' || c == '*' || c == '+' || c == '?' || c == '\\') {
      *out++ = '\\';
    }
    *out++ = line[i++];  
  }
  *out = 0;
  strcpy(line, buffer);
  return replaced;
}

// This function should return a negative integer if the first value 
// comes before the second, 0 if they are equal, or a positive integer
// if the first value comes after the second. 
static gint regexspec_compare_func(gconstpointer a, gconstpointer b)
{
  struct regexspec *rs1 = *(struct regexspec **) a;
  struct regexspec *rs2 = *(struct regexspec **) b;

  return(rs2->match - rs1->match);
}

int logregex_matchline(char *type, char *buffer, int *color)
{
  int i;
  int match = 0;
  struct typespec *ts;
  ts = g_hash_table_lookup(types, type);
  if (!ts) return 0;

  //printf("matching: %s\n", buffer);

  ts->runs++;
  if (ts->red) {
    for (i=0; i < ts->red->len; i++) {
      struct regexspec *rs = g_ptr_array_index(ts->red, i);

      //printf("red %s\n", rs->regex);
      ts->pondered++;
      if (rs && regexec(&rs->preg,  buffer, 0, 0, 0) == 0) {
        rs->match++;
        *color = STAT_RED;
        return TRUE;
      }
    }
  }

  if (ts->yellow) {
    for (i=0; i < ts->yellow->len; i++) {
      struct regexspec *rs = g_ptr_array_index(ts->yellow, i);

      //printf("yellow %s\n", rs->regex);
      ts->pondered++;
      if (rs && regexec(&rs->preg,  buffer, 0, 0, 0) == 0) {
        rs->match++;
        *color = STAT_YELLOW;
        return TRUE;
      }
    }
  }

  if (ts->green) {
    ts->gmatch++;
    if (ts->gmatch == 100 || ts->gmatch == 1000 || ts->gmatch == 10000 || ts->gmatch == 100000) {
      g_ptr_array_sort(ts->green, regexspec_compare_func);
    }
    for (i=0; i < ts->green->len; i++) {
      struct regexspec *rs = g_ptr_array_index(ts->green, i);

      ts->pondered++;
      if (rs && regexec(&rs->preg,  buffer, 0, 0, 0) == 0) {
        match = 1;
        rs->match++;
        break;
        //printf("MATCH green%s\n", rs->regex);
      } 
    }
  }
  if (!match) { 
    *color = STAT_YELLOW;
    return TRUE;
  }
  *color = STAT_GREEN;
  return FALSE;
}

