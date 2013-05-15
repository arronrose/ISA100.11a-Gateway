#!/bin/bash

#
#include files
#
if [ ! -f ./local_properties.sh ]; then
    echo "Connot find script <$(pwd)/local_properties.sh>!"
    exit 1
fi
source ./local_properties.sh

if [ ! -L "${COMPONENT_DIR}" ]; then
    echo "0.0.0" #not installed
    exit 0
fi

ls -l "${COMPONENT_DIR}" | awk '{print $10}' | sed -e 's/\/$//' | grep -o -e '[^/]*$'