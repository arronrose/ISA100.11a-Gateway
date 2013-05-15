#!/bin/bash

#
#include files
#
if [ ! -f ./local_properties.sh ]; then
    echo "Connot find script <$(pwd)/local_properties.sh>!"
    exit 1
fi
source ./local_properties.sh


if [ ! -f "${COMPONENT_DIR}/etc/stop.sh" ]; then
    echo "Component=${COMPONENT_DIR}/etc/stop.sh  not found!"
    exit 0
fi


${COMPONENT_DIR}/etc/stop.sh
exit $?
