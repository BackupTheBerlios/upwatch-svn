[+ AutoGen5 template def_mysql raw_mysql def_h res_h enum xml dtd dtd-inc +]
[+ CASE (suffix) +][+
   == def_mysql +]--
[+(dne "-- ")+]
[+ FOR probe +]
--
-- Table structure for table 'pr_[+name+]_def'
--

CREATE TABLE pr_[+name+]_def (
-- PLEASE NOTE THE FIRST FIELDS MUST STAY THIS WAY.
  id int NOT NULL auto_increment,  		-- probe unique numerical id
-- the following fields are only used when the probe definitions are aggregated
-- in one central database. 
  domid int unsigned NOT NULL default '1',      -- the id of the database this probe belongs to
  tblid int unsigned NOT NULL default '1',      -- the unique id of the probe in that database
  changed timestamp NOT NULL,			-- 
-- end aggregation fields
  pgroup int unsigned NOT NULL default '2', 	-- group id 
  server int unsigned NOT NULL default '1', 	-- server id
  contact int unsigned NOT NULL default '1',	-- user field: pointer to contact database
  notify int unsigned NOT NULL default '1',	-- notifier id
  email varchar(64) NOT NULL default '',	-- address to send warning email to
  delay int unsigned NOT NULL default '1',	-- after this many minutes of red light
  disable enum('yes', 'no') not null default 'no', -- disable this probe
  hide enum('yes', 'no') not null default 'no', -- hide probe results from viewing
  ipaddress varchar(15) NOT NULL default '127.0.0.1',	-- target ipaddress 
  description text NOT NULL default '',		-- description
  freq smallint unsigned NOT NULL default '1',	-- frequency in minutes
  yellow float NOT NULL default '[+yellow+]',	-- value for yellow alert
  red float NOT NULL default '[+red+]', 	-- value for red alert [+ 		
FOR def +][+
CASE type +][+
== float +]
  [+name+] [+type+][+
IF null +][+
ELSE +] NOT[+
ENDIF null +] NULL default '[+default+]',       -- [+descrip+][+
== varchar +]
  [+name+] [+type+][+
IF length +]([+length+])[+
ENDIF +][+
IF null +][+
ELSE +] NOT[+
ENDIF null +] NULL default '[+default+]',       -- [+descrip+][+
== text +]
  [+name+] [+type+][+
IF null +][+
ELSE +] NOT[+
ENDIF null +] NULL default '[+default+]',       -- [+descrip+][+
== char +]
  [+name+] [+type+][+
IF length +]([+length+])[+
ENDIF +][+
IF null +][+
ELSE +] NOT[+
ENDIF null +] NULL default '[+default+]',       -- [+descrip+][+
== bool +]
  [+name+] enum('yes', 'no')[+
IF null +][+
ELSE +] NOT[+
ENDIF null +] NULL default '[+default+]',       -- [+descrip+][+
== enum +]
  [+name+] enum([+enumval+]) [+
IF null +][+
ELSE +] NOT[+
ENDIF null +] NULL default '[+default+]',       -- [+descrip+][+
== tinyint +]
  [+name+] [+type+][+
IF unsigned +] unsigned[+
ENDIF+][+
IF null +][+
ELSE +] NOT[+
ENDIF null +] NULL default '[+default+]',       -- [+descrip+][+
== int +]
  [+name+] [+type+][+
IF unsigned +] unsigned[+
ENDIF+][+
IF null +][+
ELSE +] NOT[+
ENDIF null +] NULL [+
IF auto +]auto_increment,       -- [+descrip+][+
ELSE +]default '[+default+]',   -- [+descrip+][+
ENDIF +][+
== bigint +]
  [+name+] [+type+][+
IF unsigned +] unsigned[+
ENDIF+][+
IF null +][+
ELSE +] NOT[+
ENDIF null +] NULL [+
IF auto +]auto_increment,       -- [+descrip+][+
ELSE +]default '[+default+]',   -- [+descrip+][+
ENDIF +][+
ESAC type +][+ ENDFOR def+]
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
INSERT into probe set id = '[+id+]', name = '[+name+]', description = '[+descrip+]', [+
 if (exist? "expiry") +]expiry = '[+expiry+]', [+ endif +]addbyhand = '[+addbyhand+]', [+
 if (exist? "fuse") +]fuse = '[+fuse+]', [+ endif +]class = '[+class+]', graphgroup = '[+graphgroup+]', graphtypes = '[+graphtypes+]', comment = '[+comment+]';[+ ENDFOR probe +][+
   == raw_mysql +]--
[+(dne "-- ")+]
[+ FOR probe +]
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
FOR result +][+
CASE type +][+
== float +]
  [+name+] [+type+][+
IF null +][+
ELSE +] NOT[+
ENDIF null +] NULL default '[+default+]',       -- [+descrip+][+
== varchar +]
  [+name+] [+type+][+
IF length +]([+length+])[+
ENDIF +][+
IF null +][+
ELSE +] NOT[+
ENDIF null +] NULL default '[+default+]',       -- [+descrip+][+
== text +]
  [+name+] [+type+][+
IF null +][+
ELSE +] NOT[+
ENDIF null +] NULL default '[+default+]',       -- [+descrip+][+
== char +]
  [+name+] [+type+][+
IF length +]([+length+])[+
ENDIF +][+
IF null +][+
ELSE +] NOT[+
ENDIF null +] NULL default '[+default+]',       -- [+descrip+][+
== bool +]
  [+name+] enum('yes', 'no')[+
IF null +][+
ELSE +] NOT[+
ENDIF null +] NULL default '[+default+]',       -- [+descrip+][+
== tinyint +]
  [+name+] [+type+][+
IF unsigned +] unsigned[+
ENDIF+][+
IF null +][+
ELSE +] NOT[+
ENDIF null +] NULL default '[+default+]',       -- [+descrip+][+
== int +]
  [+name+] [+type+][+
IF unsigned +] unsigned[+
ENDIF+][+
IF null +][+
ELSE +] NOT[+
ENDIF null +] NULL [+
IF auto +]auto_increment,       -- [+descrip+][+
ELSE +]default '[+default+]',   -- [+descrip+][+
ENDIF +][+
== bigint +]
  [+name+] [+type+][+
IF unsigned +] unsigned[+
ENDIF+][+
IF null +][+
ELSE +] NOT[+
ENDIF null +] NULL [+
IF auto +]auto_increment,       -- [+descrip+][+
ELSE +]default '[+default+]',   -- [+descrip+][+
ENDIF +][+
ESAC type +][+ ENDFOR result+]
  message text NOT NULL default '',
  PRIMARY KEY (id),
  UNIQUE KEY probstat (probe,stattime),
  UNIQUE KEY statprob (stattime,probe)
) TYPE=MyISAM [+
IF max_rows +] MAX_ROWS=[+max_rows+] [+
ENDIF+][+
IF avg_row_length +] AVG_ROW_LENGTH=[+avg_row_length+][+
ENDIF +];

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
  UNIQUE KEY probstat (probe,stattime),
  UNIQUE KEY statprob (stattime,probe)
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
  float [+name+];	/* [+descrip+] */[+ 
== enum +]
  char [+name+][24];    /* [+descrip+] */[+
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
== bigint +][+
IF unsigned +]unsigned [+
ENDIF+]
  long long [+name+];	/* [+descrip+] */[+
== int +][+
IF unsigned +]unsigned [+
ENDIF+]
  int [+name+];		/* [+descrip+] */[+ ESAC type +][+ ENDFOR def+][+ENDFOR probe+]
[+ == res_h
+]/*
[+(dne " * ")+]
 */
/* probe specific part of C struct for [+table+] results */
[+ FOR probe +][+ FOR result +][+ CASE type +][+
== float +]
  float [+name+];       /* [+descrip+] */[+
== enum +]
  char [+name+][24];    /* [+descrip+] */[+
== varchar +]
  char *[+name+];       /* [+descrip+] */[+
== text +]
  char *[+name+];       /* [+descrip+] */[+
== char +]
  char *[+name+];       /* [+descrip+] */[+
== tinyint +]
  [+ IF unsigned +]unsigned [+
ENDIF+]int [+name+];    /* [+descrip+] */[+
== bigint +][+
IF unsigned +]unsigned [+
ENDIF+]
  long long [+name+];	/* [+descrip+] */[+
== int +]
  [+ IF unsigned +]unsigned [+
ENDIF+]int [+name+];    /* [+descrip+] */[+ ESAC type +][+ ENDFOR result+][+ ENDFOR probe +]
[+ == enum +][+ FOR probe 
+]PROBE_[+name+] = [+id+],
[+ ENDFOR probe +]
[+ == dtd-inc
+][+ FOR probe +][+ FOR element +]<!ELEMENT [+name+] (#PCDATA)>		<!-- [+descrip+] -->
[+ ENDFOR element +][+ FOR result +][+
IF (not (exist? "noelement")) +]<!ELEMENT [+name+] (#PCDATA)>		<!-- [+descrip+] -->
[+ ENDIF element +][+ ENDFOR result +][+ ENDFOR probe +][+ == dtd
+][+ FOR probe +]<!-- [+ descrip +] -->
<!ELEMENT [+name+]	([+ FOR element+][+name+][+
IF optional +]?[+ENDIF+],[+ ENDFOR element +][+ FOR result +][+
IF (not (exist? "noelement")) +][+name+],[+ ENDIF element +][+ ENDFOR +]info?,prevcolor?,notify?)>
<!ATTLIST [+name+][+FOR attribute +]	[+name+] NMTOKEN [+
IF (exist? "default") +] "[+default+]"[+ENDIF
+][+IF required+] #REQUIRED[+ENDIF required+]
[+ ENDFOR attribute +]>

[+ ENDFOR probe +][+ == xml
+]<?xml version="1.0" encoding="UTF-8"?>
[+ FOR probe +]

<sect1 id="[+name+]">
   <title>[+name+] - [+descrip+]</title>
   <para>[+doc+]</para>
   <sect2 id="[+name+]_result_layout">
      <title>[+name+] result record layout</title>

      <table label="[+name+] attributes" pgwide="1"><title>[+name+] attributes</title>
         <tgroup cols = "5">
            <colspec colname = "Name" colnum = "1" colwidth = "1.0in"/>
            <colspec colname = "Type" colnum = "2" colwidth = "1.0in"/>
            <colspec colname = "Required" colnum = "3" colwidth = "0.4in"/>
            <colspec colname = "Default" colnum = "4" colwidth = "0.4in"/>
            <colspec colname = "Description" colnum = "5" colwidth = "2.4in"/>
            <tbody>
               <row>
                  <entry colname = "Name"><para>Name</para></entry>
                  <entry colname = "Type"><para>Type </para></entry>
                  <entry colname = "Required"><para>Required</para></entry>
                  <entry colname = "Default"><para>Default </para></entry>
                  <entry colname = "Description"><para>Description</para></entry>
               </row>[+ FOR element +]
               <row>
                  <entry colname = "Name"><para>[+name+] </para></entry>
                  <entry colname = "Type"><para>NMTOKEN </para></entry>[+
 IF required +]
                  <entry colname = "Required"><para>YES </para></entry>[+
 ELSE +]
                  <entry colname = "Required"><para>NO </para></entry>[+
 ENDIF +][+ IF default +]
                  <entry colname = "Default"><para>[+ default +] </para></entry>[+
 ELSE +]
                  <entry colname = "Default"><para> </para></entry>[+
 ENDIF +]
                  <entry colname = "Description"><para>[+descrip+] </para></entry>
               </row>[+ ENDFOR element +][+ FOR result +]
               <row>
                  <entry colname = "Name"><para>[+name+] </para></entry>
                  <entry colname = "Type"><para>NMTOKEN </para></entry>[+
 IF required +]
                  <entry colname = "Required"><para>YES </para></entry>[+
 ELSE +]
                  <entry colname = "Required"><para>NO </para></entry>[+
 ENDIF +][+ IF default +]
                  <entry colname = "Default"><para>[+ default +] </para></entry>[+
 ELSE +]
                  <entry colname = "Default"><para> </para></entry>[+
 ENDIF +]
                  <entry colname = "Description"><para>[+descrip+] </para></entry>
               </row>[+ ENDFOR result +]
           </tbody>
         </tgroup>
      </table>

      <table label="[+name+] elements" pgwide="1"><title>[+name+] elements</title>
         <tgroup cols = "3">
            <colspec colname = "Name" colnum = "1" colwidth = "1.0in"/>
            <colspec colname = "Optional" colnum = "2" colwidth = "0.4in"/>
            <colspec colname = "Description" colnum = "3" colwidth = "3.0in"/>
            <tbody>
               <row>
                  <entry colname = "Name"><para>Name</para></entry>
                  <entry colname = "Optional"><para>Optional</para></entry>
                  <entry colname = "Description"><para>Description</para></entry>
               </row>[+ FOR attribute +]
               <row>
                  <entry colname = "Name"><para>[+name+] </para></entry>[+
 IF optional +]
                  <entry colname = "Optional"><para>YES </para></entry>[+
 ELSE +]
                  <entry colname = "Optional"><para>NO </para></entry>[+
 ENDIF +]
                  <entry colname = "Description"><para>[+descrip+] </para></entry>
               </row>[+ ENDFOR attribute +]
           </tbody>
         </tgroup>
      </table>
   </sect2>
   <sect2 id="[+name+]_database_layout">
      <title>[+name+] database layout</title>

      <table label="[+name+] definition record layout" pgwide="1"><title>[+name+] definition record layout</title>
         <tgroup cols = "6">
            <colspec colname = "Field" colnum = "1" colwidth = "1.0in"/>
            <colspec colname = "Type" colnum = "2" colwidth = "1.0in"/>
            <colspec colname = "Key" colnum = "3" colwidth = "0.4in"/>
            <colspec colname = "Default" colnum = "4" colwidth = "0.4in"/>
            <colspec colname = "Extra" colnum = "5" colwidth = "1.0in"/>
            <colspec colname = "Description" colnum = "6" colwidth = "1.4in"/>
            <tbody>
               <row>
                  <entry colname = "Field"><para>Field </para></entry>
                  <entry colname = "Type"><para>Type </para></entry>
                  <entry colname = "Key"><para>Key </para></entry>
                  <entry colname = "Default"><para>Default </para></entry>
                  <entry colname = "Extra"><para>Extra </para></entry>
                  <entry colname = "Description"><para>Description</para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>id </para></entry>
                  <entry colname = "Type"><para>int </para></entry>
                  <entry colname = "Key"><para>PRI </para></entry>
                  <entry colname = "Default"><para> </para></entry>
                  <entry colname = "Extra"><para>auto_increment </para></entry>
                  <entry colname = "Description"><para>probe unique numerical id </para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>pgroup </para></entry>
                  <entry colname = "Type"><para>int unsigned </para></entry>
                  <entry colname = "Key"><para>NO </para></entry>
                  <entry colname = "Default"><para>2 </para></entry>
                  <entry colname = "Extra"><para> </para></entry>
                  <entry colname = "Description"><para>group id </para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>server </para></entry>
                  <entry colname = "Type"><para>int </para></entry>
                  <entry colname = "Key"><para>NO </para></entry>
                  <entry colname = "Default"><para>1 </para></entry>
                  <entry colname = "Extra"><para> </para></entry>
                  <entry colname = "Description"><para>server id </para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>contact </para></entry>
                  <entry colname = "Type"><para>int unsigned </para></entry>
                  <entry colname = "Key"><para>YES </para></entry>
                  <entry colname = "Default"><para>1 </para></entry>
                  <entry colname = "Extra"><para> </para></entry>
                  <entry colname = "Description"><para>user field: pointer to contact database </para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>notify </para></entry>
                  <entry colname = "Type"><para>int unsigned </para></entry>
                  <entry colname = "Key"><para>YES </para></entry>
                  <entry colname = "Default"><para>1 </para></entry>
                  <entry colname = "Extra"><para> </para></entry>
                  <entry colname = "Description"><para>notifier id</para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>ipaddress </para></entry>
                  <entry colname = "Type"><para>varchar(15) </para></entry>
                  <entry colname = "Key"><para>YES </para></entry>
                  <entry colname = "Default"><para> </para></entry>
                  <entry colname = "Extra"><para> </para></entry>
                  <entry colname = "Description"><para>target ipaddress </para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>description </para></entry>
                  <entry colname = "Type"><para>text </para></entry>
                  <entry colname = "Key"><para>NO </para></entry>
                  <entry colname = "Default"><para> </para></entry>
                  <entry colname = "Extra"><para> </para></entry>
                  <entry colname = "Description"><para>description </para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>freq </para></entry>
                  <entry colname = "Type"><para>smallint unsigned </para></entry>
                  <entry colname = "Key"><para>NO </para></entry>
                  <entry colname = "Default"><para>1 </para></entry>
                  <entry colname = "Extra"><para> </para></entry>
                  <entry colname = "Description"><para>frequency in minutes </para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>yellow </para></entry>
                  <entry colname = "Type"><para>float </para></entry>
                  <entry colname = "Key"><para>NO </para></entry>
                  <entry colname = "Default"><para>[+yellow+] </para></entry>
                  <entry colname = "Extra"><para> </para></entry>
                  <entry colname = "Description"><para>value for yellow alert </para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>red </para></entry>
                  <entry colname = "Type"><para>float </para></entry>
                  <entry colname = "Key"><para>NO </para></entry>
                  <entry colname = "Default"><para>[+red+] </para></entry>
                  <entry colname = "Extra"><para> </para></entry>
                  <entry colname = "Description"><para>value for red alert </para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>disable </para></entry>
                  <entry colname = "Type"><para>enum('yes', 'no') </para></entry>
                  <entry colname = "Key"><para>NO </para></entry>
                  <entry colname = "Default"><para>no </para></entry>
                  <entry colname = "Extra"><para> </para></entry>
                  <entry colname = "Description"><para>disable this probe </para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>hide </para></entry>
                  <entry colname = "Type"><para>enum('yes', 'no') </para></entry>
                  <entry colname = "Key"><para>NO </para></entry>
                  <entry colname = "Default"><para>no </para></entry>
                  <entry colname = "Extra"><para> </para></entry>
                  <entry colname = "Description"><para>hide probe results from viewing </para></entry>
               </row>[+ FOR def +]
               <row>
                  <entry colname = "Field"><para>[+name+] </para></entry>[+
 CASE type +][+ == float +]
                  <entry colname = "Type"><para>[+type+] </para></entry>[+
 == varchar +]
                  <entry colname = "Type"><para>[+type+][+
IF length +]([+length+])[+ ENDIF +] </para></entry>[+
 == text +]
                  <entry colname = "Type"><para>[+type+] </para></entry>[+
 == char +]
                  <entry colname = "Type"><para>[+type+] </para></entry>[+
 == bool +]
                  <entry colname = "Type"><para>enum('yes', 'no') </para></entry>[+
 == tinyint +]
                  <entry colname = "Type"><para>[+type+][+
IF unsigned +] unsigned[+ ENDIF+] </para></entry>[+
 == int +]
                  <entry colname = "Type"><para>[+type+][+
IF unsigned +] unsigned[+ ENDIF+] </para></entry>[+ 
 == bigint +]
                  <entry colname = "Type"><para>[+type+][+
IF unsigned +] unsigned[+ ENDIF+] </para></entry>[+ 
 ESAC type +]
                  <entry colname = "Key"><para>NO </para></entry>
                  <entry colname = "Default"><para>[+default+] </para></entry>[+
 IF auto +]
                  <entry colname = "Extra"><para>auto_increment </para></entry>[+
 ELSE +]
                  <entry colname = "Extra"><para> </para></entry>[+
 ENDIF +]
                  <entry colname = "Description"><para>[+descrip+] </para></entry>
               </row>[+ ENDFOR def+]
           </tbody>
         </tgroup>
      </table>



      <table label="[+name+] result record layout" pgwide="1"><title>[+name+] result record layout</title>
         <tgroup cols = "6">
            <colspec colname = "Field" colnum = "1" colwidth = "1.0in"/>
            <colspec colname = "Type" colnum = "2" colwidth = "1.0in"/>
            <colspec colname = "Key" colnum = "3" colwidth = "0.4in"/>
            <colspec colname = "Default" colnum = "4" colwidth = "0.4in"/>
            <colspec colname = "Extra" colnum = "5" colwidth = "1.0in"/>
            <colspec colname = "Description" colnum = "6" colwidth = "1.4in"/>
            <tbody>
               <row>
                  <entry colname = "Field"><para>Field </para></entry>
                  <entry colname = "Type"><para>Type </para></entry>
                  <entry colname = "Key"><para>Key </para></entry>
                  <entry colname = "Default"><para>Default </para></entry>
                  <entry colname = "Extra"><para>Extra </para></entry>
                  <entry colname = "Description"><para>Description</para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>id </para></entry>
                  <entry colname = "Type"><para>bigint unsigned </para></entry>
                  <entry colname = "Key"><para>PRI </para></entry>
                  <entry colname = "Default"><para> </para></entry>
                  <entry colname = "Extra"><para>auto_increment </para></entry>
                  <entry colname = "Description"><para>unique id for result </para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>probe </para></entry>
                  <entry colname = "Type"><para>int unsigned </para></entry>
                  <entry colname = "Key"><para>YES </para></entry>
                  <entry colname = "Default"><para>1 </para></entry>
                  <entry colname = "Extra"><para> </para></entry>
                  <entry colname = "Description"><para>probe identifier </para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>yellow </para></entry>
                  <entry colname = "Type"><para>float </para></entry>
                  <entry colname = "Key"><para>NO </para></entry>
                  <entry colname = "Default"><para>[+yellow+] </para></entry>
                  <entry colname = "Extra"><para> </para></entry>
                  <entry colname = "Description"><para>value for yellow alert </para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>red </para></entry>
                  <entry colname = "Type"><para>float </para></entry>
                  <entry colname = "Key"><para>NO </para></entry>
                  <entry colname = "Default"><para>[+red+] </para></entry>
                  <entry colname = "Extra"><para> </para></entry>
                  <entry colname = "Description"><para>value for red alert </para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>stattime </para></entry>
                  <entry colname = "Type"><para>int unsigned </para></entry>
                  <entry colname = "Key"><para>YES </para></entry>
                  <entry colname = "Default"><para>0 </para></entry>
                  <entry colname = "Extra"><para> </para></entry>
                  <entry colname = "Description"><para>time when result was generated</para></entry>
               </row>
               <row>
                  <entry colname = "Field"><para>color </para></entry>
                  <entry colname = "Type"><para>smallint unsigned </para></entry>
                  <entry colname = "Key"><para>YES </para></entry>
                  <entry colname = "Default"><para>200 </para></entry>
                  <entry colname = "Extra"><para> </para></entry>
                  <entry colname = "Description"><para>color value </para></entry>
               </row>[+ FOR result +]
               <row>
                  <entry colname = "Field"><para>[+name+] </para></entry>[+
 CASE type +][+ == float +]
                  <entry colname = "Type"><para>[+type+] </para></entry>[+
 == varchar +]
                  <entry colname = "Type"><para>[+type+][+
IF length +]([+length+])[+ ENDIF +] </para></entry>[+
 == text +]
                  <entry colname = "Type"><para>[+type+] </para></entry>[+
 == char +]
                  <entry colname = "Type"><para>[+type+] </para></entry>[+
 == bool +]
                  <entry colname = "Type"><para>enum('yes', 'no') </para></entry>[+
 == tinyint +]
                  <entry colname = "Type"><para>[+type+][+
IF unsigned +] unsigned[+ ENDIF+] </para></entry>[+
 == int +]
                  <entry colname = "Type"><para>[+type+][+
IF unsigned +] unsigned[+ ENDIF+] </para></entry>[+ 
 == bigint +]
                  <entry colname = "Type"><para>[+type+][+
IF unsigned +] unsigned[+ ENDIF+] </para></entry>[+ 
 ESAC type +]
                  <entry colname = "Key"><para>NO </para></entry>
                  <entry colname = "Default"><para>[+default+] </para></entry>[+
 IF auto +]
                  <entry colname = "Extra"><para>auto_increment </para></entry>[+
 ELSE +]
                  <entry colname = "Extra"><para> </para></entry>[+
 ENDIF +]
                  <entry colname = "Description"><para>[+descrip+] </para></entry>
               </row>[+ ENDFOR result+]
           </tbody>
         </tgroup>
      </table>
      <para></para>
   </sect2>
</sect1>
[+ ENDFOR probe +]
[+ ESAC +]
