//controls page refresh
var RefreshDeviceInfoActive = false; 
var LastRefreshedString;
var LastRefreshedDate;

// page parameter
var deviceId = null;

//order direction
var OD_ASC = "ASC";
var OD_DESC = "DESC";

var OrderBy = 1;  //order column number
var OrderDirection = OD_ASC;

// paging
var CurrentPage = 1;
var TotalPages = 0;
var PageSize = 10;
var TotalNoOfRows = 0;

function InitNeighborsHealthPage() {
    SetPageCommonElements();
    InitJSON();
    deviceId = GetPageParamValue("deviceId");
    if (!isNaN(deviceId)) {  //make sure qs was not altered (is a number)
        dev = GetDeviceInformation(deviceId);
        if (dev != null) {
            SetData(dev);                        
            SetDeviceTabs(deviceId, dev.DeviceStatus, dev.DeviceRoleID, 3);    
        }
        BuildNeighborsHealthTable();
        SetRefreshLabel(deviceId);
        CheckAutoRefresh();
        setInterval("DisplaySecondsAgo()", 1000);
    }
}

function AutoRefresh() { 
    var LastCommand = GetLastCommand(CTC_NeighborHealthReport, CPC_NeighborHealthReport_DeviceID, deviceId);
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
       BuildNeighborsHealthTable();  
	}
	if (RefreshDeviceInfoActive) {		
		setTimeout(AutoRefresh, RefreshInterval);
	}	
}

function RefreshPage() {
    if (AddNeighborHealthReportCommand(deviceId) != null) {
        AutoRefresh();   
    }
}

function BuildNeighborsHealthTable() {
	PageSize = document.getElementById("ddlRowsPerPage").value;
    TotalNoOfRows = GetNeighborsHealthCount(deviceId);
    if (TotalNoOfRows > 0) {
        var data = GetNeighborsHealth(deviceId, PageSize, CurrentPage, TotalNoOfRows, OrderBy, OrderDirection);
        document.getElementById("tblNeighborsHealth").innerHTML = TrimPath.processDOMTemplate("neighborshealth_jst", data);
        SetOrderSignForCurrentView(OrderBy);
    } else {
    	document.getElementById("spnPageNumber").innerHTML = "";
        document.getElementById("tblNeighborsHealth").innerHTML = //"<span class='labels'>No records !</span>";
                "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"900px\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
     					"<tr><td class=\"tableSubHeader\" style=\"width:200px;\" align=\"center\">Neighbor</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:150px;\" align=\"center\">Link status</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:140px;\" align=\"center\">Transmitted</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:140px;\" align=\"center\">Received</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:150px;\" align=\"center\">Signal Strength (dBm)</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:120px;\" align=\"center\">Signal Quality</td></tr>" +
                        "<tr><td colspan=\"7\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
                "</td></tr></table>";
    }
    SetPager();    
}

function PageNavigate(pageNo) {
    CurrentPage = pageNo;    
    BuildNeighborsHealthTable();
}

function ChangeOrderBy(colNumber) {
    //set the column number we are ordering by
    OrderBy = colNumber;
    
    if (OrderDirection == OD_ASC) {
        OrderDirection = OD_DESC;
    } else {
        OrderDirection = OD_ASC;
    }
    
    CurrentPage = 1;
    BuildNeighborsHealthTable();
}

function RowsPerPageChanged(){
    CurrentPage = 1;
    BuildNeighborsHealthTable();	
}

function SetData(dev) {
    document.getElementById("spnEUI64").innerHTML = dev.Address64;
    document.getElementById("spnIPv6").innerHTML = dev.Address128;
}

function SetOrderSignForCurrentView(orderBy) {
    var sign = OrderDirection == OD_ASC ? "&#9650;" : "&#9660;";
	var spnCurrentOrder = document.getElementById("col" + orderBy);
	spnCurrentOrder.innerHTML = sign;
}

function BackButtonClicked() {
    document.location.href = "devicelist.html?setState";
}

function AddNeighborHealthReportCommand(deviceId) {
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
    cmdParam.ParameterCode = CPC_NeighborHealthReport_DeviceID;
    cmdParam.ParameterValue = deviceId;
    params[0] = cmdParam;
    
	var cmd = new Command();
	cmd.DeviceID = systemManager.DeviceID;
	cmd.CommandTypeCode = CTC_NeighborHealthReport;
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
    var LastCommand = GetLastCommandResponded(CTC_NeighborHealthReport, CPC_NeighborHealthReport_DeviceID, deviceId);
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
