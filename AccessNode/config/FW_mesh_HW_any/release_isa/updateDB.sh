
#this script is called on release_one_time.sh, modules are stopped

# TAKE CARE: use relative paths. Will only work from ${NIVIS_FIRMWARE}
db_path=${NIVIS_ACTIVITY_FILES}
db_file=${db_path}/Monitor_Host.db3
# The scripts inside must be named exactly updateDB_to_${sql_script_version}.sql
sql_script_path=DbUpdate/

echo "path to db is -> $db_path"

db_schema_version=$(echo "select Value from Properties where Key = 'SchemaVersion';" | sqlite3 $db_file)
if [ $? != 0 ]; then
    echo "Cannot retreive installed database schema. "
    exit 1
fi

echo "current db version is '${db_schema_version}'"
#echo "delete  from DeviceChannelsHistory;"
#echo "delete  from DeviceChannelsHistory;" | sqlite3 $db_file

# db_schema_version is equal with the version of MH that needs that change in db

#Version of initial DB (released with the initial image)
PREV_DB_VERSION="15.0.00"

#List with all successive DB versions, in the order to be applied
DB_VERSION_LIST="2.0.08 2.0.12 2.0.13 2.0.15 2.0.17 2.0.18 2.0.24 2.0.33 2.0.50"

for crt_ver in $DB_VERSION_LIST; do
	if [ "$db_schema_version" == "$PREV_DB_VERSION" ]; then
		sql_script_version=$crt_ver
		sql_script_file=${sql_script_path}/updateDB_to_${sql_script_version}.sql
		echo "Upgrading DB ver $PREV_DB_VERSION -> $sql_script_version ..."
		sqlite3 $db_file < $sql_script_file
		if [ $? != 0 ]; then
			echo      "ERROR upgrading DB to version $sql_script_version"
			log2flash "ERROR upgrading DB to version $sql_script_version"
		else
			echo      "DB upgrade OK to version $sql_script_version"
			log2flash "DB upgrade OK to version $sql_script_version"
		fi
		#next schema version for cascaded upgrade
		db_schema_version=$sql_script_version
	fi
	PREV_DB_VERSION=$crt_ver
done
