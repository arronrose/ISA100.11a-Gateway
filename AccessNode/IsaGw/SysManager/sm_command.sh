#!/bin/sh

# Created on: Jan 16, 2009
#     Author: catalin.pop

#set -x
errorMsg(){
        echo $1
        exit 1
}

PID_FILE=/tmp/SystemManager.pid
COMMAND_FILE=/access_node/profile/sm_command.ini

[ ! -f $PID_FILE ] && errorMsg "Pid file not found at $PID_FILE. SystemManager not started."

echo "Execute Read Message Statistics"

#Usage: ini_editor [-f <filename>] [-s <section>] -v <variable> {-r | -w <value>} [-a]
address=`ini_editor -f $COMMAND_FILE -s SmCommand -v Parameter -r`


#read -p "Enter address (for $address hit Enter):" address
if [ ! ${#1} = 0 ]; then
address=$1
fi

echo "Response from $address will be displayed (Exit with CTRL-C)"
ini_editor -f $COMMAND_FILE -s SmCommand -v Parameter -w $address

PID=`cat $PID_FILE`
kill -USR2 $PID
#echo "Signal for refresh sent to SystemManager with PID=$PID"
tail -f /tmp/SystemManager.log | grep -A 11 -e "Messages Statistics" -e ERROR