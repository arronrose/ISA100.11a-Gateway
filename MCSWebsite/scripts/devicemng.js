var methods = ["config.getGroupVariables",
               "config.setVariable",
               "isa100.sm.setDevice",
               "isa100.sm.delDevice",
               "misc.applyConfigChanges",
               "misc.fileDownload",
               "user.logout",
               "user.isValidSession"];

function InitDeviceMngPage() {
    SetPageCommonElements();
    InitJSON();
    PopulateAllDeviceLists();
}


function DeleteBackbone() {
    ClearOperationResults();
    var txtBackbone = document.getElementById("txtBackbone");
    if (confirm("Are you sure you want to delete the Backbone?")){
        try {
            var service = new jsonrpc.ServiceProxy(serviceURL, methods);
            var response = service.isa100.sm.delDevice({deviceType: "BACKBONE", deviceValue : txtBackbone.value});
                
            if(response) {
                PopulateAllDeviceLists();
                DisplayOperationResult("spnOperationResultBackbone", "Backbone deleted successfully.");
            };
        } catch(e) {
            HandleException(e, "Backbone not deleted or not found !");
            return;
        };
    };
}


function DeleteDevice() {
    ClearOperationResults();
    var txtDevice = document.getElementById("txtDevice");           
    if (confirm("Are you sure you want to delete the Device?")){    
        try {
            var service = new jsonrpc.ServiceProxy(serviceURL, methods);
            var response = service.isa100.sm.delDevice({deviceType: "DEVICE", deviceValue : txtDevice.value});
                
            if(response)
            {
                PopulateAllDeviceLists();
                DisplayOperationResult("spnOperationResultDevice", "Device deleted successfully.");
            };
        } catch(e) {
            HandleException(e, "Device not deleted or not found !");
            return;
        };
    };
}


function DeleteGateway() {
    ClearOperationResults();
    var txtGateway = document.getElementById("txtGateway");
    if (confirm("Are you sure you want to delete the Gateway?")){        
        try {
            var service = new jsonrpc.ServiceProxy(serviceURL, methods);
            var response = service.isa100.sm.delDevice({deviceType: "GATEWAY", deviceValue : txtGateway.value});
                
            if(response) {
                PopulateAllDeviceLists();
                DisplayOperationResult("spnOperationResultGateway", "Gateway deleted successfully.");
            };
        } catch(e) {
            HandleException(e, "Gateway not deleted or not found !");
            return;
        };
    };
}


function EditDevice() {
    var txtDevice = document.getElementById("txtDevice");
    txtDevice.value = document.getElementById("lstDevices").value;
    ClearOperationResults();
}

function EditBackbone() {
    var txtBackbone = document.getElementById("txtBackbone");
    txtBackbone.value = document.getElementById("lstBackbones").value;
    ClearOperationResults();
}

function EditGateway() {
    
    var txtGateway = document.getElementById("txtGateway");
    txtGateway.value = document.getElementById("lstGateways").value;
    ClearOperationResults();
}


function SaveBackbone() {
    ClearOperationResults();

    var txtBackbone = document.getElementById("txtBackbone");
    
    if (ValidateDevice(txtBackbone, "Backbone")) {
        try {   
            var service = new jsonrpc.ServiceProxy(serviceURL, methods);
            var response = service.isa100.sm.setDevice({deviceType: "BACKBONE", deviceValue : txtBackbone.value});
                
            if(response) {
                PopulateAllDeviceLists();
                DisplayOperationResult("spnOperationResultBackbone", "Save completed successfully.");
            } else {
                alert ("Error saving backbone !");
            }
        }
        catch(e)
        {
            HandleException(e, "Unexpected error saving backbone !");
            return;
        }
    }
}


function SaveGateway() {
    ClearOperationResults();

    var txtGateway = document.getElementById("txtGateway");
    
    if (ValidateDevice(txtGateway, "Gateway")) {
        try {
            var service = new jsonrpc.ServiceProxy(serviceURL, methods);
            var response = service.isa100.sm.setDevice({deviceType: "GATEWAY", deviceValue : txtGateway.value});
                
            if(response) {
                PopulateAllDeviceLists();
                DisplayOperationResult("spnOperationResultGateway", "Save completed successfully.");
            } else {
                alert ("Error saving Gateway !");
            }
        } catch(e) {
            HandleException(e, "Unexpected error saving gateway !");
            return;
        }
    }
}


function SaveDevice() {
    ClearOperationResults();

    var txtDevice = document.getElementById("txtDevice");
    
    if (ValidateDevice(txtDevice, "Device")) {
        try {
            var service = new jsonrpc.ServiceProxy(serviceURL, methods);
            var response = service.isa100.sm.setDevice({deviceType: "DEVICE", deviceValue : txtDevice.value});
            if(response) {
                PopulateAllDeviceLists();
                DisplayOperationResult("spnOperationResultDevice", "Save completed successfully.");
            } else {
                alert ("Error saving device !");
            }
        } catch(e) {
            HandleException(e, "Unexpected error saving device !");
            return;
        }
    }
}


function PopulateAllDeviceLists() {
    ClearAllDeviceLists();
    
    var lstDevices = document.getElementById("lstDevices");
    var lstBackbones = document.getElementById("lstBackbones");
    var lstGateways = document.getElementById("lstGateways");

    try {
        var service = new jsonrpc.ServiceProxy(serviceURL, methods);
        var response = service.config.getGroupVariables({configFile: FILE_SYSTEMMANAGER_INI, group : 'SECURITY_MANAGER'});
        var op;
            
        for (i = 0; i < response.length; i++) {   
            if(response[i].DEVICE != null) {
                op = new Option(response[i].DEVICE, response[i].DEVICE);
                lstDevices[lstDevices.length] = op;
            }

            if(response[i].BACKBONE != null) {
                op = new Option(response[i].BACKBONE, response[i].BACKBONE);
                lstBackbones[lstBackbones.length] = op;
            }

            if(response[i].GATEWAY != null) {
                op = new Option(response[i].GATEWAY, response[i].GATEWAY);
                lstGateways[lstGateways.length] = op;
            }
        }
        
        //set alternating items (devices, gws, bbrs list might come not ordered, so it needs to be done separately)
        for (i = 0; i < lstBackbones.length; i++) {
            if (i % 2 == 1) {
                lstBackbones[i].className = "listAlternateItem";
            }
        }
        for (i = 0; i < lstGateways.length; i++) {
            if (i % 2 == 1) {
                lstGateways[i].className = "listAlternateItem";
            }
        }
        for (i = 0; i < lstDevices.length; i++) {
            if (i % 2 == 1) {
                lstDevices[i].className = "listAlternateItem";
            }
        }
        //end set alternating items  
     } catch(e) {
         HandleException(e, "Unexpected error reading devices !");
     }
}


function ClearAllDeviceLists() {
   var txtDevice = document.getElementById("txtDevice");
   var lst = document.getElementById("lstDevices");
    
   for (i = lst.length; i >= 0; i--) {
      lst[i] = null;
   }
   
   txtDevice.value = "";
   
   var txtGateway = document.getElementById("txtGateway"); 
   txtGateway.value = "";
   lst = document.getElementById("lstGateways");
    
   for (i = lst.length; i >= 0; i--) {
      lst[i] = null;
   }
   
   var txtBackbone = document.getElementById("txtBackbone");
   lst = document.getElementById("lstBackbones");
    
   for (i = lst.length; i >= 0; i--) {
      lst[i] = null;
   }
   
   txtBackbone.value = "";
}


function Activate() {
    ClearOperationResults();
    
    try {
        var service = new jsonrpc.ServiceProxy(serviceURL, methods);

        var response = service.misc.applyConfigChanges({module: "SystemManager"});
            
        if(response) {
            DisplayOperationResult("spnOperationResultActivate", "Activate completed successfully.");
        } else {
            alert ("Error activating device list !");
        }
    } catch(e) {
        HandleException(e, "Unexpected error activating device list !");
        return;
    }
}


function Upload() {
    ClearOperationResults();
    var file = document.getElementById("smFile");
    if (getFileName(file.value) != FILE_SYSTEMMANAGER_INI){
        alert('The name of the uploaded file must be ' + FILE_SYSTEMMANAGER_INI);
        return false;
    } else {   
        if (confirm("The previous device list will be lost. Are you sure you want to replace the file?")){
            try {    
                var service = new jsonrpc.ServiceProxy(serviceURL, methods);
                service.user.isValidSession();        
                document.form1.call1.value = jsonrpc.JSONRPCMethod("a").jsonRequest(1,"misc.fileUpload", {file: FILE_SYSTEMMANAGER_INI});
                return 1;
            } catch (e) {
                HandleException(e, 'Unexpected error uploading file !')
            };    
        } else {
            return false;
        };
    };    
}

function Download() {
    ClearOperationResults();
    try {    
        var service = new jsonrpc.ServiceProxy(serviceURL, methods);
        service.user.isValidSession();        
        document.form1.call1.value = jsonrpc.JSONRPCMethod("a").jsonRequest(1, "misc.fileDownload", {file: FILE_SYSTEMMANAGER_INI});
        return 1 ;
    } catch (e) {
        HandleException(e, 'Unexpected error downloading file !')
    };        
}


function operationDoneListener(text) {
	if (!text.result) {
	   if (text.error) {
	       alert("Operation failed !");
	   }
	}
}


function ClearOperationResults() {
    ClearOperationResult("spnOperationResultBackbone");
    ClearOperationResult("spnOperationResultGateway");
    ClearOperationResult("spnOperationResultDevice");
    ClearOperationResult("spnOperationResultActivate");
}


function ShowHideHelp(op) {
    var divHelp = document.getElementById("divHelp");
    
    if (op == "open") {
        divHelp.style.display = "";
    } else {
        divHelp.style.display = "none";
    }
}
