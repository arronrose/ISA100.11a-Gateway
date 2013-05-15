#!/bin/sh

. common.sh


rel_start_modules_config()
{
	#TAKE CARE: nc local must be started first. cc_comm, history, rf_repeater must be started before scheduler
	_BEFORE_START_="cp take_system_snapshot.sh /tmp,rm -f /tmp/*.pipe,rm -f /tmp/shared_var.shd,rm -f /tmp/sm.unstable,watchdog&"
	_AFTER_START_="sys_monitor.sh&,ln -sf /tmp/sm.log /tmp/SystemManager.log,ln -sf /tmp/mh.log /tmp/MonitorHost.log"	#for snapshot_warning done by watchdog
	
	_NC_LOCAL_=""
	
	#TAKE CARE: must have a space between module and &, for correct wdt watch
	MDL_SystemManager="SystemManager > /tmp/SystemManagerProcess.log 2>&1 &"
	MDL_MonitorHost="MonitorHost &"
	MDL_isa_gw="isa_gw &"
	MDL_backbone="backbone &"
	MDL_modbus_gw="modbus_gw &"
	MDL_scgi_svc="/access_node/firmware/www/wwwroot/scgi_svc &"
	MDL_=""
	
	_SCHD_="scheduler &"

	
}

rel_stop_modules_config()
{
	# can this be shared with start.sh ?
	# keep modbus_gw_isa for a while until there are no VR with old FW
	MODULES="watchdog backbone sys_monitor.sh scheduler isa_gw SystemManager MonitorHost modbus_gw modbus_gw_isa sm_extra_logs_cleaner.sh sm_log_upload.sh scgi_svc"
	MOD_SML="watchdog backbone sys_monitor.sh scheduler isa_gw SystemManager MonitorHost modbus_gw modbus_gw_isa sm_extra_logs_cleaner.sh sm_log_upload.sh"
	MOD_EXC_WTD="backbone sys_monitor.sh scheduler isa_gw SystemManager MonitorHost modbus_gw modbus_gw_isa sm_extra_logs_cleaner.sh sm_log_upload.sh scgi_svc"
}


rel_renice_modules()
{
	Renice "watchdog"  -10
	Renice "backbone"  -9
	Renice "modbus_gw" -7
	Renice "scgi_svc" -5
	Renice "SystemManager" -3
	Renice "isa_gw" -2
	Renice "MonitorHost" -1
}