[+ AutoGen5 template redhat suse solaris +]
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

# Needed for glibc 2.4
export LD_POINTER_GUARD=0

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

# Needed for glibc 2.4
export LD_POINTER_GUARD=0

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
exit 0[+
ESAC +]


