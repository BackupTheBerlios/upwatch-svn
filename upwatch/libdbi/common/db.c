#include "config.h"
#include <mysql.h>
#include <mysqld_error.h>
#include <stdarg.h>
#include <generic.h>

#ifdef DMALLOC 
#include "dmalloc.h"
#endif

#ifndef HAVE_MYSQL_REAL_ESCAPE
unsigned long mysql_real_escape_string(MYSQL *mysql, char *to, const char *from, unsigned long length)
{
  return(mysql_escape_string(to, from, length));
}
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

MYSQL *open_database(char *dbhost, int dbport, char *dbname, char *dbuser, char *dbpasswd)
{
  MYSQL *mysql;

  mysql = mysql_init(NULL);
  mysql_options(mysql, MYSQL_OPT_COMPRESS, 0);
  if (!mysql_real_connect(mysql, dbhost, dbuser, dbpasswd, dbname, dbport, NULL, 0)) {
    LOG(LOG_ERR, "%s dbhost=%s,dbport=%d,dbname=%s,dbuser=%s,dbpasswd=%s",
           mysql_error(mysql), dbhost, dbport, dbname, dbuser, dbpasswd);
    return(NULL);
  }
  return(mysql);
}

MYSQL_RES *my_rawquery(MYSQL *mysql, int log_dupes, char *qry)
{
  MYSQL_RES *result;

  if (debug > 4) {
    LOGRAW(LOG_DEBUG, qry);
  }
  if (mysql_query(mysql, qry)) {
    switch (mysql_errno(mysql)) {
    case ER_DUP_ENTRY:
        if (!log_dupes) break;
    default:
      LOG(LOG_WARNING, "%s:[%u] %s", qry, mysql_errno(mysql), mysql_error(mysql));
      break;
    }
    return(NULL);
  }
  result = mysql_store_result(mysql);
  if (mysql_errno(mysql)) {
    LOG(LOG_WARNING, "%s: [%u] %s", qry, mysql_errno(mysql), mysql_error(mysql));
  }
  return(result);
}

MYSQL_RES *my_query(MYSQL *mysql, int log_dupes, char *fmt, ...)
{
static char qry[65536]; // max query size for us
  va_list arg;

  if (!mysql) return(NULL);

  va_start(arg, fmt);
  vsnprintf(qry, sizeof(qry)-1, fmt, arg);
  va_end(arg);
  return my_rawquery(mysql, log_dupes, qry);
}

