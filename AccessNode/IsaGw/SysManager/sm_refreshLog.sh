#!/bin/sh

# Created on: Jan 16, 2009
#     Author: catalin.pop


errorMsg(){
	echo $1
	exit 1
}

PID_FILE=/tmp/SystemManager.pid

[ ! -f $PID_FILE ] && errorMsg "Pid file not found at $PID_FILE. SystemManager not started."

PID=`cat $PID_FILE`
kill -USR1 $PID 
echo "Signal for refresh sent to SystemManager with PID=$PID"
