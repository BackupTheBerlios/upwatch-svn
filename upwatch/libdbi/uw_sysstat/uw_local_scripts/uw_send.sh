#!/bin/sh

# Sample script to check uw_send availlability

# sudo rules needed:
# Defaults:upwatch !authenticate
# custom installations:
# Cmnd_Alias UPWATCH = /usr/local/sbin/uw_*
# upwatch ALL=(upwatch) UPWATCH
# or for /etc/init.d/ type scripts
# upwatch ALL=/etc/init.d/uw_*

OUTPUT=/tmp/.uw_sysstat.tmp

restart()
{
  if [ -x /etc/init.d/uw_send ]
  then
	/etc/init.d/uw_send restart
  else
        sudo -u upwatch uw_send
  fi
}

if [ -z "`ps -aux | grep uw_send | grep -v grep | grep -v "uw_local_scripts"`" ]
then
  echo red > $OUTPUT
  echo "uw_send was not running, trying to restart" >> $OUTPUT
  restart
elif [ `uwq uw_send | grep ^uw_send | awk '{print $3}'` -gt 9 ] # 10 or more items in the queue
then
  echo red > $OUTPUT
  echo "uw_send was not sending results last 10 minutes, trying to restart" >> $OUTPUT
  restart
else
  echo green > $OUTPUT
fi
