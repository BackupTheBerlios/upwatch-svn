#include <mysql.h>
#include <mysqld_error.h>

extern MYSQL *mysql;
int open_database(void);
void close_database(void);
MYSQL_RES *my_query(char *qry, ...);
void my_transaction(char *qry);
