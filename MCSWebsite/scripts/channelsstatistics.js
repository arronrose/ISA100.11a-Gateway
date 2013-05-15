//controls page refresh
var RefreshDeviceInfoActive = false; 
var LastRefreshedString;
var LastRefreshedDate;

// page parameter
var deviceId = null;
// chart view
var chart = null;

function InitChannelsStatisticsPage() {
    SetPageCommonElements();
    InitJSON();
    deviceId = GetPageParamValue("deviceId");
    if (!isNaN(deviceId)) {  //make sure qs was not altered (is a number)
        dev = GetDeviceInformation(deviceId);
        if (dev != null) {
            SetData(dev);            
            SetDeviceTabs(deviceId, dev.DeviceStatus, dev.DeviceRoleID, 5);    
        }
        BuildChannelsStatisticsTable();
        SetRefreshLabel(deviceId);
        CheckAutoRefresh();
        setInterval("DisplaySecondsAgo()", 1000);
    } 
}

function AutoRefresh() {     
    var LastCommand = GetLastCommand(CTC_ChannelsStatistics, CPC_ChannelsStatistics_DeviceID, deviceId);
	if (LastCommand.CommandStatus == CS_New || LastCommand.CommandStatus == CS_Sent) {					
        RefreshDeviceInfoActive = true;					
    } else {
        if (LastCommand.CommandStatus == CS_Failed) {
            SetRefreshLabel(deviceId);
        } else {            
            RefreshDeviceInfoActive = false;
            LastRefreshedDate   = ConvertFromSQLiteDateToJSDateObject(LastCommand.TimeResponded);
            LastRefreshedString = LastCommand.TimeResponded;
        }    
        BuildChannelsStatisticsTable();
	}
	if (RefreshDeviceInfoActive) {		
		setTimeout(AutoRefresh, RefreshInterval);
	}			
}

function RefreshPage() {
    if (AddChannelsStatisticsCommand(deviceId) != null) {
        AutoRefresh();   
    }
}

function BuildChannelsStatisticsTable() {
    var data = GetChannelsStatistics(deviceId);
    if (data != null) {
    	//alert(data.statistics[1].ChannelNo+", "+data.statistics[1].Value);
    	document.getElementById("tblChannelsStatistics").innerHTML = TrimPath.processDOMTemplate("channelsstatistics_jst", data);    	
    } else {
    	document.getElementById("tblChannelsStatistics").innerHTML = 
            "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"99%\"><tr><td align=\"left\">" +
				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
 					  "<tr>" +
					  "<td class=\"tableHeader\" style=\"width: 100px;\" align=\"center\">Channel No</td>" +
					  "<td class=\"tableHeader\" style=\"width: 300px;\" align=\"center\">Value</td>"+
					  "</tr>" +
                    "<tr><td colspan=\"2\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
            "</td></tr></table>";
    }
}

function SetData(dev) {
    document.getElementById("spnEUI64").innerHTML = dev.Address64;
    document.getElementById("spnDeviceRole").innerHTML = dev.DeviceRole;
}

function BackButtonClicked() {
    document.location.href = "devicelist.html?setState";
}

function AddChannelsStatisticsCommand(deviceId) {
	var systemManager = GetSystemManagerDevice();
	if (systemManager == null) {
		return null;
	}
	if (systemManager.DeviceStatus < DS_JoinedAndConfigured) {
	   alert("System Manager not registered !");
	   return null;
    }   
    var params = Array(2);
    var cmdParam = new CommandParameter();
    var cmdParam1 = new CommandParameter();
    cmdParam.ParameterCode = CPC_ChannelsStatistics_DeviceID;
    cmdParam.ParameterValue = deviceId;
    cmdParam1.ParameterCode = CPC_ChannelsStatistics_RequestedCommittedBurst;
    cmdParam1.ParameterValue = RequestedCommittedBurstDefaultValue;
    
    params[0] = cmdParam;
    params[1] = cmdParam1;
   	var cmd = new Command();
	cmd.DeviceID = systemManager.DeviceID;
	cmd.CommandTypeCode = CTC_ChannelsStatistics;
	cmd.CommandStatus = CS_New;
	cmd.TimePosted = ConvertFromJSDateToSQLiteDate(GetUTCDate());
    cmd.ParametersDescription = GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam.ParameterCode)) + ": " + dev.Address64 + ", " +
    							GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam1.ParameterCode)) + ": " + cmdParam1.ParameterValue;

	return AddCommand(cmd, params);
}

function DisplaySecondsAgo() {
    if (RefreshDeviceInfoActive){
        document.getElementById("spnRefreshDate").innerHTML = LastRefreshedString +  (LastRefreshedString != NAString ? "  (" + Math.ceil((GetUTCDate().getTime() - LastRefreshedDate.getTime()) / 1000) + " seconds ago)" : "") + '. Refreshing ...';
    } 
    else {
        document.getElementById("spnRefreshDate").innerHTML = LastRefreshedString +  (LastRefreshedString != NAString ? "  (" + Math.ceil((GetUTCDate().getTime() - LastRefreshedDate.getTime()) / 1000) + " seconds ago)" : ""); 
    };
}

function SetRefreshLabel(deviceId){   
    var LastCommand = GetLastCommandResponded(CTC_ChannelsStatistics, CPC_ChannelsStatistics_DeviceID, deviceId);
    if (LastCommand == null) {	
        RefreshDeviceInfoActive	= false;			
        LastRefreshedString = NAString;
    } else {						        
        LastRefreshedDate   = ConvertFromSQLiteDateToJSDateObject(LastCommand.TimeResponded);
        LastRefreshedString = LastCommand.TimeResponded;
	}    
}

function CheckAutoRefresh() {
	var tmpDate = new Date()
	tmpDate = ConvertFromSQLiteDateToJSDateObject(LastRefreshedString == NAString ? '1970-01-01 00:00:00' : LastRefreshedString);
	 if (tmpDate < AddSecondsToDate(GetUTCDate(),-AUTO_REFRESH_DATA_OLDER_THEN)){ 
		RefreshPage();
	}  
}
