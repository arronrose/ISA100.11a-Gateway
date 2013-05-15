#!/bin/bash

if [ ! -f ./log.sh ]; then
    echo "Cannot find script <$(pwd)/log.sh>!"
    exit 1
fi
source ./log.sh "/usr/local/NISA/Monitor_Host_Admin/logs/Monitor_Host_update.log"

server_name="$1"
db_name="$2"
user_name="$3"
password="$4"
MYSQL=mysql

function run_db_script() {
	server_name="$1"	
	user_name="$2"
	password="$3"
	script_path="$4"
	
	declare output_result="run_db_script.1234out"
	if ! $MYSQL -h$server_name -u$user_name -p$password < $script_path > $output_result; then
		log_failed "Failed script=${script_path} output_result=$(cat $output_result)"
		return 1
	fi
	return 0
}

#
#main
#

db_schema_version=$(echo "SELECT Value FROM Properties WHERE Properties.Key = 'SchemaVersion';" | $MYSQL -h$server_name -u$user_name -p$password -D$db_name | sed 1d)
if [ $? != 0 ]; then
    log_error "Cannot retrive installed database schema. Server=${server_name}. Db=${db_name}"
    exit 1
fi

log_info "current db version is=${db_schema_version}"


# this way was before
#if [ "$db_schema_version" != "14.1.2" ]; then
#    log_info "updating db to version=14.1.2 ..."
#    ! run_db_script $server_name $user_name $password update-mysqldb-14.1.2.sql && exit 1
#    db_schema_version="14.1.2"
#fi


#keep up with intermediate db versions (db_version is equal to the version of the application made the change to db, so from now on the db vers. format is XX.X.XX)
#every update to db is made by using a script file (.sql) of type: update-mysqldb-xx.x.xx.sql (from now on)
while test 1
do
	case "$db_schema_version" in
	"14.0.0") db_schema_version="14.0.1" ;;
	"14.0.1") db_schema_version="14.0.2" ;;
	"14.0.2") db_schema_version="14.0.3" ;;
	"14.0.3") db_schema_version="14.0.4" ;;
	"14.0.4") db_schema_version="14.0.5" ;;
	"14.0.5") db_schema_version="14.0.6" ;;
	"14.0.6") db_schema_version="14.0.7" ;;
	"14.0.7") db_schema_version="14.0.8" ;;
	"14.0.8")
			log_info "updating db to version=14.1.0 ..."
    		! run_db_script $server_name $user_name $password update-mysqldb-14.1.0.sql && exit 1
			db_schema_version="14.1.0" 
			log_info "db update successfully to version=${db_schema_version}." ;;
	"14.1.0")
			log_info "updating db to version=14.1.1 ..."
			! run_db_script $server_name $user_name $password update-mysqldb-14.1.1.sql && exit 1 
			db_schema_version="14.1.1"
			log_info "db update successfully to version=${db_schema_version}." ;;
	"14.1.1")
			log_info "updating db to version=14.1.2 ..."
			! run_db_script $server_name $user_name $password update-mysqldb-14.1.2.sql && exit 1
			db_schema_version="14.1.2" 
			log_info "db update successfully to version=${db_schema_version}." ;;
	"14.1.2")
			# 14.1.3 - this was a forced version, so ignore it when progressing in the sequence
			#log_info "updating db to version=14.1.3 ..."
    		#! run_db_script $server_name $user_name $password update-mysqldb-14.1.3_1.sql && exit 1
    		#! run_db_script $server_name $user_name $password update-mysqldb-14.1.3_2.sql && exit 1
    		#! run_db_script $server_name $user_name $password update-mysqldb-14.1.3_3.sql && exit 1 
			#db_schema_version="14.1.3"
			#log_info "db update successfully to version=${db_schema_version}."
			
			#this has a problem with drop index, so...
			db_schema_version="14.1.08" ;; 
	"14.1.3")
			#this has a problem with drop index, so...
			db_schema_version="14.1.08" ;;
	"14.1.08")
			log_info "updating db to version=14.1.09 ..."
			#! run_db_script $server_name $user_name $password update-mysqldb-14.1.09_simple_ISA.sql && exit 1
			! run_db_script $server_name $user_name $password update-mysqldb-14.1.09_full_ISA.sql && exit 1
			db_schema_version="14.1.09" 
			log_info "db update successfully to version=${db_schema_version}." ;;
	"14.1.09")
			log_info "updating db to version=14.1.10 ..."
			#! run_db_script $server_name $user_name $password update-mysqldb-14.1.10_simple_ISA.sql && exit 1
			#! run_db_script $server_name $user_name $password update-mysqldb-14.1.10_full_ISA.sql && exit 1
			! run_db_script $server_name $user_name $password update-mysqldb-14.1.10_PreservedDB.sql && exit 1
			db_schema_version="14.1.10" 
			log_info "db update successfully to version=${db_schema_version}." ;;
	"14.1.10")
			break ;; #LAST DB VERSION
	*)
		log_error "unrecognized installed db version=$db_schema_version!"
	    exit 1;;
	esac
done


#this way was before
#log_info "db update successfully to version=${db_schema_version}."
