#!/bin/bash

if [ ! -f ./local_properties.sh ]; then
    echo "Cannot find script <$(pwd)/local_properties.sh>!"
    exit 1
fi
source ./local_properties.sh

if [ ! -f ./log.sh ]; then
    echo "Cannot find script <$(pwd)/log.sh>!"
    exit 1
fi
source ./log.sh "/usr/local/NISA/Monitor_Host_Admin/logs/Monitor_Host_update.log"


get_config_key() {
	declare conf_file="$1"
	declare key_name="$2"
		
	awk -v key_name="$key_name" -f config_key_get.awk $conf_file	
}

set_config_key() {
	declare conf_file="$1"
	declare key_name="$2"
	declare key_value="$3"
	
	tmp_file="$(mktemp)"
	awk -v key_name="$key_name" -v key_value="$key_value" -f config_key_set.awk $conf_file > $tmp_file

	cp -f $tmp_file $conf_file 		
}

#
#main
#

declare installing_conf="$1/etc/Monitor_Host.conf"	
declare installed_conf="$2/etc/Monitor_Host.conf"

if [ ! -f $installed_conf ]; then
	exit 0 # nothing to do
fi


GatewayHost=$(get_config_key "$installed_conf" "GatewayHost") 
GatewayPort=$(get_config_key "$installed_conf" "GatewayPort")

DatabaseServer=$(get_config_key "$installed_conf" "DatabaseServer")
DatabaseName=$(get_config_key "$installed_conf" "DatabaseName")
DatabaseUser=$(get_config_key "$installed_conf" "DatabaseUser")
DatabasePassword=$(get_config_key "$installed_conf" "DatabasePassword")

#should not be changed (the same for GatewayPacketVersion)
#GatewayListenMode=$(get_config_key "$installed_conf" "GatewayListenMode")


#it should be done here before keeping up with intermediate application versions
LogConfigPath=$(get_config_key "$installed_conf" "LogConfigPath")
set_config_key $installing_conf "LogConfigPath" $LogConfigPath

#there are other fields to update
DatabasePath=$(get_config_key "$installed_conf" "DatabasePath")
DatabaseTimeout=$(get_config_key "$installed_conf" "DatabaseTimeout")
DatabaseVacuumPeriodHours=$(get_config_key "$installed_conf" "DatabaseVacuumPeriodHours")
DatabaseRemoveEntriesCheckPeriod=$(get_config_key "$installed_conf" "DatabaseRemoveEntriesCheckPeriod")
DatabaseRemoveEntriesOlderThanHours=$(get_config_key "$installed_conf" "DatabaseRemoveEntriesOlderThanHours")
CommandsCheckPeriod=$(get_config_key "$installed_conf" "CommandsCheckPeriod")
CommandsTimeout=$(get_config_key "$installed_conf" "CommandsTimeout")
CommandsRetryCountIfTimeout=$(get_config_key "$installed_conf" "CommandsRetryCountIfTimeout")
TopologyPooling=$(get_config_key "$installed_conf" "TopologyPooling")
ReadingsSavePeriod=$(get_config_key "$installed_conf" "ReadingsSavePeriod")
ReadingsMaxEntriesBeforeSave=$(get_config_key "$installed_conf" "ReadingsMaxEntriesBeforeSave")
BatteryStatusCheckPeriodMinutes=$(get_config_key "$installed_conf" "BatteryStatusCheckPeriodMinutes")
DeviceInformationPooling=$(get_config_key "$installed_conf" "DeviceInformationPooling")
DelayPeriodBeforeFirmwareRetry=$(get_config_key "$installed_conf" "DelayPeriodBeforeFirmwareRetry")
BulkDataTransferRate=$(get_config_key "$installed_conf" "BulkDataTransferRate")
PathToFiles=$(get_config_key "$installed_conf" "PathToFiles")
PubConfigPath=$(get_config_key "$installed_conf" "PubConfigPath")


#keep up with intermediate application versions
installed_version="$(./get_installed_version.sh)"
while test 1
do
	case "$installed_version" in
	"14.0.06") installed_version="14.0.07" ;;
	"14.0.07") installed_version="14.0.08" ;;
	"14.0.08") 
	#"14.0.09" - it wasn't released
			set_config_key $installing_conf "LogConfigPath" "etc/Monitor_Host_Log.ini"
			DatabaseServer="127.0.0.1" 
			installed_version="14.1.00" ;;
	"14.1.00") installed_version="14.1.01" ;;
	"14.1.01") installed_version="14.1.02" ;;
	"14.1.02") installed_version="14.1.03" ;;
	"14.1.03") installed_version="14.1.04" ;;
	"14.1.04") installed_version="14.1.05" ;;
	"14.1.05") installed_version="14.1.06" ;;
	"14.1.06") installed_version="14.1.07" ;;
	"14.1.07") installed_version="14.1.08" ;;  #upgrader-ul 14.1.08 had a problem with drop-index
	"14.1.08") installed_version="14.1.09" ;;
	"14.1.09") break ;; #LAST INSTALLED VERSION (-> for the new 14.1.10 )
	*)
		log_error "unrecognized installed application version=$installed_version!"
	    exit 1;;
	esac
done
			

set_config_key $installing_conf "GatewayHost" $GatewayHost
set_config_key $installing_conf "GatewayPort" $GatewayPort

set_config_key $installing_conf "DatabaseServer" $DatabaseServer
set_config_key $installing_conf "DatabaseName" $DatabaseName
set_config_key $installing_conf "DatabaseUser" $DatabaseUser
set_config_key $installing_conf "DatabasePassword" $DatabasePassword

#should not be changed
#set_config_key $installing_conf "GatewayListenMode" $GatewayListenMode

#there are other fields to update
set_config_key $installing_conf "DatabasePath" $DatabasePath
set_config_key $installing_conf "DatabaseTimeout" $DatabaseTimeout
set_config_key $installing_conf "DatabaseVacuumPeriodHours" $DatabaseVacuumPeriodHours
set_config_key $installing_conf "DatabaseRemoveEntriesCheckPeriod" $DatabaseRemoveEntriesCheckPeriod
set_config_key $installing_conf "DatabaseRemoveEntriesOlderThanHours" $DatabaseRemoveEntriesOlderThanHours
set_config_key $installing_conf "CommandsCheckPeriod" $CommandsCheckPeriod
set_config_key $installing_conf "CommandsTimeout" $CommandsTimeout
set_config_key $installing_conf "CommandsRetryCountIfTimeout" $CommandsRetryCountIfTimeout
set_config_key $installing_conf "TopologyPooling" $TopologyPooling
set_config_key $installing_conf "ReadingsSavePeriod" $ReadingsSavePeriod
set_config_key $installing_conf "ReadingsMaxEntriesBeforeSave" $ReadingsMaxEntriesBeforeSave
set_config_key $installing_conf "BatteryStatusCheckPeriodMinutes" $BatteryStatusCheckPeriodMinutes
set_config_key $installing_conf "DeviceInformationPooling" $DeviceInformationPooling
set_config_key $installing_conf "DelayPeriodBeforeFirmwareRetry" $DelayPeriodBeforeFirmwareRetry
set_config_key $installing_conf "BulkDataTransferRate" $BulkDataTransferRate
set_config_key $installing_conf "PathToFiles" $PathToFiles
set_config_key $installing_conf "PubConfigPath" $PubConfigPath


log_debug "Set gateway on host: $GatewayHost, port: $GatewayPort"
log_debug "Set gateway listen mode: $GatewayListenMode"
log_debug "Set mysql database settings: server: $DatabaseServer, db: $DatabaseName, user: $DatabaseUser"

if [ "a$GatewayHost" == "a" -o "a$GatewayPort" == "a" ]; then
	log_error "Invalid old config file! Gateway host or port key not found!"
	exit 1
fi

if [ "a$DatabaseServer" == "a" -o "a$DatabaseName" == "a" -o "a$DatabaseUser" == "a" -o "a$DatabasePassword" == "a"]; then
	log_error "Invalid old config file! DatabaseServer key or DatabaseName key or DatabaseUser key or DatabasePassword key not found!"
	exit 1
fi
