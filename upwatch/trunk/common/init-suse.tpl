[+ AutoGen5 template init-suse +]
#! /bin/sh
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

DEAMON=/usr/sbin/[+prog-name+]
DEAMONCONF=/etc/upwatch.d/[+prog-name+].conf

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

