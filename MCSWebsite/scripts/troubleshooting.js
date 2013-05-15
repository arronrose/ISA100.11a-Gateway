var ALERT_TYPES = [{ObjectID:null, Class:null, Category:null, Type:null, Name:null}];
var DEVICES = null;
var SHOW = 1; // EUI64
var DISPLAY_LAST = 50; 
var AUTO_REFRESH_INTERVAL = 15;

function InitTrobleshootingPage() {
    SetPageCommonElements();
    InitJSON();
    ResetFilters();
    //PopulateFilters();   	
    //ReadFilters();  
}

function ResetFilters(){
	SHOW = 1; // EUI64
	DISPLAY_LAST = 50; 
	AUTO_REFRESH_INTERVAL = 15;
	document.getElementById("ddlShow").selectedIndex = 0;
	document.getElementById("ddlDisplayLast").selectedIndex = 0;
	document.getElementById("ddlAutoRefreshInterval").selectedIndex = 2;
}

function PopulateAlertTypes(){
	ALERT_TYPES[0]= {ObjectID:5, Class:ACL_Event, Category:AC_Process, Type:0, Name:"Device Join"}
	ALERT_TYPES[1]= {ObjectID:5, Class:ACL_Event, Category:AC_Process, Type:1, Name:"Device Join Failed"};
	ALERT_TYPES[2]= {ObjectID:5, Class:ACL_Event, Category:AC_Process, Type:2, Name:"Device Leave"};
	
	ALERT_TYPES[3]= {ObjectID:7, Class:ACL_Event, Category:AC_Process, Type:0, Name:"Transfer Started"};
	ALERT_TYPES[4]= {ObjectID:7, Class:ACL_Event, Category:AC_Process, Type:1, Name:"Transfer Progress"};
	ALERT_TYPES[5]= {ObjectID:7, Class:ACL_Event, Category:AC_Process, Type:2, Name:"Transfer End"};
	
	ALERT_TYPES[6]= {ObjectID:3, Class:ACL_Event, Category:AC_Process, Type:0, Name:"Contract Establish"};
	ALERT_TYPES[7]= {ObjectID:3, Class:ACL_Event, Category:AC_Process, Type:1, Name:"Contract Modify"};
	ALERT_TYPES[8]= {ObjectID:3, Class:ACL_Event, Category:AC_Process, Type:2, Name:"Contract Refusal"};
	ALERT_TYPES[9]= {ObjectID:3, Class:ACL_Event, Category:AC_Process, Type:3, Name:"Contract Terminate"};
	
	ALERT_TYPES[10]={ObjectID:5, Class:ACL_Event, Category:AC_Process, Type:4, Name:"Parent Change"};
	ALERT_TYPES[11]={ObjectID:5, Class:ACL_Event, Category:AC_Process, Type:5, Name:"Backup Change"};	
}

function CheckUncheckAllAlertTypes(checked){
	if (checked) {
		for (var i=0; i<ALERT_TYPES.length; i++){
			document.getElementById("chkAlertType"+i).checked = "checked";
		}	
		document.getElementById("chkAppAlertTypeDevice").checked = "checked";
		document.getElementById("chkAppAlertTypeUDOProgress").checked = "checked";
		document.getElementById("chkAppAlertTypeContract").checked = "checked";
		document.getElementById("chkAppAlertTypeTopology").checked = "checked";
	} else {
		for (var i=0; i<ALERT_TYPES.length; i++){
			document.getElementById("chkAlertType"+i).checked = "";
		}
		document.getElementById("chkAppAlertTypeDevice").checked = "";
		document.getElementById("chkAppAlertTypeUDOProgress").checked = "";
		document.getElementById("chkAppAlertTypeContract").checked = "";
		document.getElementById("chkAppAlertTypeTopology").checked = "";
	}
} 

function CheckUncheckAllDevices(checked){
	var NoOfDevices = 0;
	if (DEVICES == null){
		NoOfDevices = 0;
	} else {
		NoOfDevices = DEVICES.length; 
	};	

	if (checked) { 
		for (var i=0; i<NoOfDevices; i++){
			document.getElementById("chkDevice"+DEVICES[i].DeviceID).checked = "checked";
		}	
	} else {
		for (var i=0; i<NoOfDevices; i++){
			document.getElementById("chkDevice"+DEVICES[i].DeviceID).checked = "";
		}
	}	
}

function CheckUncheckDevice(deviceId, checked){
	if (!checked){
		document.getElementById("chkAllDevices").checked = "";		
	} else {
		for(var i=0; i<DEVICES.length; i++){
			if 	(document.getElementById("chkDevice"+DEVICES[i].DeviceID).checked == false){
				document.getElementById("chkAllDevices").checked = "";
				return;
			}		
		};
		document.getElementById("chkAllDevices").checked = "checked";
	}
}

function CheckUncheckAlertType(checked, category, id){
	if (!checked){
		document.getElementById("chkAllAlerts").checked = "";	
		switch (category){
		case 1: for(var i=0; i<=2; i++){document.getElementById("chkAlertType"+i).checked = ""};
				break
		case 2: for(var i=3; i<=5; i++){document.getElementById("chkAlertType"+i).checked = ""}; 				
				break;
		case 3: for(var i=6; i<=9; i++){document.getElementById("chkAlertType"+i).checked = ""}; 				
				break;
		case 4: for(var i=10; i<=11; i++){document.getElementById("chkAlertType"+i).checked = ""}; 				
				break;
		default:
		}
		
		if (id >= 0 && id <= 2){
			for(var i=0; i<=2; i++){
				if (document.getElementById("chkAlertType"+i).checked == false){
					document.getElementById("chkAppAlertTypeDevice").checked = "";
					return;
				};		
			};
		};
		if (id >= 3 && id <= 5){
			for(var i=3; i<=5; i++){
				if (document.getElementById("chkAlertType"+i).checked == false){
					document.getElementById("chkAppAlertTypeUDOProgress").checked = "";
					return;
				};		
			};
		};		
		if (id >= 6 && id <= 9){
			for(var i=6; i<=9; i++){
				if (document.getElementById("chkAlertType"+i).checked == false){
					document.getElementById("chkAppAlertTypeContract").checked = "";
					return;
				};		
			};
		};		
		if (id >= 10 && id <= 11){
			for(var i=10; i<=11; i++){
				if (document.getElementById("chkAlertType"+i).checked == false){
					document.getElementById("chkAppAlertTypeTopology").checked = "";
					return;
				};		
			};
		};
	} else {
		switch (category){
			case 1: for(var i=0; i<=2; i++){document.getElementById("chkAlertType"+i).checked = "checked"}; 					
					break;
			case 2: for(var i=3; i<=5; i++){document.getElementById("chkAlertType"+i).checked = "checked"};
					break;
			case 3: for(var i=6; i<=9; i++){document.getElementById("chkAlertType"+i).checked = "checked"};					
					break;			
			case 4: for(var i=10; i<=11; i++){document.getElementById("chkAlertType"+i).checked = "checked"}; 					
					break;						
			default:
		}		
		if (id >= 0 && id <= 2){
			for(var i=0; i<=2; i++){
				if (document.getElementById("chkAlertType"+i).checked == false){
					document.getElementById("chkAppAlertTypeDevice").checked = "";
					return;
				};						
			};
			document.getElementById("chkAppAlertTypeDevice").checked = "checked";
		};
		if (id >= 3 && id <= 5){
			for(var i=3; i<=5; i++){
				if (document.getElementById("chkAlertType"+i).checked == false){
					document.getElementById("chkAppAlertTypeUDOProgress").checked = "";
					return;
				};		
			};
			document.getElementById("chkAppAlertTypeUDOProgress").checked = "checked";
		};		
		if (id >= 6 && id <= 9){
			for(var i=6; i<=9; i++){
				if (document.getElementById("chkAlertType"+i).checked == false){
					document.getElementById("chkAppAlertTypeContract").checked = "";
					return;
				};		
			};
			document.getElementById("chkAppAlertTypeContract").checked = "checked";
		};		
		if (id >= 10 && id <= 11){
			for(var i=10; i<=11; i++){
				if (document.getElementById("chkAlertType"+i).checked == false){
					document.getElementById("chkAppAlertTypeTopology").checked = "";
					return;
				};		
			};
			document.getElementById("chkAppAlertTypeTopology").checked = "checked";
		};
		for(var i=0; i<ALERT_TYPES.length; i++){
			if 	(document.getElementById("chkAlertType"+i).checked == false){
				document.getElementById("chkAllAlerts").checked = "";
				return;
			}		
		};
		document.getElementById("chkAllAlerts").checked = "checked";		
	}		
}

function BuildDevicesTable() {
	DEVICES = GetDevicesForTroubleshootingPage();
    if (DEVICES != null){
        document.getElementById("tblDevices").innerHTML = TrimPath.processDOMTemplate("devices_jst", DEVICES);        
    } else {
        document.getElementById("tblDevices").innerHTML = 
                "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"700px\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
     					"<tr>" +						 
     						"<td class=\"tableSubHeader\" style=\"width: 40px;\" align=\"center\">&nbsp;</td>"+						
     						"<td class=\"tableSubHeader\" style=\"width: 180px;\" align=\"left\">EUI-64</td>"+
     						"<td class=\"tableSubHeader\" style=\"width: 220px;\" align=\"left\">IPv6 Address</td>"+
     						"<td class=\"tableSubHeader\" style=\"width: 200px;\" align=\"left\">Device Tag</td>"+
                        "<tr>" +
                        "<td colspan=\"4\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
                "</td></tr></table>";
    };    
}

function ApplyEditFilters(){
	if (document.getElementById("btnApplyEditFilters").value == "Edit Filters") {
		document.getElementById("btnApplyEditFilters").value = "Apply Filters";
		document.getElementById("tblFilters").style.display = "block";
		document.getElementById("tblFilterSummary").style.display = "none";
	    BuildDevicesTable();
	    PopulateAlertTypes();
	} else {
		document.getElementById("btnApplyEditFilters").value = "Edit Filters";		
		document.getElementById("tblFilters").style.display = "none";
		document.getElementById("tblFilterSummary").style.display = "block"; 	
		GetFilters();
	};
}

function ClearFilters(){
	CheckUncheckAllAlertTypes(true);	
	document.getElementById("chkAllAlerts").checked = "checked";
	CheckUncheckAllDevices(true);
	document.getElementById("chkAllDevices").checked = "checked";
}

function GetFilters(){
	document.getElementById("spnAlerts").innerHTML = "<strong>Alerts:</strong>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
	document.getElementById("spnDevices").innerHTML = "<strong>Devices:</strong>&nbsp;&nbsp;"
	
	if (document.getElementById("chkAllAlerts").checked == true){
		document.getElementById("spnAlerts").innerHTML = document.getElementById("spnAlerts").innerHTML + "All";
	} else {
		var alertsFilter = "";
		for(var i=0; i<ALERT_TYPES.length; i++){
			if 	(document.getElementById("chkAlertType"+i).checked == true){
				alertsFilter = alertsFilter + ALERT_TYPES[i] + ", "; 
			}		
		};
		if (alertsFilter != ""){
			alertsFilter = alertsFilter.substring(0, alertsFilter.length-2);
			document.getElementById("spnAlerts").innerHTML = document.getElementById("spnAlerts").innerHTML + alertsFilter;
		};			
	}
	
	if (document.getElementById("chkAllDevices").checked == true){
		document.getElementById("spnDevices").innerHTML = document.getElementById("spnDevices").innerHTML + "All";
	} else {
		var devicesFilter = "";
		for(var i=0; i<DEVICES.length; i++){
			if 	(document.getElementById("chkDevice"+DEVICES[i].DeviceID).checked == true){
				devicesFilter = devicesFilter + DEVICES[i].Address64 + ", "; 
			}		
		};
		if (devicesFilter != ""){
			devicesFilter = devicesFilter.substring(0, devicesFilter.length-2);
			document.getElementById("spnDevices").innerHTML = document.getElementById("spnDevices").innerHTML + devicesFilter;
		};		
	}
}

function AutoRefresh(){
	
}

function ChangeShow(){
	SHOW = document.getElementById("ddlShow").value;	
}

function ChangeDisplayLast(){
	DISPLAY_LAST = document.getElementById("ddlDisplayLast").value;	
}

function ChangeAutoRefreshInterval(){
	AUTO_REFRESH_INTERVAL = document.getElementById("ddlAutoRefreshInterval").value;	
}
