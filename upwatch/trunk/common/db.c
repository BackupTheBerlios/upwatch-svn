#include "config.h"
#include <mysql.h>
#include <stdarg.h>
#include <generic.h>
#include "cmd_options.h"

MYSQL *mysql;

/****************************
 database functions. 
 NOTE: Don't use with multithreaded programs!
 ***************************/
int open_database(void)
{
  if (mysql) return(0); // already open
  if ((mysql = (MYSQL *)malloc(sizeof(MYSQL))) == NULL) {
    LOG(LOG_ERR, "malloc: %m");
    return(1);
  }

  mysql_init(mysql);
  mysql_options(mysql,MYSQL_OPT_COMPRESS,0);
  if (!mysql_real_connect(mysql, OPT_ARG(DBHOST), OPT_ARG(DBUSER), OPT_ARG(DBPASSWD), OPT_ARG(DBNAME), 0, NULL, 0)) {
    LOG(LOG_ERR, "%s dbhost=%s,dbname=%s,dbuser=%s,dbpasswd=%s",
           mysql_error(mysql), OPT_ARG(DBHOST), OPT_ARG(DBNAME), OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
    return(1);
  }
  return(0);
}

void close_database(void)
{
  if (mysql) {
    //my_transaction("rollback");
    mysql_close(mysql);
    free(mysql);
    mysql = NULL;
  }
}

void my_transaction(char *what)
{
  if (!mysql) return;
  mysql_query(mysql, what);
}

MYSQL_RES *my_query(char *fmt, ...)
{
  MYSQL_RES *result;
  char qry[BUFSIZ];
  va_list arg;

  if (!mysql) return(NULL);

  va_start(arg, fmt);
  vsnprintf(qry, BUFSIZ, fmt, arg);
  va_end(arg);
  if (debug > 3) printf("%s\n", qry);

  if (mysql_query(mysql, qry)) {
    LOG(LOG_ERR, "%s: %s", qry, mysql_error(mysql));
    return(NULL);
  }
  result = mysql_store_result(mysql);
  return(result);
}

