#!/bin/sh

SM_PATH=/usr/local/bin
SM_PID=/tmp/SystemManager.pid
FILE_1=/tmp/f1
FILE_2=/tmp/f2
FILE_3=/tmp/f3
FILE_4=/tmp/f4
FILE_5=/tmp/f5
SCRIPT_LOG=/tmp/SystemManagerStatus.log

log()
{
        #log2flash "$*"

        echo `date` ": $*" >> $SCRIPT_LOG
}

create_files()
{
 [ ! -f $FILE_1 ] && touch $FILE_1 && log "touch $FILE_1" && sleep 1
 [ ! -f $FILE_2 ] && touch $FILE_2 && log "touch $FILE_2" && sleep 1
 [ ! -f $FILE_3 ] && touch $FILE_3 && log "touch $FILE_3" && sleep 1
 [ ! -f $FILE_4 ] && touch $FILE_4 && log "touch $FILE_4" && sleep 1
 [ ! -f $FILE_5 ] && touch $FILE_5 && log "touch $FILE_5" && sleep 1
}

# touch the 5 files so they are timestamped one for every last 5 minutes
touch_files()
{
 [ $FILE_1 -nt $FILE_2 ] && touch $FILE_2 && log "touch $FILE_2" && return 0
 [ $FILE_2 -nt $FILE_3 ] && touch $FILE_3 && log "touch $FILE_3" && return 0
 [ $FILE_3 -nt $FILE_4 ] && touch $FILE_4 && log "touch $FILE_4" && return 0
 [ $FILE_4 -nt $FILE_5 ] && touch $FILE_5 && log "touch $FILE_5" && return 0
 [ $FILE_5 -nt $FILE_1 ] && touch $FILE_1 && log "touch $FILE_1" && return 0
 log "WARN restart_sm.sh: no file touched."
}

startSM(){
cd $SM_PATH
SystemManager & 2>> $SCRIPT_LOG
touch $SM_PID
}

# compare /tmp/SystemManager.pid with the last 5 files timestamp.
#If it's older than all 5, then restart it
compare_files()
{
        # wdt will erase the pid sometimes
        [ ! -f $SM_PID ] && log "NO PID file found" && touch $SM_PID && return 0
        [ $SM_PID -nt $FILE_1 ] && log $FILE_1 old && return 0
        [ $SM_PID -nt $FILE_2 ] && log $FILE_2 old && return 0
        [ $SM_PID -nt $FILE_3 ] && log $FILE_3 old && return 0
        [ $SM_PID -nt $FILE_4 ] && log $FILE_4 old && return 0
        [ $SM_PID -nt $FILE_5 ] && log $FILE_5 old && return 0
        log "restart_sm.sh: SM LOCK detected, restarting ..."
        killall -2 SystemManager 2>> $SCRIPT_LOG
        sleep 3
        killall -9 SystemManager 2>> $SCRIPT_LOG
        sleep 3
        startSM
        
        return 1
}


[ ! -f $SM_PATH/SystemManager ] && echo "ERROR: SM binary not found in $SM_PATH/SystemManager" && exit 1
log restart_sm.sh START
create_files
startSM
sleep 1
touch $SM_PID   # 3 lines to avoid initial SN false lock detect

while true; do
        create_files
        #    > /dev/null
        touch_files
        #     > /dev/null
        compare_files
        #   > /dev/null

        # next is time to detect: 6*time
        # recommended: 50 sec: 6*50 = 5 minutes
        sleep 10        # no less than 10 sec, to avoid false detection
done
