#!/bin/sh

. common.sh

#working folder for this .sh have to be /access_node/
cd /access_node/ 

tar xf /tmp/LGFirmware.tar 
rm /tmp/LGFirmware.tar 

#first try the easy way
fw_version=`tar xzOf LGFirmware.tgz \*/version`
#if easy way does not work, try the hard way
if [ -z "$fw_version" ]; then 
	log2flash "WARN detect fw version the hard way"
	fw_version=`tar tzf LGFirmware.tgz |
	sed "s/\/.*//
	     s/an_bin_\(.*\)_lg/\1/" |\
	uniq`
fi

raise_an_ev -e 14  -v $fw_version	#Firmware download is complete

chmod -x one_time_cmd.cfg
rule_file_mgr ${NIVIS_PROFILE}rule_file.cfg one_time_cmd.cfg
