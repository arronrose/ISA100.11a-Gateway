#! /bin/sh

cd "${0%/*}/.."
absolute_path=$(pwd)

#
# main
#
if [ ! -x "${absolute_path}/etc/run.sh" ]; then
    exit 1
fi

tmpDB="/tmp/Monitor_Host.db3"
linkDB="/access_node/activity_files/Monitor_Host.db3"
currentDB="`readlink ${linkDB}`"
nextDB="/access_node/activity_files/Monitor_Host.db3_`date +%s`"

if [ "${currentDB}" = "" -a -f "${linkDB}" ]; then
	mv ${linkDB} ${nextDB}
	ln -sf ${nextDB} ${linkDB}
	currentDB="`readlink ${linkDB}`"
fi

rm -rf ${tmpDB}
cp ${currentDB} ${tmpDB}

(${absolute_path}/etc/run.sh start ${absolute_path}/MonitorHost --config ./etc/Monitor_Host.conf)

exit $?
