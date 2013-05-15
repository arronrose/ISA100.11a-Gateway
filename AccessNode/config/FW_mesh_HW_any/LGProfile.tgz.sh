#!/bin/sh

. common.sh

mv LGProfile.tgz /access_node/

cd /access_node/

tar -xzf LGProfile.tgz one_time_cmd.cfg
chmod -x one_time_cmd.cfg
rule_file_mgr ${NIVIS_PROFILE}rule_file.cfg one_time_cmd.cfg
