#!/usr/bin/perl -w

=head1 NAME

import_regex_files.pl - import the regular expression files into the database

=head1 SYNOPSIS

import_regex_files.pl

=head1 DESCRIPTION

Import a directory of regular expression files into the database. 
It reads the login info to the database from /etc/upwatch.conf
Another required parameter is the base directory. In the base directory
the following directory structure is expected:

 uw_sysstat.d/
 uw_sysstat.d/syslog/macros.txt
                     rmacros.txt
                     .....
 uw_sysstat.d/maillog/....
 os/

=head1 COMMAND LINE OPTIONS

None yet.  

=head1 AUTHOR

Ron Arts <raarts@upwatch.com>

=cut

require 5.001;

use strict;

use DBI;
use POSIX qw(strerror);
use Getopt::Std;
use File::Basename;
#use LockFile::Simple;
use vars qw/ $opt_d $opt_b /;
use vars qw/ $dbtype $dbhost $dbuser $dbpasswd $dbname /;

#------------------------------------------------------------
# log error message to the system log
#------------------------------------------------------------
sub log_error {
  my ($line, $facility, $message) = @_;
  my ($msgid, $cmd);

  $msgid = "";

  if (!defined($message)) {
    $message = "(null)";
  }
  $message =~ s/[<>'`(){}\$]//g;
  $cmd = "logger -t fill[$$] -p mail.$facility $line $msgid $message";
  system($cmd);
}

#------------------------------------------------------------
# parse the configuration file
#------------------------------------------------------------
sub parse_config {
  my ($infile)  = @_;
  my ($line);

  open(INFILE, $infile) or die "Can't open $infile: $!";
  while (<INFILE>) {     # assigns each line in turn to $_ 
    next if (/^#.*/);
    next if (/^$/);
    my $line = $_;
    my @fields = split '\s', $line; 
    if ($fields[0] =~ /^dbtype/) { $dbtype = $fields[1]; }
    if ($fields[0] =~ /^dbhost/) { $dbhost = $fields[1]; }
    if ($fields[0] =~ /^dbname/) { $dbname = $fields[1]; }
    if ($fields[0] =~ /^dbuser/) { $dbuser = $fields[1]; }
    if ($fields[0] =~ /^dbpasswd/) { $dbpasswd = $fields[1]; }
  }  
  close(INFILE);
}

#------------------------------------------------------------
# Get remote host 
#------------------------------------------------------------
sub findhostname {
  my ($ip_address) = @_; 
  my @numbers = split(/\./, $ip_address); 
  my $ip_number = pack("C4", @numbers); 
  return (gethostbyaddr($ip_number, 2));
}

#------------------------------------------------------------
# Die with error
#------------------------------------------------------------
sub die_with_exit {
  my ($line, $exitval, $msg) = @_;

#  &remove_import_regex_files_lock();
#  log_error($line, 'notice', $msg);
  print $msg . "\n";
  exit($exitval);
}

#------------------------------------------------------------
# process commandline options 
#------------------------------------------------------------
sub process_options {
  # -d debug
  # -b <base directory>
  getopts('db:');
}

#------------------------------------------------------------
# get lock file, and exit if locked
#------------------------------------------------------------
#sub get_import_regex_files_lock {
#  $obj = LockFile::Simple->make();
#  $obj->configure(-hold => '200');
#  $obj->configure(-warn => '1');
#  if (!($obj->trylock("/tmp/import_regex_files"))) {
#    print "import_regex_files update is currently running\n";
#    exit(1);
#  }
#}
#
#sub remove_import_regex_files_lock {
#  $obj->unlock("/tmp/import_regex_files");
#}

sub import_macros_txt {
  my ($logname, $infile) = @_;

  my $db = DBI->connect("DBI:$dbtype:database=$dbname;host=$dbhost;port=3306", $dbuser, $dbpasswd ) || 
    die_with_exit(__LINE__, 1, DBI::errstr);

  open(INFILE, $infile) || die_with_exit(__LINE__, 0, "$infile: " . strerror($!));
  while (<INFILE>) {     # assigns each line in turn to $_
    next if (/^#.*/);
    next if (/^$/);
    my $line = ($_);
    chomp($line);
    my ($name, $regex) = ($line =~ /^(\S+)\s+(.*)/);
    print "name=$name, regex=$regex . \n";
    
    my $query = sprintf("insert ignore into pr_macros set logtype='%s', name='%s', regex=%s", 
                $logname, $name, $db->quote($regex));
    print $query . "\n";
    my $cur = $db->prepare($query);
    my $rc = $cur->execute || die_with_exit(__LINE__, 0, $db->errstr);
  }
  close(INFILE);
  $db->disconnect;
}

sub import_rmacros_txt {
  my ($logname, $infile) = @_;

  my $db = DBI->connect("DBI:$dbtype:database=$dbname;host=$dbhost;port=3306", $dbuser, $dbpasswd ) || 
    die_with_exit(__LINE__, 1, DBI::errstr);

  open(INFILE, $infile) || die_with_exit(__LINE__, 0, "$infile: " . strerror($!));
  while (<INFILE>) {     # assigns each line in turn to $_
    next if (/^#.*/);
    next if (/^$/);
    my $line = ($_);
    chomp($line);
    my ($name, $regex) = ($line =~ /^(\S+)\s+(.*)/);
    print "name=$name, regex=$regex . \n";
    
    my $query = sprintf("insert ignore into pr_rmacros set logtype='%s', name='%s', regex=%s", 
                $logname, $name, $db->quote($regex));
    print $query . "\n";
    my $cur = $db->prepare($query);
    my $rc = $cur->execute || die_with_exit(__LINE__, 0, $db->errstr);
  }
  close(INFILE);
  $db->disconnect;
}

sub import_regex_file {
  my ($logname, $infile) = @_;
  my $package = basename($infile);

  my $db = DBI->connect("DBI:$dbtype:database=$dbname;host=$dbhost;port=3306", $dbuser, $dbpasswd ) || 
    die_with_exit(__LINE__, 1, DBI::errstr);

  open(INFILE, $infile) || die_with_exit(__LINE__, 0, "$infile: " . strerror($!));
  while (<INFILE>) {     # assigns each line in turn to $_
    next if (/^#.*/);
    next if (/^$/);
    my $line = ($_);
    chomp($line);
    my ($action, $regex) = ($line =~ /^(\S+)\s+(.*)/);
    print "action=$action, regex=$regex . \n";
    
    my $query = sprintf("insert ignore into pr_regex set logtype='%s', package='%s', action='%s', regex=%s", 
                $logname, $package, $action, $db->quote($regex));
    #print $query . "\n";
    my $cur = $db->prepare($query);
    my $rc = $cur->execute || die_with_exit(__LINE__, 0, $db->errstr);
  }
  close(INFILE);
  $db->disconnect;

}

sub import_regexes {
  my ($dir) = @_;
  my $logname;

  $logname = basename($dir);
  print "importing for $logname\n";

  opendir(DIR, "$dir") || 
    die_with_exit(__LINE__, 0, "$dir: " . strerror($!));
  my @files = grep { !/^\./ && -f "$dir/$_" } readdir(DIR);
  closedir DIR;

  foreach my $file (@files) {
    print($file . "\n");
    if ($file eq "macros.txt") {
      import_macros_txt($logname, "$dir/$file");
    } elsif ($file eq "rmacros.txt") {
      import_rmacros_txt($logname, "$dir/$file");
    } else {
      import_regex_file($logname, "$dir/$file");
    }
  }
}

#------------------------------------------------------------
# main
#
#------------------------------------------------------------
sub main {
  my $tmp;		# temp string variable
  my $cursor;		# database cursor
  my $rc;		# return code
  my $row;		# row for hashref

#  &get_import_regex_files_lock();

  process_options();
  parse_config("/etc/upwatch.conf");

  if (!defined($opt_b) || $opt_b eq "") {
    die_with_exit(__LINE__, 0, "missing -b option");
  }

  opendir(DIR, "$opt_b/uw_sysstat.d") || 
    die_with_exit(__LINE__, 0, "$opt_b/uw_sysstat.d: " . strerror($!));
  my @dots = grep { !/^\./ && -d "$opt_b/uw_sysstat.d/$_" } readdir(DIR);
  closedir DIR;

  foreach my $dir (@dots) {
    import_regexes("$opt_b/uw_sysstat.d/$dir");
  }

  exit (0);

  my $db = DBI->connect("DBI:$dbtype:database=$dbname;host=$dbhost;port=3306", $dbuser, $dbpasswd ) || 
    die_with_exit(__LINE__, 1, DBI::errstr);

  $cursor = $db->prepare("select name from probe where id > 1");
  $rc = $cursor->execute || die_with_exit(__LINE__, 0, $db->errstr);
  if ($cursor->rows == 0) {
    die_with_exit(__LINE__, 0, "table probe is empty?? Please fix");
  }
  my @probes = ();
  while ( $row = $cursor->fetchrow_hashref ) {
    push (@probes, $row->{name});
  }

  foreach my $probename (@probes) {
    $cursor = $db->prepare("select * from pr_${probename}_def where id > 1");
    $rc = $cursor->execute || die_with_exit(__LINE__, 0, $db->errstr);
    next if ($cursor->rows == 0);
    while ( $row = $cursor->fetchrow_hashref ) {
      my $id = $row->{id};
      my $description = $row->{description};
      my $ipaddress = $row->{ipaddress};

      if ($description eq $ipaddress || $description eq "") {
        my $name = findhostname($ipaddress);
        if (defined($name) && $name ne "") {
          my $query = "update pr_${probename}_def set description = '$name' where id = '$id'";
          print $query . "\n";
          my $cur = $db->prepare($query);
          $rc = $cur->execute || die_with_exit(__LINE__, 0, $db->errstr);
        }
      }
    }
  }

  $db->disconnect;

#  &remove_import_regex_files_lock();

  exit (0);
}

exit(main());
#------------------------------------------------------------
1;


