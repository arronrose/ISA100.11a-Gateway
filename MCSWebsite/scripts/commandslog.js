var methods = ["sqldal.execute", "user.logout", "sqldal.open", "sqldal.close", "user.isValidSession"];

var CurrentPage = 1;
var TotalPages = 0;
var TotalNoOfRows = 0;

//filters
var deviceId = null;
var Cmd = null;
var CmdStatus = null;
var StartTime = null;
var EndTime = null;
var ShowSystemCommands = null;
var PageSize = 10;


function InitCommandsLogPage() {
    SetPageCommonElements();   
    InitJSON();    
    PopulateFilters();
    ReadFilters();
    BuildCommandsLogTable();
}

function BuildCommandsLogTable() {
    TotalNoOfRows = GetCommandsCount(StartTime, EndTime, deviceId, CmdStatus, Cmd, ShowSystemCommands);
    if (TotalNoOfRows > 0) {
        var data = GetCommandsPage(StartTime, EndTime, deviceId, CmdStatus, Cmd, ShowSystemCommands, CurrentPage, PageSize, TotalNoOfRows, false);
        if (data != null) {
            document.getElementById("tblCommandsLog").innerHTML = TrimPath.processDOMTemplate("commands_jst", data);
            document.getElementById("btnExport").disabled = "";       
            document.getElementById("hQuery").value = GetCommandsPage(StartTime, EndTime, deviceId, CmdStatus, Cmd, ShowSystemCommands, 1, 5000, TotalNoOfRows, true);
        } else {
            document.getElementById("tblCommandsLog").innerHTML = //"<span class='labels'>No records !</span>";
                "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"950px\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
     					"<tr><td class=\"tableSubHeader\" style=\"width:  25px;\" align=\"center\">&nbsp;</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 100px;\" align=\"center\">Tracking No.</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 100px;\" align=\"center\">EUI-64 Address</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 100px;\" align=\"center\">Command</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 220px;\" align=\"center\">Parameters</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:  60px;\" align=\"center\">Status</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:  80px;\" align=\"center\">Posted Time</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:  80px;\" align=\"center\">Response Time</td>" +
		                  "<td class=\"tableSubHeader\" style=\"width: 185px;\" align=\"center\">Response</td></tr>" +
                        "<tr><td colspan=\"9\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
                "</td></tr></table>";
            document.getElementById("hQuery").value = "";
            document.getElementById("btnExport").disabled = "disabled";
        }
    } else {
    	document.getElementById("spnPageNumber").innerHTML = "";	
        document.getElementById("tblCommandsLog").innerHTML = //"<span class='labels'>No records !</span>";
                "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"950px\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
     					"<tr><td class=\"tableSubHeader\" style=\"width:  25px;\" align=\"center\">&nbsp;</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 100px;\" align=\"center\">Tracking No.</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 100px;\" align=\"center\">EUI-64 Address</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 100px;\" align=\"center\">Command</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 220px;\" align=\"center\">Parameters</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:  60px;\" align=\"center\">Status</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:  80px;\" align=\"center\">Posted Time</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:  80px;\" align=\"center\">Response Time</td>" +
		                  "<td class=\"tableSubHeader\" style=\"width: 185px;\" align=\"center\">Response</td></tr>" +
                        "<tr><td colspan=\"9\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
                "</td></tr></table>";
        document.getElementById("hQuery").value = "";
        document.getElementById("btnExport").disabled = "disabled";
    };
    SetPager();    
}

//export related functions
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
//end export related functions

function PageNavigate(pageNo) {
    CurrentPage = pageNo;   
    BuildCommandsLogTable();
}

function PopulateFilters() {
    //devices
    var ddlDevice = document.getElementById("ddlDevice");   
    var devices = GetDeviceListByType(null, null);
    ddlDevice.options[0] = new Option("All", "");
    if (devices != null) {
        for(i = 0; i < devices.length; i++) {
            ddlDevice.options[i+1] = new Option(devices[i].Address64, devices[i].DeviceID);
        }
    }

   //commands
    var ddlCommand = document.getElementById("ddlCommand");
    ddlCommand.options[0] = new Option("All","");    
    var cmdArr = GetCommandsArray();
    if (cmdArr != null) {
        for (i = 0; i < cmdArr.length; i++) {
            ddlCommand.options[i+1] = new Option(GetCommandName(cmdArr[i],null),cmdArr[i]);
        }
    }

    //commands status
    var ddlCommandStatus = document.getElementById("ddlCommandStatus");
    ddlCommandStatus.options[0] = new Option("All","");    
    ddlCommandStatus.options[1] = new Option(GetStatusName(CS_New), CS_New);      
    ddlCommandStatus.options[2] = new Option(GetStatusName(CS_Sent), CS_Sent);  
    ddlCommandStatus.options[3] = new Option(GetStatusName(CS_Responded), CS_Responded);
    ddlCommandStatus.options[4] = new Option(GetStatusName(CS_Failed), CS_Failed);

    //items per page
    var ddlRowsPerPage = document.getElementById("ddlRowsPerPage");   
    ddlRowsPerPage.options[0] = new Option("10","10");    
    ddlRowsPerPage.options[0].selected = "selected";
    ddlRowsPerPage.options[1] = new Option("15","15");      
    ddlRowsPerPage.options[2] = new Option("25","25");  
    ddlRowsPerPage.options[3] = new Option("100", "100");     
}

function ReadFilters() {
    deviceId = document.getElementById("ddlDevice").value;
    if (deviceId.length == 0) {
        deviceId = null;
    }
    Cmd = document.getElementById("ddlCommand").value;
    if (Cmd.length == 0) {
        Cmd = null;
    } else {
        var cmdArr = new Array(1);  //we need an array for query param
        cmdArr[0] = Cmd;
        Cmd = cmdArr;
    }
    
    CmdStatus = document.getElementById("ddlCommandStatus").value;
    if (CmdStatus.length == 0) {
        CmdStatus = null;
    }
    
    ShowSystemCommands = document.getElementById("chkShowSystemGenerated").checked;
    PageSize = document.getElementById("ddlRowsPerPage").value;
}

function Search() {
	ReadFilters();
    CurrentPage = 1;
    BuildCommandsLogTable();
}