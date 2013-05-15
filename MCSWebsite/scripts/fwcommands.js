var methods = ["sqldal.execute", "user.logout", "sqldal.open", "sqldal.close", "file.remove", "file.create", "file.exists"];
var systemManager = null;
var gateway = null;
var firmwareType = null;
var firmwareFile = null;
var revision = null;
var eui64Address = null;
var deviceTag = null;
var bandwidth = null;
var tsapID = null;
var objectID = null;
var filteredDevices = new Array();
var selectedDevices = new Array(); 
var deviceTypes;
var level = null;

//order direction
var OD_ASC = "ASC";
var OD_DESC = "DESC";

var OrderBy = 1;  //order column number
var OrderDirection = OD_DESC;

function objDevice(){
	this.DeviceID = null;
	this.Address64 = null;
}

var CurrentPage = 1;
var PageSize = 10;
var TotalPages = 0;
var TotalNoOfRows = 0;
var ShowDevicesFilter = 0;

// html page elements
var rbFirmwareType;
var chkAllDevices;

var ddlFirmwareFile;
var ddlBandwith;
var ddlRevision;
var ddlLevel

var txtEUI64Address;
var txtDeviceTag;
var	txtTSAPID;
var	txtObjectID;

var spnTargetDevices;
var spnBandwidth;
var spnTSAPID;
var spnObjectID;

var helpPopup = new Popup();
helpPopup.autoHide = false;
helpPopup.offsetLeft = 10;
helpPopup.constrainToScreen = true;
helpPopup.position = 'above adjacent-right';
helpPopup.style = {'border':'2px solid black','backgroundColor':'#EEE'};

var helpContent =  
	'<table cellpadding="5"><tr><td>'+
	'<a href="#" onclick="helpPopup.hide();return false;"><b>x</b></a>' +
	'<p>The bandwidth allocation is based on the CommittedBurst parameter of the GW -> Device contract.'+
	'<p>The device must set a similar value for CommittedBurst parameter in the Device -> GW contract which is used to respond to BULK requests; the whole bulk transfer will be done at the lowest of the two transfer rates.'+
	'<p>The SM will decide the transfer Rate for Radio FW, and may decide to modify the transfer rate based on availability.'+
	'<p>The transfer rate for Sensor Board FW is determined by the user input AND the parameter CommittedBurst set by the Sensor Board in the contract from device to GW used to respond to bulk requests.'+
	'</td></tr></table>'

function InitFirmwareUpdatePage() {
    SetPageCommonElements();
    InitJSON();
    systemManager = GetSystemManagerDevice();
    gateway = GetGatewayDevice();
    SetData();
    ResetFilters();
    Search();
}

function SetFirmwareType(){
	rbFirmwareType = document.getElementsByName("rbFirmwareType");
	if (rbFirmwareType[0].checked){ 	//  radio fwType is checked
		firmwareType = FT_Device;			
		return;
	}; 
	if (rbFirmwareType[1].checked) {	// sensor fwType is checked	
		firmwareType = FT_AquisitionBoard;
		return;
	}; 
	if (rbFirmwareType[2].checked) {	// BBR fwType is checked		
		firmwareType = FT_BackboneRouter;
		return;
	};
}

function SetFirmwareFiles(firmwareType){
	ddlFirmwareFile = document.getElementById("ddlFirmwareFile");
	ClearList("ddlFirmwareFile");
	var firmwareFiles = GetFirmwareFiles([firmwareType],[US_SuccessfullyUploaded]);
	ddlFirmwareFile.options[0] = new Option("<none>", "");
	if (firmwareFiles) {	
		for(var i=0; i<firmwareFiles.length; i++) {    	
			ddlFirmwareFile.options[i] = new Option(firmwareFiles[i].FileName, firmwareFiles[i].FileName);
		};
	};	
}

function SetBandwith(firmwareType){
	ddlBandwith = document.getElementById("ddlBandwith");
	spnBandwidth = document.getElementById("spnBandwidth");
	spnTSAPID = document.getElementById("spnTSAPID");
	spnObjectID = document.getElementById("spnObjectID");
	
	txtTSAPID = document.getElementById("txtTSAPID");
	txtObjectID = document.getElementById("txtObjectID");
	
	ClearList("ddlBandwith");
	if (firmwareType == FT_AquisitionBoard){		
		ddlBandwith.options[0] = new Option("Low..........(1 packet every 15 seconds)", -15);
		ddlBandwith.options[1] = new Option("Normal.....(1 packet  per second)", 1);
		ddlBandwith.options[2] = new Option("High.........(4 packets per second)", 4);

		ddlBandwith.disabled = false;
		txtTSAPID.disabled = false;
		txtObjectID.disabled = false;
		
		spnBandwidth.style.color = "black";
		spnTSAPID.style.color = "black";
		spnObjectID.style.color = "black";

		txtTSAPID.value = 2;
		
		bandwidth = ddlBandwith[0].value;
		tsapID = txtTSAPID.value;
		objectID = txtObjectID.value;
	} else {		
		ddlBandwith.options[0] = new Option("<N/A>", "");
		
		ddlBandwith.disabled = true;
		txtTSAPID.disabled = true;
		txtObjectID.disabled = true;
		
		spnBandwidth.style.color = "gray";
		spnTSAPID.style.color = "gray";
		spnObjectID.style.color = "gray";
	
		txtTSAPID.value = "";
		txtObjectID.value = "";		
	};	
}

function SetDeviceTypes(firmwareType){
	switch (firmwareType){ 
		case FT_Device:{ 				
			deviceTypes = [DT_Device, DT_DeviceNonRouting, DT_DeviceIORouting, DT_HartISAAdapter];		
			return;
		}	
		case FT_AquisitionBoard:{
			deviceTypes = [DT_Device, DT_DeviceNonRouting, DT_DeviceIORouting, DT_HartISAAdapter];
			return;
		}	
		case FT_BackboneRouter:{
			deviceTypes = [DT_BackboneRouter];
			return;
		}	
		default:{
			deviceTypes = null;
		} 			
	};	
}

function SetRevisions(deviceTypes){
	ClearList("ddlRevision");
	ddlRevision = document.getElementById("ddlRevision");
	var revisions = GetAllDevicesRevisions(deviceTypes);
	ddlRevision.options[0] = new Option("All", "");
    if (revisions != null) {
        for(i=0; i<revisions.length; i++) {
        	ddlRevision.options[i+1] = new Option(revisions[i].RevisionString, revisions[i].RevisionHex);
        }
    }
}

function SetLevels(firmwareType){
	ddlLevel = document.getElementById("ddlLevel");
	ClearList("ddlLevel");
	ddlLevel.options[0] = new Option("All", "");
	if (firmwareType != FT_BackboneRouter){
		var maxLevel = GetMaxDeviceLevel();
		if (maxLevel == null) {
			maxLevel = 0
		};		
		for (var i=1; i<=maxLevel; i++){
			ddlLevel.options[i] = new Option(i,i);	
		};		
	}; 
}


function ReadFilters(){
	txtEUI64Address = document.getElementById("txtEUI64Address");
	txtDeviceTag 	= document.getElementById("txtDeviceTag");
	ddlRevision 	= document.getElementById("ddlRevision");
	ddlLevel		= document.getElementById("ddlLevel");
	
	eui64Address 	= txtEUI64Address.value;		 
	deviceTag 		= txtDeviceTag.value;
	revision 		= ddlRevision[ddlRevision.selectedIndex].value;
	level			= ddlLevel.selectedIndex <= 0 ? null : ddlLevel[ddlLevel.selectedIndex].value;	
	
	PageSize = document.getElementById("ddlRowsPerPage").value
	CurrentPage = 1; 
}

function ResetFilters(){
	txtEUI64Address = document.getElementById("txtEUI64Address")
	txtDeviceTag 	= document.getElementById("txtDeviceTag")
	ddlRevision 	= document.getElementById("ddlRevision")
	ddlLevel		= document.getElementById("ddlLevel");

	txtEUI64Address.value = "";
	txtDeviceTag.value = "";
	ddlRevision.selectedIndex = 0;
	ddlLevel.selectedIndex = 0; 
}

function FirmareTypeChanged(){
	SetFirmwareType();
	SetFirmwareFiles(firmwareType);
	SetBandwith(firmwareType);
	SetDeviceTypes(firmwareType);
	SetRevisions(deviceTypes);
	SetLevels(firmwareType);
	
	ResetFilters();
	document.getElementById("spnTargetDevices").innerHTML = "none";
	Search();
	selectedDevices = new Array();	
}

function SetData() {
	rbFirmwareType = document.getElementsByName("rbFirmwareType");
	rbFirmwareType[0].checked = true; // set the fwType to <Radio>
	
	SetFirmwareType();
	SetFirmwareFiles(firmwareType);
	SetBandwith(firmwareType);
	SetDeviceTypes(firmwareType);
	SetRevisions(deviceTypes);
	SetLevels(firmwareType);
}

function Search(){
	ReadFilters();
	BuildDeviceTable();
	document.getElementById("spnTargetDevices").innerHTML = "none";
}

function Reset(){
	ResetFilters();
	ReadFilters();
	BuildDeviceTable();
	document.getElementById("spnTargetDevices").innerHTML = "none";
}

function PageNavigate(pageNo) {
	PageSize = document.getElementById("ddlRowsPerPage").value;
    CurrentPage = pageNo;
    BuildDeviceTable();
}

function BuildDeviceTable() {
	filteredDevices = new Array();	
	TotalNoOfRows = GetDeviceCount(ShowDevicesFilter, eui64Address, GetHexString(deviceTag), revision, deviceTypes, level);	
    if (TotalNoOfRows > 0) {       	
        var data = GetDevicePageForFirmwareExecution(PageSize, CurrentPage, TotalNoOfRows, eui64Address, GetHexString(deviceTag), revision, deviceTypes, level, OrderBy, OrderDirection);        
        if (data != null) {
            document.getElementById("tblDevices").innerHTML = TrimPath.processDOMTemplate("devices_jst", data);
            SetOrderSignForCurrentView(OrderBy);
            for(var i=0; i<data.length; i++){
            	var dev = new objDevice();
            	dev.DeviceID  = data[i].DeviceID;
            	dev.Address64 = data[i].Address64;
            	filteredDevices[i] = dev;            	
            }               
        } else {
            document.getElementById("tblDevices").innerHTML =
                "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"950px\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
     					"<tr>" +
     					    "<td class=\"tableSubHeader\" style=\"width:150px;\" align=\"center\">EUI-64 Address</td>" +
						    "<td class=\"tableSubHeader\" style=\"width:200px;\" align=\"left\">Device Tag</td>" +
						    "<td class=\"tableSubHeader\" style=\"width:300px;\" align=\"left\">Device Role/Model</td>" +
						    "<td class=\"tableSubHeader\" style=\"width:200px;\" align=\"left\">Revision</td>" +
						    "<td class=\"tableSubHeader\" style=\"width:50px;\" align=\"left\">Level</td>" +
						    "<td class=\"tableSubHeader\" style=\"width:50px;\" align=\"right\">All</td>" +
						    "<td class=\"tableSubHeader\">&nbsp;</td></tr>" +
                        "<tr><td colspan=\"6\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
                "</td></tr></table>";
        }
    } else {
    	document.getElementById("spnPageNumber").innerHTML = "";
        document.getElementById("tblDevices").innerHTML = 
                "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"950px\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
     				  "<tr>" +
     				      "<td class=\"tableSubHeader\" style=\"width:150px;\" align=\"center\">EUI-64 Address</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:200px;\" align=\"left\">Device Tag</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:300px;\" align=\"left\">Device Role/Model</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:200px;\" align=\"left\">Revision</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:50px;\" align=\"left\">Level</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:50px;\" align=\"right\">All</td>" +
						  "<td class=\"tableSubHeader\">&nbsp;</td></tr>" +
                     "<tr><td colspan=\"6\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
                "</td></tr></table>";
    }
    SetPager();
}

function BackButtonClicked() {
    document.location.href = "devicefw.html";
}

function AddRemoveOneDevice(){			
	chkAllDevices = document.getElementById("chkAllDevices");
	chkAllDevices.checked = false;		

	var chkDevice;
	var tmpTargetDevices = "";
	
	for (var i=0; i<filteredDevices.length; i++){
		chkDevice = document.getElementById("chkDevice" + filteredDevices[i].DeviceID);
		if (chkDevice.checked){
			tmpTargetDevices = tmpTargetDevices + filteredDevices[i].Address64 + ", ";	
		}
	}
	if (tmpTargetDevices.length > 0){
		tmpTargetDevices = tmpTargetDevices.substring(0, tmpTargetDevices.length-2) 
	}	
	document.getElementById("spnTargetDevices").innerHTML = tmpTargetDevices;		
}

function AddRemoveAllDevices(isChecked){	
	var chkDevice;
	for (var i=0; i<filteredDevices.length; i++){
		chkDevice = document.getElementById("chkDevice" + filteredDevices[i].DeviceID);
		chkDevice.checked = isChecked;	
	}
	chkAllDevices = document.getElementById("chkAllDevices"); 
	var tmpTargetDevices = ""; 
	if (chkAllDevices.checked){		
		for (var i=0; i<filteredDevices.length; i++){
			tmpTargetDevices = tmpTargetDevices + filteredDevices[i].Address64 + ", ";
		}		  
	} 
	if (tmpTargetDevices.length > 0){
		tmpTargetDevices = tmpTargetDevices.substring(0, tmpTargetDevices.length-2) 
	}	
	document.getElementById("spnTargetDevices").innerHTML = tmpTargetDevices;	
}

function GetSelectedDevices(){
	selectedDevices = new Array();
	var chkDevice;
	var j=0;
	for (var i=0; i<filteredDevices.length; i++){
		chkDevice = document.getElementById("chkDevice" + filteredDevices[i].DeviceID);
		if (chkDevice.checked){
			var selDevice = new objDevice();
			selDevice.DeviceID = filteredDevices[i].DeviceID;
			selDevice.Address64 = filteredDevices[i].Address64;			
			selectedDevices[j] = selDevice;
			j=j+1;
		}
	} 
}

function GetFirmwareParameters(){
	ddlFirmwareFile = document.getElementById("ddlFirmwareFile");
	ddlBandwith 	= document.getElementById("ddlBandwith");
	txtTSAPID 		= document.getElementById("txtTSAPID");
	txtObjectID 	= document.getElementById("txtObjectID");

	firmwareFile 	= ddlFirmwareFile[ddlFirmwareFile.selectedIndex].value;
	bandwidth 		= ddlBandwith[ddlBandwith.selectedIndex].value;
	tsapID 			= txtTSAPID.value;
	objectID 		= txtObjectID.value;
}

function Execute(){
	txtTSAPID 		= document.getElementById("txtTSAPID");
	txtObjectID 	= document.getElementById("txtObjectID");
	
	GetFirmwareParameters();	
	if (firmwareFile == ""){
		alert("You have to select a firmware file!")
		return;		
	};	
	GetSelectedDevices();	
	if (selectedDevices.length == 0){
		alert("You have to select target device(s)!")
		return;
	};				
	
	if (!confirm("A firmware update operation will be initiated on your target devices!  Are you sure you want to start this operation?")){
		return;
	};
	 
	//for every device verify if there is a fw upgrade in progress or canceling	
	var checkDevices = [];	
	for (var i=0; i<selectedDevices.length; i++){
		checkDevices[i] = selectedDevices[i].DeviceID;
	}
	
	var tmp = GetDevicesStatus(checkDevices);
	var devicesWithFwInProgress = "";
	var executionDevices = new Array();		
	
	for (var i=0; i< tmp.length; i++){		
		if (tmp[i].HasFwInProgress == 1){			
			devicesWithFwInProgress = devicesWithFwInProgress + tmp[i].Address64 + ","
		} else {
			var selDevice = new objDevice();			
			selDevice.DeviceID = tmp[i].DeviceID;
			selDevice.Address64 = tmp[i].Address64;
			executionDevices.push(selDevice);		
		} 		
	} 
	
	var noOfExecutedCommands = 0;
    if (firmwareType == FT_Device || firmwareType == FT_BackboneRouter) {    	
    	for (var i=0; i<executionDevices.length; i++){
    	    var trackingNo;    
    	    var cmd = new Command();
    	    var cmdParam1 = new CommandParameter();
    	    var cmdParam2 = new CommandParameter();

	        cmdParam1.ParameterCode = CPC_FirmwareUpdate_DeviceID;
	        cmdParam1.ParameterValue = executionDevices[i].DeviceID;        
	        cmdParam2.ParameterCode = CPC_FirmwareUpdate_FileName;
	        cmdParam2.ParameterValue = firmwareFile;	        
	
	        cmd.TimePosted = ConvertFromJSDateToSQLiteDate(GetUTCDate());
	        cmd.DeviceID = systemManager.DeviceID;
	        cmd.CommandTypeCode = CTC_FirmwareUpdate;
	        cmd.CommandStatus = CS_New;
	        cmd.ParametersDescription = GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam1.ParameterCode)) + ": " + executionDevices[i].Address64 + ", " + 
	                                    GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam2.ParameterCode)) + ": " + cmdParam2.ParameterValue;
	
	        var cmdParams = new Array(2);
	        cmdParams[0] = cmdParam1;
	        cmdParams[1] = cmdParam2;
	       
	        trackingNo = AddCommand(cmd, cmdParams);
	        if (trackingNo != null) {
	        	noOfExecutedCommands = noOfExecutedCommands + 1; 
	        	AddFwDownload(executionDevices[i].DeviceID, firmwareType);	
	        }	        
    	}
    }    
    
    if (firmwareType == FT_AquisitionBoard) {     	
    	if (!ValidateRequired(txtTSAPID, "TSAP ID") ||
    			!ValidateRange(txtTSAPID,"TSAP ID",0, 15)){
            	return;
        }    	
        if (!ValidateRequired(txtObjectID, "ObjectID") ||
        		!ValidateRange(txtObjectID,"ObjectID",1, 65535)){
            	return;
        }
    	    	
    	for (var i=0; i<executionDevices.length; i++){
    		var trackingNo;    
    	    var cmd = new Command();
    	    var cmdParam1 = new CommandParameter();
    	    var cmdParam2 = new CommandParameter();
    	    var cmdParam3 = new CommandParameter();
    	    var cmdParam4 = new CommandParameter();
    	    var cmdParam5 = new CommandParameter();
    		
	        cmdParam1.ParameterCode = CPC_UpdateSensorBoardFirmware_TargetDeviceID;
		    cmdParam1.ParameterValue = executionDevices[i].DeviceID;
	    	cmdParam2.ParameterCode = CPC_UpdateSensorBoardFirmware_FileName;
		    cmdParam2.ParameterValue = firmwareFile;
		    cmdParam3.ParameterCode = CPC_UpdateSensorBoardFirmware_TSAPID;
		    cmdParam3.ParameterValue = tsapID;
		    cmdParam4.ParameterCode = CPC_UpdateSensorBoardFirmware_ObjID;
		    cmdParam4.ParameterValue = objectID;	    	    
		    cmdParam5.ParameterCode = CPC_UpdateSensorBoardFirmware_RequestedCommittedBurst;
	        cmdParam5.ParameterValue = bandwidth;
	
	        cmd.TimePosted = ConvertFromJSDateToSQLiteDate(GetUTCDate()); 
	        cmd.DeviceID = gateway.DeviceID;
	        cmd.CommandTypeCode = CTC_UpdateSensorBoardFirmware;
	        cmd.CommandStatus = CS_New;
	        cmd.ParametersDescription = GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam1.ParameterCode)) + ": " + executionDevices[i].Address64 + ", " +
	        							GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam2.ParameterCode)) + ": " + cmdParam2.ParameterValue + ", " +
	        							GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam3.ParameterCode)) + ": " + cmdParam3.ParameterValue + ", " +
	        							GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam4.ParameterCode)) + ": " + cmdParam4.ParameterValue + ", " +
	        							GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam5.ParameterCode)) + ": " + cmdParam5.ParameterValue;
	        var cmdParams = new Array(5);
	        cmdParams[0] = cmdParam1;
	        cmdParams[1] = cmdParam2;        
	        cmdParams[2] = cmdParam3;
	        cmdParams[3] = cmdParam4;
	        cmdParams[4] = cmdParam5;
	        trackingNo = AddCommand(cmd, cmdParams);
	        if (trackingNo != null) {
	        	noOfExecutedCommands = noOfExecutedCommands + 1; 
	        	AddFwDownload(executionDevices[i].DeviceID, firmwareType);
	        }
    	}         
    }
    var operationMessage = noOfExecutedCommands + " firmware upgrade operation(s) started! ";
    if (devicesWithFwInProgress.length != 0){
    	devicesWithFwInProgress = devicesWithFwInProgress.substring(0, devicesWithFwInProgress.length-2);
    	operationMessage = operationMessage + " The following devices already have a firmware upgrade in progress: " + devicesWithFwInProgress;
    }    	    
    CreateCookie("FW_UPLOAD_MESSAGE",operationMessage, null); 	       
    document.location.href = "devicefw.html";   		
}

function ShowHideHelp(op) {
    var divHelp = document.getElementById("divHelp");
    
    if (op == "open") {
        divHelp.style.display = "";
    } else {
        divHelp.style.display = "none";
    }
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
    BuildDeviceTable();
}

function SetOrderSignForCurrentView(orderBy) {
    var sign = OrderDirection == OD_ASC ? "&#9650;" : "&#9660;";
	var spnCurrentOrder = document.getElementById("col" + orderBy);
	spnCurrentOrder.innerHTML = sign;
}