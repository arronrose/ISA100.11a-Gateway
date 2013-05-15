var deviceId = null;

//filters
var RegistrationStatus = null;
var StartTime = null;
var EndTime = null;
var PageSize = 10;

//order direction
var OD_ASC = "ASC";
var OD_DESC = "DESC";

var CurrentPage = 1;
var TotalPages = 0;
var TotalNoOfRows = 0;
var OrderBy = 1;  //order column number
var OrderDirection = OD_ASC;

function InitRegistrationLogPage() {
    SetPageCommonElements();    
    InitJSON();    
    var url = parent.document.URL;
    deviceId = Number(url.substring(url.indexOf('?') + 10, url.length));    
    if (!isNaN(deviceId)) {  //make sure qs was not altered (is a number)
        dev = GetDeviceInformation(deviceId);
        if (dev != null) {
            SetData(dev);
            SetDeviceTabs(deviceId, dev.DeviceStatus, dev.DeviceRoleID, 2);
        }
        PopulateFilters();
        BuildRegistrationLogTable();
    }
}

function Search() {
    if (ValidateFilters()) {
        ReadFilters();
        CurrentPage = 1;
        BuildRegistrationLogTable();
    }
}

function DeleteHistory() {
    if (confirm("Are you sure you want to delete device history ?")) {
        DeleteDeviceHistory(deviceId);
    }
    CurrentPage = 1;
    BuildRegistrationLogTable();
}

function ReadFilters() {
    RegistrationStatus = document.getElementById("ddlRegistrationStatus").value;
    if (RegistrationStatus.length == 0) {
        RegistrationStatus = null;
    }
    
    StartTime = ReadDateTime("txtStartDate", "txtStartDateHours", "txtStartDateMinutes", "ddlStartDateAMPM", true);
    EndTime = ReadDateTime("txtEndDate", "txtEndDateHours", "txtEndDateMinutes", "ddlEndDateAMPM", false);
    PageSize = document.getElementById("ddlRowsPerPage").value;
}

function BuildRegistrationLogTable() {
    TotalNoOfRows = GetDeviceHistoryCount(deviceId, StartTime, EndTime, RegistrationStatus);    
    document.getElementById("btnDelete").style.display = "";    
    if (TotalNoOfRows > 0) {
        var data = GetDeviceHistoryPage(deviceId, StartTime, EndTime, RegistrationStatus, OrderBy, OrderDirection, PageSize, CurrentPage, TotalNoOfRows);
        document.getElementById("tblRegistrationLog").innerHTML = TrimPath.processDOMTemplate("registrationlog_jst", data);    
        SetOrderSignForCurrentView(OrderBy);
    } else {
        document.getElementById("btnDelete").style.display = "none";
        document.getElementById("spnPageNumber").innerHTML = "";
        document.getElementById("tblRegistrationLog").innerHTML = //"<span class='labels'>No records !</span>";
                 "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"600px\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
     					"<tr><td class=\"tableSubHeader\" style=\"width:300px;\" align=\"center\">Timestamp</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:300px;\" align=\"center\">Device Status</td></tr>" +
                        "<tr><td colspan=\"2\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
                "</td></tr></table>";
   }
   SetPager();
}

function PopulateFilters() {
    //registration status
    var ddlRegistrationStatus = document.getElementById("ddlRegistrationStatus");
    ddlRegistrationStatus.options[0] = new Option("All", "");
    ddlRegistrationStatus.options[1] = new Option(GetDeviceStatusName(DS_SecJoinRequestReceived), DS_SecJoinRequestReceived);
    ddlRegistrationStatus.options[2] = new Option(GetDeviceStatusName(DS_SecJoinResponseSent), DS_SecJoinResponseSent);
    ddlRegistrationStatus.options[3] = new Option(GetDeviceStatusName(DS_SmJoinReceived), DS_SmJoinReceived);
    ddlRegistrationStatus.options[4] = new Option(GetDeviceStatusName(DS_SmJoinResponseSent), DS_SmJoinResponseSent);
    ddlRegistrationStatus.options[5] = new Option(GetDeviceStatusName(DS_SmContractJoinReceived), DS_SmContractJoinReceived);
    ddlRegistrationStatus.options[6] = new Option(GetDeviceStatusName(DS_SmContractJoinResponseSent), DS_SmContractJoinResponseSent);
    ddlRegistrationStatus.options[7] = new Option(GetDeviceStatusName(DS_SecConfirmReceived), DS_SecConfirmReceived);
    ddlRegistrationStatus.options[8] = new Option(GetDeviceStatusName(DS_SecConfirmResponseSent), DS_SecConfirmResponseSent);
    ddlRegistrationStatus.options[9] = new Option(GetDeviceStatusName(DS_JoinedAndConfigured), DS_JoinedAndConfigured);
    ddlRegistrationStatus.options[10] = new Option(GetDeviceStatusName(DS_NotJoined), DS_NotJoined);

     //items per page
    var ddlRowsPerPage = document.getElementById("ddlRowsPerPage");   
    
    ddlRowsPerPage.options[0] = new Option("10","10");    
    ddlRowsPerPage.options[1] = new Option("15","15");      
    ddlRowsPerPage.options[2] = new Option("25","25");  
    ddlRowsPerPage.options[3] = new Option("100", "100");
}

function SetData(dev) {
    document.getElementById("spnEUI64").innerHTML = dev.Address64;
    document.getElementById("spnIPv6").innerHTML = dev.Address128;
}

function PageNavigate(pageNo) {
	PageSize = document.getElementById("ddlRowsPerPage").value;
    CurrentPage = pageNo;
    BuildRegistrationLogTable();
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
    BuildRegistrationLogTable();
}

function SetOrderSignForCurrentView(orderBy) {
    var sign = OrderDirection == OD_ASC ? "&#9650;" : "&#9660;";
	var spnCurrentOrder = document.getElementById("col" + orderBy);
	spnCurrentOrder.innerHTML = sign;
}

function ValidateFilters() {
    if (!ValidateDateTime("txtStartDate", "txtStartDateHours", "txtStartDateMinutes")) {
        return false;
    }
    if (!ValidateDateTime("txtEndDate", "txtEndDateHours", "txtEndDateMinutes")) {
        return false;
    }
    
    var st = ReadDateTime("txtStartDate", "txtStartDateHours", "txtStartDateMinutes", "ddlStartDateAMPM", true);
    var et = ReadDateTime("txtEndDate", "txtEndDateHours", "txtEndDateMinutes", "ddlEndDateAMPM", false);
    
    if (st != null && et != null) {
        st = new Date(st);
        et = new Date(et);
        if (st > et) {
            alert("Start Time must be less than or equal to End Time !");
            return false;
        }
    }
    return true;
}

function BackButtonClicked() {
    document.location.href = "devicelist.html?setState";
}
