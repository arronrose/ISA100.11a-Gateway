function Device() {
    this.DeviceID = null;
    this.DeviceRole = null;
    this.DeviceRoleID = null;
    this.Address128 = null;
    this.Address64 = null;
    this.DeviceTag = null;
    this.DeviceStatus = null;
    this.LastRead = null;
    this.DeviceLevel = null;
    this.RejoinCount = null;
    this.DPDUsTransmitted = null;
    this.DPDUsReceived = null;
    this.DPDUsFailedTransmission = null;
    this.DPDUsFailedReception = null;
    this.PowerSupplyStatus = null;
    this.Manufacturer = null;
    this.Model = null;
    this.Revision = null;
    this.SubnetID = null;
}

function DashboardDevice() {
    this.SlotNumber = null;
    this.Deviceid = null;
    this.ChannelNo = null;
    this.GaugeType = null;
    this.MinValue = null;
    this.MaxValue = null;
}

function DeviceHistory() {
    this.Timestamp = null;
    this.DeviceStatus = null;
}


// device status
var DS_NotJoined = 1;                    // Device not joined
var DS_SecJoinRequestReceived = 4;       // Security join request received
var DS_SecJoinResponseSent = 5;          // Security join response sent
var DS_SmJoinReceived = 6;               // Network join request received (System_Manager_Join method)
var DS_SmJoinResponseSent = 7;           // Network join response sent
var DS_SmContractJoinReceived = 8;       // Join contract request received (System_Manager_Contract method)
var DS_SmContractJoinResponseSent = 9;   // Join contract response sent (System_Manager_Contract method)
var DS_SecConfirmReceived = 10;          // Security join confirmation received (Security_Confirm)
var DS_SecConfirmResponseSent = 11;      // Security join confirmation response sent (Security_Confirm)
var DS_JoinedAndConfigured = 20;         // Joined & Configured & All info available


function GetDeviceStatusName(devStatus) {
    switch (devStatus) {
    case DS_NotJoined:
        return "NOT_JOINED";
	case DS_SecJoinRequestReceived:
		return "SEC_JOIN_Req";   //"Security join request received";
	case DS_SecJoinResponseSent:
		return "SEC_JOIN_Rsp";   //"Security join response sent";
	case DS_SmJoinReceived:
		return "NETWORK_Req";    //"Network join request received";
	case DS_SmJoinResponseSent:
		return "NETWORK_Rsp";    //"Network join response sent";
	case DS_SmContractJoinReceived:
		return "CONTRACT_Req";   //"Join contract request received";
	case DS_SmContractJoinResponseSent:
		return "CONTRACT_Rsp";   //"Join contract response sent";
	case DS_SecConfirmReceived:
		return "SEC_CNFRM_Req";  //"Security join confirmation received";
	case DS_SecConfirmResponseSent:
		return "SEC_CNFRM_Rsp";  //"Security join confirmation response sent";
    case DS_JoinedAndConfigured:
	    return "FULL_JOIN";      //"Joined & Configured & All info available";
	default:
        return "UNKNOWN";
    }
}


function GetDeviceStatusColor(devStatus) {
    if (devStatus >= DS_JoinedAndConfigured) {
        return "#008800";
    }
    if (devStatus < DS_JoinedAndConfigured && devStatus >= DS_SecJoinRequestReceived) {
        return "#FF6600";
    }
    if (devStatus < DS_SecJoinRequestReceived) {
        return "#CC0000";
    }
}


function GetDeviceStatusDescription(devStatus) {
    switch (devStatus) {
	case DS_SecJoinRequestReceived:
		return "Security join request received";
	case DS_SecJoinResponseSent:
		return "Security join response sent";
	case DS_SmJoinReceived:
		return "Network join request received";
	case DS_SmJoinResponseSent:
		return "Network join response sent";
	case DS_SmContractJoinReceived:
		return "Join contract request received";
	case DS_SmContractJoinResponseSent:
		return "Join contract response sent";
	case DS_SecConfirmReceived:
		return "Security join confirmation received";
	case DS_SecConfirmResponseSent:
		return "Security join confirmation response sent";
    case DS_JoinedAndConfigured:
	    return "Joined & Configured & All info available";
	default:
        return "unknown";
    }
}

//  device types
//  ! VERY IMPORTANT !
//  when adding removing a device you must verify the methods below in order to update their arrays.
var DT_SystemManager = 1;
var DT_Gateway = 2;
var DT_BackboneRouter = 3;
var DT_Device = 10;
var DT_DeviceNonRouting = 11;
var DT_DeviceIORouting = 12;
var DT_HartISAAdapter = 1000;
var DT_WirelessHartDevice = 1001;

function GetDeviceTypeArrayForReadingsReport(){
    return Array(DT_Device, DT_DeviceNonRouting, DT_DeviceIORouting, DT_HartISAAdapter, DT_WirelessHartDevice);
}

function GetDeviceTypeArrayForFirmwareExecution(){
    return Array(DT_Device, DT_DeviceNonRouting, DT_DeviceIORouting, DT_HartISAAdapter, DT_WirelessHartDevice, DT_BackboneRouter)
}

function GetDeviceTypeArrayForSensorBoardFirmwareExecution(){
    return Array(DT_Device, DT_DeviceNonRouting, DT_DeviceIORouting, DT_HartISAAdapter, DT_WirelessHartDevice)
}

function IsFieldDevice(deviceRole) {
	return ((deviceRole == DT_Device) || 
			(deviceRole == DT_DeviceNonRouting) || 
			(deviceRole == DT_DeviceIORouting) || 
			(deviceRole == DT_HartISAAdapter) || 
			(deviceRole == DT_WirelessHartDevice));
}

function GetDeviceRole(deviceRole) {
    $deviceRoleName = null;	
	switch (deviceRole) {
	case DT_SystemManager:
		return "System Manager";
	case DT_Gateway:
		return "Gateway";
	case DT_BackboneRouter:
		return "Backbone Router";
	case DT_Device:
		return "Router Device";
	case DT_DeviceNonRouting:
		return "IO Device";
	case DT_DeviceIORouting:
		return "IO Router Device";
	case DT_HartISAAdapter:
		return "HART Adapter";
	case DT_WirelessHartDevice:
		return "WirelessHart Device";
	default:
		return "Unknown - " + deviceRole;
	}
}


function GetDeviceRoleColor(deviceRole) {
    $deviceRoleName = null;	
	switch (deviceRole) {
	case DT_SystemManager:
		return "#337777";
	case DT_Gateway:
		return "#773377";
	case DT_BackboneRouter:
		return "#151B8D";
	case DT_Device:
		return "#0000AA";
	case DT_DeviceIORouting:
		return "#AA0000";
	case DT_DeviceNonRouting:
		return "#00AA00";
	case DT_HartISAAdapter:
		return "#FFCC00";
	case DT_WirelessHartDevice:
		return "#CCFF00";
	default:
		return "#000000";
	}
}


function GetDeviceRoleImage(deviceRole, size) {		
	switch (deviceRole) {
	case DT_BackboneRouter:
        switch(size) {
        case 1: return "backboneRouter_small.gif";
        case 2: return "backboneRouter.gif";
        case 3: return "backboneRouter1p25.gif";
        case 4: return "backboneRouter1p5.gif";
        case 5: return "backboneRouter1p75.gif";
		default: return "backboneRouter.gif";
		}
	case DT_Device:
        switch(size) {
        case 1: return "device_small.gif";
        case 2: return "device.gif";
        case 3: return "device1p25.gif";
        case 4: return "device1p5.gif";
        case 5: return "device1p75.gif";
		default: return "device.gif";
		}
	case DT_DeviceNonRouting:
        switch(size) {
        case 1: return "device_small.gif";
        case 2: return "device.gif";
        case 3: return "device1p25.gif";
        case 4: return "device1p5.gif";
        case 5: return "device1p75.gif";
		default: return "device.gif";
		}
	case DT_DeviceIORouting:
        switch(size) {
        case 1: return "device_small.gif";
        case 2: return "device.gif";
        case 3: return "device1p25.gif";
        case 4: return "device1p5.gif";
        case 5: return "device1p75.gif";
		default: return "device.gif";
		}
	case DT_HartISAAdapter:
        switch(size) {
        case 1: return "device_small.gif";
        case 2: return "device.gif";
        case 3: return "device1p25.gif";
        case 4: return "device1p5.gif";
        case 5: return "device1p75.gif";
		default: return "device.gif";
		}
	case DT_Gateway:
        switch(size) {
        case 1: return "gateway_small.gif";
        case 2: return "gateway.gif";
        case 3: return "gateway1p25.gif";
        case 4: return "gateway1p5.gif";
        case 5: return "gateway1p75.gif";
		default: return "gateway.gif";
		}
	case DT_SystemManager:
        switch(size) {
        case 1: return "systemManager_small.gif";
        case 2: return "systemManager.gif";
        case 3: return "systemManager1p25.gif";
        case 4: return "systemManager1p5.gif";
        case 5: return "systemManager1p75.gif";
		default: return "systemManager.gif";
		}
	case DT_WirelessHartDevice:
        switch(size) {
        case 1: return "device_small.gif";
        case 2: return "device.gif";
        case 3: return "device1p25.gif";
        case 4: return "device1p5.gif";
        case 5: return "device1p75.gif";
		default: return "device.gif";
		}
	default:
        switch(size) {
        case 1: return "device_small.gif";
        case 2: return "device.gif";
        case 3: return "device1p25.gif";
        case 4: return "device1p5.gif";
        case 5: return "device1p75.gif";
		default: return "device.gif";
		}
    }
}  

function PreloadImages() {
	var img1 = new Image();
	img1.src = "styles/images/"+ GetDeviceRoleImage(DT_SystemManager, 2);
    var img2 = new Image();
    img2.src = "styles/images/"+ GetDeviceRoleImage(DT_Gateway, 2);
    var img3 = new Image();
    img3.src = "styles/images/"+ GetDeviceRoleImage(DT_BackboneRouter, 2);
    var img4 = new Image();
    img4.src = "styles/images/"+ GetDeviceRoleImage(DT_Device, 2);
    var img5 = new Image();
    img5.src = "styles/images/"+ GetDeviceRoleImage(DT_DeviceNonRouting, 2);
    var img6 = new Image();
    img6.src = "styles/images/"+ GetDeviceRoleImage(DT_DeviceIORouting, 2);
    var img7 = new Image();
    img7.src = "styles/images/"+ GetDeviceRoleImage(DT_HartISAAdapter, 2);
    var img8 = new Image();
    img8.src = "styles/images/"+ GetDeviceRoleImage(DT_WirelessHartDevice, 2);
}
