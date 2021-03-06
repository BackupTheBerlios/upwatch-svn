#include "config.h"
#include <ctype.h>
#include <generic.h>
#include <sys/time.h>
#include <sys/types.h>

#include "uw_syncprobes.h"

int init(void)
{
  daemonize = TRUE;
  every = EVERY_MINUTE;
  xmlSetGenericErrorFunc(NULL, UpwatchXmlGenericErrorFunc);
  return(1);
}

void sync_table(database *upwatch, struct dbspec *db, const char *table);

int run(void)
{
  dbi_result result;
  database *upwatch;

  upwatch = open_database(OPT_ARG(DBTYPE), OPT_ARG(DBHOST), OPT_VALUE_DBPORT, OPT_ARG(DBNAME),
                        OPT_ARG(DBUSER), OPT_ARG(DBPASSWD));
  if (!upwatch) return 0;

  LOG(LOG_INFO, "syncing..");
  uw_setproctitle("reading info from database");
  result = db_query(upwatch, 0, "select pr_realm.id, pr_realm.name, pr_realm.host, "
                                "       pr_realm.port, pr_realm.dbtype, pr_realm.dbname, pr_realm.dbuser, "
                                "       pr_realm.dbpassword, probe.name as tbl "
                                "from   probe, pr_realm " 
                                "where  probe.id > 1 and pr_realm.id > 1");
  if (result) {
    while (dbi_result_next_row(result)) {
      struct dbspec db;

      db.domid = dbi_result_get_int(result, "id");
      strcpy(db.realm, dbi_result_get_string(result, "name"));
      strcpy(db.host, dbi_result_get_string_copy(result, "host"));
      db.port = dbi_result_get_int(result, "port");
      strcpy(db.dbtype, dbi_result_get_string(result, "dbtype"));
      strcpy(db.dbname, dbi_result_get_string(result, "dbname"));
      strcpy(db.dbuser, dbi_result_get_string(result, "dbuser"));
      strcpy(db.dbpassword, dbi_result_get_string(result, "dbpassword"));
      uw_setproctitle("synching %s:pr_%s_def", db.realm, dbi_result_get_string(result, "tbl"));
      LOG(LOG_DEBUG, "syncing %s:pr_%s_def", db.realm, dbi_result_get_string(result, "tbl"));
      sync_table(upwatch, &db, dbi_result_get_string(result, "tbl"));
    }
    dbi_result_free(result);
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
struct deftable *read_def(database *db, const char *table, int realmid, int isaggr, int *count)
{
  dbi_result result;
  struct deftable *tbl;
  int tbl_max=1000;

  if (isaggr) {
    result = db_query(db, 0, "select id, domid, tblid, unix_timestamp(changed) as changed " 
                             "from   pr_%s_def "
                             "where  id > 1 and tblid > 1 and domid = %u "
                             "order  by tblid asc", table, realmid);
  } else {
    result = db_query(db, 0, "select id, domid, tblid, unix_timestamp(changed) as changed " 
                             "from   pr_%s_def "
                             "where  id > 1 "
                             "order  by id asc", table);
  }
  if (!result) {
    return(NULL);
  }
  tbl = calloc(SIZE_STEP, sizeof(struct deftable));
  *count = 0; // preset
  while (dbi_result_next_row(result)) {
    tbl[*count].id = dbi_result_get_int(result, "id");
    tbl[*count].domid = dbi_result_get_int(result, "domid");
    tbl[*count].tblid = dbi_result_get_int(result, "tblid");
    tbl[*count].changed = dbi_result_get_int(result, "changed");

    if (++(*count) == tbl_max) {
      tbl_max += 1000;
      tbl = realloc(tbl, tbl_max * sizeof(struct deftable));
    }
  }
  dbi_result_free(result);
  return(tbl);
}

void update_record(database *db, const char *name, unsigned tblid, unsigned upwid, database *upwatch)
{
  dbi_result result;
  int i, numfields;
  char buffer[8192], tmp[8192];

  result = db_query(db, 0, "select * from pr_%s_def where id = '%u'", name, tblid);
  if (!result) return;

  if (!dbi_result_next_row(result)) {
    dbi_result_free(result);
    return;
  }

  numfields = dbi_result_get_numfields(result);

  sprintf(buffer, "update pr_%s_def (", name);
  for (i=3; i<numfields; i++) {
    sprintf(tmp, "%s, ", dbi_result_get_field_name(result, i));
    strcat(buffer, tmp);
  }
  buffer[strlen(buffer)-2] = 0; // kill last comma and space
  strcat(buffer, ") values (");
  for (i=3; i<numfields; i++) {
    char *escaped;

    strcpy(escaped, dbi_result_get_string_idx(result, i));
    dbi_conn_escape_string(db->conn, &escaped);
    sprintf(tmp, "'%s', ", escaped);
    strcat(buffer, tmp);
    free(escaped);
  }
  buffer[strlen(buffer)-2] = 0;
  sprintf(tmp, ") where id = '%u'", upwid);
  strcat(buffer, tmp);
  dbi_result_free(result);

  result = db_query(upwatch, 0, buffer);
  if (result) dbi_result_free(result);
}

void delete_record(database *upwatch, const char *name, unsigned id)
{
  dbi_result result;

  result = db_query(upwatch, 0, "delete from pr_%s_def where id = '%u'", name, id);
  if (result) dbi_result_free(result);

}

void insert_record(database *db, const char *name, unsigned tblid, unsigned realmid, database *upwatch)
{
  dbi_result result;
  int i, numfields;
  char buffer[8192], tmp[8192];

  result = db_query(db, 0, "select * from pr_%s_def where id = '%u'", name, tblid);
  if (!result) return;

  if (dbi_result_next_row(result)) {
    dbi_result_free(result);
    return;
  }

  numfields = dbi_result_get_numfields(result);

  sprintf(buffer, "insert into pr_%s_def (domid, tblid, ", name);
  for (i=3; i<numfields; i++) {
    sprintf(tmp, "%s, ", dbi_result_get_field_name(result, i));
    strcat(buffer, tmp);
  }
  buffer[strlen(buffer)-2] = 0;
  sprintf(tmp, ") values ('%u', tblid = '%u', ", realmid, tblid);
  strcat(buffer, tmp);
  for (i=3; i<numfields; i++) {
    char *escaped;

    strcpy(escaped, dbi_result_get_string_idx(result, i));
    dbi_conn_escape_string(db->conn, &escaped);
    sprintf(tmp, "'%s', ", escaped);
    strcat(buffer, tmp);
    free(escaped);
  }
  buffer[strlen(buffer)-2] = 0;
  dbi_result_free(result);
  result = db_query(upwatch, 0, buffer);
  if (result) dbi_result_free(result);
}

void sync_table(database *upwatch, struct dbspec *dbspec, const char *table)
{
  database *db;
  struct deftable *tbl, *upw;
  int tbl_size=0, upw_size=0;
  int i, j;
  int added=0, updated=0, deleted=0;

  db = open_database(dbspec->dbtype, dbspec->host, dbspec->port, dbspec->dbname,
                     dbspec->dbuser, dbspec->dbpassword);
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

