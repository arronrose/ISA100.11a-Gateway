#! /bin/sh

cd "${0%/*}/.."
absolute_path=$(pwd)

#
# main
#
if [ ! -x ${absolute_path}/etc/run.sh ]; then
    exit 1
fi

(${absolute_path}/etc/run.sh stop ${absolute_path}/MonitorHost --config ./etc/Monitor_Host.conf)

tmpDB="/tmp/Monitor_Host.db3"
linkDB="/access_node/activity_files/Monitor_Host.db3"
currentDB="`readlink /access_node/activity_files/Monitor_Host.db3`"
nextDB="/access_node/activity_files/Monitor_Host.db3_`date +%s`"

cp  ${tmpDB}  ${nextDB}

if [ "$?" = "0" ] ; then
	ln -sf ${nextDB} ${linkDB}
	if [ "$?" = "0" ] ; then
		rm -rf ${currentDB}
	else
		rm -rf ${nextDB}
	fi
else
	rm -rf ${nextDB}
fi

exit $?
