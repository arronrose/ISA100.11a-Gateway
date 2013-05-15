#!/bin/bash
arguments=$1
numargs=$#
args=("$@")
manual="This is how you use the script:\n
		\t calculateMatchingChannelAdv.sh taiRouter offsetTaiDevice routerAdvLinkOffset routerAdvlinkPeriod routerChannelOffset chBirth sfBirth"
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

taiRouter=$1
offsetTaiDevice=$2
linkOffset=$3
linkPeriod=$4
channelOffset=$5 
chBirth=$6
sfBirth=$7

case $taiRouter in
  *[^0-9]*) echo taiRouter not integer; exit 1 ;;
esac

case $taiDevice in
  *[^0-9]*) echo taiDevice not integer; exit 1 ;;
esac

case $linkOffset in
  *[^0-9]*) echo linkOffset not integer; exit 1 ;;
esac

case $linkPeriod in
  *[^0-9]*) echo linkPeriod not integer; exit 1 ;;
esac

case $channelOffset in
  *[^0-9]*) echo channelOffset not integer; exit 1 ;;
esac

case $chBirth in
  *[^0-9]*) echo chBirth not integer; exit 1 ;;
esac

case $sfBirth in
  *[^0-9]*) echo sfBirth not integer; exit 1 ;;
esac


#echo $linkOffset
#echo $linkPeriod
#echo $channelOffset


frequencies=(8 1 9 13 5 12 7 14 3 10 0 4 11 6 2 15)
frequenciesDevice=(8 5 3 11);
ovarlappedFreqDev=(0 0 0 0 );
routerTaiSlots=$[$taiRouter*100]
taiDevice=$[$taiRouter+$offsetTaiDevice];
deviceTaiSlots=$[$taiDevice*100];
slotAbs=$[$routerTaiSlots-$sfBirth];
absChannel=$[$slotAbs%16];
startChannel=$[$absChannel-$chBirth]

startDeviceChannel=$[$absDeviceChannel-$chBirth]
#echo "router start channel" $startChannel
#echo "device start channel" $startDeviceChannel
iteration=0

while [ $iteration -le 10000 ]; do
	slotA=$[$slotAbs+$[$iteration*3000]];
	#echo "slotA=" $slotA;
	linkOffset1=$linkOffset;
	while [ $linkOffset1 -le 3000 ]; do
		abCh=$[$[$slotAb]%16];
		#echo "slotAb=" $slotAb;
		channel=$[$[$abCh+$channelOffset]-$chBirth];
		if [ $channel -le -1 ]; then
			channel=$[$channel+16];
		fi
		channel=$[$channel%16]
		#echo "channel=" $channel
		slotAb=$[$slotA+$linkOffset1];
		if [ $slotAb -ge $deviceTaiSlots ] ; then
			channelDev=$[$[$[$slotAb-$deviceTaiSlots]%250]%4];
			
			if [ ${frequenciesDevice[$channelDev]} -eq ${frequencies[$channel]} ]; then
				ovarlappedFreqDev[$channelDev]=$[${ovarlappedFreqDev[$channelDev]}+1];
				if [ ${ovarlappedFreqDev[$channelDev]} -eq 2 ]; then
					seconds=$[$[$slotAb-$deviceTaiSlots]];
					echo "time untill overlaps=" $[$seconds/100] "seconds";
					echo  "on frequence=" ${frequenciesDevice[$channelDev]};
					exit;
				fi
			fi
		fi			
		#echo  ${frequencies[$channel]};
		linkOffset1=$[$linkOffset1+$linkPeriod];
	done; 
	iteration=$[$iteration+1];
done;

