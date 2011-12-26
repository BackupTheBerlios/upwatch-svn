--
-- Table structure for table 'pr_group'
--

CREATE TABLE pr_group (
  id serial,
  name varchar(24) NOT NULL default '',
  description varchar(128) NOT NULL default '',
  PRIMARY KEY  (id)
);

INSERT INTO pr_group (id, name, description) values ('1', 'empty', 'do NOT delete!');
INSERT INTO pr_group (id, name) values ('2', 'Default');

--
-- Table structure for table 'pr_hist'
--

CREATE TABLE pr_hist (
  id serial,
  server uinteger NOT NULL default '1',
  hide varchar(3) not null default 'no' CHECK (hide IN ('yes','no')),
  contact uinteger NOT NULL default '1',  -- user field: pointer to contact database
  class usmallint NOT NULL default '1',
  probe uinteger NOT NULL default '1',
  pgroup uinteger NOT NULL default '1',     -- group id
  stattime uinteger NOT NULL default '0',
  prv_color usmallint NOT NULL default '200',
  color usmallint NOT NULL default '200',
  message text NOT NULL default '',
  PRIMARY KEY  (id)
);

CREATE UNIQUE INDEX pr_hist_probtime_index on pr_hist (probe,stattime);
CREATE UNIQUE INDEX pr_hist_probclstat_index on pr_hist (probe,class,stattime);
CREATE INDEX pr_hist_stattime_index on pr_hist (stattime);
CREATE INDEX pr_hist_contact_index on pr_hist (contact);
CREATE INDEX pr_hist_statprobcl_index on pr_hist (stattime,probe,class);
CREATE INDEX pr_hist_probclstatnot_index on pr_hist (probe,class,stattime,notified);
CREATE INDEX pr_hist_contacttime_index on pr_hist (contacttime);
CREATE INDEX pr_hist_server_index on pr_hist (server);

INSERT INTO pr_hist (id, message) values ('1', 'do NOT delete!');

--
-- Table structure for table 'pr_status'
--

CREATE TABLE pr_status (
  id serial,
  class uinteger NOT NULL default '1',
  probe uinteger NOT NULL default '1',
  server uinteger NOT NULL default '1',
  hide varchar(3) not null default 'no' CHECK (hide IN ('yes','no')),
  contact uinteger NOT NULL default '1',  -- user field: pointer to contact database
  yellow float NOT NULL default '0',
  red float NOT NULL default '0',
  stattime uinteger NOT NULL default '0',
  expires uinteger NOT NULL default '0',
  color usmallint NOT NULL default '200',
  message text NOT NULL,
  PRIMARY KEY  (id)
);

CREATE UNIQUE INDEX pr_status_uprobe_index on pr_status (class,probe);
CREATE INDEX pr_status_class_index on pr_status (class);
CREATE INDEX pr_status_probe_index on pr_status (probe);
CREATE INDEX pr_status_server_index on pr_status (server);
CREATE INDEX pr_status_expires_index on pr_status (expires);
CREATE INDEX pr_status_contact_index on pr_status (contact);

INSERT INTO pr_status (id, message) values ('1', 'do NOT delete!');

--
-- Table structure for table 'rack'
--
  
CREATE TABLE rack (
  id serial,
  contact int NOT NULL default 1,
  name varchar(128) NOT NULL default '',
  comment text NOT NULL,
  color usmallint NOT NULL default '0',
  PRIMARY KEY  (id) 
);

INSERT INTO rack (id, name, comment) values ('1', 'empty', 'do NOT delete!');

--
-- Table structure for table 'server'
--

CREATE TABLE server (
  id serial,
  name varchar(128) NOT NULL default '',
  os varchar(32) NOT NULL default '',
  comment text NOT NULL,
  color usmallint NOT NULL default '0',
  PRIMARY KEY  (id)
);

INSERT INTO server (id, name, comment) values ('1', 'empty', 'do NOT delete!');

--
-- Table structure for table 'notify'
--

CREATE TABLE notify (
  id serial,
  name varchar(128) NOT NULL default '',
  contact int NOT NULL default 1,
  mail int NOT NULL default 1,
  sms int NOT NULL default 1,
  status enum('enabled', 'disabled', 'ok') NOT NULL default 'enabled',
  comment text NOT NULL,
  PRIMARY KEY  (id)
);

CREATE INDEX notify_contact_index on notify (contact);

INSERT INTO notify (id, name, status, comment) values ('1', 'empty', 'disabled', 'do NOT delete!');

--
-- Table structure for table 'notifycond'
--

CREATE TABLE notifycond (
  id serial,
  notify int NOT NULL default 1,
  bycolor varchar(3) not null default 'yes' CHECK (bycolor IN ('yes','no')),
  atcolor smallint NOT NULL default '400',
  consecutive smallint NOT NULL default 1,
  bydate varchar(3) not null default 'no' CHECK (bydate IN ('yes','no')),
  fromwday tinyint NOT NULL default 0,
  fromhour tinyint NOT NULL default 0,
  frommin tinyint NOT NULL default 0,
  towday tinyint NOT NULL default 0,
  tohour tinyint NOT NULL default 0,
  tomin tinyint NOT NULL default 0,
  persist varchar(3) not null default 'no' CHECK (persist IN ('yes','no')),
  comment text NOT NULL,
  PRIMARY KEY  (id),
  KEY (notify)
);

INSERT INTO notifycond (id, notify) values ('1', '1');

