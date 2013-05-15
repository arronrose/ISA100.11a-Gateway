#!/bin/bash
tai=100000;
deviceTaiOffset=1;
linkOffset=103;
linkPeriod=700;
channelOffset=0;
chBirth=3;
superframeBirth=28;

MY_DIR=`dirname $0`
script=$MY_DIR/calculateMatchingChannelAdv.sh

echo "ROUTER ADV LINK PERIOD=700..device start offset=[0,3600]"
while [ $deviceTaiOffset -le  3600 ]; do
	echo "deviceStartOffset=" $deviceTaiOffset;
	$script $tai  $deviceTaiOffset $linkOffset $linkPeriod $channelOffset  $chBirth $superframeBirth
	deviceTaiOffset=$[$deviceTaiOffset+1];
done

#linkPeriod=3000;
#deviceTaiOffset=1;	

#echo "ROUTER ADV LINK PERIOD=3000..device start offset=[0,3600]";
#while [ $deviceTaiOffset -le  3600 ]; do
#	echo "deviceStartOffset=" $deviceTaiOffset;
#	$script $tai  $deviceTaiOffset $linkOffset $linkPeriod $channelOffset  $chBirth $superframeBirth
#	deviceTaiOffset=$[$deviceTaiOffset+1];
#done
