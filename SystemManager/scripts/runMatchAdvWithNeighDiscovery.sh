#!/bin/bash
advSuperframeBirth=3000;



MY_DIR=`dirname $0`
script=$MY_DIR/neighborDiscoveryAdvLinksMatch.sh
periods=( 1 7 9 11 13 14 17 19 21 22 23 26 29 )
offsets=(3 5 13 15)
counter=0;
while [ $counter -le 12 ]; do
	advPeriod=${periods[$counter]};
	advPeriod=$[$advPeriod*100];
	offset=1;
	offsetCounter=0;
	while [ $offsetCounter -le 3 ]; do	
		repetition=0;
		offset=${offsets[$offsetCounter]}
		channelBirth=$[$offset%100];
		channelOffet=0;
		sfRepetition=150;
		while [ $offset -le $advPeriod ]; do
			echo "ADV_OFFSET=" $offset;
			echo "ADV_PERIOD=" $advPeriod;
			neighDiscPeriod=$[$advPeriod+100];	
			echo "NEIGH_DISC_PERIOD=" $neighDiscPeriod
			$script $offset  $advPeriod $channelOffet $sfRepetition $channelOffset  $advSuperframeBirth $neighDiscPeriod $channelBirth
			neighDiscPeriod=5800;	
			echo "ADV_OFFSET=" $offset;
			echo "ADV_PERIOD=" $advPeriod;
			echo "NEIGH_DISC_PERIOD=" $neighDiscPeriod
			$script $offset  $advPeriod $channelOffet $sfRepetition $channelOffset  $advSuperframeBirth $neighDiscPeriod $channelBirth
			offset=$[$offset+$advPeriod];
		done;
	offsetCounter=$[$offsetCounter+1];
	done;
counter=$[counter+1];	
done;

