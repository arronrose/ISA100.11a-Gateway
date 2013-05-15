#!/bin/sh
arguments=$1
numargs=$#
args=("$@")
manual="This is how you use the script:\n
		\t neighborDiscoveryAdvLinksMatch.sh advLinkOffset advLinkPeriod advLinksChannelOffset nrOfNeighDiscSuperframeRepetition advSuperframeLength neighDiscoverySuperframeLength advChBirth"
if  [ $numargs -ne 7 ]; then
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
neighDiscSfRepetition=$4
advSuperframeLength=$5;
neighDiscSuperframeLength=$6;
advChBirth=$7;

case $linkOffset in
  *[^0-9]*) echo linkOffset not integer; exit 1 ;;
esac

case $linkPeriod in
  *[^0-9]*) echo linkPeriod not integer; exit 1 ;;
esac

case $channelOffset in
  *[^0-9]*) echo channelOffset not integer; exit 1 ;;
esac

case $neighDiscSfRepetition in
  *[^0-9]*) echo neighDiscSfRepetition not integer; exit 1 ;;
esac

case $advSuperframeLength in
  *[^0-9]*) echo advSuperframeLength not integer; exit 1 ;;
esac

case $neighDiscSuperframeLength in
  *[^0-9]*) echo neighDiscSuperframeLength not integer; exit 1 ;;
esac


#echo $linkOffset
#echo $linkPeriod
#echo $channelOffset

secondsFromLastRepetition=0;

frequencies=(8 1 9 13 5 12 7 14 3 10 0 4 11 6 2 15)
frequenciesDevice=(11 8 5 3 11 8 5)

match=0;
superframeRepetition=1;
neighDiscLinkOffset=$[$linkOffset%100];
while [ $superframeRepetition -le $neighDiscSfRepetition  ]; do
	absSlot=$[$[$superframeRepetition*$neighDiscSuperframeLength]+$neighDiscLinkOffset];
	abCh=$[$absSlot%16];
	abAdvCh=$[$abCh+$channelOffset];
	channel=$[$abAdvCh-$advChBirth];
	if [ $channel -le -1 ]; then
		channel=$[$channel+16];
	fi
	channel=$[$channel%16]
	channelDev=$[$absSlot%7];
	if [ $[$[$absSlot%$advSuperframeLength]%$linkPeriod] -eq $linkOffset ]; then
		if [ ${frequenciesDevice[$channelDev]} -eq ${frequencies[$channel]} ]; then
			seconds=$[$[$superframeRepetition*$neighDiscSuperframeLength]/100];
			minutes=$[$seconds/60];
			match=1;
			echo "MATCH on channel" ${frequencies[$channel]} "at" $superframeRepetition " repetition at" $seconds "seconds/" $minutes "minutes";
			if [ $superframeRepetition -ne 1 ]; then
				passedSeconds=$[$seconds-$secondsFromLastRepetition];
				passedMinutes=$[$passedSeconds/60];
				echo "............" $passedSeconds "seconds"/ $passedMinutes "minutes have passed from last match"
			fi
			secondsFromLastRepetition=$seconds;
		fi
	fi
	superframeRepetition=$[$superframeRepetition+1]; 
done
if [ $match -eq 0 ]; then
	echo "NEVER MACTH!!!!!!!!!!!!!"
fi
