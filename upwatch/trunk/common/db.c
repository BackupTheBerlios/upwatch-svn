#include "config.h"
#include <mysql.h>
#include <stdarg.h>
#include <generic.h>
#include "cmd_options.h"

#ifdef DMALLOC 
#include "dmalloc.h"
#endif

/****************************
 database functions. 
 NOTE: Don't use with multithreaded programs!
 ***************************/
void close_database(MYSQL *mysql)
{
  if (mysql) {
    //my_transaction("rollback");
    mysql_close(mysql);
  }
}

MYSQL *open_database(char *dbhost, char *dbname, char *dbuser, char *dbpasswd, int options)
{
  MYSQL *mysql;

  mysql = mysql_init(NULL);
  mysql_options(mysql, options, 0);
  if (!mysql_real_connect(mysql, dbhost, dbuser, dbpasswd, dbname, 0, NULL, 0)) {
    LOG(LOG_ERR, "%s dbhost=%s,dbname=%s,dbuser=%s,dbpasswd=%s",
           mysql_error(mysql), dbhost, dbname, dbuser, dbpasswd);
    return(NULL);
  }
  return(mysql);
}

MYSQL_RES *my_query(MYSQL *mysql, int log_dupes, char *fmt, ...)
{
  MYSQL_RES *result;
  char qry[65535];
  va_list arg;

  if (!mysql) return(NULL);

  va_start(arg, fmt);
  vsnprintf(qry, sizeof(qry), fmt, arg);
  va_end(arg);
  if (debug > 3) LOG(LOG_DEBUG, qry);

  if (mysql_query(mysql, qry)) {
    switch (mysql_errno(mysql)) {
    case ER_DUP_ENTRY:
        if (!log_dupes) break;
    default:
      LOG(LOG_ERR, "%s: %s", qry, mysql_error(mysql));
      break;
    }
    return(NULL);
  }
  result = mysql_store_result(mysql);
  if (mysql_errno(mysql)) {
    LOG(LOG_ERR, "%s: %s", qry, mysql_error(mysql));
  }
  return(result);
}

