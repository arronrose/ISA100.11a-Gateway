#!/bin/bash

SQLITE=/usr/local/NISA/Monitor_Host_Admin/scripts/sqlite3.bin
db_path=/usr/local/NISA/Monitor_Host/db/Monitor_Host.db3

if [ ! -f $db_path ]; then
    echo "Cannot find the database file!"
    exit 1
fi

if [ ! -f $SQLITE ]; then
    echo "Cannot find the sqlite3.bin file!"
    exit 1
fi
	
if ! $SQLITE $db_path 'UPDATE Users SET Role='0' WHERE UserID='1''; then
		echo "Failed to run update script!"
		exit 3
fi

echo "Update admin role succesfull!"