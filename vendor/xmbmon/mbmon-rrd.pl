#!/usr/bin/perl
#
#	usage:
#		mbmon-rrd.pl <rrdfile>
#
#	description:
#		5分毎にcronから起動すると、mbmonの出力をrrdtoolのRRDファイルに
#		書き出す。
#
#		指定した<rrdfile>が存在しない場合は作成する。作成する際のRRDファ
#		イル中のデータソースは`mbmon -rc1`で出力されたタグ名(fan0等)を
#		辞書順にソートした順序となる。
#

use	RRDs ;


#  -----  default settings  -----

$dbg	= 0 ;
$usage	= "usage: mbmon-rrd.pl <rrdfile>\n" ;



sub	create_rrd()
{
	local( $rrdfile, %status )	= @_ ;
	local( @args, $e ) ;

	print( "create_rrd(): \$rrdfile=\"$rrdfile\"\n" )	if($dbg) ;
	@args		= ( $rrdfile ) ;

	foreach $_ ( sort( keys %status )) {
		push( @args, "DS:$_:GAUGE:600:U:U" ) ;
	}

	push( @args, "RRA:AVERAGE:0.5:1:600" ) ;
	push( @args, "RRA:AVERAGE:0.5:6:700" ) ;
	push( @args, "RRA:AVERAGE:0.5:24:775" ) ;
	push( @args, "RRA:AVERAGE:0.5:288:797" ) ;
	push( @args, "RRA:MAX:0.5:1:600" ) ;
	push( @args, "RRA:MAX:0.5:6:700" ) ;
	push( @args, "RRA:MAX:0.5:24:775" ) ;
	push( @args, "RRA:MAX:0.5:288:797" ) ;
	push( @args, "RRA:MIN:0.5:1:600" ) ;
	push( @args, "RRA:MIN:0.5:6:700" ) ;
	push( @args, "RRA:MIN:0.5:24:775" ) ;
	push( @args, "RRA:MIN:0.5:288:797" ) ;
	push( @args, "RRA:LAST:0.5:1:600" ) ;
	push( @args, "RRA:LAST:0.5:6:700" ) ;
	push( @args, "RRA:LAST:0.5:24:775" ) ;
	push( @args, "RRA:LAST:0.5:288:797" ) ;

	print( "create $rrdfile " . join( ", ", @args ))	if($dbg) ;
	RRDs::create( @args ) ;
	$e	= RRDs::error() ;
	die "ERROR: Cannot create logfile: $e\n"	if( $e ) ;
}


sub	read_status()
{
	local( %status ) ;
	local( $key, $val ) ;

	open( FD, "mbmon -rc1|" )	|| die "ERROR: Cannot run mbmon\n" ;
	while( $_ = <FD> ) {
		next	unless( /^([A-Za-z][^:\s]+)\s*:\s*([+\-]{0,1}[\d\.]+)/ ) ;
		$key	= $1 ;
		$val	= $2 ;
		$key	=~ y/A-Z/a-z/ ;
		$status{$key}	= $val ;
		print( "\$status{$key} = \"$val\"\n" )	if($dbg) ;
	}
	close( FD ) ;
	return( %status ) ;
}


sub	update_rrd()
{
	local( %status )	= @_ ;
	local( @ds, @val ) ;
	local( $template, $value, $e ) ;

	foreach $_ ( sort ( keys %status )) {
		push( @ds, $_ ) ;
		push( @val, $status{$_} ) ;
	}

	$template	= join( ':', @ds ) ;
	$value		= "N:" . join( ':', @val ) ;

	print( "update template = '$template'\n" )	if($dbg) ;
	print( "update value    = '$value'\n" )		if($dbg) ;

	RRDs::update( $rrdfile, "--template", $template, $value ) ;
	$e	= RRDs::error() ;
	die "ERROR Cannot update $rrdfile with '$str' $e\n"	if( $e ) ;
}


#  -----  argument check  -----
if( $#ARGV != 0 ) {
	print $usage ;
	exit( 1 ) ;
}
$rrdfile	= $ARGV[0] ;


#  -----  read status from mbmon  -----
%status	= &read_status ;


#  -----  check and create rrdfile  -----
if( ! -e $rrdfile ) {
	&create_rrd( $rrdfile, %status ) ;
}


#  -----  parse status  -----
&update_rrd( %status ) ;

exit( 0 ) ;
