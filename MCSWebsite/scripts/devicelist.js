var LastRefreshedString;
var LastRefreshedDate;
var RefreshDeviceInfoActive = false; 

//order direction
var OD_ASC = "ASC";
var OD_DESC = "DESC";

var CurrentPage = 1;
var PageSize = 10;
var TotalPages = 0;
var TotalNoOfRows = 0;
var ShowDevicesFilter = 0;
var OrderBy = 1;
var OrderDirection = OD_ASC;

// filtered fields
var EUI64Address = "";
var DeviceTag = ""	;

function InitDeviceListPage() {
    SetPageCommonElements();
    InitJSON();
	PreloadImages();	
	if (IsParameter("setState")) {
        LoadPageState();
    } else {
    	CurrentPage = 1;    	
    } 
    ReadFilters();	
	BuildDeviceTable();   
	DisplayLastRefreshDate();
	CheckAutoRefresh();
	setInterval("DisplaySecondsAgo()", 1000);
}

function AutoRefresh() { 
    var cmdLastValidCmdInProgress = GetLastCommandInProgress(CTC_GetDeviceList);
	if (cmdLastValidCmdInProgress != null) {					
	   RefreshDeviceInfoActive = true;	
    } else {						
        RefreshDeviceInfoActive = false;      
	}               
	DisplayLastRefreshDate();
	if (RefreshDeviceInfoActive) {		
		setTimeout(AutoRefresh, RefreshInterval);
	}
}

function RefreshPage() {
    var trackingNo = AddGetDeviceListCommand();      
    if (trackingNo != null) {
        AutoRefresh();           
        BuildDeviceTable();  
    }
}

function ShowDevicesChanged() {    
    ReadFilters();
    SavePageState();
    CurrentPage = 1;
    BuildDeviceTable();
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

function BuildDeviceTable() {
	TotalNoOfRows = GetDeviceCount(ShowDevicesFilter, EUI64Address, GetHexString(DeviceTag));	
    if (TotalNoOfRows > 0) {
        var data = GetDevicePage(PageSize, CurrentPage, TotalNoOfRows, ShowDevicesFilter, OrderBy, OrderDirection, EUI64Address, GetHexString(DeviceTag)); 
        if (data != null) {
            document.getElementById("tblDevices").innerHTML = TrimPath.processDOMTemplate("devices_jst", data);
            SetOrderSignForCurrentView(OrderBy);          
            SavePageState();
        } else {
            document.getElementById("tblDevices").innerHTML =
                "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"950px\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
     				  "<tr><td class=\"tableSubHeader\" style=\"width:50px;\" align=\"center\">&nbsp;</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:150px;\" align=\"left\">EUI-64 Address</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:180px;\" align=\"left\">IPv6 Address</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:120px;\" align=\"left\">Tag</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:100px;\" align=\"left\">Revision</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:120px;\" align=\"left\">Role/Model</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:100px;\" align=\"left\">Status</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:100px;\" align=\"center\">Last read</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:30px;\" align=\"center\"></td></tr>" +
                     "<tr><td colspan=\"9\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
                "</td></tr></table>";
        }              
    } else {
    	document.getElementById("spnPageNumber").innerHTML = "";
        document.getElementById("tblDevices").innerHTML = //"<span class='labels'>No records !</span>";
            "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"950px\"><tr><td align=\"left\">" +
			"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
				  "<tr><td class=\"tableSubHeader\" style=\"width:50px;\" align=\"center\">&nbsp;</td>" +
				  "<td class=\"tableSubHeader\" style=\"width:150px;\" align=\"left\">EUI-64 Address</td>" +
				  "<td class=\"tableSubHeader\" style=\"width:180px;\" align=\"left\">IPv6 Address</td>" +
				  "<td class=\"tableSubHeader\" style=\"width:120px;\" align=\"left\">Tag</td>" +
				  "<td class=\"tableSubHeader\" style=\"width:100px;\" align=\"left\">Revision</td>" +
				  "<td class=\"tableSubHeader\" style=\"width:120px;\" align=\"left\">Role/Model</td>" +
				  "<td class=\"tableSubHeader\" style=\"width:100px;\" align=\"left\">Status</td>" +
				  "<td class=\"tableSubHeader\" style=\"width:100px;\" align=\"center\">Last read</td>" +
				  "<td class=\"tableSubHeader\" style=\"width:30px;\" align=\"center\"></td></tr>" +
             "<tr><td colspan=\"9\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
        "</td></tr></table>";
    }
    SetPager();
}

function DeleteDevice(deviceId) {
    if (!confirm("Are you sure you want to delete the device and all the related data ?")) {
        return;
    }
    DeleteDeviceAndData(deviceId);
    BuildDeviceTable();
}

function PageNavigate(pageNo) {
	PageSize = document.getElementById("ddlRowsPerPage").value;
    CurrentPage = pageNo;
    BuildDeviceTable();
}

function SavePageState() {
    CreateCookie("DEVICELIST_CURRENTPAGE", CurrentPage, 1); 
    CreateCookie("DEVICELIST_SHOWDEVICESFILTER", ShowDevicesFilter, 1);     
    CreateCookie("DEVICELIST_EUI64ADDRESS", EUI64Address, 1);
    CreateCookie("DEVICELIST_DEVICETAG", DeviceTag, 1);    
    CreateCookie("DEVICELIST_ORDERBY", OrderBy, 1); 
    CreateCookie("DEVICELIST_ORDERDIRECTION", OrderDirection, 1);
    CreateCookie("DEVICELIST_PAGESIZE", PageSize, 1);
}

function LoadPageState() {
    if (ReadCookie("DEVICELIST_CURRENTPAGE") != null) {
        CurrentPage = ReadCookie("DEVICELIST_CURRENTPAGE");        
    }
    if (ReadCookie("DEVICELIST_SHOWDEVICESFILTER") != null) {
        var ddlShowDevices = document.getElementById("ddlShowDevices");
    	ShowDevicesFilter = ReadCookie("DEVICELIST_SHOWDEVICESFILTER");
        ddlShowDevices.selectedIndex = ShowDevicesFilter;
    }
    if (ReadCookie("DEVICELIST_EUI64ADDRESS") != null) {
    	EUI64Address = ReadCookie("DEVICELIST_EUI64ADDRESS");    
    	document.getElementById("txtEUI64Address").value = EUI64Address;
    }
    if (ReadCookie("DEVICELIST_DEVICETAG") != null) {
    	DeviceTag = ReadCookie("DEVICELIST_DEVICETAG");    
    	document.getElementById("txtDeviceTag").value = DeviceTag;
    }        
    if (ReadCookie("DEVICELIST_ORDERBY") != null) {
        OrderBy = ReadCookie("DEVICELIST_ORDERBY");
    }
    if (ReadCookie("DEVICELIST_ORDERDIRECTION") != null) {
        OrderDirection = ReadCookie("DEVICELIST_ORDERDIRECTION");
    }
    if (ReadCookie("DEVICELIST_PAGESIZE") != null) {
        PageSize = ReadCookie("DEVICELIST_PAGESIZE");
        document.getElementById("ddlRowsPerPage").value = PageSize;
    }    
}

function ReadFilters() {	
    document.getElementById("txtEUI64Address").value = document.getElementById("txtEUI64Address").value.trim()
    document.getElementById("txtDeviceTag").value = document.getElementById("txtDeviceTag").value.trim()

    ShowDevicesFilter = parseInt(document.getElementById("ddlShowDevices").value);
    EUI64Address = document.getElementById("txtEUI64Address").value;
    DeviceTag = document.getElementById("txtDeviceTag").value;
    PageSize = document.getElementById("ddlRowsPerPage").value;
}

function RowsPerPageChanged() {
    ReadFilters();
    CurrentPage = 1;   
    BuildDeviceTable();
}

function Reset() {
    EUI64Address = "";
    DeviceTag = "";
    PageSize = 10
    ShowDevicesFilter = 0; 
    document.getElementById("txtEUI64Address").value = EUI64Address;
    document.getElementById("txtDeviceTag").value = DeviceTag;
    document.getElementById("ddlRowsPerPage").selectedIndex = 0;
    document.getElementById("ddlShowDevices").selectedIndex = ShowDevicesFilter; ;
    CurrentPage = 1;
    BuildDeviceTable();
}

function AddGetDeviceListCommand(deviceId) {
	var gateway = GetGatewayDevice();
    
	var cmd = new Command();
	cmd.DeviceID = gateway.DeviceID;
	cmd.CommandTypeCode = CTC_GetDeviceList;
	cmd.CommandStatus = CS_New;
	cmd.TimePosted = ConvertFromJSDateToSQLiteDate(GetUTCDate());
    cmd.ParametersDescription = "";

 	return AddCommand(cmd, "");
}

function DisplaySecondsAgo() {
    if (RefreshDeviceInfoActive){
        document.getElementById("spnRefreshDate").innerHTML = LastRefreshedString +  (LastRefreshedString != NAString ? "  (" + Math.ceil((GetUTCDate().getTime() - LastRefreshedDate.getTime()) / 1000) + " seconds ago)" : "") + '. Refreshing ...';
    } 
    else {
        document.getElementById("spnRefreshDate").innerHTML = LastRefreshedString +  (LastRefreshedString != NAString ? "  (" + Math.ceil((GetUTCDate().getTime() - LastRefreshedDate.getTime()) / 1000) + " seconds ago)" : ""); 
    };
}

function Search(){
	DisplayLastRefreshDate();
	PreloadImages();    
    ReadFilters();
    SavePageState();
    CurrentPage = 1;
    BuildDeviceTable();	
}

function DisplayLastRefreshDate(){ 
	var lastRespondedCommand = GetLastCommandResponded(CTC_GetDeviceList);
	if (lastRespondedCommand != null && lastRespondedCommand != NullString) {
	    LastRefreshedDate = ConvertFromSQLiteDateToJSDateObject(lastRespondedCommand.TimeResponded);
	    LastRefreshedString = lastRespondedCommand.TimeResponded;
	} else {
	    LastRefreshedString = NAString;	    
	}
}

function CheckAutoRefresh() {
	var tmpDate = new Date()
	tmpDate = ConvertFromSQLiteDateToJSDateObject(LastRefreshedString == NAString ? '1970-01-01 00:00:00' : LastRefreshedString);
	if (tmpDate < AddSecondsToDate(GetUTCDate(),-AUTO_REFRESH_DATA_OLDER_THEN)){
		RefreshPage();
	}  
}
