#!/bin/bash

#
#include files
#
if [ ! -f ./local_properties.sh ]; then
    echo "Connot find script <$(pwd)/local_properties.sh>!"
    exit 1
fi
source ./local_properties.sh
ls ${COMPONENT_NAME}*.tgz | awk -F '-' '{ print $2 }'
