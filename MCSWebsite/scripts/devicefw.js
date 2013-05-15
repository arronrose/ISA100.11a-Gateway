var CurrentPage = 1;
var TotalPages = 0;
var TotalNoOfRows = 0; 
var PageSize = 10;
var devAddress64 = null;
var fwDownloadStatus = null;
var fwDownloadType = null;
var paper = null;
var chkRefresh = null;
//order direction
var OD_ASC = "ASC";
var OD_DESC = "DESC";
var OrderBy = 2;  //order column number
var OrderDirection = OD_DESC;

// USED FOR REFRESH MECHANISM
var starttime;
var nowtime;
var secondssinceloaded = 0;
var timer;

function InitDeviceFwPage() {
    SetPageCommonElements();
    InitJSON();    
    PopulateFilters();
   	ResetFilters();
    ReadFilters();  
    BuildFirmwareDownloadsTable();
    start();    
    if (ReadCookie("FW_UPLOAD_MESSAGE")){
        document.getElementById("spnOperationResultAdd").innerHTML =  ReadCookie("FW_UPLOAD_MESSAGE");
        EraseCookie("FW_UPLOAD_MESSAGE");    	
    }
}

function Refresh(){
	ReadFilters();  
    BuildFirmwareDownloadsTable();
    start();
}

function BuildFirmwareDownloadsTable() {
	TotalNoOfRows = GetFwDownloadCount(devAddress64, fwDownloadType, fwDownloadStatus);
    if (TotalNoOfRows > 0) {
        data = GetFwDownloadPage(devAddress64, fwDownloadType, fwDownloadStatus, CurrentPage, PageSize, TotalNoOfRows, false, OrderBy, OrderDirection);                
        document.getElementById("tblFirmwareDownloads").innerHTML = TrimPath.processDOMTemplate("fwdownload_jst", data);        
        DisplayProgressBar(data);
        SetOrderSignForCurrentView(OrderBy);
        document.getElementById("btnExport").disabled = "";      
        document.getElementById("hQuery").value = GetFwDownloadPage(devAddress64, fwDownloadType, fwDownloadStatus, 1, 5000, TotalNoOfRows, true, OrderBy, OrderDirection);
    } else {
        document.getElementById("tblFirmwareDownloads").innerHTML = 
                "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"950px\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
     					"<tr>" +						 
		                    "<td class=\"tableSubHeader\" style=\"width: 140px;\" align=\"center\">EUI-64 Address</td>"+
							"<td class=\"tableSubHeader\" style=\"width: 70px;\" align=\"left\">Type</td>"+
							"<td class=\"tableSubHeader\" style=\"width: 70px;\" align=\"left\">Status</td>"+
							"<td class=\"tableSubHeader\" style=\"width: 100px;\" align=\"left\">Completed(%)</td>"+
							"<td class=\"tableSubHeader\" style=\"width: 110px;\" align=\"center\">Avg (msg/min)</td>"+
							"<td class=\"tableSubHeader\" style=\"width: 75px;\" align=\"center\">Remaining (hh:mm:ss)</td>"+
							"<td class=\"tableSubHeader\" style=\"width: 75px;\" align=\"center\">Duration (hh:mm:ss)</td>"+
		        			"<td class=\"tableSubHeader\" style=\"width: 140px;\" align=\"center\">Started On</td>"+
							"<td class=\"tableSubHeader\" style=\"width: 140px;\" align=\"center\">Completed On</td>"+
							"<td class=\"tableSubHeader\" style=\"width: 30px;\" align=\"center\">&nbsp;</td>"+							
                        "<tr>" +
                        "<td colspan=\"9\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
                "</td></tr></table>";
        document.getElementById("hQuery").value = "";
        document.getElementById("btnExport").disabled = "disabled";
    }
    SetPager();    
}

function PageNavigate(pageNo) {
    CurrentPage = pageNo;
    BuildFirmwareDownloadsTable();
}

function Export() {
    try {    
        var service = new jsonrpc.ServiceProxy(serviceURL, methods);
        service.user.isValidSession();        
        var q = document.getElementById("hQuery").value;
        document.form1.call1.value = jsonrpc.JSONRPCMethod("a").jsonRequest(1,"sqldal.getCsv", {query:q}) ;
        return 1;
    } catch (e) {
        HandleException(e, 'Unexpected error exporting data !')
    }; 
}

function uploadComplete(text) {
    if (!text.result) {
        alert("Export failed !");
    }
}

function PopulateFilters() {   	
    //fw download status
    var ddlDownloadStatus = document.getElementById("ddlDownloadStatus");
    ddlDownloadStatus.options[0] = new Option("All","");    
    ddlDownloadStatus.options[1] = new Option(GetFirmwareDownloadStatus(FDS_InProgress), FDS_InProgress);      
    ddlDownloadStatus.options[2] = new Option(GetFirmwareDownloadStatus(FDS_Canceling), FDS_Canceling);
    ddlDownloadStatus.options[3] = new Option(GetFirmwareDownloadStatus(FDS_Completed), FDS_Completed);  
    ddlDownloadStatus.options[4] = new Option(GetFirmwareDownloadStatus(FDS_Failed), FDS_Failed);    
    ddlDownloadStatus.options[5] = new Option(GetFirmwareDownloadStatus(FDS_Cancelled), FDS_Cancelled);    
    
    //fw download ype
    var ddlType = document.getElementById("ddlType");
    ddlType.options[0] = new Option("All","");    
    ddlType.options[1] = new Option(GetFirmwareTypeName(FT_Device), FT_Device);      
    ddlType.options[2] = new Option(GetFirmwareTypeName(FT_AquisitionBoard), FT_AquisitionBoard);  
    ddlType.options[3] = new Option(GetFirmwareTypeName(FT_BackboneRouter), FT_BackboneRouter);
}

function ResetFilters() {
	document.getElementById("txtDevice").value = "";
	document.getElementById("ddlDownloadStatus").selectedIndex = 1;
	document.getElementById("ddlType").selectedIndex = 0;
	document.getElementById("ddlRowsPerPage").selectedIndex = 0;
	document.getElementById("chkRefresh").checked = "checked";
}

function ReadFilters() {
	devAddress64 = document.getElementById("txtDevice").value;     
    fwDownloadStatus = document.getElementById("ddlDownloadStatus").value;       
    fwDownloadType = document.getElementById("ddlType").value;       
    PageSize = document.getElementById("ddlRowsPerPage").value;
    chkRefresh = document.getElementById("chkRefresh").checked;
}

function Search() {
    ReadFilters();
    CurrentPage = 1;
    BuildFirmwareDownloadsTable();
}

function NavigateToExecute() {
    document.location.href = "fwcommands.html";
}

function NavigateToFWFiles() {
    document.location.href = "fwfilelist.html";
}

function CancelFWDownload(deviceId, deviceAddress64, fwType, startedOn, status){
	if (status == FDS_InProgress){
		var cmd = new Command();	
		if (fwType == FT_Device || fwType == FT_BackboneRouter) {
			var systemManager = GetSystemManagerDevice();
			var cmdParam = new CommandParameter();
	        cmdParam.ParameterCode = CPC_CancelFirmwareUpdate_DeviceID;
	        cmdParam.ParameterValue = deviceId;
	                
	        cmd.TimePosted =  ConvertFromJSDateToSQLiteDate(GetUTCDate());        
	        cmd.DeviceID = systemManager.DeviceID;
	        cmd.CommandTypeCode = CTC_CancelFirmwareUpdate;
	        cmd.CommandStatus = CS_New;
	        cmd.ParametersDescription = GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam.ParameterCode)) + ": " + deviceAddress64; 
	   
	        AddCommand(cmd, [cmdParam]);                	
	    }
	
	    if (fwType == FT_AquisitionBoard){
	    	var gateway = GetGatewayDevice();
	    	var cmdParam1 = new CommandParameter();
	    	var cmdParam2 = new CommandParameter();
	    	cmdParam1.ParameterCode = CPC_CancelSensorBoardFirmwareUpdate_TargetDeviceID;
		    cmdParam1.ParameterValue = deviceId;
	        cmdParam2.ParameterCode = CPC_CancelSensorBoardFirmwareUpdate_RequestedCommittedBurst;
	        cmdParam2.ParameterValue = -15;
	
	        cmd.TimePosted = ConvertFromJSDateToSQLiteDate(GetUTCDate()); 
	        cmd.DeviceID = gateway.DeviceID;
	        cmd.CommandTypeCode = CTC_CancelSensorBoardFirmwareUpdate;
	        cmd.CommandStatus = CS_New;
	        cmd.ParametersDescription = GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam1.ParameterCode)) + ": " + deviceAddress64 + ", " +
	        							GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam2.ParameterCode)) + ": " + cmdParam2.ParameterValue ;
	                       
	        AddCommand(cmd, [cmdParam1, cmdParam2]);
	    }
	}
	UpdateFwDownload(deviceId, fwType, startedOn);
	Search();
}

function DisplayProgressBar(data){
	var paper = [];
	for(var i=0; i<data.length; i++){
		var holderName = "holder"+i;
	    paper[i] = Raphael(holderName, 101, 21);
	    var percent = data[i].Completed == "NULL" ? 0 : data[i].Completed;
	    var r1 = paper[i].rect(0, 3, 	 100, 14, 2);
	    var r2 = paper[i].rect(0, 3, percent, 14, 2);	
	    r1.attr({fill:"white", stroke:"black", "fill-opacity":0.10, "stroke-width":1});
	    r2.attr({fill:"blue", stroke:"black", "fill-opacity":0.40, "stroke-width":1});
	    paper[i].text(50, 12, percent+" %");
	}    	
	return true;
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
    BuildFirmwareDownloadsTable();
}


function SetOrderSignForCurrentView(orderBy) {
    var sign = OrderDirection == OD_ASC ? "&#9650;" : "&#9660;";
	var spnCurrentOrder = document.getElementById("col" + orderBy);
	spnCurrentOrder.innerHTML = sign;
}

function enableDisableRefresh(isChecked){
    if (isChecked){
        start();   
    }
    else {
        clearTimeout(timer);
    }
}

function start(){
	starttime = new Date();
    starttime = starttime.getTime();
    countdown();
}

function countdown(){
    nowtime = new Date();
    nowtime = nowtime.getTime();
    secondssinceloaded = (nowtime - starttime) / 1000;
    if (AUTO_REFRESH_DEVICE_FW_UPGRADE >= secondssinceloaded){
    	timer = setTimeout("countdown()", 1000);     
    } else {
    	clearTimeout(timer);
    	Refresh();       
    }
}

function DeleteFwDownload(deviceId, fwType, startedOn){	
	RemoveFwDownload(deviceId, fwType, startedOn);
	Search();
}