

--
-- Table structure for table 'probe'
--

CREATE TABLE probe (
  id serial,
  name varchar(24) NOT NULL default '',
  description varchar(128) NOT NULL default '',
  expiry varchar(3) not null default 'yes' CHECK (expiry IN ('yes','no')),
  addbyhand varchar(3) not null default 'yes' CHECK (addbyhand IN ('yes','no')),
  fuse varchar(3) not null default 'no' CHECK (fuse IN ('yes','no')),
  addbycust varchar(3) not null default 'no' CHECK (addbycust IN ('yes','no')),
  lagwarn varchar(3) not null default 'no' CHECK (lagwarn IN ('yes','no')),
  maxlag uinteger NOT NULL default '300',
  lastseen uinteger NOT NULL default '0',
  class varchar(16) NOT NULL default '',
  graphgroup varchar(16) NOT NULL default '',
  graphtypes varchar(255) NOT NULL default 'default',
  comment text NOT NULL,
  PRIMARY KEY  (id)
);

INSERT INTO probe (id, name, addbyhand, comment) values('1', 'empty', 'no', 'do NOT delete!');

