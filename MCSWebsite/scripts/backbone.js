var methods = ["config.getGroupVariables",
               "config.setVariable",
               "user.logout",
               "isa100.sm.setDevice",
               "misc.applyConfigChanges"];


function InitBackbonePage() {
    SetPageCommonElements();
    InitJSON();
    GetData();
}


function GetData() {
    ClearOperationResult("spnOperationResult");

    var eui64 = document.getElementById("txtEUI64");
    var ipv6 = document.getElementById("txtIPv6Address");	
    var udpPort = document.getElementById("txtUDPPortNumber");
    var bbrTag = document.getElementById("txtBBRTag");
    var subnetId = document.getElementById("txtSubnetId");
    var subnetMask = document.getElementById("txtSubnetMask");
    var appJoinKey = document.getElementById("txtAppJoinKey");
    var serialName = document.getElementById("txtSerialName");
    var stackLoggingLevel = document.getElementsByName("rbStackLoggingLevel");

    var stackLoggingLevelValue;

    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       var result = service.config.getGroupVariables({group : "BACKBONE"});

       for (i = 0; i < result.length; i++) {
          if(result[i].BACKBONE_EUI64 != null) {
              eui64.value = result[i].BACKBONE_EUI64;
          }
          if(result[i].BACKBONE_IPv6 != null) {
              ipv6.value = result[i].BACKBONE_IPv6;
          }
          if(result[i].BACKBONE_Port != null) {
              udpPort.value = result[i].BACKBONE_Port;
          }
          if(result[i].BACKBONE_TAG != null) {
              bbrTag.value = result[i].BACKBONE_TAG;
          }
          if(result[i].FilterTargetID != null) {
              subnetId.value = result[i].FilterTargetID;
          }
          if(result[i].FilterBitMask != null) {
              subnetMask.value = result[i].FilterBitMask;
          }
          if(result[i].AppJoinKey != null) {
              appJoinKey.value = result[i].AppJoinKey;
          }
          if(result[i].TTY_DEV != null) {
              serialName.value = result[i].TTY_DEV;
          }
          if(result[i].LOG_LEVEL_STACK != null) {
              stackLoggingLevelValue = result[i].LOG_LEVEL_STACK;
          }
       }                
       if (stackLoggingLevelValue >= 1 && stackLoggingLevelValue <= 3) {
           stackLoggingLevel[stackLoggingLevelValue-1].checked = true;
       }
    } catch(e) {
        HandleException(e, "Unexpected error reading values !");
    }
}


function SaveData() {
    ClearOperationResult("spnOperationResult");
    
    if(ValidateInput()) {
        var eui64 = document.getElementById("txtEUI64");
        var ipv6 = document.getElementById("txtIPv6Address");	
        var udpPort = document.getElementById("txtUDPPortNumber");
        var bbrTag = document.getElementById("txtBBRTag");
        var subnetId = document.getElementById("txtSubnetId");
        var subnetMask = document.getElementById("txtSubnetMask");
        var appJoinKey = document.getElementById("txtAppJoinKey");
        var serialName = document.getElementById("txtSerialName");
        var stackLoggingLevel = document.getElementsByName("rbStackLoggingLevel");

        var saveFailed = false;

        try {
            var service = new jsonrpc.ServiceProxy(serviceURL, methods);
            if (!service.config.setVariable({group : "BACKBONE", varName : "BACKBONE_EUI64", varValue: eui64.value})) {
                alert("Error saving EUI64 !");
                saveFailed = true;
            }
            if (!service.config.setVariable({group : "BACKBONE", varName : "BACKBONE_IPv6", varValue: ipv6.value})) {
                alert("Error saving IPv6 Address !"); 
                saveFailed = true;
            }
            if (!service.config.setVariable({group : "BACKBONE", varName : "BACKBONE_Port", varValue: udpPort.value})) {
                alert("Error saving UDP Port Number !");
                saveFailed = true;
            }
            if (!service.config.setVariable({group : "BACKBONE", varName : "BACKBONE_TAG", varValue: bbrTag.value})) {
                alert("Error saving BBR Tag !");
                saveFailed = true;
            }
            if (!service.config.setVariable({group : "BACKBONE", varName : "FilterTargetID", varValue: subnetId.value})) {
                alert("Error saving Subnet ID !");
                saveFailed = true;
            }
            if(!service.config.setVariable({group : "BACKBONE", varName : "FilterBitMask", varValue: subnetMask.value})) {
                alert("Error saving Subnet Mask !");
                saveFailed = true;
            }
            if(!service.config.setVariable({group : "BACKBONE", varName : "AppJoinKey", varValue: appJoinKey.value})) {
                alert("Error saving App Join Key !");
                saveFailed = true;
            }
            if (!service.config.setVariable({group : "BACKBONE", varName : "TTY_DEV", varValue: serialName.value})) {
                alert("Error saving Serial Name !");
                saveFailed = true;
            }
            
            var stackLoggingLevelValue;
            for (var i=0; i < stackLoggingLevel.length; i++) {
                if (stackLoggingLevel[i].checked) {
                    stackLoggingLevelValue = i + 1;
                }
            }
         
            if (!service.config.setVariable({group : "BACKBONE", varName : "LOG_LEVEL_STACK", varValue: stackLoggingLevelValue})) {
                alert("Error saving Stack Logging level !");
                saveFailed = true;                   
            }

            if (!service.misc.applyConfigChanges({module : "backbone"})) {
                alert("Error applying config changes !");
                saveFailed = true;                   
            }
        } catch(e) {
             HandleException(e, "Unexpected error saving values !");
             return;
        }
        
        if(!saveFailed) {
            DisplayOperationResult("spnOperationResult", "Save completed successfully.");
        }
    }
}


function ValidateInput() {
    var eui64 = document.getElementById("txtEUI64");
    var ipv6 = document.getElementById("txtIPv6Address");	
    var udpPort = document.getElementById("txtUDPPortNumber");
    var bbrTag = document.getElementById("txtBBRTag");
    var subnetId = document.getElementById("txtSubnetId");
    var subnetMask = document.getElementById("txtSubnetMask");
    var appJoinKey = document.getElementById("txtAppJoinKey");
    var serialName = document.getElementById("txtSerialName");
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
    if(!ValidateRequired(bbrTag, "BBR Tag")) {
        return false;
    }
    if(!ValidateRequired(subnetId, "Subnet ID")) {
        return false;
    }
    if(!Validate4DigitHex(subnetId, "Subnet ID")) {
        return false;
    }
    if(!ValidateRequired(subnetMask, "Subnet Mask")) {
        return false;
    }
    if(!Validate4DigitHex(subnetMask, "Subnet Mask")) {
        return false;
    }
    if(!ValidateRequired(appJoinKey, "App Join Key")) {
        return false;
    }
    if(!Validate16BHex(appJoinKey, "App Join Key")) {
        return false;
    }     
    if(!ValidateRequired(serialName, "Serial Name")) {
        return false;
    }
    if(!ValidateRequiredRadio(stackLoggingLevel, "Stack Logging level")) {
        return false;
    }
    return true;
}
