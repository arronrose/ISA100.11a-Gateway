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


if [ -e "$COMPONENT_DIR" ]; then
	rm -f "$COMPONENT_DIR"
	if [ -e "$COMPONENT_DIR" ]; then
		log_error "failed to delete link=$COMPONENT_DIR ..."
		exit 1
	fi 
fi

if ! (cd "$NISA_DIR" && ln -s "$installing_dir" "$COMPONENT_NAME" ); then
	log_error "failed to create link=$COMPONENT_NAME into $NISA_DIR !"
	exit 2 
fi 

log_info "current link to the host application created..."
exit 0 # success

