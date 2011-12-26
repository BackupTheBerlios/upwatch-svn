
--
-- Table structure for table 'pr_realm'
--

CREATE TABLE pr_realm (
  id serial,
  name varchar(24) NOT NULL default '',
  host varchar(64) NOT NULL default '',
  port uinteger NOT NULL default '3306',
  db varchar(24) NOT NULL default '',
  "user" varchar(24) NOT NULL default '',
  password varchar(24) NOT NULL default '',
  srvrbyname varchar(255) NOT NULL default '',
  srvrbyid varchar(245) NOT NULL default '',
  srvrbyip varchar(245) NOT NULL default '',
  comment text NOT NULL,
  PRIMARY KEY  (id)
);

INSERT INTO pr_realm (id, name, comment) values ('1', 'empty', 'do NOT delete!');

CREATE TABLE pr_regex (
  id serial,
  logtype varchar(24) NOT NULL default '',
  package varchar(24) NOT NULL default '',
  action varchar(6) NOT NULL default 'green' CHECK(action in ('ignore', 'red', 'yellow', 'green')),
  regex varchar(255) NOT NULL,
  PRIMARY KEY  (id)
);

CREATE UNIQUE INDEX pr_regex_packex_index on pr_regex (package,regex);
INSERT INTO pr_regex (id, logtype, package, action, regex) values ('1', 'none', 'none', 'green', 'do NOT delete!');

CREATE TABLE pr_macros (
  id serial,
  logtype varchar(24) NOT NULL default '',
  name varchar(24) NOT NULL default '',
  regex varchar(255) NOT NULL,
  PRIMARY KEY  (id)
);

CREATE UNIQUE INDEX pr_macros_typename_index on pr_macros (logtype, name);
INSERT INTO pr_macros (id, logtype, name, regex) values ('1', 'none', 'empty', 'do NOT delete!');

CREATE TABLE pr_rmacros (
  id serial,
  logtype varchar(24) NOT NULL default '',
  regex varchar(255) NOT NULL,
  name varchar(24) NOT NULL default '',
  PRIMARY KEY  (id)
);

CREATE UNIQUE INDEX pr_rmacros_typex_index on pr_rmacros (logtype, regex);
INSERT INTO pr_rmacros (id, logtype, name, regex) values ('1', 'none', 'empty', 'do NOT delete!');

