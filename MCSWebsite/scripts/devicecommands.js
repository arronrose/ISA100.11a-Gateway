var deviceId = null;
var deviceRoleId = null;

function InitDeviceCommandsPage() {
    SetPageCommonElements();    
    InitJSON();    
    deviceId = GetPageParamValue("deviceId");
    if (!isNaN(deviceId)) {
        dev = GetDeviceInformation(deviceId);
        if (dev != null) {
            SetData(dev);
            deviceRoleId = dev.DeviceRoleID;            
            SetDeviceTabs(deviceId, dev.DeviceStatus, dev.DeviceRoleID, 6);                   
        }
    }
}

function SetData(dev) {
    document.getElementById("spnEUI64").innerHTML = dev.Address64;
    document.getElementById("spnIPv6").innerHTML = dev.Address128;
    var cmds = GetCommandsForDeviceType(dev.DeviceRoleID);
    var ddlCommands = document.getElementById("ddlCommands");
    ddlCommands.options[0] = new Option("<select>","");
    
    if (cmds) {
        var k=1;
        for(var i=0; i<cmds.length; i++) {
            if (cmds[i] == CTC_ISACSRequest) {
                var reqTypes = GetRequestTypesForCommand(cmds[i]);
                if (reqTypes) {
                    for(var j=0; j<reqTypes.length; j++) {
                        ddlCommands.options[k++] = new Option(GetCommandName(cmds[i], reqTypes[j]), cmds[i] + "." + reqTypes[j]);
                    }
                }
            } else {
                ddlCommands.options[k++] = new Option(GetCommandName(cmds[i], null),cmds[i]);
            }
        }
    }
}

function CommandSelectionChanged() {
    ClearOperationResult("spnExecuteResult");
    HideCommandParameters();
    
    var commandCode = document.getElementById("ddlCommands").value;
    var commandId = Number(commandCode.split(".")[0]);

    var spnRequestedCommittedBurst = document.getElementById("spnRequestedCommittedBurst");
    var txtRequestedCommittedBurst = document.getElementById("txtRequestedCommittedBurst");   
    
    switch(commandId) {
    case CTC_ReadValue:
        var spnChannels = document.getElementById("spnChannels");
        var ddlChannels = document.getElementById("ddlChannels");
                
        spnChannels.style.display = "";       
        ClearList("ddlChannels");
        
        ddlChannels.options[0] = new Option("<select>","");
        var channels = GetChannelsForDevice(deviceId);
        for(i = 0;i < channels.length; i++) {
            ddlChannels.options[i] = new Option(channels[i].ChannelName,channels[i].ChannelNo);
        }                        

        spnRequestedCommittedBurst.style.display = "";
        txtRequestedCommittedBurst.value= RequestedCommittedBurstDefaultValue;        
        break;            
    case CTC_ResetDevice:
        var spnRestartType = document.getElementById("spnRestartType");
        var ddlRestartType = document.getElementById("ddlRestartType");
        
        spnRestartType.style.display = "";
        ClearList("ddlRestartType");

        ddlRestartType.options[0] = new Option("<select>","");
        ddlRestartType.options[1] = new Option("Warm Restart","2");
        ddlRestartType.options[2] = new Option("Restart as provisioned","3");
        if (deviceRoleId != DT_Gateway) {
            ddlRestartType.options[3] = new Option("Reset to factory defaults","4");
        }
        spnRequestedCommittedBurst.style.display = "none";
        txtRequestedCommittedBurst.value= RequestedCommittedBurstDefaultValue;               
        break;
    case CTC_ISACSRequest:
        var spnObjectAttributes = document.getElementById("spnObjectAttributes");
        var spnAttributeMethodId = document.getElementById("spnAttributeMethodId");
        var spnDataBuffer = document.getElementById("spnDataBuffer");
        var spnDataBufferLabel = document.getElementById("spnDataBufferLabel");
        var requestType = Number(commandCode.split(".")[1]);
        switch (requestType) {
        case CPTC_ISACSRequest_RequestType_Read:
            spnObjectAttributes.style.display = "";
            spnAttributeMethodId.innerHTML = "Attribute&nbsp;ID&nbsp;........................&nbsp;";
            spnDataBuffer.style.display = "none";
            spnDataBufferLabel.style.display = "none";
            ClearAttributeFields();
            break;
        case CPTC_ISACSRequest_RequestType_Write:
            spnObjectAttributes.style.display = "";
            spnAttributeMethodId.innerHTML = "Attribute&nbsp;ID&nbsp;........................&nbsp;";
            spnDataBuffer.style.display = "";
            spnDataBufferLabel.style.display = "";
            spnDataBufferLabel.innerHTML = "Values&nbsp;(HEX)&nbsp;......................&nbsp;";
            ClearAttributeFields();
            break;
        case CPTC_ISACSRequest_RequestType_Execute:
            spnObjectAttributes.style.display = "";
            spnAttributeMethodId.innerHTML = "Method&nbsp;ID&nbsp;..........................&nbsp;";
            spnDataBuffer.style.display = "";
            spnDataBufferLabel.style.display = "";
            spnDataBufferLabel.innerHTML = "Details&nbsp;(HEX)&nbsp;......................&nbsp;";
            ClearAttributeFields();
            break;
        default: ;
        }
        document.getElementById("txtIndex1").value = "0";
        document.getElementById("txtIndex2").value = "0";
        spnRequestedCommittedBurst.style.display = "";
        txtRequestedCommittedBurst.value= RequestedCommittedBurstDefaultValue;        
        break;
    default:
        document.getElementById("spnChannels").style.display = "none";
        ClearList("ddlChannels");
        document.getElementById("spnRestartType").style.display = "none";
        ClearList("ddlRestartType");
        document.getElementById("spnObjectAttributes").style.display = "none";
        document.getElementById("spnDataBuffer").style.display = "none";
        document.getElementById("spnDataBufferLabel").style.display = "none";
        ClearAttributeFields();
        break;    
    }
}

function GetCommandsForDeviceType(devType) {
    var cmdList = new Array(1); //use this style when there is one element for the array, pay attention to the Array constructor
    switch (devType) {
        case DT_Device:
        case DT_DeviceNonRouting:
        case DT_DeviceIORouting:
        case DT_HartISAAdapter:
            return new Array(CTC_ReadValue, CTC_ResetDevice, CTC_ISACSRequest); 
        case DT_BackboneRouter:     
            cmdList[0] = CTC_ResetDevice;
			return cmdList;
		case DT_Gateway:
            cmdList[0] = CTC_ResetDevice;
            return cmdList;
		case DT_SystemManager:
		      return null;
		default:
			return null;
    }
}

function GetRequestTypesForCommand(commandCode) {
    switch (commandCode) {
        case CTC_ISACSRequest:
            return new Array(CPTC_ISACSRequest_RequestType_Read, CPTC_ISACSRequest_RequestType_Write, CPTC_ISACSRequest_RequestType_Execute); 
		default:
			return null;
    }
}

function SystemManagerCheck() {
    var systemManager = GetSystemManagerDevice();
	if (systemManager == null) {
		alert("No registered System Manager !");
		return false;
	}
	if (systemManager.DeviceStatus < DS_JoinedAndConfigured) {
	    alert("No registered System Manager !");
  		return false;
    }
    return true;
}

function ExecuteCommand() {	
    var ddlCommands = document.getElementById("ddlCommands");
    if(ddlCommands.value == "") {
        alert("Field 'Command' is required !");
        return;   
    }

    var txtRequestedCommittedBurst = document.getElementById("txtRequestedCommittedBurst");
    if(txtRequestedCommittedBurst.value == ""){
    	alert("Filed 'Requested Committed Burst' is required!");
    	txtRequestedCommittedBurst.focus();
    	return;
    }	
    if(!Number(txtRequestedCommittedBurst.value)){
    	alert("Filed 'Requested Committed Burst' must be numeric!");
    	txtRequestedCommittedBurst.focus();
    	return;    	
    }    
    if (!ValidateRange(txtRequestedCommittedBurst,"Requested Committed Burst", -32768, 4)){
    	return;
    };
    
    var trackingNo;
    var cmd = new Command();
    var cmdParam = new CommandParameter();
    var cmdParam1 = new CommandParameter();
    var cmdParam2 = new CommandParameter();
    var cmdParam3 = new CommandParameter();
    var cmdParam4 = new CommandParameter();
    var cmdParam5 = new CommandParameter();
    var cmdParam6 = new CommandParameter();
    var cmdParam7 = new CommandParameter();
    var cmdParam8 = new CommandParameter();
    
    var spnChannels = document.getElementById("spnChannels");
    var systemManager = GetSystemManagerDevice();
    if (systemManager == null) {
        return;
    }
    
    var commandCode = ddlCommands.value;
    var commandId = Number(commandCode.split(".")[0]);
    switch(commandId) {
    case CTC_ReadValue:
        var ddlChannels = document.getElementById("ddlChannels");
        if(ddlChannels.value == "") {
            alert("Field 'Channel' is required!");
            return;
        }                
        var txtRequestedCommittedBurst = document.getElementById("txtRequestedCommittedBurst");
        
        // command parameters
        cmdParam.ParameterCode = CPC_ReadValue_Channel;
        cmdParam.ParameterValue = ddlChannels.value;
        cmdParam1.ParameterCode =  CPC_ReadValue_RequestedCommittedBurst;
        cmdParam1.ParameterValue = txtRequestedCommittedBurst.value;

        // ReadValue command   
        cmd.DeviceID = deviceId;
        cmd.CommandTypeCode = commandId;
        cmd.CommandStatus = CS_New;
        cmd.TimePosted = ConvertFromJSDateToSQLiteDate(GetUTCDate());              
        cmd.ParametersDescription = GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam.ParameterCode)) + ": " + ddlChannels.options[ddlChannels.selectedIndex].text + ", " +
        							GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam1.ParameterCode)) + ": " + cmdParam1.ParameterValue; 
        var cmdParams = new Array;
        cmdParams[0] = cmdParam;
        cmdParams[1] = cmdParam1;
        
        trackingNo = AddCommand(cmd, cmdParams);
        break;
    case CTC_ResetDevice:
        var ddlRestartType = document.getElementById("ddlRestartType");
        if(ddlRestartType.value == "") {
            alert("Field 'Restart Type' is required!");
            return;
        }
        var txtRequestedCommittedBurst = document.getElementById("txtRequestedCommittedBurst");
        
        // command parameters
        cmdParam.ParameterCode = CPC_ResetDevice_DeviceID;
        cmdParam.ParameterValue = deviceId;        
        cmdParam1.ParameterCode = CPC_ResetDevice_RestartType;
        cmdParam1.ParameterValue = ddlRestartType.value;        
        
        // Reset device command        
        cmd.DeviceID = systemManager.DeviceID;
        cmd.CommandTypeCode = commandId;
        cmd.CommandStatus = CS_New;        
        cmd.TimePosted = ConvertFromJSDateToSQLiteDate(GetUTCDate());        
        cmd.ParametersDescription = GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam.ParameterCode)) + ":" +  document.getElementById("spnEUI64").innerHTML + ", " + 
                                    GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam1.ParameterCode)) + ":" + GetRestartTypeName(Number(cmdParam1.ParameterValue));
        var cmdParams = new Array;
        cmdParams[0] = cmdParam;
        cmdParams[1] = cmdParam1;
        
        trackingNo = AddCommand(cmd, cmdParams);
        break;
    case CTC_ISACSRequest:
        var requestType = Number(commandCode.split(".")[1]);
        var txtTsapId, txtObjectId, txtAttributeMethodId, txtIndex1, txtIndex2, txtDataBuffer;
        if (requestType == CPTC_ISACSRequest_RequestType_Read ||
            requestType == CPTC_ISACSRequest_RequestType_Write ||
            requestType == CPTC_ISACSRequest_RequestType_Execute) {
            
            txtTsapId = document.getElementById("txtTsapId");
            txtObjectId = document.getElementById("txtObjectId");
            txtAttributeMethodId = document.getElementById("txtAttributeMethodId");
            txtIndex1 = document.getElementById("txtIndex1");
            txtIndex2 = document.getElementById("txtIndex2");

            if (!ValidateRequired(txtTsapId, "TSAP ID") || 
                !ValidateNumber(txtTsapId, "TSAP ID", 0, 15)) {
                return;
            }
            if (!ValidateRequired(txtObjectId, "Object ID") || 
                !ValidateNumber(txtObjectId, "Object ID", 0, 65535)) {
                return;
            }
            if (!ValidateRequired(txtAttributeMethodId, "Attribute/Method ID") || 
                !ValidateNumber(txtAttributeMethodId, "Attribute/Method ID", 0, 4095)) {
                return;
            }
            if (!ValidateRequired(txtIndex1, "Index1") || 
                !ValidateNumber(txtIndex1, "Index1", 0, 32767)) {
                return;
            }
            if (!ValidateRequired(txtIndex2, "Index2") || 
                !ValidateNumber(txtIndex2, "Index2", 0, 32767)) {
                return;
            }
        }
        var txtRequestedCommittedBurst = document.getElementById("txtRequestedCommittedBurst");
        
        switch (requestType) {
        case CPTC_ISACSRequest_RequestType_Read:
            // command parameters
            cmdParam.ParameterCode = CPC_ISACSRequest_TSAPID;
            cmdParam.ParameterValue = txtTsapId.value;        
            cmdParam1.ParameterCode = CPC_ISACSRequest_RequestType;
            cmdParam1.ParameterValue = requestType;        
            cmdParam2.ParameterCode = CPC_ISACSRequest_ObjectID;
            cmdParam2.ParameterValue = txtObjectId.value;        
            cmdParam3.ParameterCode = CPC_ISACSRequest_AttributeMethodID;
            cmdParam3.ParameterValue = txtAttributeMethodId.value;        
            cmdParam4.ParameterCode = CPC_ISACSRequest_AttributeIndex1;
            cmdParam4.ParameterValue = txtIndex1.value;        
            cmdParam5.ParameterCode = CPC_ISACSRequest_AttributeIndex2;
            cmdParam5.ParameterValue = txtIndex2.value;        
            cmdParam7.ParameterCode = CPC_ISACSRequest_ReadAsPublish;
            cmdParam7.ParameterValue = "0";
            cmdParam8.ParameterCode = CPC_ISACSRequest_RequestedCommittedBurst;
            cmdParam8.ParameterValue = txtRequestedCommittedBurst.value;
            
            // Reset device command        
            cmd.DeviceID = deviceId;
            cmd.CommandTypeCode = commandId;
            cmd.CommandStatus = CS_New;
            cmd.TimePosted = ConvertFromJSDateToSQLiteDate(GetUTCDate());        
            cmd.ParametersDescription = GetRequestTypeName(requestType) + ", " +
            							"TsapID:" + cmdParam.ParameterValue + ", " +                                          
                                        "Obj:" + cmdParam2.ParameterValue + ", " + 
                                        "Attribute:" + cmdParam3.ParameterValue + ", " + 
                                        "Idx1:" + cmdParam4.ParameterValue + ", " + 
                                        "Idx2:" + cmdParam5.ParameterValue + ", " +
                                        "CdtBurst:" + cmdParam8.ParameterValue;

            trackingNo = AddCommand(cmd, new Array(cmdParam, cmdParam1, cmdParam2, cmdParam3, cmdParam4, cmdParam5, cmdParam7, cmdParam8));
            break;
        case CPTC_ISACSRequest_RequestType_Write:
            txtDataBuffer = document.getElementById("txtDataBuffer");
            if (!ValidateRequired(txtDataBuffer, "Value") ||
                !ValidateHex(txtDataBuffer, "Value") ) {
                return;
            }
            if (txtDataBuffer.value.length%2 != 0){
            	alert("Values field must contain 2n characters!");
            	return;
            }
            // command parameters
            cmdParam.ParameterCode = CPC_ISACSRequest_TSAPID;
            cmdParam.ParameterValue = txtTsapId.value;        
            cmdParam1.ParameterCode = CPC_ISACSRequest_RequestType;
            cmdParam1.ParameterValue = requestType;        
            cmdParam2.ParameterCode = CPC_ISACSRequest_ObjectID;
            cmdParam2.ParameterValue = txtObjectId.value;        
            cmdParam3.ParameterCode = CPC_ISACSRequest_AttributeMethodID;
            cmdParam3.ParameterValue = txtAttributeMethodId.value;        
            cmdParam4.ParameterCode = CPC_ISACSRequest_AttributeIndex1;
            cmdParam4.ParameterValue = txtIndex1.value;        
            cmdParam5.ParameterCode = CPC_ISACSRequest_AttributeIndex2;
            cmdParam5.ParameterValue = txtIndex2.value;        
            cmdParam6.ParameterCode = CPC_ISACSRequest_DataBuffer;
            cmdParam6.ParameterValue = txtDataBuffer.value;
            cmdParam7.ParameterCode = CPC_ISACSRequest_ReadAsPublish;
            cmdParam7.ParameterValue = "0";
            cmdParam8.ParameterCode = CPC_ISACSRequest_RequestedCommittedBurst;
            cmdParam8.ParameterValue = txtRequestedCommittedBurst.value;
            
            // Reset device command        
            cmd.DeviceID = deviceId;
            cmd.CommandTypeCode = commandId;
            cmd.CommandStatus = CS_New;
            cmd.TimePosted = ConvertFromJSDateToSQLiteDate(GetUTCDate());        
            cmd.ParametersDescription = GetRequestTypeName(requestType) + ', ' +
            							"TsapID:" + cmdParam.ParameterValue + ', ' +                                          
                                        "Obj:" + cmdParam2.ParameterValue + ', ' + 
                                        "Attribute:" + cmdParam3.ParameterValue + ', ' + 
                                        "Idx1:" + cmdParam4.ParameterValue + ', ' + 
                                        "Idx2:" + cmdParam5.ParameterValue + ', ' + 
                                        "Values:" + cmdParam6.ParameterValue+ ", " +
                                        "CdtBurst:" + cmdParam8.ParameterValue;
            
            trackingNo = AddCommand(cmd, new Array(cmdParam, cmdParam1, cmdParam2, cmdParam3, cmdParam4, cmdParam5, cmdParam6, cmdParam7, cmdParam8));
            break;
        case CPTC_ISACSRequest_RequestType_Execute:
            txtDataBuffer = document.getElementById("txtDataBuffer");
            if (!ValidateRequired(txtDataBuffer, "Details") ||
                !ValidateHex(txtDataBuffer, "Details")) {
                return;
            }
            if (txtDataBuffer.value.length%2 != 0){
            	alert("Details field must contain 2n characters!");
            	return;
            }            
            // command parameters
            cmdParam.ParameterCode = CPC_ISACSRequest_TSAPID;
            cmdParam.ParameterValue = txtTsapId.value;        
            cmdParam1.ParameterCode = CPC_ISACSRequest_RequestType;
            cmdParam1.ParameterValue = requestType;        
            cmdParam2.ParameterCode = CPC_ISACSRequest_ObjectID;
            cmdParam2.ParameterValue = txtObjectId.value;        
            cmdParam3.ParameterCode = CPC_ISACSRequest_AttributeMethodID;
            cmdParam3.ParameterValue = txtAttributeMethodId.value;        
            cmdParam4.ParameterCode = CPC_ISACSRequest_AttributeIndex1;
            cmdParam4.ParameterValue = txtIndex1.value;        
            cmdParam5.ParameterCode = CPC_ISACSRequest_AttributeIndex2;
            cmdParam5.ParameterValue = txtIndex2.value;        
            cmdParam6.ParameterCode = CPC_ISACSRequest_DataBuffer;
            cmdParam6.ParameterValue = txtDataBuffer.value;        
            cmdParam7.ParameterCode = CPC_ISACSRequest_ReadAsPublish;
            cmdParam7.ParameterValue = "0";
            cmdParam8.ParameterCode = CPC_ISACSRequest_RequestedCommittedBurst;
            cmdParam8.ParameterValue = txtRequestedCommittedBurst.value;            
                        
            // Reset device command        
            cmd.DeviceID = deviceId;
            cmd.CommandTypeCode = commandId;
            cmd.CommandStatus = CS_New;        
            cmd.TimePosted = ConvertFromJSDateToSQLiteDate(GetUTCDate());        
            cmd.ParametersDescription = GetRequestTypeName(requestType) + ', ' +
            							"TsapID:" + cmdParam.ParameterValue + ', ' +                                         
                                        "Obj:" + cmdParam2.ParameterValue + ', ' + 
                                        "Method:" + cmdParam3.ParameterValue + ', ' + 
                                        "Idx1:" + cmdParam4.ParameterValue + ', ' + 
                                        "Idx2:" + cmdParam5.ParameterValue + ', ' + 
                                        "Details:" + cmdParam6.ParameterValue + ", " +
                                        "CdtBurst:" + cmdParam8.ParameterValue;
                                        
            trackingNo = AddCommand(cmd, new Array(cmdParam, cmdParam1, cmdParam2, cmdParam3, cmdParam4, cmdParam5, cmdParam6, cmdParam7, cmdParam8));
            break;
        default: ;
        }
        break;
    default: ;
    }
    
    if (trackingNo != null && trackingNo > 0) {
        DisplayOperationResult("spnExecuteResult", "Command sent successfully. <br />  Tracking no: " + trackingNo + ". Go to <a href='commandslog.html'>Commands Log.</a>");
    } else {
        DisplayOperationResult("spnExecuteResult", "<font color='#FF0000'>Command sent error!</font>");
    }
}

function CancelClicked() {
    HideCommandParameters();
    document.getElementById("ddlCommands").selectedIndex = 0;
    ClearOperationResult("spnExecuteResult");
}

function HideCommandParameters() {
    var spnChannels = document.getElementById("spnChannels");
    spnChannels.style.display = "none";
    var spnRestartType = document.getElementById("spnRestartType");
    spnRestartType.style.display = "none";
    var spnObjectAttributes = document.getElementById("spnObjectAttributes");
    spnObjectAttributes.style.display = "none";
    var spnRequestedCommittedBurst = document.getElementById("spnRequestedCommittedBurst");
    spnRequestedCommittedBurst.style.display = "none";
}

function BackButtonClicked() {
    document.location.href = "devicelist.html?setState";
}

function ClearAttributeFields() {
    document.getElementById("txtTsapId").value = "";
    document.getElementById("txtObjectId").value = "";
    document.getElementById("txtAttributeMethodId").value = "";
    document.getElementById("txtIndex1").value = "";
    document.getElementById("txtIndex2").value = "";
    document.getElementById("txtDataBuffer").value = "";
}
