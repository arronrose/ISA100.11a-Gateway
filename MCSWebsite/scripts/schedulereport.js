var deviceId = null;
var dev = null;
var CurrentPage = 1;
var TotalPages = 0;
var TotalNoOfRows = 0;
var PageSize = 10;
var LastRefreshedString;
var LastRefreshedDate;
var RefreshDeviceInfoActive = false; 

function InitScheduleReportPage() {
	PageSize = 10;
    SetPageCommonElements();    
    InitJSON();
    deviceId = GetPageParamValue("deviceId");
    if (!isNaN(deviceId)) { //make sure qs was not altered (is a number)
        dev = GetDeviceInformation(deviceId);
        if (dev != null) {
            SetData(dev);    
            SetDeviceTabs(deviceId, dev.DeviceStatus, dev.DeviceRoleID, 4);
        };
        BuildScheduleReportTable();
        BuildRFChannelsTable();
        SetRefreshLabel(deviceId);
        CheckAutoRefresh();
        setInterval("DisplaySecondsAgo()", 1000); 
    }
}

function AutoRefresh() { 
    var LastCommand = GetLastCommand(CTC_ScheduleReport, CPC_ScheduleReport_DeviceID, deviceId);
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
        BuildScheduleReportTable();
	}
	if (RefreshDeviceInfoActive) {		
		setTimeout(AutoRefresh, RefreshInterval);
	}		
}

function RefreshPage() {   
    if (AddDeviceScheduleReportCommand(deviceId) != null) {
        AutoRefresh();   
     }
}

function BuildScheduleReportTable() {
	PageSize = document.getElementById("ddlRowsPerPage").value;
    TotalNoOfRows = GetDeviceScheduleReportCount(deviceId);
    if (TotalNoOfRows > 0) {
        var data = GetDeviceScheduleReportPage(deviceId, PageSize, CurrentPage, TotalNoOfRows);
        document.getElementById("tblScheduleReport").innerHTML = TrimPath.processDOMTemplate("schedulereport_jst", data);    
    } else {
    	document.getElementById("spnPageNumber").innerHTML = "";
        document.getElementById("tblScheduleReport").innerHTML = //"<span class='labels'>No records !</span>";
                "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"800px\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
     					"<tr><td class=\"tableSubHeader\" style=\"width:150px;\" align=\"center\">Superframe ID</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:150px;\" align=\"center\">Time Slots</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:200px;\" align=\"center\">Start Time</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:400px;\" align=\"center\">Links</td></tr>" +
                        "<tr><td colspan=\"4\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
                "</td></tr></table>";
    }
    SetPager();
}

function BuildRFChannelsTable() {
    var data = GetRFChannels();
    var stringRFChannels = "RF Channels: ";
    
    if (data != null) {
        for (var i=0; i<data.length; i++) {
            stringRFChannels = stringRFChannels + "<span style='background:" + GetRFChannelColor(data[i].ChannelStatus) + 
                ";color:#000000;border-style:solid;border-width:1px;'>&nbsp;&nbsp;<font color='#FFFFFF'>" + 
                data[i].ChannelNumber + "</font>&nbsp;&nbsp;</span>&nbsp;"
        }
        document.getElementById("divRFChannels").innerHTML = stringRFChannels;
    } else {
        document.getElementById("divRFChannels").innerHTML = "RF Channels: <span class='labels'>No records !</span>";
    }
}

function GetRFChannelColor(channelStatus) {
    switch (channelStatus) {
        case 0: return "#FF3737";
        case 1: return "#3737FF";
        default: return "#000000";
    }
}

function SetData(dev) {
    document.getElementById("spnEUI64").innerHTML = dev.Address64;
    document.getElementById("spnIPv6").innerHTML = dev.Address128;
}

function PageNavigate(pageNo) {
	PageSize = document.getElementById("ddlRowsPerPage").value;
    CurrentPage = pageNo;
    BuildScheduleReportTable();
}

function BackButtonClicked() {
    document.location.href = "devicelist.html?setState";
}

function AddDeviceScheduleReportCommand(deviceId) {
	var systemManager = GetSystemManagerDevice();
	if (systemManager == null) {
		return null;
	}
	if (systemManager.DeviceStatus < DS_JoinedAndConfigured) {
	   alert("System Manager not registered !");
	   return null;
    }   
    var params = Array(1);
    var cmdParam = new CommandParameter();
    cmdParam.ParameterCode = CPC_ScheduleReport_DeviceID;
    cmdParam.ParameterValue = deviceId;
    params[0] = cmdParam;
    
	var cmd = new Command();
	cmd.DeviceID = systemManager.DeviceID;
	cmd.CommandTypeCode = CTC_ScheduleReport;
	cmd.CommandStatus = CS_New;
	cmd.TimePosted = ConvertFromJSDateToSQLiteDate(GetUTCDate());
    cmd.ParametersDescription = GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam.ParameterCode)) + ": " + dev.Address64;			

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
    var LastCommand = GetLastCommandResponded(CTC_ScheduleReport, CPC_ScheduleReport_DeviceID, deviceId);
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
