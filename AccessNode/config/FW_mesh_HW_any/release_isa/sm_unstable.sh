#!/bin/sh


FILE_UNSTABLE=$NIVIS_TMP/'sm.unstable'
# called from scheduler events periodically

if [ ! -f $FILE_UNSTABLE ]; then
	exit 0; 
fi


#FILE_TIME=`cat $FILE_UNSTABLE`


. $FILE_UNSTABLE

FILE_TIME=$time
[ -z "$FILE_TIME" ] && FILE_TIME=0

VR_TIME=`date +%s`

NEED_RESTART=0
if [ $(($VR_TIME - $FILE_TIME)) -le 3600 ]; then
	NEED_RESTART=1
else
	NEED_RESTART=0
	log2flash "sm_unstable.sh: file $FILE_UNSTABLE exist but is old reason=$reason"
fi

rm -f $FILE_UNSTABLE

if [ $NEED_RESTART -eq 1 ]; then 
	log2flash "sm_unstable.sh: file $FILE_UNSTABLE exist (reason=$reason) -> start.sh"
	{ sleep 3; start.sh } &
fi

rm -f $FILE_UNSTABLE