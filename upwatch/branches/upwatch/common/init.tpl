[+ AutoGen5 template init +]
#! /bin/sh
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

# See how we were called.
case "$1" in
  start)
	echo -n "Starting [+prog-name+]: "
	daemon [+prog-name+]
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

