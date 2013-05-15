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



#
#main
#
installing_dir="${COMPONENT_DIR}_versions/$(./get_installing_version.sh)"

if [ -d "$installing_dir" ]; then
    log_info "cleaning up the dir=$installing_dir"
    rm -r "$installing_dir"
fi

log_info "rollbacked component=${COMPONENT_NAME}"

exit 0