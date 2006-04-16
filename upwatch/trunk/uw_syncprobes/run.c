#include "config.h"
#include <ctype.h>
#include <generic.h>
#include <sys/time.h>
#include <sys/types.h>

#include "uw_syncprobes.h"

struct dbspec {
  int domid;
  char *realm;
  char *host;
  int port;
  char *db;
  char *user;
  char *password;
};

int init(void)
{
  daemonize = TRUE;
  every = EVERY_MINUTE;
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

void sync_table(MYSQL *upwatch, struct dbspec *db, char *table);

int run(void)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  MYSQL *upwatch;

  upwatch = open_database(OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME),
                        OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
  if (!upwatch) return 0;

  LOG(LOG_INFO, "syncing..");
  uw_setproctitle("reading info from database");
  result = my_query(upwatch, 0, "select pr_realm.id, pr_realm.name, pr_realm.host, "
                                "       pr_realm.port, pr_realm.db, pr_realm.user, "
                                "       pr_realm.password, probe.name as tbl "
                                "from   probe, pr_realm " 
                                "where  probe.id > 1 and pr_realm.id > 1");
  if (result) {
    while ((row = mysql_fetch_row(result)) != NULL) {
      struct dbspec db;

      db.domid = atoi(row[0]);
      db.realm = row[1];
      db.host = row[2];
      db.port = atoi(row[3]);
      db.db = row[4];
      db.user = row[5];
      db.password = row[6];
      uw_setproctitle("synching %s:pr_%s_def", row[1], row[7]);
      LOG(LOG_DEBUG, "syncing %s:pr_%s_def", row[1], row[7]);
      sync_table(upwatch, &db, row[7]);
    }
    mysql_free_result(result);
  }
  close_database(upwatch);
  LOG(LOG_INFO, "sleeping");
  return 1;
}

struct deftable {
  unsigned id;
  unsigned int domid;
  unsigned int tblid;
  time_t changed;
};
 
#define SIZE_STEP 1000
struct deftable *read_def(MYSQL *db, char *table, int realmid, int isaggr, int *count)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct deftable *tbl;
  int tbl_max=1000;

  if (isaggr) {
    result = my_query(db, 0, "select id, domid, tblid, unix_timestamp(changed) " 
                             "from   pr_%s_def "
                             "where  id > 1 and tblid > 1 and domid = %u "
                             "order  by tblid asc", table, realmid);
  } else {
    result = my_query(db, 0, "select id, domid, tblid, unix_timestamp(changed) " 
                             "from   pr_%s_def "
                             "where  id > 1 "
                             "order  by id asc", table);
  }
  if (!result) {
    return(NULL);
  }
  tbl = calloc(SIZE_STEP, sizeof(struct deftable));
  *count = 0; // preset
  while ((row = mysql_fetch_row(result)) != NULL) {
    tbl[*count].id = atoi(row[0]);
    tbl[*count].domid = atoi(row[1]);
    tbl[*count].tblid = atoi(row[2]);
    tbl[*count].changed = atoi(row[3]);

    if (++(*count) == tbl_max) {
      tbl_max += 1000;
      tbl = realloc(tbl, tbl_max * sizeof(struct deftable));
    }
  }
  mysql_free_result(result);
  return(tbl);
}

void update_record(MYSQL *db, char *name, unsigned tblid, unsigned upwid, MYSQL *upwatch)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  MYSQL_FIELD *fields;
  int i, numfields;
  char buffer[8192], tmp[8192];

  result = my_query(db, 0, "select * from pr_%s_def where id = '%u'", name, tblid);
  if (!result) return;

  row = mysql_fetch_row(result);
  if (!row) {
    mysql_free_result(result);
    return;
  }

  numfields = mysql_num_fields(result);
  fields = mysql_fetch_fields(result);

  sprintf(buffer, "update pr_%s_def set ", name);
  for (i=3; i<numfields; i++) {
    char escaped[16384];

    mysql_real_escape_string(db, escaped, row[i], strlen(row[i]));
    sprintf(tmp, "%s = '%s', ", fields[i].name, escaped);
    strcat(buffer, tmp);
  }
  buffer[strlen(buffer)-2] = 0;
  sprintf(tmp, " where id = '%u'", upwid);
  strcat(buffer, tmp);
  mysql_free_result(result);

  result = my_query(upwatch, 0, buffer);
  if (result) mysql_free_result(result);
}

void delete_record(MYSQL *upwatch, char *name, unsigned id)
{
  MYSQL_RES *result;

  result = my_query(upwatch, 0, "delete from pr_%s_def where id = '%u'", name, id);
  if (result) mysql_free_result(result);

}

void insert_record(MYSQL *db, char *name, unsigned tblid, unsigned realmid, MYSQL *upwatch)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  MYSQL_FIELD *fields;
  int i, numfields;
  char buffer[8192], tmp[8192];

  result = my_query(db, 0, "select * from pr_%s_def where id = '%u'", name, tblid);
  if (!result) return;

  row = mysql_fetch_row(result);
  if (!row) {
    mysql_free_result(result);
    return;
  }

  numfields = mysql_num_fields(result);
  fields = mysql_fetch_fields(result);

  sprintf(buffer, "insert into pr_%s_def set domid = '%u', tblid = '%u', ", name, realmid, tblid);
  for (i=3; i<numfields; i++) {
    char escaped[16384];

    mysql_real_escape_string(db, escaped, row[i], strlen(row[i]));
    sprintf(tmp, "%s = '%s', ", fields[i].name, escaped);
    strcat(buffer, tmp);
  }
  buffer[strlen(buffer)-2] = 0;
  mysql_free_result(result);

  result = my_query(upwatch, 0, buffer);
  if (result) mysql_free_result(result);
}

void sync_table(MYSQL *upwatch, struct dbspec *dbspec, char *table)
{
  MYSQL *db;
  struct deftable *tbl, *upw;
  int tbl_size=0, upw_size=0;
  int i, j;
  int added=0, updated=0, deleted=0;

  db = open_database(dbspec->host, dbspec->port, dbspec->db,
                     dbspec->user, dbspec->password);
  if (!db) return;

  tbl = read_def(db, table, dbspec->domid, FALSE, &tbl_size);
  if (!tbl) {
    close_database(db);
    return;
  }

  upw = read_def(upwatch, table, dbspec->domid, TRUE, &upw_size);
  if (!upw) {
    free(tbl);
    close_database(db);
    return;
  }

// tb[i] 1 2 3 4 5 6 7 8 9
//           | 
// uw[j] 1 2 4 5 6 7 8 9

  // now sync the tables
  for (i=0,j=0;;) {
    if (i == tbl_size) { // end of source table
      if (j == upw_size) break; // we're through
      delete_record(upwatch, table, upw[j].id);
      deleted++;
      j++;
      continue;
    }
    if (j == upw_size) { // end of dest table
      insert_record(db, table, tbl[i].id, dbspec->domid, upwatch);
      added++;
      i++;
      continue;
    }
    if (tbl[i].id == upw[j].tblid) { // ok, found
      if (tbl[i].changed > upw[j].changed) {
        update_record(db, table, tbl[i].id, upw[j].id, upwatch);
        updated++;
      }
      i++;
      j++;
      continue;
    }
    if (tbl[i].id < upw[j].tblid) {
      insert_record(db, table, tbl[i].id, dbspec->domid, upwatch);
      added++;
      i++;
      continue;
    }
    if (tbl[i].id > upw[j].tblid) {
      delete_record(upwatch, table, upw[j].id);
      deleted++;
      j++;
      continue;
    }
  }
  free(upw);
  free(tbl);
  close_database(db);
  if (added || updated || deleted) {
    LOG(LOG_INFO, "synced %s:pr_%s_def, added %u, updated %u, deleted %u", 
                    dbspec->realm, table, added, updated, deleted);
  }
}

