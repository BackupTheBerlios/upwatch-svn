#include <mysql.h>
#include <mysqld_error.h>

MYSQL *open_database(char *dbhost, int dbport, char *dbname, char *dbuser, char *dbpasswd);
void close_database(MYSQL *mysql);
MYSQL_RES *my_query(MYSQL *mysql, int log_dupes, char *qry, ...);
MYSQL_RES *my_rawquery(MYSQL *mysql, int log_dupes, char *qry);
