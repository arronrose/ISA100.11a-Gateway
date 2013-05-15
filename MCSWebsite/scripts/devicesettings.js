//controls page refresh
var RefreshDeviceInfoActive = false; 
var deviceId = null;
var LastRefreshedString;
var LastRefreshedDate;

function InitDeviceSettingsPage() {
    SetPageCommonElements();    
    InitJSON();
    deviceId = GetPageParamValue("deviceId"); 
    if (!isNaN(deviceId)) { //make sure qs was not altered (is a number)
        var dev = GetDeviceInformation(deviceId);        
        if (dev != null) {
            var btnRefresh = document.getElementById("btnRefresh");
            if(dev.DeviceStatus >= DS_JoinedAndConfigured) {
                btnRefresh.style.display = "";
            }
            SetData(dev);            
            SetDeviceTabs(deviceId, dev.DeviceStatus, dev.DeviceRoleID, 1);
            SetRefreshLabel(); 
            CheckAutoRefresh();
            BuildNeighborsTable();
            BuildRoutesTable();
            BuildGraphsTable();
            setInterval("DisplaySecondsAgo()", 1000);
        }
    }
}

function SetRefreshLabel(){   
    var LastCommand1 = GetLastCommandResponded(CTC_RequestTopology);
    var LastCommand2 = GetLastCommandResponded(CTC_GetContractsAndRoutes);
    //both are null
    if (LastCommand1 == null && LastCommand2 == null) {			
        RefreshDeviceInfoActive	= false;
        LastRefreshedString = NAString;
        return ;
    };

    var LastResponse;
    //both are not null
    if (LastCommand1 != null && LastCommand2 != null) {
       LastResponse = (LastCommand1.TimeResponded > LastCommand2.TimeResponded ? LastCommand1.TimeResponded : LastCommand2.TimeResponded);
    } 
    //only one is null
    else {
       LastResponse = (LastCommand1 != null ? LastCommand1.TimeResponded : LastCommand2.TimeResponded);
    };

    // set the label
    LastRefreshedDate   = ConvertFromSQLiteDateToJSDateObject(LastResponse);
    LastRefreshedString = LastResponse; 
}

function AutoRefresh() { //to do give a better function name

    var LastCommand1 = GetLastCommand(CTC_RequestTopology);
    var LastCommand2 = GetLastCommand(CTC_GetContractsAndRoutes);
	if ((LastCommand1.CommandStatus == CS_New || LastCommand1.CommandStatus == CS_Sent) &&
	    (LastCommand2.CommandStatus == CS_New || LastCommand2.CommandStatus == CS_Sent)){					
        RefreshDeviceInfoActive = true;					
    } else {
        if (LastCommand1.CommandStatus == CS_Failed && LastCommand2.CommandStatus == CS_Failed) {
            SetRefreshLabel();
        } 
        // at least one has a response
        else {            
            RefreshDeviceInfoActive = false;            
            var LastResponse = (LastCommand1.CommandStatus == CS_Responded ? LastCommand1.TimeResponded : LastCommand2.TimeResponded)
            LastRefreshedDate   = ConvertFromSQLiteDateToJSDateObject(LastResponse);
            LastRefreshedString = LastResponse;
        }    
        BuildNeighborsTable();
        BuildRoutesTable();
        BuildGraphsTable();    
	}
	if (RefreshDeviceInfoActive) {		
		setTimeout(AutoRefresh, RefreshInterval);
	}
}

function RefreshPage() {
    if (AddGetTopologyCommand() != null && AddGetContractsAndRoutes() != null) {
        AutoRefresh();    
    }
}

function SetData(dev) {
    document.getElementById("spnEUI64").innerHTML = dev.Address64;
    document.getElementById("spnIPv6").innerHTML = dev.Address128;
}

function BuildNeighborsTable() {
    //obtain data from database
    var data = GetDeviceNeighbors(deviceId);
    //process template 
    document.getElementById("tblNeighbors").innerHTML = TrimPath.processDOMTemplate("neighbors_jst", data);
}

function BuildRoutesTable() {
    //obtain data from database
    var data = GetDeviceRoutes(deviceId);
    //process template 
    document.getElementById("tblRoutes").innerHTML = TrimPath.processDOMTemplate("routes_jst", data);
}

function BuildGraphsTable() {
    //obtain data from database
    var data = GetDeviceGraphs(deviceId);
    //process template 
    document.getElementById("tblGraphs").innerHTML = TrimPath.processDOMTemplate("graphs_jst", data);
}

function BackButtonClicked() {
    document.location.href = "devicelist.html?setState";
}

function AddGetTopologyCommand() {
	var systemManager = GetSystemManagerDevice();
	if (systemManager == null) {
		return null;
	}
	if (systemManager.DeviceStatus < DS_JoinedAndConfigured) {
	   alert("System Manager not registered !");
	   return null;
    }   
	var cmd = new Command();
	cmd.DeviceID = systemManager.DeviceID;
	cmd.CommandTypeCode = CTC_RequestTopology;
	cmd.CommandStatus = CS_New;
	cmd.TimePosted = ConvertFromJSDateToSQLiteDate(GetUTCDate());
	cmd.ParametersDescription = "";
			
	return AddCommand(cmd, "");
}

function AddGetContractsAndRoutes() {
	var systemManager = GetSystemManagerDevice();
	if (systemManager == null) {
		return null;
	}
	if (systemManager.DeviceStatus < DS_JoinedAndConfigured) {
	   alert("System Manager not registered !");
	   return null;
    }   
	var cmd = new Command();
	cmd.DeviceID = systemManager.DeviceID;
	cmd.CommandTypeCode = CTC_GetContractsAndRoutes;
	cmd.CommandStatus = CS_New;
	cmd.TimePosted = ConvertFromJSDateToSQLiteDate(GetUTCDate());

	var cmdParam = new CommandParameter()
	cmdParam.ParameterCode = CPC_GetContractsAndRoutes_RequestedCommittedBurst;
	cmdParam.ParameterValue = RequestedCommittedBurstDefaultValue;	
	cmd.ParametersDescription = GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam.ParameterCode)) + ": " + cmdParam.ParameterValue; 
			
    var cmdParams = new Array;
    cmdParams[0] = cmdParam;	
	return AddCommand(cmd, cmdParams);
}

function DisplaySecondsAgo() {
    if (RefreshDeviceInfoActive){
        document.getElementById("spnRefreshDate").innerHTML = LastRefreshedString +  (LastRefreshedString != NAString ? "  (" + Math.ceil((GetUTCDate().getTime() - LastRefreshedDate.getTime()) / 1000) + " seconds ago)" : "") + '. Refreshing ...';
    } 
    else {
        document.getElementById("spnRefreshDate").innerHTML = LastRefreshedString +  (LastRefreshedString != NAString ? "  (" + Math.ceil((GetUTCDate().getTime() - LastRefreshedDate.getTime()) / 1000) + " seconds ago)" : ""); 
    };
}

function CheckAutoRefresh() {
	var tmpDate = new Date()
	tmpDate = ConvertFromSQLiteDateToJSDateObject(LastRefreshedString == NAString ? '1970-01-01 00:00:00' : LastRefreshedString);
	if (tmpDate < AddSecondsToDate(GetUTCDate(),-AUTO_REFRESH_DATA_OLDER_THEN)){ 
		RefreshPage();
	}  
}
