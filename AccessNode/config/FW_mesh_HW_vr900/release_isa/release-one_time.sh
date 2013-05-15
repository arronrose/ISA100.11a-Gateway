#! /bin/sh

set -x

#only if there is in archive and not already updated
if [ -f an_sys_patch/mini_httpd ]; then
	rm -f /access_node/bin/mini_httpd
	mv an_sys_patch/mini_httpd /usr/bin/mini_httpd
	chmod +x /usr/bin/mini_httpd
fi

currentDB="/access_node/activity_files/Monitor_Host.db3"

if [ -h "${currentDB}" ]; then
	realDB="`readlink ${currentDB}`"
	if [ "${realDB}" != "" ]; then
		rm  ${currentDB}
		mv  ${realDB} ${currentDB}
	else
		[ -f Monitor_Host.db3.fixture ] && cp Monitor_Host.db3.fixture ${currentDB}
	fi
fi

if [ -f an_sys_patch/Monitor_Host.db3 -a ! -f "/access_node/activity_files/Monitor_Host.db3" ]; then
	mv an_sys_patch/Monitor_Host.db3 ${currentDB}
fi

[ -x updateDB.sh ] && updateDB.sh

[ ! -d /etc/dropbear/ ] && mkdir /etc/dropbear
if [ ! -f /access_node/activity_files/dropbear_rsa_host_key ]; then
	echo      "Create dropbear_rsa_host_key. Please wait..."
	log2flash "Create dropbear_rsa_host_key..."
	dropbearkey -t rsa -f /access_node/activity_files/dropbear_rsa_host_key
	ln -sf /access_node/activity_files/dropbear_rsa_host_key   /etc/dropbear/dropbear_rsa_host_key
fi

if [ ! -f /access_node/activity_files/dropbear_dss_host_key ]; then
	echo      "Create dropbear_dss_host_key. Please wait..."
	log2flash "Create dropbear_dss_host_key..."
	dropbearkey -t dss -f /access_node/activity_files/dropbear_dss_host_key
	ln -sf /access_node/activity_files/dropbear_dss_host_key   /etc/dropbear/dropbear_dss_host_key
fi


