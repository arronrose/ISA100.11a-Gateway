#!/bin/bash

if [ ! -f ./log.sh ]; then
    echo "Cannot find script <$(pwd)/log.sh>!"
    exit 1
fi
source ./log.sh "/usr/local/NISA/Monitor_Host_Admin/logs/Monitor_Host_update.log"

db_path="$1"
SQLITE=./sqlite3.bin


function run_db_script() {
	db_path="$1"
	script_path="$2"
	
	declare output_result="run_db_script.1234out"
	if ! $SQLITE $db_path < $script_path &> $output_result; then
		log_failed "Failed script=${script_path} output_result=$(cat $output_result)"
		return 1
	fi
	return 0
}

#
#main
#

db_schema_version=$(echo "SELECT Value FROM Properties WHERE Key = 'SchemaVersion' OR Key= 'Version';" | $SQLITE $db_path)
if [ $? != 0 ]; then
    log_error "invalid db file=${db_path}"
    exit 1
fi

log_info "current db version is=${db_schema_version}"

if [ "$db_schema_version" == "1.3.8" ]; then
    log_info "updating db to version=1.3.10 ..."
    ! run_db_script "$db_path" update-db-1.3.10.sql && exit 1
    db_schema_version="1.3.10"
fi

if [ "$db_schema_version" == "1.3.10" ]; then
    log_info "updating db to version=1.3.10.1 ..."
    ! run_db_script "$db_path" update-db-1.3.10.1.sql && exit 1
    db_schema_version="1.3.10.1"
fi

if [ "$db_schema_version" == "1.3.10.1" ]; then
    log_info "updating db to version=1.4.13 ..."
    ! run_db_script "$db_path" update-db-1.4.13.sql && exit 1
    db_schema_version="1.4.13"
fi

if [ "$db_schema_version" == "1.4.12" -o "$db_schema_version" == "1.4.13" ]; then
    log_info "updating db to version=11.2.17 ..."
    ! run_db_script "$db_path" update-db-11.2.17.sql && exit 1
    db_schema_version="11.2.17"
fi

log_info "db update successfully to version=${db_schema_version}."
