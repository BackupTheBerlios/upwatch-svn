#!/usr/bin/perl -w

=head1 NAME

uw_maint.pl - maintenance script.

=head1 SYNOPSIS

uw_maint.pl [-v] [-o] [-s seconds] [-r days] [-d days] [-w weeks] [-m months] [-y years] [-5 years]

=head1 DESCRIPTION

This script deletes old data from the database. It reads the database info from /etc/upwatch.conf

=head1 COMMAND LINE OPTIONS

-r days
   how many days to keep raw results - default 62

-d days
   how many days to keep day results - default 62

-w weeks
   how many weeks to keep week results - default 104

-m months
   how many months to keep month results - default 24

-y years
   how many years to keep year results - default 2

-5 years
   how many years to keep 5-year results - default 10

-o
  optimize tables

-s seconds
   how many seconds to sleep between queries

-l limit
   how many records to delete in one query

-v
  verbose

=head1 AUTHOR

Ron Arts <raarts@upwatch.org>

=cut

require 5.001;

use strict;

use DBI;
use Getopt::Std;
#use LockFile::Simple;
use vars qw/ $opt_s $opt_l $opt_o $opt_r $opt_d $opt_w $opt_m $opt_y $opt_5 $opt_v /;
use vars qw/ $dbtype $dbhost $dbuser $dbpasswd $dbname /;

$|=1;

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
  $cmd = "logger -t uw_maint.pl[$$] -p user.$facility $line $msgid $message";
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
# Die with error
#------------------------------------------------------------
sub die_with_exit {
  my ($line, $exitval, $msg) = @_;

#  &remove_fill_probe_description_lock();
#  log_error($line, 'notice', $msg);
  print $msg . "\n";
  exit($exitval);
}

#------------------------------------------------------------
# process commandline options 
#------------------------------------------------------------
sub process_options {
  $opt_r = 62;
  $opt_d = 62;
  $opt_w = 104;
  $opt_m = 24;
  $opt_y = 2;
  $opt_5 = 10;
  $opt_s = 1;
  $opt_l = 500;
  getopts('ol:s:r:d:w:m:y:5:v');
}

#------------------------------------------------------------
# Actually delete rows, optionally showing progress
#------------------------------------------------------------
sub delete_rows {
  my ($domain, $db, $q, $limit) = @_;
  my $total_deleted;
  my $deleted;

  if ($opt_v) { print "$domain: $q\n"; }
  $total_deleted = 0;
  do {
    printf("Deleted %u rows. Working..    \b\b\b\b", $total_deleted);
    $deleted = $db->do($q);
    $total_deleted += $deleted;
    if ($opt_v) { printf("\rDeleted %u rows.", $total_deleted); }
    if ($total_deleted > 100) { printf(" Sleeping.."); sleep $opt_s; printf("\r"); }
  } while ($deleted eq $limit);
  print "\n";
}

#------------------------------------------------------------
# get lock file, and exit if locked
#------------------------------------------------------------
#sub get_fill_probe_description_lock {
#  $obj = LockFile::Simple->make();
#  $obj->configure(-hold => '200');
#  $obj->configure(-warn => '1');
#  if (!($obj->trylock("/tmp/fill_probe_description"))) {
#    print "fill_probe_description update is currently running\n";
#    exit(1);
#  }
#}
#
#sub remove_fill_probe_description_lock {
#  $obj->unlock("/tmp/fill_probe_description");
#}


#------------------------------------------------------------
# do one database
#------------------------------------------------------------
sub maint {
  my ($domain, $name, $host, $port, $user, $password) = @_;
  my $cursor;		# database cursor
  my $rc;		# return code
  my $row;		# row for hashref

  my $db = DBI->connect("DBI:mysql:database=$name;host=$host;port=$port", $user, $password ) || 
    die_with_exit(__LINE__, 1, DBI::errstr);

  $cursor = $db->prepare("select name, graphgroup from probe where id > 1 order by name");
  $rc = $cursor->execute || die_with_exit(__LINE__, 0, $db->errstr);
  if ($cursor->rows == 0) {
    die_with_exit(__LINE__, 0, "table probe is empty?? Please fix");
  }
  my @probes = ();
  while ( $row = $cursor->fetchrow_hashref ) {
    #print $row->{name}, $row->{graphgroup}, "\n";
    next if ($row->{graphgroup} eq "");
    push (@probes, $row->{name}); # list of all probenames to be cleaned
  }

  foreach my $probename (@probes) {
    my $lastid;
    my $limit = $opt_l;
    my $q;

    $cursor = $db->prepare("select id from pr_${probename}_def order by id desc limit 1");
    $rc = $cursor->execute || die_with_exit(__LINE__, 0, $db->errstr);
    next if ($cursor->rows == 0);
    while ( $row = $cursor->fetchrow_hashref ) {
      $lastid = $row->{id};
    }
    #print "$probename $lastid\n";

    $q = "delete from pr_${probename}_raw where " . 
         "probe >= 2 and probe <= $lastid and " .
         "stattime < UNIX_TIMESTAMP() - ($opt_r * 86400) LIMIT $limit";
    delete_rows($domain, $db, $q, $limit);

    $q = "delete from pr_${probename}_day where " . 
         "probe >= 2 and probe <= $lastid and " .
         "stattime < UNIX_TIMESTAMP() - ($opt_d * 86400) LIMIT $limit";
    delete_rows($domain, $db, $q, $limit);

    $q = "delete from pr_${probename}_week where " . 
         "probe >= 2 and probe <= $lastid and " .
         "stattime < UNIX_TIMESTAMP() - ($opt_w * 86400*7) LIMIT $limit";
    delete_rows($domain, $db, $q, $limit);

    $q = "delete from pr_${probename}_month where " . 
         "probe >= 2 and probe <= $lastid and " .
         "stattime < UNIX_TIMESTAMP() - ($opt_m * 86400*31) LIMIT $limit";
    delete_rows($domain, $db, $q, $limit);

    $q = "delete from pr_${probename}_year where " . 
         "probe >= 2 and probe <= $lastid and " .
         "stattime < UNIX_TIMESTAMP() - ($opt_y * 86400*365) LIMIT $limit";
    delete_rows($domain, $db, $q, $limit);

    $q = "delete from pr_${probename}_5year where " . 
         "probe >= 2 and probe <= $lastid and " .
         "stattime < UNIX_TIMESTAMP() - ($opt_5 * 86400*365) LIMIT $limit";
    delete_rows($domain, $db, $q, $limit);

    if ($opt_o) {
      $q = "optimize table pr_${probename}_raw";
      if ($opt_v) { print "$domain: $q\n"; }
      $db->do($q);
      sleep 5;

      $q = "optimize table pr_${probename}_day";
      if ($opt_v) { print "$domain: $q\n"; }
      $db->do($q);
      sleep 5;

      $q = "optimize table pr_${probename}_week";
      if ($opt_v) { print "$domain: $q\n"; }
      $db->do($q);
      sleep 5;

      $q = "optimize table pr_${probename}_month";
      if ($opt_v) { print "$domain: $q\n"; }
      $db->do($q);
      sleep 5;

      $q = "optimize table pr_${probename}_year";
      if ($opt_v) { print "$domain: $q\n"; }
      $db->do($q);
      sleep 5;

      $q = "optimize table pr_${probename}_5year";
      if ($opt_v) { print "$domain: $q\n"; }
      $db->do($q);
      sleep 5;
    }
  }

  $db->disconnect;
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

#  &get_fill_probe_description_lock();

  log_error(__LINE__, 'notice', "started " . @ARGV);

  process_options();
  parse_config("/etc/upwatch.conf");

  my $db = DBI->connect("DBI:$dbtype:database=$dbname;host=$dbhost;port=3306", $dbuser, $dbpasswd ) || 
    die_with_exit(__LINE__, 1, DBI::errstr);

  $cursor = $db->prepare("select * from pr_domain where id > 1");
  $rc = $cursor->execute || die_with_exit(__LINE__, 0, $db->errstr);
  if ($cursor->rows == 0) {
    die_with_exit(__LINE__, 0, "table probe is empty?? Please fix");
  }
  while ( $row = $cursor->fetchrow_hashref ) {
    maint($row->{name}, $row->{db}, $row->{host}, $row->{port}, $row->{user}, $row->{password}); 
  }

#  &remove_fill_probe_description_lock();

  log_error(__LINE__, 'notice', "ended");

  exit (0);
}

exit(main());
#------------------------------------------------------------
1;


