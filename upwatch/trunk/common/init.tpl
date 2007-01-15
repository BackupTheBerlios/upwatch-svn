[+ AutoGen5 template redhat suse solaris debian +]
[+ CASE (suffix) +][+
   == redhat
+]#! /bin/sh
#
# [+prog-name+]	       [+prog-title+]
#
# chkconfig: 2345 40 60
# description: 	[+(string-substitute (get "detail") '("\n") '("\n#\t\t") )+]
#
# processname: [+prog-name+]
# pidfile: /var/run/[+prog-name+].pid

# Source function library.
. /etc/rc.d/init.d/functions

if [ -f /etc/sysconfig/upwatch ]
then
  . /etc/sysconfig/upwatch
fi

# See how we were called.
case "$1" in
  start)
	if [ -n "$[+prog-name+]_nofile" ]
	then
		ulimit -n $[+prog-name+]_nofile
	else
		if [ -n "$nofile" ]
		then
			ulimit -n $nofile
		fi
	fi
	echo -n "Starting [+prog-name+]: "
	daemon --user upwatch [+(getenv "sbindir")+]/[+prog-name+]
	echo
	touch /var/lock/subsys/[+prog-name+]
	;;
  stop)
	echo -n "Stopping [+prog-name+]: "
	killproc [+prog-name+]
	echo
	rm -f /var/lock/subsys/[+prog-name+]
	;;
  restart)
	$0 stop
	$0 start
	;;
  status)
	status [+prog-name+]
	;;
  *)
	echo "Usage: [+prog-name+] {start|stop|restart|status}"
	exit 1
esac

exit 0

[+ == suse
+]#! /bin/sh
#
# [+prog-name+]        [+prog-title+]
#
# Author: Ron Arts <raarts@upwatch.com>
#
# /etc/init.d/[+prog-name+]
#
### BEGIN INIT INFO
# Provides:            [+prog-name+]
# Required-Start:      $network
# Required-Stop:
# Default-Start:       2 3 5
# Default-Stop:        0 1 6
# Description:         start [+prog-name+]
### END INIT INFO

DEAMON=[+(getenv "sbindir")+]/[+prog-name+]

test -x $DEAMON || exit 5

# Shell functions sourced from /etc/rc.status:
#      rc_check         check and set local and overall rc status
#      rc_status        check and set local and overall rc status
#      rc_status -v     ditto but be verbose in local rc status
#      rc_status -v -r  ditto and clear the local rc status
#      rc_failed        set local and overall rc status to failed
#      rc_failed <num>  set local and overall rc status to <num><num>
#      rc_reset         clear local rc status (overall remains)
#      rc_exit          exit appropriate to overall rc status
. /etc/rc.status

# First reset status of this service
rc_reset

# Return values acc. to LSB for all commands but status:
# 0 - success
# 1 - generic or unspecified error
# 2 - invalid or excess argument(s)
# 3 - unimplemented feature (e.g. "reload")
# 4 - insufficient privilege
# 5 - program is not installed
# 6 - program is not configured
# 7 - program is not running
# 
# Note that starting an already running service, stopping
# or restarting a not-running service as well as the restart
# with force-reload (in case signalling is not supported) are
# considered a success.

case "$1" in
    start)
	echo -n "Starting [+prog-name+]"
        startproc $DEAMON 
	rc_status -v
	rc_reset
	;;
    stop)
	echo -n "Shutting down [+prog-name+]:"
	killproc -TERM $DEAMON
	rc_status -v ; rc_reset
	;;
    try-restart)
	$0 status >/dev/null &&  $0 restart
	rc_status
	;;
    restart)
	$0 stop
	$0 start
	rc_status
	;;
    force-reload)
	echo -n "Reload service [+prog-name+]:"
	killproc -HUP $DEAMON
	rc_status -v
	;;
    reload)
	echo -n "Reload service [+prog-name+]:"
	killproc -HUP $DEAMON
	rc_status -v
	;;
    status)
	echo -n "Checking for service [+prog-name+]:"
	checkproc $DEAMON
	rc_status -v
	rc_reset
	;;
    *)
	echo "Usage: $0 {start|stop|try-restart|restart|force-reload|reload|status}"
	exit 1
	;;
esac
rc_exit

[+ == solaris
+]#!/sbin/sh
#
# [+prog-name+]	       [+prog-title+]
#
# chkconfig: 2345 40 60
# description: 	[+(string-substitute (get "detail") '("\n") '("\n#\t\t") )+]
#
# processname: [+prog-name+]
# 
# All rights reserved.
#

case "$1" in
'start')
        [ ! -x [+(getenv "sbindir")+]/[+prog-name+] ] && exit 0

        # check for a [+prog-name+] process and exit
        # if the daemon is already running.

        if /usr/bin/pgrep -x -u 0 -P 1 [+prog-name+] >/dev/null 2>&1; then
               echo "$0: [+prog-name+] is already running"
               exit 0
        fi

        [+(getenv "sbindir")+]/[+prog-name+] &
        ;;

'stop')
        /usr/bin/pkill -x -u 0 -P 1 [+prog-name+]
        ;;

*)
        echo "Usage: $0 { start | stop }"
        exit 1
        ;;
esac
exit 0
[+ == debian

+]#!/bin/sh
### BEGIN INIT INFO
# Provides:          [+prog-name+]
# Required-Start:    $local_fs $remote_fs
# Required-Stop:     $local_fs $remote_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Upwatch client daemon
# Description: 	[+(string-substitute (get "detail") '("\n") '("\n#\t\t") )+]
### END INIT INFO

# Author: Wijnand Wiersma <wijnand@nedbsd.eu>

# Do NOT "set -e"

# PATH should only include /usr/* if it runs after the mountnfs.sh script
PATH=/usr/sbin:/usr/bin:/sbin:/bin
DESC="[+(string-substitute (get "detail") '("\n") '("\n#\t\t") )+]"
NAME=[+prog-name+]
DAEMON=/usr/sbin/$NAME
DAEMON_ARGS=""
PIDFILE=/var/run/$NAME.pid
SCRIPTNAME=/etc/init.d/$NAME

# Exit if the package is not installed
[ -x "$DAEMON" ] || exit 0

# Read configuration variable file if it is present
[ -r /etc/default/$NAME ] && . /etc/default/$NAME

# Load the VERBOSE setting and other rcS variables
[ -f /etc/default/rcS ] && . /etc/default/rcS

# Define LSB log_* functions.
# Depend on lsb-base (>= 3.0-6) to ensure that this file is present.
. /lib/lsb/init-functions

#
# Function that starts the daemon/service
#
do_start()
{
	# Return
	#   0 if daemon has been started
	#   1 if daemon was already running
	#   2 if daemon could not be started
	start-stop-daemon --start --quiet --pidfile $PIDFILE -c upwatch --exec $DAEMON --test > /dev/null \
		|| return 1
	start-stop-daemon --start --quiet --pidfile $PIDFILE -c upwatch --exec $DAEMON -- \
		$DAEMON_ARGS \
		|| return 2
}

#
# Function that stops the daemon/service
#
do_stop()
{
	# Return
	#   0 if daemon has been stopped
	#   1 if daemon was already stopped
	#   2 if daemon could not be stopped
	#   other if a failure occurred
	start-stop-daemon --stop --quiet --retry=TERM/30/KILL/5 --pidfile $PIDFILE --name $NAME
	RETVAL="$?"
	[ "$RETVAL" = 2 ] && return 2
	# Wait for children to finish too if this is a daemon that forks
	# and if the daemon is only ever run from this initscript.
	# If the above conditions are not satisfied then add some other code
	# that waits for the process to drop all resources that could be
	# needed by services started subsequently.  A last resort is to
	# sleep for some time.
	start-stop-daemon --stop --quiet --oknodo --retry=0/30/KILL/5 --exec $DAEMON
	[ "$?" = 2 ] && return 2
	# Many daemons don't delete their pidfiles when they exit.
	rm -f $PIDFILE
	return "$RETVAL"
}

#
# Function that sends a SIGHUP to the daemon/service
#
do_reload() {
	#
	# If the daemon can reload its configuration without
	# restarting (for example, when it is sent a SIGHUP),
	# then implement that here.
	#
	start-stop-daemon --stop --signal 1 --quiet --pidfile $PIDFILE --name $NAME
	return 0
}

case "$1" in
  start)
	[ "$VERBOSE" != no ] && log_daemon_msg "Starting $DESC" "$NAME"
	do_start
	case "$?" in
		0|1) [ "$VERBOSE" != no ] && log_end_msg 0 ;;
		2) [ "$VERBOSE" != no ] && log_end_msg 1 ;;
	esac
	;;
  stop)
	[ "$VERBOSE" != no ] && log_daemon_msg "Stopping $DESC" "$NAME"
	do_stop
	case "$?" in
		0|1) [ "$VERBOSE" != no ] && log_end_msg 0 ;;
		2) [ "$VERBOSE" != no ] && log_end_msg 1 ;;
	esac
	;;
  #reload|force-reload)
	#
	# If do_reload() is not implemented then leave this commented out
	# and leave 'force-reload' as an alias for 'restart'.
	#
	#log_daemon_msg "Reloading $DESC" "$NAME"
	#do_reload
	#log_end_msg $?
	#;;
  restart|force-reload)
	#
	# If the "reload" option is implemented then remove the
	# 'force-reload' alias
	#
	log_daemon_msg "Restarting $DESC" "$NAME"
	do_stop
	case "$?" in
	  0|1)
		do_start
		case "$?" in
			0) log_end_msg 0 ;;
			1) log_end_msg 1 ;; # Old process is still running
			*) log_end_msg 1 ;; # Failed to start
		esac
		;;
	  *)
	  	# Failed to stop
		log_end_msg 1
		;;
	esac
	;;
  *)
	#echo "Usage: $SCRIPTNAME {start|stop|restart|reload|force-reload}" >&2
	echo "Usage: $SCRIPTNAME {start|stop|restart|force-reload}" >&2
	exit 3
	;;
esac

:
[+ESAC +]
