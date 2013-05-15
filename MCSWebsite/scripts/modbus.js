var methods = ["isa100.modbus.server.setRow",
               "isa100.modbus.server.delRow",
               "misc.applyConfigChanges",
               "config.getConfig",
               "file.create",
               "user.logout",
               "user.isValidSession" ];

//config file content
var CONTENT = null;

function InitModbusPage() {
    SetPageCommonElements();   
    InitJSON();    
    ClearPageControls();
    ReadConfigFromDisk();
    PopulateInputRegistersList();
    PopulateHoldingRegistersList();
}

function ReadConfigFromDisk() {
    try {    
        var service = new jsonrpc.ServiceProxy(serviceURL, methods);
        // create if not exists
        service.file.create({file: FILE_MODBUS_INI});
        CONTENT = service.config.getConfig({configFile: FILE_MODBUS_INI});
    } catch(e) {
        HandleException(e, "Unexpected error reading data from " + FILE_MODBUS_INI + "!");
    }
}

function PopulateInputRegistersList() {
    ClearList("lstInputRegisters");
    document.getElementById("txtInputRegister").value = "";
    
    var rowCounter = 0;
    var lst = document.getElementById("lstInputRegisters");
    if (CONTENT != null) {
        for (var i = 0; i < CONTENT.length; i++) {   
            if (CONTENT[i].group != null && 
                CONTENT[i].group == "INPUT_REGISTERS") {
                for (var j = 0; j < CONTENT[i].variables.length; j++) {
                    if (CONTENT[i].variables[j].REGISTER != null) {
                        var op = new Option(CONTENT[i].variables[j].REGISTER, CONTENT[i].variables[j].REGISTER);
                        if ( rowCounter % 2 == 1) {
                            op.className = "listAlternateItem";
                        }
                        lst[lst.length] = op;
                        rowCounter++;
                    }
                }
            }
        }
    }
}

function PopulateHoldingRegistersList() {
    ClearList("lstHoldingRegisters");
    document.getElementById("txtHoldingRegister").value = "";
    
    var rowCounter = 0;
    var lst = document.getElementById("lstHoldingRegisters");
    if (CONTENT != null) {
        for (var i = 0; i < CONTENT.length; i++) {   
            if (CONTENT[i].group != null && 
                CONTENT[i].group == "HOLDING_REGISTERS") { 
                for (var j = 0; j < CONTENT[i].variables.length; j++) {
                    if (CONTENT[i].variables[j].REGISTER != null) {
                        var op = new Option(CONTENT[i].variables[j].REGISTER, CONTENT[i].variables[j].REGISTER);
                        if ( rowCounter % 2 == 1) {
                            op.className = "listAlternateItem";
                        }
                        lst[lst.length] = op;
                        rowCounter++;
                    }
                }
            }
        }
    }
}

function EditInputRegister() {
    var txtInputRegister = document.getElementById("txtInputRegister");
    txtInputRegister.value = document.getElementById("lstInputRegisters").value;
    ClearOperationResults();
}

function SaveInputRegister() {
    ClearOperationResults();
    document.getElementById("txtInputRegister").value = document.getElementById("txtInputRegister").value.trimAll();
    var txtInputRegister = document.getElementById("txtInputRegister");
    if (ValidateRegisterServer(txtInputRegister, "Input Register")) {
        try {   
            var service = new jsonrpc.ServiceProxy(serviceURL, methods);
            var response = service.isa100.modbus.server.setRow({section: "INPUT_REGISTERS", rowValue: txtInputRegister.value});
            if(response) {
                ReadConfigFromDisk();
                PopulateInputRegistersList();
                DisplayOperationResult("spnOperationResultInputRegister", "Input register saved successfully.");
            } else {
                alert ("Error saving input register !");
            }
        } catch(e) {
            HandleException(e, "Unexpected error saving input register !");
            return;
        }
    }
}

function DeleteInputRegister() {
    ClearOperationResults();
    document.getElementById("txtInputRegister").value = document.getElementById("txtInputRegister").value.trimAll();
    var txtInputRegister = document.getElementById("txtInputRegister");   
    if (confirm("Are you sure you want to delete the Input Register?")){                 
        if (ValidateRegisterServer(txtInputRegister, "Input register")) {
            try {
                var service = new jsonrpc.ServiceProxy(serviceURL, methods);
                var response = service.isa100.modbus.server.delRow({section: "INPUT_REGISTERS", rowValue: txtInputRegister.value});
                if(response) {
                    ReadConfigFromDisk();
                    PopulateInputRegistersList();
                    DisplayOperationResult("spnOperationResultInputRegister", "Input register deleted successfully.");
                };
            } catch(e) {
                HandleException(e, "Input register not deleted or not found !");
                return;
            };
        };
    };        
}

function EditHoldingRegister() {
    var txtHoldingRegister = document.getElementById("txtHoldingRegister");
    txtHoldingRegister.value = document.getElementById("lstHoldingRegisters").value;
    ClearOperationResults();
}

function SaveHoldingRegister() {
    ClearOperationResults();
    document.getElementById("txtHoldingRegister").value = document.getElementById("txtHoldingRegister").value.trimAll();
    var txtHoldingRegister = document.getElementById("txtHoldingRegister");
    if (ValidateRegisterServer(txtHoldingRegister, "Holding Register")) {
        try {   
            var service = new jsonrpc.ServiceProxy(serviceURL, methods);
            var response = service.isa100.modbus.server.setRow({section: "HOLDING_REGISTERS", rowValue: txtHoldingRegister.value});
            if(response) {
                ReadConfigFromDisk();
                PopulateHoldingRegistersList();
                DisplayOperationResult("spnOperationResultHoldingRegister", "Holding register saved successfully.");
            } else {
                alert ("Error saving holding register !");
            }
        } catch(e) {
            HandleException(e, "Unexpected error saving holding register !");
            return;
        }
    }
}

function DeleteHoldingRegister() {
    ClearOperationResults();
    document.getElementById("txtHoldingRegister").value = document.getElementById("txtHoldingRegister").value.trimAll();
    var txtHoldingRegister = document.getElementById("txtHoldingRegister");
    if (confirm("Are you sure you want to delete the Holding Register?")){
        if (ValidateRegisterServer(txtHoldingRegister, "Holding register")) {
            try {
                var service = new jsonrpc.ServiceProxy(serviceURL, methods);
                var response = service.isa100.modbus.server.delRow({section: "HOLDING_REGISTERS", rowValue: txtHoldingRegister.value});
                if(response) {
                    ReadConfigFromDisk();
                    PopulateHoldingRegistersList();
                    DisplayOperationResult("spnOperationResultHoldingRegister", "Holding register deleted successfully.");
                };
            } catch(e) {
                HandleException(e, "Holding register not deleted or not found !");
                return;
            };
        };
    };
}

function ClearOperationResults() {
    ClearOperationResult("spnOperationResultInputRegister");
    ClearOperationResult("spnOperationResultHoldingRegister");
    ClearOperationResult("spnOperationResultActivate");
}

function Activate() {
    ClearOperationResults();
    try {
        var service = new jsonrpc.ServiceProxy(serviceURL, methods);

        var response = service.misc.applyConfigChanges({module: "modbus_gw"});
            
        if(response) {
            DisplayOperationResult("spnOperationResultActivate", "Activate completed successfully.");
        } else {
            alert ("Error activating registers!");
        }
    } catch(e) {
        HandleException(e, "Unexpected error activating registers!");
        return;
    }
}

function ClearPageControls() {
    ClearList("lstInputRegisters");
    ClearList("lstHoldingRegisters");
    document.getElementById("txtInputRegister").value = "";
    document.getElementById("txtHoldingRegister").value = "";
}

function Download() {
    ClearOperationResults();
    try {    
        var service = new jsonrpc.ServiceProxy(serviceURL, methods);
        service.user.isValidSession();            
	    document.form1.call1.value = jsonrpc.JSONRPCMethod("a").jsonRequest(1, "misc.fileDownload", {file: FILE_MODBUS_INI});
        return 1 ;
    } catch (e) {
        HandleException(e, 'Unexpected error downloading file !')
    };        
}

function Upload() {
    ClearOperationResults();
    var file = document.getElementById("modbusFile");
    if (getFileName(file.value) != FILE_MODBUS_INI){
        alert('The name of the uploaded file must be ' + FILE_MODBUS_INI);
        return false;
    } else {   
        if (confirm("The previous host list will be lost. Are you sure you want to replace the file?")){
            try {    
                var service = new jsonrpc.ServiceProxy(serviceURL, methods);
                service.user.isValidSession();            
                document.form1.call1.value = jsonrpc.JSONRPCMethod("a").jsonRequest(1,"misc.fileUpload", {file: FILE_MODBUS_INI});
                return 1 ;
            } catch (e) {
                HandleException(e, 'Unexpected error downloading file !')
            };        
        } else {
            return false;
        };
    };    
}

function ShowHideHelp(op) {
    var divHelp = document.getElementById("divHelp");
    
    if (op == "open") {
        divHelp.style.display = "";
    } else {
        divHelp.style.display = "none";
    }
}

function operationDoneListener(text) 
{
    if (text.error)
	{
        alert("Error uploading "+ FILE_MODBUS_INI+" file !");
	}
}
