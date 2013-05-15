#!/bin/sh

PROD_FILE="/access_node/activity_files/production.txt"
PROD_STEP="/access_node/activity_files/production.step"
RC_NET_INFO="/access_node/etc/rc.d/rc.net.info"
MSP_FW_FILE="/access_node/dn_fw_default.txt"

KERNEL_DIR=/lib/modules/2.6.25.20/kernel
#`ini_editor -s GLOBAL -v AN_ID -r | cut -f2 -d' '`

#vt100 allows us to edit with joe
export TERM=vt100

. ${NIVIS_FIRMWARE}common.sh
. ${NIVIS_FIRMWARE}comm/comm_help.sh

log()
{
	echo "$*"
	echo -n "`date \"+%Y/%m/%d %T\"` " >> $PROD_FILE
	echo "$*" >> $PROD_FILE
}

is_answer_yes()
{
	local tmp

	read -p "$1" tmp
	if [ "$tmp" = "y" -o "$tmp" = "Y" ]; then
		return 0	# true
	else
		return 1	# false
	fi
}

# $1 step_name
sync_step_start()
{
	local tmp="a"

	while [ true ]; do
		echo ""
		echo "Press 's' to start step $1"
		echo "      'e' to exit to console"
		read tmp
		[ "$tmp" = "e" ] && exit 0
		[ "$tmp" = "s" ] && return 0
	done
}

base_settings()
{
	local IP_VAL
	local MASK_VAL
	local GW_VAL
	local AN_ID_CRT
	local AN_ID
	local passed
	local tmp
	local PROD_FILE_TMP
	local NS_VAL
	local ns_done
	local ETH_NAME

	sync_step_start base_settings
	passed=0
	while [ $passed -ne 1 ]; do

		echo Board settings...
		echo "Board flavor: VR900"
		touch /etc/flavor_vr900

		AN_ID_CRT=`ini_editor -s GLOBAL -v AN_ID -r | cut -f2 -d' '`

		read -p "Enter AN_ID (Ex: 0001BC. Current $AN_ID_CRT): "	AN_ID
		AN_ID=${AN_ID:-$AN_ID_CRT}

		[ `echo ${AN_ID} | grep "^[0-9ABCDEFabcdef]\{6\}$" | wc -l` -ne 1 ] && echo "Error: AN_ID ${AN_ID} is not valid" && continue

		ini_editor -s GLOBAL -v AN_ID -w "00 ${AN_ID}"

		ETH_NAME_DEF="eth0"	#TODO maybe read from /etc/rc.d/rc.net.info ?
		read -p "Enter ETH interface name (Use eth1 for RevB, eth0 for all others. Current $ETH_NAME_DEF): " ETH_NAME
		ETH_NAME=${ETH_NAME:-$ETH_NAME_DEF}
		[ "$ETH_NAME" != "eth0" -a "$ETH_NAME" != "eth1" ] &&  echo "Error: Eth name ${ETH_NAME} is not valid" && continue

		read -p "Enter IP: "    IP_VAL
		[ `echo $IP_VAL | grep "^[0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+$" | wc -l` -ne 1 ] && echo "Error: IP ${IP_VAL} is not valid" && continue

		read -p "Enter Mask: " 	MASK_VAL
		[ `echo $MASK_VAL | grep "^[0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+$" | wc -l` -ne 1 ] && echo Error: "MASK ${MASK_VAL} is not valid" && continue

		read -p "Enter gw: "    GW_VAL
		[ `echo $GW_VAL | grep "^[0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+$" | wc -l` -ne 1 ] && echo "Error: GW ${GW_VAL} is not valid." && continue

		read -p "Enter ETH0 MAC (leave empty to use default depending on IP - RECOMMENDED): " ETH0_MAC_VAL
		[ ! echo "${ETH0_MAC_VAL}" | grep -q "^[0-9ABCDEFabcdef]\{12\}$" ] && echo "Error: MAC ${ETH0_MAC_VAL} is not valid. Use 12 digit hex number" & continue
		
		read -p "Enter ETH1 MAC (leave empty if ETH1 is not used): " ETH1_MAC_VAL
		[ ! echo "${ETH1_MAC_VAL}" | grep -q "^[0-9ABCDEFabcdef]\{12\}$" ] && echo "Error: MAC ${ETH1_MAC_VAL} is not valid. Use 12 digit hex number" & continue
		
		log "Configuring An_${AN_ID}: IP $IP_VAL, GW $GW_VAL, mask $MASK_VAL MAC0 $ETH0_MAC_VAL MAC1 $ETH1_MAC_VAL"

		if ! is_answer_yes "Are these values correct (y/n)?" ; then
			continue
		fi

		log "Values were confirmed by the user"

		echo Updating $RC_NET_INFO
		echo '#AUTOMATICALLY GENERATED. '	>   $RC_NET_INFO
		echo "ETH0_IP=$IP_VAL"				>>  $RC_NET_INFO
		echo "ETH0_MASK=$MASK_VAL"			>>  $RC_NET_INFO
		echo "ETH0_GW=$GW_VAL"				>>  $RC_NET_INFO
		echo "ETH_NAME=$ETH_NAME"			>>  $RC_NET_INFO
		[ ! -z "$ETH0_MAC_VAL" ] && echo "ETH0_MAC=$ETH0_MAC_VAL"	>>  $RC_NET_INFO
		[ ! -z "$ETH1_MAC_VAL" ] && echo "ETH1_MAC=$ETH1_MAC_VAL"	>>  $RC_NET_INFO
		chmod 0777 $RC_NET_INFO

		echo creating /access_node/etc/hosts
		echo "127.0.0.1         localhost"      >  /access_node/etc/hosts
		echo "$IP_VAL           An_${AN_ID}"    >> /access_node/etc/hosts

		#add name servers to be used on ETH
		dns_add

		#just in case put down eth1
		ifconfig eth1 down
		/access_node/etc/rc.d/rc.net
		ifconfig eth1
		if [ $? -ne 0 ]; then
			echo
			log "base_settings: FAILED"
		else
			log "base_settings: PASSED "
			passed=1
		fi
	done
}

dns_add()
{
	echo "Must enter at least one nameserver"
	ns_added=0
	cat /dev/null > /tmp/resolv.conf.eth
	while [ $ns_added -ne 99 ]; do
		[ $ns_added -eq 0 ] && read -p "Enter nameserver: "  NS_VAL
		[ $ns_added -gt 0 ] && read -p "Enter nameserver (leave empty to stop adding): "  NS_VAL

		[ -z "$NS_VAL" ] && [ $ns_added -gt 0 ] && log "DNS added: $ns_added" && ns_added=99 && continue
		[ `echo $NS_VAL | grep "^[0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+$" | wc -l` -ne 1 ] && echo "DNS ${NS_VAL} is not valid" && continue

		echo "nameserver ${NS_VAL}" >> /tmp/resolv.conf.eth
		ns_added=$(($((ns_added))+1))
	done
	mv /tmp/resolv.conf.eth /etc/resolv.conf.eth
}

eth_check()
{
	echo ""
	echo "Step: eth_check"

	sync_step_start eth_check

	passed=0
	while [ $passed -ne 1 ]; do
		echo "DHCP test"
		killall udhcpc 2>/dev/null
		. /etc/rc.d/rc.net.info
		[ -z "$ETH_NAME" ] && ETH_NAME="eth1"
		
		if is_answer_yes "Enable DHCP (y/n)?" ; then
			rm -f /etc/config/no_dhcp
			log "DHCP enabled"
			udhcpc -b -s /etc/udhcpc/udhcpc.sh -i ${ETH_NAME}:0
			sleep 1
			echo "	DHCP settings:"
			ifconfig ${ETH_NAME}:0 | grep "inet addr"
			if [ $? -eq 0 ]; then
				log  "DHCP test: PASSED"
			else
				log  "DHCP test: FAILED"
				echo "WARNING: can't get a DHCP-assiged IP. No DHCP server or bad ETH cable."
				echo "However, DHCP client is enabled and will try to get an IP at boot up"
			fi
		else
			touch /etc/config/no_dhcp
			log "DHCP disabled"
		fi

		if is_answer_yes "Disable route to 10.0.0.0 (n for Nivis, y for customers) (y/n)?" ; then
			sed -i "s/\(.*10\.0\.0\.0.*\)/#\1/" /etc/rc.d/rc.net		# comment out
		else
			sed -i "s/\(#*\)\(.*10\.0\.0\.0.*\)/\2/" /etc/rc.d/rc.net	# remove comment, enable command
		fi

		if is_answer_yes "Disable backup IP 172.17.17.17 (reccommended y) (y/n)?" ; then
			sed -i "s/\(.*172\.17\.17\.17.*\)/#\1/" /etc/rc.d/rc.net		# comment out the line
		else
			sed -i "s/\(#*\)\(.*172\.17\.17\.17.*\)/\2/" /etc/rc.d/rc.net	# remove comment, enable command
		fi
		
		echo "	ETH settings:"
		ifconfig ${ETH_NAME} | grep "inet addr"

		echo "From another computer, telnet to the IP of this board"
		if is_answer_yes "Were you able to telnet (y/n)?" ; then
			passed=1
			log "Ethernet connection test: PASSED"
		else
			log "Ethernet connection test: FAILED"
			echo "MALFUNCTION: check that ETH cable is NOT crossover"
			echo "  Repeating test"
		fi
	done

	echo "Optionally, the test can be continued from telnet session"
	echo "   by typing (on the telnet  window!): "
	echo "board_setup.sh 2"
	echo "   and ignoring this screen"
	echo ""
	sleep 1
}

#try to read time from several time servers and set it on the board
set_time_auto()
{	#alternate: `ntpd -q -g`
	#The problem with alternate: it takes 3 minute to discover the NTP servers are not reacheable
	local out
	NTP_SERVER_LIST="pool.ntp.org 199.240.130.1"
	for S in $NTP_SERVER_LIST; do
		log " trying to get time from $S"
		#ntpclient will return non-zero on name resolution failure
		# but it will return ZERO (suzzess) ion timeout
		# Therefore, we must check for both retcode and out string
		out=`ntpclient -s -h $S -i 20 2> /dev/null`
		[ $? -eq 0 -a -n "$out" ] && log " set_time_auto: ok" && return 0;
	done
	log " set_time_auto: failed"
	return 1
}

i2c_check()
{
	sync_step_start i2c_check
	echo ""
	echo "Step: RTC"
	passed=0
	while [ $passed -ne 1 ]; do
		echo ""
		echo "RTC check"
		echo "Make sure that RTC battery is in place and Ethernet cable is connected"
		read -p "Press Enter to continue" tmp
		
		#Try to get time automatically from several internet time servers
		#This assume we are now connected to the internet 
		passed=1
		set_time_auto
		if [ $? -ne 0 ]; then
			time_ok=0
			while [ $time_ok -ne 1 ]; do
				echo -n "Enter current date/hour (YY/MM/DD hh:mm:ss):"
				read tmp
				
				local YY	# year
				local MM	# month
				local DD	# day
				local hh	# hour
				local mm	# minute
				local ss 	# second

				tmp=`echo "$tmp" | tr -s '/:' ' '`
				YY=`echo $tmp | cut -f1 -d ' '`;
				MM=`echo $tmp | cut -f2 -d ' '`; 
				DD=`echo $tmp | cut -f3 -d ' '`; 
				hh=`echo $tmp | cut -f4 -d ' '`; 
				mm=`echo $tmp | cut -f5 -d ' '`; 
				ss=`echo $tmp | cut -f6 -d ' '`;
				
				if [ -z "$YY" -o -z "$MM" -o -z "$DD" -o -z "$hh" -o -z "$mm" -o -z "$ss" ]; then
					if ! is_answer_yes "Incorrect date. Do you want to try again (y/n)?"; then
						time_ok=1
						passed=0
					fi
				elif [ 	`echo "$tmp" | grep "^[0-9 ]\+$" | wc -l` -ne 1 ]; then 
					#must have space in grep [0-9 ] to match the separator too
					if ! is_answer_yes "Incorrect date. Do you want to try again (y/n)?"; then
						time_ok=1
						passed=0
					fi
				else
					if [ 	$YY -lt  8 -o \
							$MM -gt 12 -o $MM -lt 1 -o \
							$DD -gt 31 -o $DD -lt 1 -o \
							$hh -gt 24 -o \
							$mm -gt 59 -o \
							$ss -gt 59 ]; then
						if ! is_answer_yes "Incorrect date. Do you want to try again (y/n)?"; then
							time_ok=1
							passed=0
						fi
					else
						log "Date input by user: $tmp"
						date ${MM}${DD}${hh}${mm}20${YY}.${ss}
						if [ $? -eq 0 ]; then 
							time_ok=1
						else
							if ! is_answer_yes "Incorrect date. Do you want to try again (y/n)?"; then
								time_ok=1
								passed=0
							fi
						fi
					fi
				fi
			done
		fi
		
		if [ $passed -eq 1 ]; then
			i2c-rtc -d 0xD0 --sync -w 0 || passed=0
		fi
		
		if [ $passed -eq 1 ]; then
			echo "Date apparently set ok"
			echo "Reading back"
			/access_node/bin/i2c-rtc -d 0xD0 --date -r || passed=0
			if [ $passed -eq 1 ]; then
				is_answer_yes "Did you read valid date (UTC date + few sec (y/n)?" \
					&& /access_node/bin/i2c-rtc -d 0xD0 --sync -r || passed=0
			fi
		fi
		[ $passed -eq 1 ] \
			&& log "VR RTC Set/Test: PASSED"\
			|| log "VR RTC Set/Test: FAILED"
		is_answer_yes "Repeat i2c_check tests (y/n)?"
		passed=$?
	done
}


set_final_dest()
{
	local APP_ID_CRT
	local PROXY_CRT

	sync_step_start set_final_dest
	echo ""
	echo "Step: set_final_dest"

	passed=0
	while [ $passed -ne 1 ]; do
		APP_ID_CRT=`ini_editor -s GLOBAL -v APP_ID -r`
		PROXY_CRT=`ini_editor -s "CC_COMM - TCP_DEVICE" -v PROXY -r`
		read -p "Enter APP_ID (current $APP_ID_CRT) " APP_ID
		read -p "Enter PROXY (current $PROXY_CRT) " PROXY

		APP_ID=${APP_ID:-$APP_ID_CRT}
		PROXY=${PROXY:-$PROXY_CRT}
		echo "APP_ID=$APP_ID PROXY=$PROXY"
		if is_answer_yes "Are the values correct (y/n)?"; then
			passed=1
			log "Setting APP_ID=$APP_ID PROXY=$PROXY"
			ini_editor -s GLOBAL -v APP_ID -w ${APP_ID}
			ini_editor -s "CC_COMM - TCP_DEVICE" -v PROXY -w ${PROXY}
			log "set_final_dest: PASSED"
		else
			log "set_final_dest: FAILED"
		fi
	done
}

if [ -z $1 ]; then
	echo "$0 [action]"
	echo "  list of actions:"
	echo "    prod            - normal production"
	echo "    force           - force normal production, IGNORE steps already performed"

	echo "    tr_fw           - verify correct TR fw settings"
	echo "    load_msp_fw     - load fw on msp from file /access_node/dn_fw_default.txt"
	echo "    check_i2c       - check I2C"
	echo "    dns             - add nameservers for ETH link"
	echo "    set_final_dest  - set deployment PROXY and APP_ID"
	echo ""
	exit 0
fi

if [ "$1" = "prod" ]; then
	echo "Choose next step: (Production: hit ENTER)"
	echo " ENTER - Start normal Production testing"
	echo " 0 - base_settings"
	echo " 1 - eth_check"
	echo " 2 - i2c_check"	
	echo " 6 - skip all"

	read tmp
else
	tmp=$1
fi

#AN_ID_CRT=`ini_editor -s GLOBAL -v AN_ID -r | cut -f2 -d' '`

case $tmp in
	help )
		exit 0
	;;

	set_final_dest )
		set_final_dest
		exit 0
	;;

	base)
		base_settings
		exit 0;;

	dns )
	  dns_add
	  exit 0;;

	check_i2c )
	  i2c_check
	  exit 0;;

	force )
	  rm -f $PROD_STEP
	  next_step=0 ;;

	1 ) next_step=1;;
	2 ) next_step=2;;
	* ) next_step=0;;
esac

touch $PROD_FILE
log "VersaRouter tests starting..." >> $PROD_FILE
echo "File system information:" >> $PROD_FILE
cat /access_node/etc/fs_info >> $PROD_FILE

#Check for interrupted production procedure which occurs when the baseboard is
# tested/configured separated by the production/configuration of the daughter board
if [ -f $PROD_STEP ]; then
	next_step=`cat $PROD_STEP`
	log "Production procedure RESUMED at step $next_step"
	echo "If you need to restart the whole production procedure hit Ctrl-C then type:"
	echo "  board_setup.sh force"
	echo "If the units is just recalled from storage, please continue."
fi

[ $next_step -le 0 ] && base_settings 	&& echo "1" > $PROD_STEP
discovery&	#it shoud work even in production stage
[ $next_step -le 1 ] && eth_check		&& echo "2" > $PROD_STEP
[ $next_step -le 2 ] && i2c_check 		&& echo "3" > $PROD_STEP

log "The base board is now ready for STORAGE"

if [ $next_step -lt 6 ]; then
	echo "Press ENTER to see Production Test File"
	echo ""
	read tmp
	cat $PROD_FILE

	echo ""
	echo "If the production test file is ok, will prepare AN for deployment"
	echo "TAKE CARE: The production testing won't work anymore when you answer y"
	if is_answer_yes "Prepare the unit for deployment (y/n)?"; then
		rm /etc/config/in_production_stage
		log2flash "Production procedure DONE"

		log "The VR is READY for delivery"

		echo ""
		echo "PLEASE upload Production Test File to a ftp server"
		echo "  the file name is $PROD_FILE"
		echo "Keep the file for later reference"
		echo ""
		echo "After uploading the file, power cycle the board"
		echo ""
	else
		log "The VR is NOT ready for delivery"
	fi

else
	log "All production steps SKIPPED"
	log "This unit is NOT ready to deliver!"
fi
