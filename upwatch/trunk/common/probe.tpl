[+ AutoGen5 template mysql def_h res_h enum +]
[+ CASE (suffix) +][+
   == mysql +]--
[+(dne "-- ")+]
[+ FOR probe +]
--
-- Table structure for table 'pr_[+name+]_def'
--

CREATE TABLE pr_[+name+]_def (
  id int NOT NULL auto_increment,  		-- probe unique numerical id
  pgroup int unsigned NOT NULL default '2', 	-- group id 
  server int unsigned NOT NULL default '1', 	-- server id
  contact int unsigned NOT NULL default '1',	-- user field: pointer to contact database
  notify int unsigned NOT NULL default '1',	-- notifier id
  ipaddress varchar(15) NOT NULL default '',	-- target ipaddress 
  description text NOT NULL default '',		-- description
  freq smallint unsigned NOT NULL default '1',	-- frequency in minutes
  yellow float NOT NULL default '[+yellow+]',	-- value for yellow alert
  red float NOT NULL default '[+red+]', 	-- value for red alert [+ 		
FOR def +]
  [+name+] [+type+][+
IF length +]([+length+])[+ 
ENDIF +] [+
IF unsigned +]unsigned [+
ENDIF+][+
IF null +][+ 
ELSE +]NOT [+
ENDIF null +]NULL default '[+default+]',	-- [+descrip+][+ 
ENDFOR def +]
  PRIMARY KEY  (id),
  KEY server (server),
  KEY notify (notify),
  KEY ipaddress (ipaddress)
) TYPE=MyISAM;
[+
  (if (not (exist? "id"))
      (error (sprintf "Missing probe id for probe %s" (get "probe.name")) )
  )
  (if (not (exist? "addbyhand"))
      (error (sprintf "addbyhand not set for probe %s" (get "probe.name")) )
  )
+]
INSERT into pr_[+name+]_def set id = '1', description = 'empty';
INSERT into probe set id = '[+id+]', name = '[+name+]', description = '[+descrip+]', addbyhand = '[+addbyhand+]', class = '[+class+]', graphtypes = '[+graphtypes+]', comment = '[+comment+]';
[+ IF (count "result") +]
--
-- Table structure for table 'pr_[+name+]_raw'
--

CREATE TABLE pr_[+name+]_raw (
  id bigint unsigned NOT NULL auto_increment,   -- unique id for result
  probe int unsigned NOT NULL default '1',      -- probe identifier
  yellow float NOT NULL default '[+yellow+]',   -- value for yellow alert
  red float NOT NULL default '[+red+]',         -- value for red alert
  stattime int unsigned NOT NULL default '0',   -- time when result was generated
  color smallint unsigned NOT NULL default '200', -- color value [+
FOR result +]
  [+name+] [+type+][+
IF length +]([+length+])[+
ENDIF +] [+
IF null +][+
ELSE +]NOT [+
ENDIF null +]NULL default '[+default+]',        -- [+descrip+][+
ENDFOR result +]
  message text NOT NULL default '',
  PRIMARY KEY (id),
  UNIQUE KEY probstat (probe,stattime)
) TYPE=MyISAM;

[+ FOR period +]
--
-- Table structure for table 'pr_[+name+]_[+period+]'
--

CREATE TABLE pr_[+name+]_[+period+] (
  id bigint unsigned NOT NULL auto_increment,	-- unique id for result
  probe int unsigned NOT NULL default '1',	-- probe identifier
  yellow float NOT NULL default '[+yellow+]',	-- value for yellow alert
  red float NOT NULL default '[+red+]', 	-- value for red alert 
  slot smallint(5) unsigned NOT NULL default '0', -- timeslot in this period
  stattime int unsigned NOT NULL default '0',	-- time when result was generated
  color smallint unsigned NOT NULL default '200', -- color value [+ 
FOR result +]
  [+name+] [+type+][+
IF length +]([+length+])[+ 
ENDIF +] [+
IF null +][+ 
ELSE +]NOT [+
ENDIF null +]NULL default '[+default+]',	-- [+descrip+][+ 
ENDFOR result +]
  message text NOT NULL default '',
  PRIMARY KEY (id),
  UNIQUE KEY probstat (probe,stattime)
) TYPE=MyISAM;
[+ ENDFOR period +]
[+ ENDIF  result +]
[+ ENDFOR probe +]
[+ == def_h
+]/*
[+(dne " * ")+]
 */
/* probe specific part of C struct for [+table+] definition */
[+ FOR probe+][+ FOR def +][+ CASE type +][+ 
== float +]
  float *[+name+];	/* [+descrip+] */[+ 
== varchar +]
  char *[+name+];	/* [+descrip+] */[+ 
== text +]
  char *[+name+];	/* [+descrip+] */[+
== char +]
  char *[+name+];	/* [+descrip+] */[+
== tinyint +][+
IF unsigned +]unsigned [+
ENDIF+]
  char *[+name+];	/* [+descrip+] */[+
== int +][+
IF unsigned +]unsigned [+
ENDIF+]
  char *[+name+];	/* [+descrip+] */[+ ESAC type +][+ ENDFOR def+][+ENDFOR probe+]
[+ == res_h
+]/*
[+(dne " * ")+]
 */
/* probe specific part of C struct for [+table+] results */
[+ FOR probe +][+ FOR result +][+ CASE type +][+ 
== float +]
  float [+name+];	/* [+descrip+] */[+ 
== varchar +]
  char *[+name+];	/* [+descrip+] */[+ 
== text +]
  char *[+name+];	/* [+descrip+] */[+
== char +]
  char *[+name+];	/* [+descrip+] */[+
== tinyint +][+
IF unsigned +]unsigned [+
ENDIF+]
  int [+name+];	/* [+descrip+] */[+
== int +][+
IF unsigned +]unsigned [+
ENDIF+]
  int [+name+];	/* [+descrip+] */[+ ESAC type +][+ ENDFOR def+][+ ENDFOR probe +]
[+ == enum +][+ FOR probe 
+]PROBE_[+name+] = [+id+],
[+ ENDFOR probe +][+ ESAC +]
