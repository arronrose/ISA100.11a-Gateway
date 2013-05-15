var methods = ["config.getGroupVariables",
               "config.setVariable",
               "file.create",
               "isa100.sm.getLogLevel",
               "isa100.sm.setLogLevel",
               "misc.applyConfigChanges",
               "user.logout"];

function InitSystemManagerPage() {
    SetPageCommonElements();
    InitJSON();
    GetData();
}
             
               
function SaveData() {
    ClearOperationResult("spnOperationResult");
    
    if(ValidateInput()) {
        var eui64 = document.getElementById("txtEUI64");
	    var ipv6 = document.getElementById("txtIPv6Address");	
    	var udpPort = document.getElementById("txtUDPPortNumber");
    	
        var maxDeviceNumber = document.getElementById("txtMaxDeviceNumber");
    	var maxDesiredLatency = document.getElementById("txtMaxDesiredLatency");
	    var deviceTimeOutInterval = document.getElementById("txtDeviceTimeOutInterval");
	    var joinAdvPeriod = document.getElementById("txtJoinAdvPeriod");
	    var joinLinksPeriod = document.getElementById("txtJoinLinksPeriod");
        
        var loggingLevel = document.getElementById("txtLoggingLevel");
        
        var saveFailed = false;
        
        try {
            var service = new jsonrpc.ServiceProxy(serviceURL, methods);
        
            if (!service.config.setVariable({configFile : FILE_CONFIG_INI, group : "SYSTEM_MANAGER", varName : "SECURITY_MANAGER_EUI64", varValue: eui64.value})) {
                alert("Error saving EUI64 !");
                saveFailed = true;
            }
            if (!service.config.setVariable({configFile : FILE_CONFIG_INI, group : "SYSTEM_MANAGER", varName : "SYSTEM_MANAGER_IPv6", varValue: ipv6.value})) {
                alert("Error saving IPv6 Address !");
                saveFailed = true;
            }
            if (!service.config.setVariable({configFile : FILE_CONFIG_INI, group : "SYSTEM_MANAGER", varName : "SYSTEM_MANAGER_Port", varValue: udpPort.value})) {
                alert("Error saving UDP Port Number !");
                saveFailed = true;
            }
            if (!service.config.setVariable({configFile : FILE_CONFIG_INI, group : "SYSTEM_MANAGER", varName : "NETWORK_MAX_NODES", varValue: maxDeviceNumber.value})) {
                alert("Error saving Max device number (NSD) !");
                saveFailed = true;
            }
            if (!service.config.setVariable({configFile : FILE_CONFIG_INI, group : "SYSTEM_MANAGER", varName : "NETWORK_MAX_LATENCY", varValue: maxDesiredLatency.value})) {
                alert("Error saving Max desired latency (%) !");
                saveFailed = true;                
            }
            if (!service.config.setVariable({configFile : FILE_CONFIG_INI, group : "SYSTEM_MANAGER", varName : "NETWORK_MAX_DEVICE_TIMEOUT", varValue: deviceTimeOutInterval.value})) {
                alert("Error saving Device timeout interval (s) !");
                saveFailed = true; 
            }

            // create sm_subnet.ini file if not exists
            service.file.create({file : FILE_SM_SUBNET_INI});
            if (!service.config.setVariable({configFile : FILE_SM_SUBNET_INI, group : "SYSTEM_MANAGER", varName : "JOIN_ADV_PERIOD", varValue: joinAdvPeriod.value})) {
                alert("Error saving Join advertise period (s) !");
                saveFailed = true; 
            }
            if (!service.config.setVariable({configFile : FILE_SM_SUBNET_INI, group : "SYSTEM_MANAGER", varName : "JOIN_LINKS_PERIOD", varValue: joinLinksPeriod.value})) {
                alert("Error saving Join links period (s) !");
                saveFailed = true; 
            }
            if (!service.config.setVariable({configFile : FILE_SM_SUBNET_INI, group : "SYSTEM_MANAGER", varName : "CHANNEL_LIST", varValue: GetChannelsList()})) {
                alert("Error saving Channel list !");
                saveFailed = true; 
            }
            if (!service.isa100.sm.setLogLevel({logLevel : loggingLevel.value})) {
                alert("Error saving Logging level !");
                saveFailed = true;
            }

            if (!service.misc.applyConfigChanges({module : "SystemManager"})) {
                alert("Error applying config changes !");
                saveFailed = true;
            }
        } catch(e) {
            HandleException(e, "Unexpected error saving values !");
            return;
        }
        if (!saveFailed) {
            DisplayOperationResult("spnOperationResult", "Save completed successfully.");
        }
    }
}


function GetData() {
    ClearOperationResult("spnOperationResult");
    
    var eui64 = document.getElementById("txtEUI64");
    var ipv6 = document.getElementById("txtIPv6Address");	
   	var udpPort = document.getElementById("txtUDPPortNumber");
    	
    var maxDeviceNumber = document.getElementById("txtMaxDeviceNumber");
  	var maxDesiredLatency = document.getElementById("txtMaxDesiredLatency");
    var deviceTimeOutInterval = document.getElementById("txtDeviceTimeOutInterval");
    var joinAdvPeriod = document.getElementById("txtJoinAdvPeriod");
    var joinLinksPeriod = document.getElementById("txtJoinLinksPeriod");
        
    var loggingLevel = document.getElementById("txtLoggingLevel");
    
    try  {
        var service = new jsonrpc.ServiceProxy(serviceURL, methods);
        var result1 = service.config.getGroupVariables({configFile : FILE_CONFIG_INI, group : "SYSTEM_MANAGER"});
            
        for (var i = 0; i < result1.length; i++) {
            if(result1[i].SECURITY_MANAGER_EUI64 != null) {
                eui64.value = result1[i].SECURITY_MANAGER_EUI64;
            }
            if(result1[i].SYSTEM_MANAGER_IPv6 != null) {
                ipv6.value = result1[i].SYSTEM_MANAGER_IPv6;
            } 
            if(result1[i].SYSTEM_MANAGER_Port != null) {
                udpPort.value = result1[i].SYSTEM_MANAGER_Port;
            } 
            if(result1[i].NETWORK_MAX_NODES != null) {
                maxDeviceNumber.value = result1[i].NETWORK_MAX_NODES;
            } 
            if(result1[i].NETWORK_MAX_LATENCY != null) {
                maxDesiredLatency.value = result1[i].NETWORK_MAX_LATENCY;
            } 
            if(result1[i].NETWORK_MAX_DEVICE_TIMEOUT != null) {
                deviceTimeOutInterval.value = result1[i].NETWORK_MAX_DEVICE_TIMEOUT;
            } 
        }
    } catch(e) {
        HandleException(e, "Unexpected error reading values !");
    }

    try {     
        var result2 = service.config.getGroupVariables({configFile : FILE_SM_SUBNET_INI, group : "SYSTEM_MANAGER"});
		joinAdvPeriod.value = "";
		joinLinksPeriod.value = "";
		SetChannelsList([]);
		if (result2 != null) {
	        for (var i = 0; i < result2.length; i++) {
	            if(result2[i].JOIN_ADV_PERIOD != null) {
	                joinAdvPeriod.value = result2[i].JOIN_ADV_PERIOD;
	            } 
	            if(result2[i].JOIN_LINKS_PERIOD != null) {
	                joinLinksPeriod.value = result2[i].JOIN_LINKS_PERIOD;
	            } 
	            if(result2[i].CHANNEL_LIST != null && result2[i].CHANNEL_LIST != "") {
	            	SetChannelsList(result2[i].CHANNEL_LIST.split(","));
	            }
	        }
		}
    } catch(e) {
        HandleException(e, "Unexpected error reading values !");
    }

    try {     
        loggingLevel.value = service.isa100.sm.getLogLevel();
    } catch(e) {
        HandleException(e, "Unexpected error reading values !");
    }
}


function ValidateInput() {
	var eui64 = document.getElementById("txtEUI64");
	var ipv6 = document.getElementById("txtIPv6Address");	
	var udpPort = document.getElementById("txtUDPPortNumber");
	
	var maxDeviceNumber = document.getElementById("txtMaxDeviceNumber");
	var maxDesiredLatency = document.getElementById("txtMaxDesiredLatency");
	var deviceTimeOutInterval = document.getElementById("txtDeviceTimeOutInterval");
    var joinAdvPeriod = document.getElementById("txtJoinAdvPeriod");
    var joinLinksPeriod = document.getElementById("txtJoinLinksPeriod");

	var loggingLevel = document.getElementById("txtLoggingLevel");
 
    if(!ValidateRequired(eui64, "EUI64")) {
        return false;
    }
    if(!ValidateEUI64(eui64, "EUI64")) {
        return false;
    }
    if(!ValidateRequired(ipv6, "IPv6 Address")) {
        return false;
    }
    if(!ValidateIPv6(ipv6, "IPv6 Address"))  {
        return false;
    }
    if(!ValidateRequired(udpPort, "UDP Port number")) {
        return false;
    }
    if (!ValidatePosUShort(udpPort, "UDP Port Number")) {
        return false;
    }    
    if(!ValidateRequired(maxDeviceNumber, "Max device number (NSD)")) {
        return false;
    }
    if(!ValidateNumber(maxDeviceNumber, "Max device number (NSD)", 0, 65535)) {
        return false;
    }
    if(!ValidateRequired(maxDesiredLatency, "Max desired latency (%)")) {
        return false;
    }
    if(!ValidateNumber(maxDesiredLatency, "Max desired latency (%)", 0, 99)) {
        return false;
    }
    if(!ValidateRequired(deviceTimeOutInterval, "Device timeout interval (s)")) {
        return false;
    }
    if(!ValidateNumber(deviceTimeOutInterval, "Device timeout interval (s)", 60, 65535)) {
        return false;
    }
    if(!ValidateRequired(joinAdvPeriod, "Advertise period (s)")) {
        return false;
    }
    if(!ValidateNumberSet(joinAdvPeriod, "Advertise period (s)", "1,4,7,8,9,11,13,14")) {
        return false;
    }
    if(!ValidateRequired(joinLinksPeriod, "Join links period (s)")) {
        return false;
    }
    if(!ValidateNumber(joinLinksPeriod, "Join links period (s)", 1, 8)) {
        return false;
    }
    if(!ValidateRequired(loggingLevel, "Logging level")) {
        return false;
    }
    if(!ValidateLoggingLevel(loggingLevel, "Logging level")) {
        return false;
    }
    return true;
}

function GetChannelsList() {
    var channelsList = "";
    for (var i=0; i<15; i++) {
        if (document.getElementById("chkChannel" + i).checked) {
            channelsList = channelsList + i + ",";
        }
    }
    if (channelsList != "") {
        channelsList = channelsList.substring(0, channelsList.length - 1);
    }
    return channelsList;
}

function SetChannelsList(varChannelsList) {
    for (var i=0; i<15 ; i++){
        document.getElementById("chkChannel" + i).checked = false;
    }
    for (var j=0; j<varChannelsList.length; j++) {
       document.getElementById("chkChannel" + varChannelsList[j]).checked = true;
    }
}
