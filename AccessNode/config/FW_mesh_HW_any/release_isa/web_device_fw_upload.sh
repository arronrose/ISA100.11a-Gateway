#!/bin/sh

set +x

. /etc/profile

. ${NIVIS_FIRMWARE}common.sh

DIR_UPGRADE=upgrade_web/

log2flash "Web request - Upload device firmware"


old_path=`pwd`

if [ ! -d ${NIVIS_TMP}$DIR_UPGRADE ]; then
	echo "ER_RESULT=ERROR no upload folder"
	log2flash "Upload ERROR no upload folder"
	exit 1
fi

cd ${NIVIS_TMP}$DIR_UPGRADE

FW_FILE=`ls -1 * | tail -n 1`

if [ -z $FW_FILE ]; then
	tmp_var=`ls`
	echo "ER_RESULT=ERROR no FW_FILE "
	echo "Uploaded files: $tmp_var"
	log2flash "Upload ERROR no FW_FILE "
	log2flash "Uploaded Files: $tmp_var"
	cd $old_path
	exit 1
fi

echo "FW_FILE=$FW_FILE"
 
mv $FW_FILE ${NIVIS_ACTIVITY_FILES}/devices_fw_upgrade/ 2>&1 >${NIVIS_TMP}${DIR_UPGRADE}/act_out.txt
cat ${NIVIS_TMP}${DIR_UPGRADE}/act_out.txt

echo "ER_RESULT=SUCCESS"
log2flash "ER_RESULT=SUCCESS"

cd $old_path
rm -R  ${NIVIS_TMP}$DIR_UPGRADE

exit 0
