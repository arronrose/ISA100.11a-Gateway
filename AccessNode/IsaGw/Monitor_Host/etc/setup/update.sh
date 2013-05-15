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


#if [ ! -f ./create_boot_script.sh ]; then
#    echo "Cannot find script <$(pwd)/create_boot_script.sh>!"
#    exit 1
#fi
#source ./create_boot_script.sh

ABSOLUTE_PATH=$(pwd)

#[Ovidiu]remove this method from here.
get_config_key() {
	declare conf_file="$1"
	declare key_name="$2"
		
	awk -v key_name="$key_name" -f config_key_get.awk $conf_file	
}

copy_new_files() {
	declare installing_dir="$1"	
	declare installed_dir="$2"	
	declare installed_version="$3"
	
	mkdir -p "$installing_dir"
	if [ ! -d "$installing_dir" ]; then
	    log_error "Failed to create dir=$installing_dir !"
	    return 1
	fi

	if ! (cd "$installing_dir" && tar -opxzf $ABSOLUTE_PATH/${COMPONENT_NAME}*.tgz); then
	    log_error "Failed to untar $COMPONENT_NAME into dir=$installing_dir."
	    return 2
	fi
	
	#copy old config files 
	declare merge_result="merge_configs.1234out"		
	if ! (./update-config.sh "$installing_dir" "$installed_dir"  &> $merge_result) ; then
		log_error "failed to update config file! Error=$(cat $merge_result)"
		return 3
	fi

	# update db data & schema
	declare configFile="$installed_dir/etc/Monitor_Host.conf"	
	ServerName=$(get_config_key "$configFile" "DatabaseServer")
	DatabaseName=$(get_config_key "$configFile" "DatabaseName")
	UserName=$(get_config_key "$configFile" "DatabaseUser")
	UserPassword=$(get_config_key "$configFile" "DatabasePassword")
	declare output_result="update-db.1234out" 
	log_info "update the mysql database: $DatabaseName on server: $ServerName with user: $UserName and password: $UserPassword"
	if ! (./update-mysql-db.sh $ServerName $DatabaseName $UserName $UserPassword &> $output_result) ; then
		log_error "failed to update schema! Error=$(cat $output_result)"
		return 4
	fi	
	
	declare installingconfigFile="$installing_dir/etc/Monitor_Host.conf"
	# replace the new Publishers '.conf file' with the previous installed one if exists
	installed_m_h_publishers=$(get_config_key "$configFile" "PubConfigPath")
	installing_m_h_publishers=$(get_config_key "$installingconfigFile" "PubConfigPath")
	declare installed_pub_configFile="$installed_dir/$installed_m_h_publishers"
	declare installing_pub_configFile="$installing_dir/$installing_m_h_publishers"
	if test -f "$installed_pub_configFile"
	then 
		cp "$installed_pub_configFile" "$installing_pub_configFile"
		if test $? -ne 0
		then
			log_error "failed to replace $installing_pub_configFile file with $installed_pub_configFile file"
			return 5
		fi
	fi
		
	# replace the new Monitor_Host_Log '.ini file' with the previous installed one if exists
	installed_log_iniFile=$(get_config_key "$configFile" "LogConfigPath")
	installing_log_iniFile=$(get_config_key "$installingconfigFile" "LogConfigPath")
	declare installed_log_iniFile="$installed_dir/$installed_log_iniFile"
	declare installing_log_iniFile="$installing_dir/$installing_log_iniFile"
	if test -f "$installed_log_iniFile"
	then 
		cp "$installed_log_iniFile" "$installing_log_iniFile"
		if test $? -ne 0
		then
			log_error "failed to replace $installing_log_iniFile file with $installed_log_iniFile file"
		return 6
		fi
	fi
	
	return 0			
}

#
# main program
#
installing_version="$(./get_installing_version.sh)"
installed_version="$(./get_installed_version.sh)"

installing_dir="${COMPONENT_DIR}_versions/$installing_version"
installed_dir="${COMPONENT_DIR}_versions/$installed_version"


log_info "try to upgrade $COMPONENT_NAME : $installed_version -> $installing_version ..."

if [ "a$installed_dir" == "a$installing_dir" ]; then
    log_error "the host application version=$installed_dir already installed!"
    exit 1
fi

#do not allow downgrade but only upgrade
if [[ "$installed_version" > "$installing_version" ]]; then
    log_error "Downgrade not allowed -> installed host application version=$installed_version!"
    exit 1
fi


if [ -z "${COMPONENT_NAME}*.tgz" ]; then
    log_error "the host application package <${COMPONENT_NAME}*.tgz> not found!"
    exit 2
fi

if [ -d "$installing_dir" ]; then
    log_info "the dir=$installing_dir already exists !"
    rm -r "$installing_dir"
    
    if [ -d "$installing_dir" ]; then
    	log_error "delete dir=$installing_dir failed!"
    	exit 2
    fi
fi

if ! copy_new_files "$installing_dir" "$installed_dir" "$installed_version"; then
    log_error "Failed to copy files into $installing_dir !"
    exit 3
fi


# this feature is disabled, couse the minipc alredy should have the configuration
#if ! create_boot_script "$COMPONENT_DIR/etc/start.sh"; then
#    log_error "Failed to create boot starter script ..."
#    exit 10
#fi