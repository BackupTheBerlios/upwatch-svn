#include "config.h"
#include <dbi/dbi.h>
#include <stdarg.h>
#include <generic.h>

#ifdef DMALLOC 
#include "dmalloc.h"
#endif

/****************************
 database functions. 
 NOTE: Don't use with multithreaded programs!
 ***************************/
void close_database(dbi_conn conn)
{
  dbi_conn_close(conn);
}

dbi_conn open_database(const char *dbtype, const char *dbhost, const char *dbport, const char *dbname, const char *dbuser, const char *dbpasswd)
{
  dbi_conn conn;

  dbi_initialize(NULL);
  conn = dbi_conn_new(dbtype);
  if ( ! conn ) { LOG(LOG_ERR, "Cannot start a connection with driver %s", dbtype); }

  dbi_conn_set_option(conn, "host", dbhost);
  dbi_conn_set_option(conn, "port", dbport);
  dbi_conn_set_option(conn, "username", dbuser);
  dbi_conn_set_option(conn, "password", dbpasswd);
  dbi_conn_set_option(conn, "dbname", dbname);
  dbi_conn_set_option(conn, "encoding", "UTF-8");

  if (dbi_conn_connect(conn) < 0) {
    const char *errmsg;
    dbi_conn_error(conn, &errmsg);
    LOG(LOG_ERR, "%s dbhost=%s,dbport=%d,dbname=%s,dbuser=%s,dbpasswd=%s",
           errmsg, dbhost, dbport, dbname, dbuser, dbpasswd);
    return(NULL);
  }
  return(conn);
}

dbi_result db_rawquery(dbi_conn conn, int log_dupes, const char *qry)
{
  dbi_result result;

  if (debug > 4) {
    LOGRAW(LOG_DEBUG, qry);
  }
  result = dbi_conn_query(conn, qry);
  if (dbi_conn_error_flag(conn)) {
    const char *errmsg;
    dbi_conn_error(conn, &errmsg);
/*
    switch (mysql_errno(mysql)) {
    case ER_DUP_ENTRY:
        if (!log_dupes) break;
    default:
*/
    LOG(LOG_WARNING, "%s: %s", qry, errmsg);
/*
      break;
    }
*/
    return(NULL);
  }
  return(result);
}

dbi_result db_query(dbi_conn conn, int log_dupes, const char *fmt, ...)
{
static char qry[65536]; // max query size for us
  va_list arg;

  if (conn) return(NULL);

  va_start(arg, fmt);
  vsnprintf(qry, sizeof(qry)-1, fmt, arg);
  va_end(arg);
  return db_rawquery(conn, log_dupes, qry);
}

