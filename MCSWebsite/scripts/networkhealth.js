var CurrentPage = 1;
var TotalPages = 0;
var PageSize = 10;
var LastRefreshedDate;
var LastRefreshedString;
var RefreshDeviceInfoActive = false;
var TotalNoOfRows = 0;

function InitNetworkHealth() {
	SetPageCommonElements();	
	AddValueToArray(methods, "user.logout");
	AddValueToArray(methods, "config.getGroupVariables");
	InitJSON();
    SetRefreshLabel();
    CheckAutoRefresh()
    setInterval("DisplaySecondsAgo()", 1000);
	PageSize = document.getElementById("ddlRowsPerPage").value;
    BuildNetworkHealthHeaderTable();	
	BuildNetworkHealthTable();	    
}

function AutoRefresh() {
    var LastCommand = GetLastCommand(CTC_NetworkHealthReport,null,null);
	if (LastCommand.CommandStatus == CS_New || LastCommand.CommandStatus == CS_Sent) {					
        RefreshDeviceInfoActive = true;					
    } else {
        if (LastCommand.CommandStatus == CS_Failed) {
            SetRefreshLabel();
        } else {            
            RefreshDeviceInfoActive = false;
            LastRefreshedDate   = ConvertFromSQLiteDateToJSDateObject(LastCommand.TimeResponded);
            LastRefreshedString = LastCommand.TimeResponded;
        }
        PageSize = document.getElementById("ddlRowsPerPage").value;    
        BuildNetworkHealthHeaderTable();
        BuildNetworkHealthTable();         
	}
	if (RefreshDeviceInfoActive) {		
		setTimeout(AutoRefresh, RefreshInterval);
	}
}

function RefreshPage() {    
    if (AddGetNetworkHealthCommand() != null) {
        AutoRefresh();    
    }
}

function DisplaySecondsAgo() {
    if (RefreshDeviceInfoActive){
        document.getElementById("spnLastRefreshed").innerHTML = LastRefreshedString +  (LastRefreshedString != NAString ? "  (" + Math.ceil((GetUTCDate().getTime() - LastRefreshedDate.getTime()) / 1000) + " seconds ago)" : "") + '. Refreshing ...';
    } 
    else {
        document.getElementById("spnLastRefreshed").innerHTML = LastRefreshedString +  (LastRefreshedString != NAString ? "  (" + Math.ceil((GetUTCDate().getTime() - LastRefreshedDate.getTime()) / 1000) + " seconds ago)" : ""); 
    };
}

function BuildNetworkHealthHeaderTable() {
    var net = GetNetworkHealthReportHeader();
	if (net != null) {
        document.getElementById("spnLastRefreshed").innerHTML = LastRefreshedString;
	    document.getElementById("spnNetworkID").innerHTML = net.NetworkID;
	    document.getElementById("spnNetworkType").innerHTML = net.NetworkType;
	    document.getElementById("spnDeviceCount").innerHTML = net.DeviceCount;
	    document.getElementById("spnStartDate").innerHTML = net.StartDate;
	    document.getElementById("spnDPDUsSent").innerHTML = net.DPDUsSent;
	    document.getElementById("spnDPDUsLost").innerHTML = net.DPDUsLost;
	    document.getElementById("spnGPDULatency").innerHTML = net.GPDULatency;
	    document.getElementById("spnGPDUPathReliability").innerHTML = net.GPDUPathReliability;
	    document.getElementById("spnGPDUDataReliability").innerHTML = net.GPDUDataReliability;
	    document.getElementById("spnJoinCount").innerHTML = net.JoinCount;
	    document.getElementById("spnCurrentDate").innerHTML = net.CurrentDate;
    } else {
        document.getElementById("spnLastRefreshed").innerHTML = NAString;
	    document.getElementById("spnNetworkID").innerHTML = NAString;
	    document.getElementById("spnNetworkType").innerHTML = NAString;
	    document.getElementById("spnDeviceCount").innerHTML = NAString;
	    document.getElementById("spnStartDate").innerHTML = NAString;
	    document.getElementById("spnDPDUsSent").innerHTML = NAString;
	    document.getElementById("spnDPDUsLost").innerHTML = NAString;
	    document.getElementById("spnGPDULatency").innerHTML = NAString;
	    document.getElementById("spnGPDUPathReliability").innerHTML = NAString;
	    document.getElementById("spnGPDUDataReliability").innerHTML = NAString;
	    document.getElementById("spnJoinCount").innerHTML = NAString;
	    document.getElementById("spnCurrentDate").innerHTML = NAString;
    }
	
	document.getElementById("spnGPDUStatisticsPeriod").innerHTML = 600;	
	//document.getElementById("spnGPDUMaxLatencyPercent").innerHTML = 30;
	try {
	    var service = new jsonrpc.ServiceProxy(serviceURL, methods);
	    var result = service.config.getGroupVariables({group : "GATEWAY"});
	        
	    for (i = 0; i < result.length; i++) {
	        if(result[i].GPDU_STATISTICS_PERIOD != null) {
	        	document.getElementById("spnGPDUStatisticsPeriod").innerHTML = result[i].GPDU_STATISTICS_PERIOD;
	        }; 
	        /*if(result[i].GPDU_MAX_LATENCY_PERCENT != null) {
	        	document.getElementById("spnGPDUMaxLatencyPercent").innerHTML = result[i].GPDU_MAX_LATENCY_PERCENT;
	        };*/ 
	    };    
    } catch(e) {
        HandleException(e, "Unexpected error reading values !");
    };
}

function BuildNetworkHealthTable() {
    TotalNoOfRows = GetNetworkHealthReportPageCount();
    if (TotalNoOfRows > 0) {
        var data = GetNetworkHealthReportPage(PageSize, CurrentPage, TotalNoOfRows);  
        if (data != null) {
            document.getElementById("tblNetDevices").innerHTML = TrimPath.processDOMTemplate("netdevices_jst", data);
        } else {
            document.getElementById("tblNetDevices").innerHTML = //"<span class='labels'>No records !</span>";
                "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"900px\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
     					"<tr><td class=\"tableSubHeader\" style=\"width:  100px;\" align=\"center\">EUI-64 Address</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:180px;\" align=\"center\">Start Date</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 100px;\" align=\"center\">DPDUs Sent</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 100px;\" align=\"center\">DPDUs Lost</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 100px;\" align=\"center\">GPDU Lantecy (%)</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 120px;\" align=\"center\">GPDU Path Reliability (%)</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 120px;\" align=\"center\">GPDU Data Reliability (%)</td>" +
		                  "<td class=\"tableSubHeader\" style=\"width: 80px;\" align=\"center\">Join Count</td></tr>" +
                        "<tr><td colspan=\"9\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
                "</td></tr></table>";
        }
    } else {
    	document.getElementById("spnPageNumber").innerHTML = "";
        document.getElementById("tblNetDevices").innerHTML = //"<span class='labels'>No records !</span>";
                "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"900px\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
     					"<tr><td class=\"tableSubHeader\" style=\"width:  100px;\" align=\"center\">EUI-64 Address</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:180px;\" align=\"center\">Start Date</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 100px;\" align=\"center\">DPDUs Sent</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 100px;\" align=\"center\">DPDUs Lost</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 100px;\" align=\"center\">GPDU Lantecy (%)</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 120px;\" align=\"center\">GPDU Path Reliability (%)</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 120px;\" align=\"center\">GPDU Data Reliability (%)</td>" +
		                  "<td class=\"tableSubHeader\" style=\"width: 80px;\" align=\"center\">Join Count</td></tr>" +
                        "<tr><td colspan=\"9\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
                "</td></tr></table>";
    }
    SetPager();    
}

function PageNavigate(pageNo) {
	PageSize = document.getElementById("ddlRowsPerPage").value;
    CurrentPage = pageNo;    
    BuildNetworkHealthTable();
}

function AddGetNetworkHealthCommand() {
	var gateway = GetGatewayDevice();

	var cmd = new Command();
	cmd.DeviceID = gateway.DeviceID;
	cmd.CommandTypeCode = CTC_NetworkHealthReport;
	cmd.CommandStatus = CS_New;
	cmd.TimePosted = ConvertFromJSDateToSQLiteDate(GetUTCDate());
	cmd.ParametersDescription = "";
			
	return AddCommand(cmd, "");
}

function SetRefreshLabel(){
    var LastCommand = GetLastCommandResponded(CTC_NetworkHealthReport);
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
