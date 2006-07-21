#!/usr/bin/perl -w

=head1 NAME

fill_probe_description.pl - fill descriptions for probes

=head1 SYNOPSIS

fill_probe_description.pl

=head1 DESCRIPTION

This script fills the description fields of all defined probes in the definitions
tables, with the reverse DNS lookupname of the ipaddress. Only description fields
which are empty or which contain the IP address are changed.

=head1 COMMAND LINE OPTIONS

None yet.  

=head1 AUTHOR

Ron Arts <raarts@upwatch.org>

=cut

require 5.001;

use strict;

use DBI;
use Getopt::Std;
#use LockFile::Simple;
use vars qw/ $opt_d /;
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

#  &remove_fill_probe_description_lock();
#  log_error($line, 'notice', $msg);
  print $msg . "\n";
  exit($exitval);
}

#------------------------------------------------------------
# process commandline options 
#------------------------------------------------------------
sub process_options {
  # -d debug
  getopts('d');
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

  parse_config("/etc/upwatch.conf");

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

#  &remove_fill_probe_description_lock();

  exit (0);
}

exit(main());
#------------------------------------------------------------
1;


