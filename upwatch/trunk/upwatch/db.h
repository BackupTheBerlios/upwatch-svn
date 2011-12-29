#if !defined(__DB_H) 
#define __DB_H

#include <dbi/dbi.h>

typedef struct _database {
  char driver[80];
  char name[80];
  char host[80];
  char port[8];
  char username[24];
  char password[24];
  dbi_conn conn;
  int debug;
} database;

void close_database(database *db);
database *open_database(const char *dbtype, const char *dbhost, int dbport, const char *dbname, const char *dbuser, const char *dbpasswd);

// note the blank before the last comma. This is required, see http://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html
#define db_rawquery(a, b, c, args...) _db_rawquery(__FILE__, __LINE__, a, b, c , ##args)
#define db_query(a, b, c, args...) _db_query(__FILE__, __LINE__, a, b, c , ##args)
dbi_result _db_rawquery(const char *file, int line, database *db, int log_dupes, char *qry);
dbi_result _db_query(const char *file, int line, database *db, int log_dupes, char *fmt, ...);

const char *dbi_result_get_string_default(dbi_result Result, const char *fieldname, const char *def);
const char *dbi_result_get_string_default_copy(dbi_result Result, const char *fieldname, const char *def);
int dbi_duplicate_entry(dbi_conn conn);

#endif /* __DB_H */
