#!/bin/bash
arguments=$1
numargs=$#
args=("$@")
manual="This is how you use the script:\n
		\t detectUsedChannelsByLink.sh linkOffset linkPeriod channelOffset"
if  [ $numargs -ne 3 ]; then
	if [ $numargs = 1 ] ; then
		string="-h"
		if [ $1 = $string ] ; then
			echo -e $manual
			exit 1
		else
			echo type -h for help
		fi
	
	else 
		echo type -h for help
		exit 1
	fi

fi

linkOffset=$1
linkPeriod=$2
channelOffset=$3 
case $linkOffset in
  *[^0-9]*) echo linkOffset not integer; exit 1 ;;
esac

case $linkPeriod in
  *[^0-9]*) echo linkPeriod not integer; exit 1 ;;
esac

case $channelOffset in
  *[^0-9]*) echo channelOffset not integer; exit 1 ;;
esac


#echo $linkOffset
#echo $linkPeriod
#echo $channelOffset

echo "USED CHANNELS:"
 
while [ $linkOffset -le 3000 ]
do
	channel=$[$[$linkOffset+$channelOffset]%14]
	echo $channel
	linkOffset=$[$linkOffset+$linkPeriod]

done 

