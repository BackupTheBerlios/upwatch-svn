#!/usr/bin/perl -w

=head1 NAME

uw_maint.pl - maintenance script.

=head1 SYNOPSIS

uw_maint.pl [-v] [-r days] [-d days] [-w weeks] [-m months] [-y years] [-5 years]

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
use vars qw/ $opt_r $opt_d $opt_w $opt_m $opt_y $opt_5 $opt_v /;
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
  getopts('r:d:w:m:y:5:v');
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
# main
#
#------------------------------------------------------------
sub main {
  my $tmp;		# temp string variable
  my $cursor;		# database cursor
  my $rc;		# return code
  my $row;		# row for hashref

#  &get_fill_probe_description_lock();

  process_options();
  parse_config("/etc/upwatch.conf");

  my $db = DBI->connect("DBI:$dbtype:database=$dbname;host=$dbhost;port=3306", $dbuser, $dbpasswd ) || 
    die_with_exit(__LINE__, 1, DBI::errstr);

  $cursor = $db->prepare("select name, graphgroup from probe where id > 1");
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

    $cursor = $db->prepare("select id from pr_${probename}_def order by id desc limit 1");
    $rc = $cursor->execute || die_with_exit(__LINE__, 0, $db->errstr);
    next if ($cursor->rows == 0);
    while ( $row = $cursor->fetchrow_hashref ) {
      $lastid = $row->{id};
    }
    #print "$probename $lastid\n";

    for (my $probe = 2; $probe <= $lastid; $probe++) {
      my $q;

      $q = "delete from pr_${probename}_raw where probe = $probe and " .
           "stattime < UNIX_TIMESTAMP() - ($opt_r * 86400)";
      if ($opt_v) { print "$q\n"; }
      $cursor = $db->prepare($q);
      $rc = $cursor->execute || die_with_exit(__LINE__, 0, $db->errstr);

      $q = "delete from pr_${probename}_day where probe = $probe and " .
           "stattime < UNIX_TIMESTAMP() - ($opt_d * 86400)";
      if ($opt_v) { print "$q\n"; }
      $cursor = $db->prepare($q);
      $rc = $cursor->execute || die_with_exit(__LINE__, 0, $db->errstr);

      $q = "delete from pr_${probename}_week where probe = $probe and " .
           "stattime < UNIX_TIMESTAMP() - ($opt_w * 86400*7)";
      if ($opt_v) { print "$q\n"; }
      $cursor = $db->prepare($q);
      $rc = $cursor->execute || die_with_exit(__LINE__, 0, $db->errstr);

      $q = "delete from pr_${probename}_month where probe = $probe and " .
           "stattime < UNIX_TIMESTAMP() - ($opt_m * 86400*7*31)";
      if ($opt_v) { print "$q\n"; }
      $cursor = $db->prepare($q);
      $rc = $cursor->execute || die_with_exit(__LINE__, 0, $db->errstr);

      $q = "delete from pr_${probename}_year where probe = $probe and " .
           "stattime < UNIX_TIMESTAMP() - ($opt_y * 86400*365)";
      if ($opt_v) { print "$q\n"; }
      $cursor = $db->prepare($q);
      $rc = $cursor->execute || die_with_exit(__LINE__, 0, $db->errstr);

      $q = "delete from pr_${probename}_5year where probe = $probe and " .
           "stattime < UNIX_TIMESTAMP() - ($opt_5 * 86400*365)";
      if ($opt_v) { print "$q\n"; }
      $cursor = $db->prepare($q);
      $rc = $cursor->execute || die_with_exit(__LINE__, 0, $db->errstr);
    }
  }

  $db->disconnect;

#  &remove_fill_probe_description_lock();

  exit (0);
}

exit(main());
#------------------------------------------------------------
1;


