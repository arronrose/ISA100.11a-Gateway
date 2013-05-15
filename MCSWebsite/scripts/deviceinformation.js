var CurrentPage = 1;
var PageSize = 10;
var TotalPages = 0;
var TotalNoOfRows = 0;
var deviceId = null;
var dev = null;
var LastRefreshedString;
var LastRefreshedDate;
var RefreshDeviceInfoActive = false;

function InitDeviceInformationPage() {
	PageSize = 10;
    SetPageCommonElements();    
    InitJSON();    
    deviceId = GetPageParamValue("deviceId"); 

    if (!isNaN(deviceId)) {
        dev = GetDeviceInformation(deviceId);
        if (dev != null) {
            SetData(dev);    
            SetDeviceTabs(deviceId, dev.DeviceStatus, dev.DeviceRoleID, 0);
        };
        BuildHeaderValuesTable(deviceId);
        BuildProcessValuesTable();            
        if (dev.DeviceRoleID != DT_SystemManager && dev.DeviceRoleID != DT_Gateway) {
            SetRefreshLabel(deviceId);    
            CheckAutoRefresh()
            setInterval("DisplaySecondsAgo()", 1000);                	
        } else {
        	document.getElementById("spnRefreshDate").innerHTML = NAString;
        	document.getElementById("btnRefresh").disabled = true;
        }
    }     
}

function AutoRefresh() { 
    var LastCommand = GetLastCommand(CTC_DeviceHealthReport, CPC_DeviceHealthReport_DeviceID, deviceId);
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
       BuildHeaderValuesTable(deviceId)
       BuildProcessValuesTable();  
	}
	if (RefreshDeviceInfoActive) {		
		setTimeout(AutoRefresh, RefreshInterval);
	}	
}

function RefreshPage() {
    if (AddDeviceHealthReportCommand(deviceId) != null) {
        AutoRefresh();                   
    }   
}

function SetData(dev) {
    document.getElementById("spnEUI64").innerHTML = dev.Address64;
    document.getElementById("spnIPv6").innerHTML = dev.Address128;
    document.getElementById("spnSubnetId").innerHTML = dev.SubnetID;
    document.getElementById("spnDeviceRole").innerHTML = dev.DeviceRole;
    document.getElementById("spnDeviceStatus").innerHTML = GetDeviceStatusName(dev.DeviceStatus);
    document.getElementById("spnLastRead").innerHTML = dev.LastRead;
    var spnPowerSupplyStatus = document.getElementById("spnPowerSupplyStatus");
    switch (dev.PowerSupplyStatus) {
    	case 0:
    		spnPowerSupplyStatus.innerHTML = "<img src=\"./styles/images/pssGreen.png\" title=\"Line Powered\">"
    		break;
    	case 1:
    		spnPowerSupplyStatus.innerHTML = "<img src=\"./styles/images/pssBlue.png\" title=\">75%\">"
    		break;
    	case 2:
    		spnPowerSupplyStatus.innerHTML = "<img src=\"./styles/images/pssYellow.png\" title=\"[25%,75%]\">"
    		break;	
    	case 3:
    		spnPowerSupplyStatus.innerHTML = "<img src=\"./styles/images/pssRed.png\" title=\"<25%\">"
    		break;	
    	default:
    		spnPowerSupplyStatus.innerHTML = dev.PowerSupplyStatus; 
    };

    document.getElementById("spnManufacturer").innerHTML = dev.Manufacturer;
    document.getElementById("spnModel").innerHTML = dev.Model;
    document.getElementById("spnRevision").innerHTML = dev.Revision;
    
    document.getElementById("spnDPDUsTransmitted").innerHTML = dev.DPDUsTransmitted;    
    document.getElementById("spnDPDUsReceived").innerHTML = dev.DPDUsReceived;
    document.getElementById("spnDPDUsFailedTransmission").innerHTML = dev.DPDUsFailedTransmission;
    document.getElementById("spnDPDUsFailedReception").innerHTML = dev.DPDUsFailedReception;     
}


function BackButtonClicked() {
    document.location.href = "devicelist.html?setState";
}

function BuildProcessValuesTable() {
	PageSize = document.getElementById("ddlRowsPerPage").value;
    TotalNoOfRows = GetDeviceProcessValuesCount(deviceId);    
    if (TotalNoOfRows > 0) {
        //obtain data from database
        var data = GetDeviceProcessValues(deviceId, PageSize, CurrentPage, TotalNoOfRows); 
        //process template 
        document.getElementById("tblProcessValues").innerHTML = TrimPath.processDOMTemplate("processvalues_jst", data);        
    } else {
    	document.getElementById("spnPageNumber").innerHTML = "";
        document.getElementById("tblProcessValues").innerHTML =
                "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"99%\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
     					"<tr><td class=\"tableSubHeader\" style=\"width:170px;\" align=\"left\">Name</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:100px;\" align=\"center\">M.U.</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:100px;\" align=\"center\">Format</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:100px;\" align=\"center\">TSAP ID</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:100px;\" align=\"center\">Object ID</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 90px;\" align=\"center\">Attribute ID</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 90px;\" align=\"center\">Index1</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 90px;\" align=\"center\">Index2</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 30px;\" align=\"center\">ATD</td>" +
                        "<tr><td colspan=\"9\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
                "</td></tr></table>";
    };
    SetPager();
}

function PageNavigate(pageNo) {
	PageSize = document.getElementById("ddlRowsPerPage").value;
    CurrentPage = pageNo;  
    BuildProcessValuesTable();
}

function AddDeviceHealthReportCommand(deviceId) {
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
    cmdParam.ParameterCode = CPC_DeviceHealthReport_DeviceID;
    cmdParam.ParameterValue = deviceId;
    params[0] = cmdParam;
    
	var cmd = new Command();
	cmd.DeviceID = systemManager.DeviceID;
	cmd.CommandTypeCode = CTC_DeviceHealthReport;
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
    var LastCommand = GetLastCommandResponded(CTC_DeviceHealthReport, CPC_DeviceHealthReport_DeviceID, deviceId);
    if (LastCommand == null) {			
        RefreshDeviceInfoActive	= false;
        LastRefreshedString = NAString;
    } else {						        
        LastRefreshedDate   = ConvertFromSQLiteDateToJSDateObject(LastCommand.TimeResponded);
        LastRefreshedString = LastCommand.TimeResponded;
	}    
}

function BuildHeaderValuesTable(deviceId){
     if (!isNaN(deviceId)) {  //make sure qs was not altered
        dev = GetDeviceInformation(deviceId);   
        if (dev != null) {
        	SetDeviceTabs(deviceId, dev.DeviceStatus, dev.DeviceRoleID, 0);
            SetData(dev);
        }
    }
}

function CheckAutoRefresh() {
	var tmpDate = new Date()
	tmpDate = ConvertFromSQLiteDateToJSDateObject(LastRefreshedString == NAString ? '1970-01-01 00:00:00' : LastRefreshedString);
	if (tmpDate < AddSecondsToDate(GetUTCDate(),-AUTO_REFRESH_DATA_OLDER_THEN)){ 
		RefreshPage();
	}  
}
