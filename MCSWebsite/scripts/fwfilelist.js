//order direction
var OD_ASC = "ASC";
var OD_DESC = "DESC";

var CurrentPage = 1;
var PageSize = 10;
var TotalPages = 0;
var TotalNoOfRows = 0;

function InitFirmwareFileListPage() {
	PageSize = 10;
    SetPageCommonElements();
    InitJSON();    
    var url = parent.document.URL;
    qsparam = url.substring(url.indexOf('?') + 1 , url.length);
    if (qsparam == "setState") {
        LoadPageState();
    }
    BuildFirmwareTable();    
}

function BuildFirmwareTable() {
	PageSize = document.getElementById("ddlRowsPerPage").value;
    TotalNoOfRows = GetFirmwareCount();    
    if (TotalNoOfRows > 0) {
        var data = GetFirmwaresPage(CurrentPage, PageSize, TotalNoOfRows, false);
        document.getElementById("tblFirmwares").innerHTML = TrimPath.processDOMTemplate("firmwares_jst", data);    
        SavePageState();
    } else {
        document.getElementById("tblFirmwares").innerHTML = //"<span class='labels'>No records !</span>";
                "<table cellpadding=\"0\" cellspacing=\"0\" class=\"containerDiv\" width=\"950px\"><tr><td align=\"left\">" +
    				"<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
     					"<tr><td class=\"tableSubHeader\" style=\"width:190px;\" align=\"left\">&nbsp;File Name</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:70px;\" align=\"center\">Version</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:120px;\" align=\"left\">Firmware Type</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:150px;\" align=\"left\">Upload Status</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:170px;\" align=\"left\">Description</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:150px;\" align=\"left\">Upload Date</td>" +
						  "<td class=\"tableSubHeader\" style=\"width:30px;\" align=\"center\">&nbsp;</td></tr>" +
                        "<tr><td colspan=\"7\" class=\"labels\" style=\"text-align:center;\">No records!</td></tr></table>" +
                "</td></tr></table>";
    };    
    SetPager();
}

function PageNavigate(pageNo) {
    CurrentPage = pageNo;
    BuildFirmwareTable();
}

function SavePageState() {
    CreateCookie("FIRMWAREFILELIST_CURRENTPAGE", CurrentPage, 1); 
}

function Refresh() {
    document.location.href = "fwfilelist.html";
}

function Back() {
    document.location.href = "devicefw.html";
}

function NavigateToUpload() {
    document.location.href = "fwdetails.html";
}

function LoadPageState() {
    if (ReadCookie("FIRMWAREFILELIST_CURRENTPAGE") != null) {
        CurrentPage = ReadCookie("FIRMWAREFILELIST_CURRENTPAGE");    
    }
}

function DeleteFWFile(firmwareID, uploadStatus) {	
	if (!confirm("Are you sure you want to delete the selected firmware file?")) {
		return;
	}
    DeleteFirmware(firmwareID);
    BuildFirmwareTable();
}