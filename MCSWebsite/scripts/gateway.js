var methods = ["config.getGroupVariables",
               "config.setVariable",
               "user.logout",
               "isa100.sm.setDevice",
               "misc.applyConfigChanges"];
                   
function InitGatewayPage() {
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
    	var gwTag = document.getElementById("txtGWTag");
    	var subnetId = document.getElementById("txtSubnetId");
    	var appJoinKey = document.getElementById("txtAppJoinKey");
	    var appLoggingLevel = document.getElementsByName("rbAppLoggingLevel");
        var stackLoggingLevel = document.getElementsByName("rbStackLoggingLevel");
        
        var saveFailed = false;
        
        try {
            var service = new jsonrpc.ServiceProxy(serviceURL, methods);
        
            if (!service.config.setVariable({group : "GATEWAY", varName : "GATEWAY_EUI64", varValue: eui64.value})) {
                alert("Error saving EUI64 !");
                saveFailed = true;
            }
            if (!service.config.setVariable({group : "GATEWAY", varName : "GATEWAY_IPv6", varValue: ipv6.value})) {
                alert("Error saving IPv6 Address !");
                saveFailed = true;
            }
            if (!service.config.setVariable({group : "GATEWAY", varName : "GATEWAY_UDPPort", varValue: udpPort.value})) {
                alert("Error saving UDP Port Number !");
                saveFailed = true;   
            }
            if (!service.config.setVariable({group : "GATEWAY", varName : "GATEWAY_TAG", varValue: gwTag.value})) {
                alert("Error saving GW Tag !");
                saveFailed = true;   
            }
            if (!service.config.setVariable({group : "GATEWAY", varName : "SubnetID", varValue: subnetId.value})) {
                alert("Error saving Subnet ID !");
                saveFailed = true;                   
            }
            if (!service.config.setVariable({group : "GATEWAY", varName : "AppJoinKey", varValue: appJoinKey.value})) {
                alert("Error saving App Join Key !");
                saveFailed = true;                                   
            }
            
            var appLoggingLevelValue;
            for (var i=0; i < appLoggingLevel.length; i++) {
                if (appLoggingLevel[i].checked) {
                    appLoggingLevelValue = i + 1;
                }
            }
            
            var stackLoggingLevelValue;
            for (var i=0; i < stackLoggingLevel.length; i++) {
                if (stackLoggingLevel[i].checked) {
                    stackLoggingLevelValue = i + 1;
                }
            }
            
            if (!service.config.setVariable({group : "GATEWAY", varName : "LOG_LEVEL_APP", varValue: appLoggingLevelValue})) {
                alert("Error saving App Logging level !");
                saveFailed = true;                   
            }
            if (!service.config.setVariable({group : "GATEWAY", varName : "LOG_LEVEL_STACK", varValue: stackLoggingLevelValue})) {
                alert("Error saving Stack Logging level !");
                saveFailed = true;                   
            }
            
/*
            var chkSync = document.getElementById("chkSync");
            if (chkSync.checked) {
                //compose bbDev
                var gwDev = eui64.value + "," + appJoinKey.value + "," + subnetId.value;

                if (!service.isa100.sm.setDevice({deviceType : "GATEWAY", deviceValue : gwDev})) {
                    alert("Error syncronizing with SM provision file !");
                    saveFailed = true;
                }
            }
*/
            
            if (!service.misc.applyConfigChanges ({module : "isa_gw"})) {
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
   	var gwTag = document.getElementById("txtGWTag");
   	var subnetId = document.getElementById("txtSubnetId");
   	var appJoinKey = document.getElementById("txtAppJoinKey");
    var appLoggingLevel = document.getElementsByName("rbAppLoggingLevel");
    var stackLoggingLevel = document.getElementsByName("rbStackLoggingLevel");
    
    var appLoggingLevelValue;
    var stackLoggingLevelValue;
        
    try {
        var service = new jsonrpc.ServiceProxy(serviceURL, methods);
        var result = service.config.getGroupVariables({group : "GATEWAY"});
            
        for (i = 0; i < result.length; i++) {
            if(result[i].GATEWAY_EUI64 != null) {
                eui64.value = result[i].GATEWAY_EUI64;
            }
            if(result[i].GATEWAY_IPv6 != null) {
                ipv6.value = result[i].GATEWAY_IPv6;
            }
            if(result[i].GATEWAY_UDPPort != null) {
                udpPort.value = result[i].GATEWAY_UDPPort;
            }
            if(result[i].GATEWAY_TAG != null) {
                gwTag.value = result[i].GATEWAY_TAG;
            }
            if(result[i].SubnetID != null) {
                subnetId.value = result[i].SubnetID;
            }
            if(result[i].AppJoinKey != null) {
                appJoinKey.value = result[i].AppJoinKey;
            }
            if(result[i].LOG_LEVEL_APP != null) {
                appLoggingLevelValue = result[i].LOG_LEVEL_APP;
            }
            if(result[i].LOG_LEVEL_STACK != null) {
                stackLoggingLevelValue = result[i].LOG_LEVEL_STACK;
            }
        }
        if (appLoggingLevelValue >= 1 && appLoggingLevelValue <= 3) {
            appLoggingLevel[appLoggingLevelValue-1].checked = true;
        }
        if (stackLoggingLevelValue >= 1 && stackLoggingLevelValue <= 3) {
            stackLoggingLevel[stackLoggingLevelValue-1].checked = true;
        }
    } catch(e) {
         HandleException(e, "Unexpected error reading values !");
    }
}


function ValidateInput() {
	var eui64 = document.getElementById("txtEUI64");
	var ipv6 = document.getElementById("txtIPv6Address");	
	var udpPort = document.getElementById("txtUDPPortNumber");
   	var gwTag = document.getElementById("txtGWTag");
	var subnetId = document.getElementById("txtSubnetId");
	var appJoinKey = document.getElementById("txtAppJoinKey");
	var appLoggingLevel = document.getElementsByName("rbAppLoggingLevel");
    var stackLoggingLevel = document.getElementsByName("rbStackLoggingLevel");

    if(!ValidateRequired(eui64, "EUI64")) {
        return false;
    }
    if(!ValidateEUI64(eui64, "EUI64")) {
        return false;
    }
    if(!ValidateRequired(ipv6, "IPv6 Address")) {
        return false;
    }
    if(!ValidateIPv6(ipv6, "IPv6 Address")) {
        return false;
    }
    if(!ValidateRequired(udpPort, "UDP Port Number")) {
        return false;
    }
    if (!ValidatePosUShort(udpPort, "UDP Port Number")) {
        return false;
    }    
    if(!ValidateRequired(gwTag, "GW Tag")) {
        return false;
    }
    if(!ValidateRequired(subnetId, "Subnet ID")) {
        return false;
    }
    if (!ValidatePosUShort(subnetId, "Subnet ID")) {
        return false;
    }
    
    if(!ValidateRequired(appJoinKey, "App Join Key")) {
        return false;
    }
    if(!Validate16BHex(appJoinKey, "App Join Key")) {
        return false;
    }     
    if(!ValidateRequiredRadio(appLoggingLevel, "App Logging level")) {
        return false;
    }
    if(!ValidateRequiredRadio(stackLoggingLevel, "Stack Logging level")) {
        return false;
    }
    return true;
}
