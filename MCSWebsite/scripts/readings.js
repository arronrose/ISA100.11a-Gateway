var CurrentPage = 1;
var TotalPages = 0;
var TotalNoOfRows = 0;

//filters
var deviceId = null; //might come from QS also, read QS on InitREadingsPage
var Channel = null;
var ReadingType = null;
 
function InitReadingsPage() {
    SetPageCommonElements();
    
    //obtain deviceId;
    var url = parent.document.URL;
    deviceId = Number(url.substring(url.indexOf('?') + 10, url.length));
    if (isNaN(deviceId)) {  //make sure qs was not altered (is a number)
        deviceId = null;
    }
        
    InitJSON();
    PopulateFilters();
    ReadFilters();
    BuildReadingsTable();
}

function PopulateFilters() {
    //devices
    var ddlDevice = document.getElementById("ddlDevice");     
    var devices = GetDeviceListByType(GetDeviceTypeArrayForReadingsReport(), null);
    
    ddlDevice.options[0] = new Option("All", "");
    if (devices != null) {
        for(i = 0; i < devices.length; i++) {
            ddlDevice.options[i+1] = new Option(devices[i].Address64, devices[i].DeviceID);
            if (deviceId != null && deviceId == devices[i].DeviceID) {
                //select device from QS
                ddlDevice.options[i+1].selected = "selected";     
            }
        }
    }
    
    var ddlChannel = document.getElementById("ddlChannel");
    ddlChannel.options[0] = new Option("All","");

    var ddlReadingType = document.getElementById("ddlReadingType");   
    ddlReadingType.options[0] = new Option("All", "");
    ddlReadingType.options[1] = new Option(GetReadingTypeName(RT_OnDemand), RT_OnDemand);
    ddlReadingType.options[2] = new Option(GetReadingTypeName(RT_PublishSubscribe), RT_PublishSubscribe);    
}

function DeviceSelectionChanged() {
    ClearList("ddlChannel");
    
    var ddlChannel = document.getElementById("ddlChannel");
    var ddlDevice = document.getElementById("ddlDevice");
    var deviceId = ddlDevice.value;
    
    ddlChannel.options[0] = new Option("All","");
    
    if (deviceId != "") {
        var channels = GetChannelsForDevice(deviceId);
        if (channels != null) {
            for(i = 0;i < channels.length; i++) {
                ddlChannel.options[i+1] = new Option(channels[i].ChannelName,channels[i].ChannelNo);
            }
        }
    }
}

function ReadFilters() {
    deviceId = document.getElementById("ddlDevice").value;
    if (deviceId.length == 0) {
        deviceId = null;
    }
    Channel = document.getElementById("ddlChannel").value;
    if (Channel.length == 0) {
        Channel = null;
    }
    ReadingType = document.getElementById("ddlReadingType").value;
    if (ReadingType.length == 0) {
        ReadingType = null;
    }
    PageSize = document.getElementById("ddlRowsPerPage").value;
}

function Search() {
    ReadFilters();
    CurrentPage = 1;
    BuildReadingsTable();
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

function BuildReadingsTable() {
    TotalNoOfRows = GetReadingsCount(deviceId, Channel, ReadingType);    
    if (TotalNoOfRows > 0) {
        var data = GetReadings(deviceId, Channel, ReadingType, CurrentPage, PageSize, TotalNoOfRows, false);
        if (data != null) {
            document.getElementById("tblReadings").innerHTML = TrimPath.processDOMTemplate("readings_jst", data);
            document.getElementById("btnExport").disabled = "";       
            document.getElementById("hQuery").value = GetReadings(deviceId, Channel, ReadingType, 1, 5000, TotalNoOfRows, true);
        } else {
            document.getElementById("tblReadings").innerHTML = //"<span class='labels'>No records !</span>";
                "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"950px\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
    					"<tr><td class=\"tableSubHeader\" style=\"width: 160px; text-align: center;\">EUI-64 Address</td>" +
    						"<td class=\"tableSubHeader\" style=\"width: 160px; text-align: center;\">Timestamp</td>" +
    						"<td class=\"tableSubHeader\" style=\"width: 150px; text-align: center;\">Channel Name</td>" +
    						"<td class=\"tableSubHeader\" style=\"width: 100px; text-align: center;\">Value</td>" +
    						"<td class=\"tableSubHeader\" style=\"width: 150px; text-align: center;\">Unit Of Measurement</td>" +
    						"<td class=\"tableSubHeader\" style=\"width: 130px; text-align: center;\">Reading type</td></tr>" +
                        "<tr><td colspan=\"6\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
                "</td></tr></table>";
            document.getElementById("hQuery").value = "";
            document.getElementById("btnExport").disabled = "disabled";
        }
    } else {
    	document.getElementById("spnPageNumber").innerHTML = "";
        document.getElementById("tblReadings").innerHTML = //"<span class='labels'>No records !</span>";
                "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"950px\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
    					"<tr><td class=\"tableSubHeader\" style=\"width: 160px; text-align: center;\">EUI-64 Address</td>" +
    						"<td class=\"tableSubHeader\" style=\"width: 160px; text-align: center;\">Timestamp</td>" +
    						"<td class=\"tableSubHeader\" style=\"width: 150px; text-align: center;\">Channel Name</td>" +
    						"<td class=\"tableSubHeader\" style=\"width: 100px; text-align: center;\">Value</td>" +
    						"<td class=\"tableSubHeader\" style=\"width: 150px; text-align: center;\">Unit Of Measurement</td>" +
    						"<td class=\"tableSubHeader\" style=\"width: 130px; text-align: center;\">Reading type</td></tr>" +
                        "<tr><td colspan=\"6\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
                "</td></tr></table>";
        document.getElementById("hQuery").value = "";
        document.getElementById("btnExport").disabled = "disabled";
    };
    SetPager();    
}

function PageNavigate(pageNo) {
    CurrentPage = pageNo;
    BuildReadingsTable();
}
