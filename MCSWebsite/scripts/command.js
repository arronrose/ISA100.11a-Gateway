function Command() {
    this.TrackingNo = null;
    this.DeviceID = null;
    this.CommandTypeCode = null;
    this.CommandStatus = null;
    this.TimePosted = null;
    this,TimeResponded = null;
    this.DeviceAddress64 = null;
    this.DeviceAddress128 = null;
    this.Response = null;
    this.ParametersDescription = null;
}

function CommandParameter() {
    this.ParameterCode = null;
    this.ParameterValue = null;
}

//command status
var CS_New = 0;
var CS_Sent = 1;
var CS_Responded = 2;
var CS_Failed = 3;
	
function GetStatusName(stat) {		
	switch (stat) {
	case CS_New:
	  return "New";
	case CS_Sent:
	  return "Sent";
	case CS_Responded:
	  return "Responded";
	case CS_Failed:
	  return "Failed";			
	default:
	  return NAString;
	}
}

//command type codes and command parameters codes 
var CTC_RequestTopology = 0;

var CTC_ReadValue = 1;
    var CPC_ReadValue_Channel =  10;
    var CPC_ReadValue_RequestedCommittedBurst = 1230;

var CTC_PublishSubscribe = 3
    var CPC_PublishSubscribe_Channel = 30;
    var CPC_PublishSubscribe_Frequency = 31;
    var CPC_PublishSubscribe_Offset = 32;
    var CPC_PublishSubscribe_SubscriberChannel = 33;
    var CPC_PublishSubscribe_SubscriberDevice = 34;
    var CPC_PublishSubscribe_ConcentratorObjectID = 35;
    var CPC_PublishSubscribe_TSAPID = 36;
    var CPC_PublishSubscribe_DataStaleLimit = 37;

var CTC_ISACSRequest = 10;
    var CPC_ISACSRequest_TSAPID = 80;
    var CPC_ISACSRequest_RequestType = 81;
    var CPC_ISACSRequest_ObjectID = 82;
    var CPC_ISACSRequest_AttributeMethodID = 83;
    var CPC_ISACSRequest_AttributeIndex1 = 84;
    var CPC_ISACSRequest_AttributeIndex2 = 85;
    var CPC_ISACSRequest_DataBuffer = 86;
    var CPC_ISACSRequest_ReadAsPublish = 87;
    var CPC_ISACSRequest_RequestedCommittedBurst = 1230;
    
    var CPTC_ISACSRequest_RequestType_Read = 3;
    var CPTC_ISACSRequest_RequestType_Write = 4;
    var CPTC_ISACSRequest_RequestType_Execute = 5;
    
var CTC_CreateLease = 100;
	
var CTC_FirmwareUpdate = 104;
	var CPC_FirmwareUpdate_DeviceID = 1040;
	var CPC_FirmwareUpdate_FileName = 1041;        
    var CPC_FirmwareUpdate_RequestedCommittedBurst = 1230;	

var CTC_GetFirmwareUpdateStatus = 105;
    var CPC_GetFirmwareUpdateStatus_DeviceID = 1050;
    var CPC_GetFirmwareUpdateStatus_RequestedCommittedBurst = 1230;
    
var CTC_CancelFirmwareUpdate = 106;
    var CPC_CancelFirmwareUpdate_DeviceID = 1060;
    var CPC_CancelFirmwareUpdate_RequestedCommittedBurst = 1230;    

var CTC_ResetDevice = 108;
    var CPC_ResetDevice_DeviceID = 1080;
    var CPC_ResetDevice_RestartType = 1081;
    var CPC_ResetDevice_RequestedCommittedBurst =  1230;
    
var CTC_FirmwareUpload = 110;
	var CPC_FirmwareUpload_FileName = 1100;
	var CPC_FirmwareUpload_FirmwareID = 1101;
	var CPC_FirmwareUpload_RequestedCommittedBurst = 1230;	

var CTC_DeleteSubscriberContract = 112;
    var CPC_DeleteSubscriberContract_LeaseID = 38;    

var CTC_NetworkHealthReport = 113;

var CTC_ScheduleReport = 114;
	var CPC_ScheduleReport_DeviceID = 61;

var CTC_NeighborHealthReport = 115;
    var CPC_NeighborHealthReport_DeviceID = 62;

var CTC_DeviceHealthReport = 116;
	var CPC_DeviceHealthReport_DeviceID = 63;

var CTC_NetworkResourceReport = 117;

var CTC_GetContractsAndRoutes = 119;
	var CPC_GetContractsAndRoutes_RequestedCommittedBurst = 64;

var CTC_GetDeviceList = 120;

var CTC_UpdateSensorBoardFirmware = 121;
	var CPC_UpdateSensorBoardFirmware_TargetDeviceID = 1200;
	var CPC_UpdateSensorBoardFirmware_FileName = 1201;
	var CPC_UpdateSensorBoardFirmware_TSAPID = 1202;
	var CPC_UpdateSensorBoardFirmware_ObjID = 1203;
	var CPC_UpdateSensorBoardFirmware_RequestedCommittedBurst = 1230;
	
var CTC_CancelSensorBoardFirmwareUpdate = 122;	
	var CPC_CancelSensorBoardFirmwareUpdate_TargetDeviceID = 1210;
	var CPC_CancelSensorBoardFirmwareUpdate_RequestedCommittedBurst = 1230;

var CTC_ChannelsStatistics = 123;
    var CPC_ChannelsStatistics_DeviceID = 1220;
    var CPC_ChannelsStatistics_RequestedCommittedBurst = 1230;    

        
function GetCommandName(commandCode, commandType) {
    switch(commandCode) {
    case CTC_RequestTopology: 
        return "Request Topology";
    case CTC_ReadValue: 
        return "Read Value";
    case CTC_CreateLease: 
        return "Create Lease";
    case CTC_ResetDevice: 
        return "Reset Device";
    case CTC_GetFirmwareUpdateStatus: 
        return "Get Firmware Update Status";
    case CTC_CancelFirmwareUpdate: 
        return "Cancel Firmware Update";
    case CTC_FirmwareUpdate: 
        return "Firmware Update";
    case CTC_FirmwareUpload: 
        return "Firmware Upload";
    case CTC_ScheduleReport: 
        return "Schedule Report";
    case CTC_NeighborHealthReport: 
        return "Neighbor Health Report";            
    case CTC_DeviceHealthReport: 
        return "Device Health Report";
    case CTC_NetworkResourceReport: 
        return "Network Resource Report";
    case CTC_GetDeviceList: 
        return "Get Device List";
    case CTC_GetContractsAndRoutes: 
        return "Get Contracts and Routes";
    case CTC_ChannelsStatistics:
        return "Get Channels Statistics";           
    case CTC_NetworkHealthReport:    
        return "Network Health Report";           
    case CTC_ISACSRequest:
        switch (commandType) {
        case CPTC_ISACSRequest_RequestType_Read: 
            return "Read Object Attribute";
        case CPTC_ISACSRequest_RequestType_Write: 
            return "Write Object Attribute";
        case CPTC_ISACSRequest_RequestType_Execute: 
            return "Execute Object Attribute";
        case null:
            return "ISA Request";                        
        default: 
            return NAString;
        }
    case CTC_DeleteSubscriberContract:
    	return "Delete Lease";	
    case CTC_UpdateSensorBoardFirmware:
    	return "Update Sensor Board Firmware";
    case CTC_CancelSensorBoardFirmwareUpdate:
    	return "Cancel Sensor Board Firmware Update";    	
    case CTC_PublishSubscribe:
    	return "Publish Subscribe";
    default: 
        return NAString;
    }
}


function GetCommandsArray() {
    return  Array(
				CTC_CancelFirmwareUpdate,
			    CTC_CancelSensorBoardFirmwareUpdate,
			    CTC_CreateLease,
			    CTC_DeviceHealthReport,
			    CTC_DeleteSubscriberContract,
			    CTC_ChannelsStatistics,
			    CTC_ISACSRequest,
			    CTC_FirmwareUpdate,     
			    CTC_FirmwareUpload,
			    CTC_GetContractsAndRoutes,
			    CTC_GetDeviceList,
			    CTC_GetFirmwareUpdateStatus,
			    CTC_NeighborHealthReport,
			    CTC_NetworkHealthReport,
			    CTC_NetworkResourceReport,
			    CTC_PublishSubscribe,
			    CTC_ReadValue, 
			    CTC_RequestTopology, 
			    CTC_ResetDevice,
			    CTC_UpdateSensorBoardFirmware,
			    CTC_ScheduleReport
    			);    
}


function GetFirmwareCommandsArray() {
    return	Array(
    			CTC_CancelFirmwareUpdate,
    			CTC_CancelSensorBoardFirmwareUpdate,
    			CTC_FirmwareUpdate, 
    			CTC_GetFirmwareUpdateStatus,     			    			
    			CTC_UpdateSensorBoardFirmware
    			);
}


// RestartTypes
var RT_WarmRestart = 2;
var RT_RestartAsProvisioned = 3;
var RT_ResetToFactoryDefaults = 4;


function GetRestartTypeName(type) {
	switch(type) {
	case RT_WarmRestart:
		return "Warm Restart";
	case RT_RestartAsProvisioned:
		return "Restart as provisioned";
	case RT_ResetToFactoryDefaults:
		return "Reset to factory defaults";
	default:
		return type;
	}
}


function GetRequestTypeName(requestType) {
	switch(requestType) {
	case CPTC_ISACSRequest_RequestType_Read:
		return "Read";
	case CPTC_ISACSRequest_RequestType_Write:
		return "Write";
	case CPTC_ISACSRequest_RequestType_Execute:
		return "Execute";
	default:
		return requestType;
	}
}


function GetParameterName(commandCode, parameterCode) {		
	switch (commandCode) {
	case CTC_RequestTopology:
		return NAString;
	case CTC_ReadValue:
		switch (parameterCode) {					
		case CPC_ReadValue_Channel:
			return "Channel";
		case CPC_ReadValue_RequestedCommittedBurst:
			return "Committed Burst";
		default:
			return NAString;
		}
	case CTC_PublishSubscribe:
		switch (parameterCode) {
		case CPC_PublishSubscribe_Channel:
			return "Publish Channel";
		case CPC_PublishSubscribe_Frequency:
			return "Publish Frequency";	
		case CPC_PublishSubscribe_Offset:
			return "Publish Offset";
		case CPC_PublishSubscribe_SubscriberChannel:
			return FALSE; //this parameter cannot be viewed in UI
		case CPC_PublishSubscribe_SubscriberDevice:
			return FALSE; //this parameter cannot be viewed in UI
		case CPC_PublishSubscribe_ConcentratorObjectID:
			return "Concentrator Object ID";		
		case CPC_PublishSubscribe_TSAPID:
			return "TSAP ID";
		case CPC_PublishSubscribe_DataStaleLimit:
			return "Data Stale Limit";
		default:
			return NAString;
		}	
	case CTC_FirmwareUpdate:
		switch (parameterCode) {	
		case CPC_FirmwareUpdate_DeviceID:
			return "Target Device";				
		case CPC_FirmwareUpdate_FileName:
			return "File Name";
		case CPC_FirmwareUpdate_RequestedCommittedBurst:
			return "Committed Burst";
		default:
			return NAString;
		}
	case CTC_GetFirmwareUpdateStatus:
		switch (parameterCode) {	
		case CPC_GetFirmwareUpdateStatus_DeviceID:
			return "Target Device";
		case CPC_GetFirmwareUpdateStatus_RequestedCommittedBurst:
			return "Committed Burst";
		default:
			return NAString;
		}	
	case CTC_CancelFirmwareUpdate:
		switch (parameterCode) {	
		case CPC_CancelFirmwareUpdate_DeviceID:
			return "Target Device";
		case CPC_CancelFirmwareUpdate_RequestedCommittedBurst:
			return "Committed Burst";						
		default:
			return NAString;
		}	
	case CTC_ResetDevice:
		switch (parameterCode) {
		case CPC_ResetDevice_DeviceID:
			return "Target Device";
		case CPC_ResetDevice_RestartType:
			return "Restart Type";
		case CPC_ResetDevice_RequestedCommittedBurst:
			return "Committed Burst";						
		default:
			return NAString;
		}	
	case CTC_FirmwareUpload:
		switch (parameterCode) {
		case CPC_FirmwareUpload_FirmwareID:
			return FALSE; //this parameter cannot be viewed in UI
		case CPC_FirmwareUpload_FileName:
			return "File Name"; 					
		case CPC_FirmwareUpload_RequestedCommittedBurst:
			return "Committed Burst";						
		default:
			return NAString;
		}
	case CTC_DeleteSubscriberContract:
		switch (parameterCode) {
		case CPC_DeleteSubscriberContract_LeaseID:
			return "Lease ID"; 		
		default:
			return NAString;			
		}
	case CTC_ScheduleReport:
		switch (parameterCode) {
		case CPC_ScheduleReport_DeviceID:
			return "Device ID";
		default:
			return NAString;				
		}
	case CTC_DeviceHealthReport:
		switch (parameterCode) {
		case CPC_DeviceHealthReport_DeviceID:
			return "Device ID";
		default:
			return NAString;				
		}
    case CTC_NeighborHealthReport:			
		switch (parameterCode) {
		case CPC_NeighborHealthReport_DeviceID:
			return "Device ID";
		default:
			return NAString;				
		}    
    case CTC_GetContractsAndRoutes:
    	switch (parameterCode) {
    	case CPC_GetContractsAndRoutes_RequestedCommittedBurst:
    		return "Committed Burst";
    	default:
    		return NAString;
    	}	    		
    case CTC_ISACSRequest:			
		switch (parameterCode) {
		case CPC_ISACSRequest_TSAPID:
		  return "TSAP ID";
        case CPC_ISACSRequest_RequestType:
		  return "Request Type";
        case CPC_ISACSRequest_ObjectID:
		  return "Object ID";
        case CPC_ISACSRequest_AttributeMethodID:
		  return "Attribute/Method ID";
        case CPC_ISACSRequest_AttributeIndex1:
		  return "Attribute Index1";
        case CPC_ISACSRequest_AttributeIndex2:
		  return "Attribute Index2";
        case CPC_ISACSRequest_DataBuffer:
		  return "Data Buffer";
        case CPC_ISACSRequest_ReadAsPublish:
		  return "Read as publish";
		case CPC_ISACSRequest_RequestedCommittedBurst:
			return "Committed Burst";		  		  
		default:
		  return NAString;				
		}    
    case CTC_ChannelsStatistics:
        switch (parameterCode){
        case CPC_ChannelsStatistics_DeviceID:
            return "Target Device ID";
		case CPC_ChannelsStatistics_RequestedCommittedBurst:
			return "Committed Burst";						
        default: 
            return NAString;     
        }	
    case CTC_UpdateSensorBoardFirmware:
    	switch (parameterCode) {
    	case CPC_UpdateSensorBoardFirmware_TargetDeviceID:
			return "Target Device ID";
		case CPC_UpdateSensorBoardFirmware_FileName:
			return "File Name";
		case CPC_UpdateSensorBoardFirmware_TSAPID:	
			return "TSAP ID";
		case CPC_UpdateSensorBoardFirmware_ObjID:
			return "ObjID"; 
		case CPC_UpdateSensorBoardFirmware_RequestedCommittedBurst:
			return "Committed Burst";						
		default:
			return NAString;
		}
    case CTC_CancelSensorBoardFirmwareUpdate:
    	switch (parameterCode) {
		case CPC_CancelSensorBoardFirmwareUpdate_TargetDeviceID:
			return "Target Device ID";
		case CPC_CancelSensorBoardFirmwareUpdate_RequestedCommittedBurst:
			return "Committed Burst";									
		default:
			return NAString;
		}
	default:
		return NAString;			
	}
}


