#if !defined(__DB_H) 
#define __DB_H

#include <mysql.h>
#include <mysqld_error.h>

MYSQL *open_database(const char *dbhost, int dbport, const char *dbname, const char *dbuser, const char *dbpasswd);
void close_database(MYSQL *mysql);
MYSQL_RES *my_query(MYSQL *mysql, int log_dupes, char *qry, ...);
MYSQL_RES *my_rawquery(MYSQL *mysql, int log_dupes, char *qry);

#endif /* __DB_H */
