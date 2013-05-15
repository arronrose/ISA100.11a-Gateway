#!/bin/sh

USER=nivis
PASS=nivis
HOST=10.32.0.12
FTPDIR_BASE=/tmp/logs_an/

renice 5 -p $$

FTPDIR=$FTPDIR_BASE`hostname`
LOCAL_DIR=`dirname $1`
old_path=`pwd`
cd $LOCAL_DIR

log_file=`basename $1`

if [ ! -f /access_node/no_tail_kill.flag ]; then
	log_to_kill=`echo $log_file | cut -d'.' -f1`
	pids_to_kill=`ps | grep "tail.*$log_to_kill" | grep -v grep | awk '{print $1}'`
	[ ! -z "$pids_to_kill" ] && kill -9 $pids_to_kill
fi

ftpput -u "$USER" -p "$PASS" $HOST "$FTPDIR/$log_file" "$log_file"
ret=$?

[ -z "$2" ] && rm -f $1 

cd $old_path

exit $ret