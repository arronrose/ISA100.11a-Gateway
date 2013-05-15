var methods = ["sqldal.execute", "user.logout", "sqldal.open", "sqldal.close", "user.isValidSession"];

var CurrentPage = 1;
var TotalPages = 0;
var TotalNoOfRows = 0;
 
//filters
var deviceId = null;
var category = null;
var priority = null;
var clasa = null;
var StartTime = null;
var EndTime = null;
var PageSize = 10;

//order direction
var OD_ASC = "ASC";
var OD_DESC = "DESC";

var OrderBy = 2;  //order column number
var OrderDirection = OD_DESC;

function InitAlertsLogPage() {
    SetPageCommonElements();   
    InitJSON();    
    PopulateFilters();
    ReadFilters();        
    BuildAlarmsLogTable();
}

function BuildAlarmsLogTable() {    
    TotalNoOfRows = GetAlertsCount(deviceId, category, priority, clasa, StartTime, EndTime);
    if (TotalNoOfRows > 0) {
        //obtain data from database
        var data = GetAlertsPage(deviceId, category, priority, clasa, StartTime, EndTime, CurrentPage, PageSize, TotalNoOfRows, false, OrderBy, OrderDirection)        
        if (data != null) {
            document.getElementById("tblAlertsLog").innerHTML = TrimPath.processDOMTemplate("alerts_jst", data);            
            document.getElementById("btnExport").disabled = "";
            SetOrderSignForCurrentView(OrderBy);                   
            document.getElementById("hQuery").value = GetAlertsPage(deviceId, category, priority, clasa, StartTime, EndTime, 1, 5000, TotalNoOfRows, true, 2, OD_DESC)
        } else {
            document.getElementById("tblAlertsLog").innerHTML =
                "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"945px\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
     				  "<tr><td class=\"tableSubHeader\" style=\"width: 140p  x;\" align=\"left\">EUI-64 Address</td>" +						  
						  "<td class=\"tableSubHeader\" style=\"width: 30px;\" align=\"left\">TsapID</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 50px;\" align=\"left\">ObjectID</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 130px;\" align=\"left\">Time</td>" +						  						  					  
						  "<td class=\"tableSubHeader\" style=\"width: 40px;\" align=\"center\">Class</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 70px;\" align=\"center\">Direction</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 160px;\" align=\"left\">Category</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 20px;\" align=\"left\">Type</td>" +						  
						  "<td class=\"tableSubHeader\" style=\"width: 60px;\" align=\"center\">Priority</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 200px;\" align=\"left\">Value</td>" +						  
                        "<tr><td colspan=\"10\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
                "</td></tr></table>";           	
            document.getElementById("hQuery").value = "";
            document.getElementById("btnExport").disabled = "disabled";
        }
    } else {
        document.getElementById("tblAlertsLog").innerHTML =
                "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"945px\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
     				  "<tr><td class=\"tableSubHeader\" style=\"width: 140px;\" align=\"left\">EUI-64 Address</td>" +     					
						  "<td class=\"tableSubHeader\" style=\"width: 30px;\" align=\"left\">TsapID</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 50px;\" align=\"left\">ObjectID</td>" +     					
						  "<td class=\"tableSubHeader\" style=\"width: 130px;\" align=\"left\">Time</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 40px;\" align=\"center\">Class</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 70px;\" align=\"center\">Direction</td>" +						  
						  "<td class=\"tableSubHeader\" style=\"width: 160px;\" align=\"left\">Category</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 20px;\" align=\"left\">Type</td>" +						  
						  "<td class=\"tableSubHeader\" style=\"width: 60px;\" align=\"center\">Priority</td>" +
						  "<td class=\"tableSubHeader\" style=\"width: 200px;\" align=\"left\">Value</td>" +
						"<tr><td colspan=\"10\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
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
    BuildAlarmsLogTable();
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

   //categories
    var ddlCategory = document.getElementById("ddlCategory");
    ddlCategory.options[0] = new Option("All","");    
    var categoryArr = GetAlertCategoriesArray();
    if (categoryArr != null) {
        for (i = 0; i < categoryArr.length; i++) {
            ddlCategory.options[i+1] = new Option(GetAlertCategoryName(categoryArr[i]),categoryArr[i]);
        }
    }

    //priorities
    var ddlPriority = document.getElementById("ddlPriority");
    ddlPriority.options[0] = new Option("All","");    
    var arrPriority = GetAlertPrioritiesArray();
    if (arrPriority != null) {
        for (i = 0; i < arrPriority.length; i++) {
            ddlPriority.options[i+1] = new Option(GetAlertPriorityName(arrPriority[i]),arrPriority[i]);
        }
    }

    //classes
    var ddlClass = document.getElementById("ddlClass");
    ddlClass.options[0] = new Option("All","");    
    var arrClass = GetAlertClassArray();
    if (arrClass != null) {
        for (i = 0; i < arrClass.length; i++) {
            ddlClass.options[i+1] = new Option(GetAlertClassName(arrClass[i]),arrClass[i]);
        }
    }

    //set start date = now - 1 hour
   var dateNow = GetUTCDate()
    dateNow.setHours(dateNow.getHours() - 24);
    var txtStartDate = document.getElementById("txtStartDate");
    var txtStartDateHours = document.getElementById("txtStartDateHours");
    var txtStartDateMinutes = document.getElementById("txtStartDateMinutes");
    txtStartDate.value = (dateNow.getMonth() + 1).toString() + "/" + dateNow.getDate() + "/" + dateNow.getFullYear();
    var ddlStartDateAMPM = document.getElementById("ddlStartDateAMPM");
    
    if (dateNow.getHours() > 12) {
        txtStartDateHours.value = dateNow.getHours() % 12;
        ddlStartDateAMPM.selectedIndex = 1;
    } else {   
        txtStartDateHours.value = dateNow.getHours();
        ddlStartDateAMPM.selectedIndex = 0;
    }
    txtStartDateMinutes.value = dateNow.getMinutes();
}

function ReadFilters() {
    deviceId = document.getElementById("ddlDevice").value;
    if (deviceId.length == 0) {
        deviceId = null;
    }
    category = document.getElementById("ddlCategory").value;
    if (category.length == 0) {
        category = null;
    } else {
        var categoryArr = new Array(1);  //we need an array for query param
        categoryArr[0] = category;
        category = categoryArr;
    }
    
    priority = document.getElementById("ddlPriority").value;
    if (priority.length == 0) {
        priority = null;
    }

    clasa = document.getElementById("ddlClass").value;
    if (clasa.length == 0) {
        clasa = null;
    }
    
    StartTime = ReadDateTime("txtStartDate", "txtStartDateHours", "txtStartDateMinutes", "ddlStartDateAMPM", true);
    EndTime = ReadDateTime("txtEndDate", "txtEndDateHours", "txtEndDateMinutes", "ddlEndDateAMPM", false);
    PageSize = document.getElementById("ddlRowsPerPage").value;
}

function Search() {
    if (ValidateFilters()) {
    ReadFilters();
    CurrentPage = 1;
    BuildAlarmsLogTable();
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
    BuildAlarmsLogTable();
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
