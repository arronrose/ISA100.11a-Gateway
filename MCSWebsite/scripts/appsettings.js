var APP_LastValidDeviceInformationInterval = 10; //the interval for valid get device information commands in progress (in minutes)
var APP_DateTimeFormat = "yyyy-mm-dd HH:MM:ss";
var WEB_DateTimeFormat = "mm/dd/yyyy HH:MM TT";

// the interval between 2 checks of a get device information command completion
var RefreshInterval = 5000; //miliseconds 
var RequestedCommittedBurstDefaultValue = -15;
var AUTO_REFRESH_DATA_OLDER_THEN = 600; 	//in seconds
var AUTO_REFRESH_DEVICE_FW_UPGRADE = 60;//in seconds

// congi files
var FILE_CONFIG_INI = "config.ini";
var FILE_SM_SUBNET_INI = "sm_subnet.ini";
var FILE_SYSTEMMANAGER_INI = "system_manager.ini";
var FILE_MODBUS_INI = "modbus_gw.ini";
var FILE_MH_PUBLISHERS_CONF = "Monitor_Host_Publishers.conf";

// Device image size
var MAX_DEVICE_ICON_SIZE = 32; // in pixels
var MAX_DEVICE_ICON_FILE_SIZE = 10; // in Kbytes

//Company logo image size
var MAX_DEVICE_LOGO_WIDTH = 252; // in pixels
var MAX_DEVICE_LOGO_HEIGHT = 72; // in pixels

