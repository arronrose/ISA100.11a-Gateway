//JSON common stuff
jsolait.baseURI = "/jsolait/";
var serviceURL = "/rpc.cgi";
//jsolait.baseURI = "http://10.16.0.245/jsolait/";
//var serviceURL = "http://10.16.0.245/rpc.cgi";
//jsolait.baseURI = "http://10.32.0.22/jsolait/";
//var serviceURL = "http://10.32.0.22/rpc.cgi";

//controls wether exception details are shown
var DEBUG = false;
var Version = "v2.5.3"

//force SSL for login page
var EnableSSL = false;

var NAString = "N/A";
var NullString = "NULL";
var WebsitePath = "/access_node/firmware/www/wwwroot/app/";

//jsonrpc object, used on most of the pages
var jsonrpc = null;

//========================================================
// Initializes jsonrpc object
//========================================================
function InitJSON() {
    try {
        jsonrpc = imprt("jsonrpc");
    } catch (e) {
        alert(e);
    }	
}

//========================================================
// Displays a green message in a given control
//========================================================
function DisplayOperationResult(controlName, msg) {
    var control = document.getElementById(controlName);
    control.className = "opResultGreen";
    control.innerHTML = msg;
}

function ClearOperationResult(controlName) { //to do, mai elegant
    DisplayOperationResult(controlName, "");   
    var control = document.getElementById(controlName);
    control.className = "";
}

//========================================================
// Handles exceptions and session timeouts
//========================================================
function HandleException(ex, msg) {
    //session expired case
    if (ex.message != null && 
        ex.message.indexOf("login first") >= 0 ||
        ex.name == "InvalidServerResponse") {
        LoggedUser = null;
        document.location.href = "login.html?exp";
        //setTimeout("", 100); //test me
        return;
    };

    if(!DEBUG) {
        alert(msg);
    } else {
        alert(msg + "\n" + "DEBUG: " + ex);
    };
}

//=======================================================
// Sets menu content, page title, header and footer text
//=======================================================
function SetPageCommonElements() {	
	document.title = "Monitoring Control System";                     
	var divHeader = document.getElementById("header");
        divHeader.innerHTML = "<table width='100%' border='0'>" +
                                "<tr>" +
                                  "<td><span style='font-size:x-large'><b>Monitoring Control System</b></span></td>" +
                                  "<td><img src='styles/images/CompanyLogo.png' width='252' height='72'></td>" +
                                  "<td><img src='styles/images/isa100wireless.png' width='151' height='69'></td>" +
                                "</tr>" +
                              "</table>";  

    var divFooter = document.getElementById("footer");
    divFooter.innerHTML = "<p>VR900 Monitoring Control System " + Version + " &nbsp;&nbsp;&nbsp;&nbsp;&nbsp; NIVIS&reg; 2010</p>";

    var divMenu = document.getElementById("columnB");    
    if (divMenu != null) {
        var menuContent = "<h3>Network</h3>" +
                          "<ul class='list1'>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='dashboard.html'>Dashboard</a></li>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='topology.html'>Topology</a></li>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='devicelist.html'>Devices</a></li>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='networkhealth.html'>Network Health</a></li>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='readings.html'>Readings</a></li>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='commandslog.html'>Commands Log</a></li>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='alertslog.html'>Alerts</a></li>" +
                           /* "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='troubleshooting.html'>Troubleshooting</a></li>" +*/                             
                          "</ul>" +
                          "<h3>Configuration</h3>" +
                            "<ul class='list1'>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='backbone.html'>Backbone Router</a></li>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='gateway.html'>Gateway</a></li>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='systemmanager.html'>System Manager</a></li>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='devicemng.html'>Device Management</a></li>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='mhconfig.html'>Monitoring Host</a></li>"+	
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='modbus.html'>MODBUS</a>" + 
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='alertsubscription.html'>Alert Subscription</a></li>" +                            
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='advanced.html'>Advanced Settings</a></li>" +
                          "</ul>" +
                          "<h3>Statistics</h3>" +
                          "<ul class='list1'>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='systemstatus.html'>System Status</a></li>" +
                          "</ul>" +   
                          "<h3>Administration</h3>" +
                          "<ul class='list1'>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='devicefw.html'>Device Firmwares</a></li>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='fwupgrade.html'>System Upgrade</a></li>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='customicons.html'>Custom Icons</a></li>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='customsettings.html'>Custom Settings</a></li>" +
                          "</ul>" +
                          "<h3>Session</h3>" +
                          "<ul class='list1'>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='changepassword.html'>Change Password</a></li>" +
                            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<a href='javascript:Logout();'>Logout</a></li>" +
                          "</ul>" +
                          "<p></p>"; 
     
        divMenu.innerHTML = menuContent;
    }
}



//=======================================================
// Cookie management
//=======================================================
function CreateCookie(name, value, days) 
{
    var expires = "";
    var date = new Date();
    
	if (days) 
    {
		date.setTime(date.getTime()+(days*24*60*60*1000));
		expires = "; expires="+date.toGMTString();
	}
    
	document.cookie = name + "=" + value + expires + "; path=/";
}

function ReadCookie(name) 
{
	var nameEQ = name + "=";
	var ca = document.cookie.split(';');
	for(var i=0; i < ca.length; i++) 
    {
		var c = ca[i];
		while (c.charAt(0)==' ') c = c.substring(1,c.length);
		if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length,c.length);
	}
	return null;
}

function EraseCookie(name) 
{
    CreateCookie(name, "", -1);
}


// this function works only if the strDate param respects the  mm/dd/yyyy HH:mi AM/PM format
function StringToDate(strDate)
{
    var d = new Date();

/*  mm/dd/yyyy HH:mi AM */
/*  0  3  6    11 14 17 */

    var hour = Number(strDate.substr(11,2));
      
    if (strDate.substr(17,2).toUpperCase() == "PM")
    {
        hour = hour + 12;
    }    

    d.setYear(strDate.substr(6,4));
    d.setMonth(Number(strDate.substr(0,2))-1, Number(strDate.substr(3,2)));
    d.setHours(hour, Number(strDate.substr(14,2)), 0, 0);

    return d;

/*  yyy-mm-dd HH:MI:ss

    d.setYear(strDate.substr(0,4));
    d.setMonth(strDate.substr(5,2)-1, strDate.substr(8,2));
    d.setHours(strDate.substr(11,2), strDate.substr(14,2), strDate.substr(17,2),0);
*/    
}



//some string manipulation functions
function trim(str, chars) {
	return ltrim(rtrim(str, chars), chars);
}

function ltrim(str, chars) {
	chars = chars || "\\s";
	return str.replace(new RegExp("^[" + chars + "]+", "g"), "");
}

function rtrim(str, chars) {
	chars = chars || "\\s";
	return str.replace(new RegExp("[" + chars + "]+$", "g"), "");
}


var STR_PAD_LEFT = 1;
var STR_PAD_RIGHT = 2;
var STR_PAD_BOTH = 3;

function pad(str, len, pad, dir) {
 
	if (typeof(len) == "undefined") { var len = 0; }
	if (typeof(pad) == "undefined") { var pad = ' '; }
	if (typeof(dir) == "undefined") { var dir = STR_PAD_RIGHT; }
 
	if (len + 1 >= str.length) {
 
		switch (dir){
 
			case STR_PAD_LEFT:
				str = Array(len + 1 - str.length).join(pad) + str;
			break;
 
			case STR_PAD_BOTH:
				var right = Math.ceil((padlen = len - str.length) / 2);
				var left = padlen - right;
				str = Array(left+1).join(pad) + str + Array(right+1).join(pad);
			break;
 
			default:
				str = str + Array(len + 1 - str.length).join(pad);
			break;
 
		} // switch
 
	}
 
	return str;
}


//reads datetime value from controls
function ReadDateTime(txtDate, txtHour, txtMinute, ddlAMPM, isStartDate)
{
    var DT = null;
    var date = trim(document.getElementById(txtDate).value);
    var hour = trim(document.getElementById(txtHour).value);
    var minute = trim(document.getElementById(txtMinute).value);
    var ampm = trim(document.getElementById(ddlAMPM).value);
    
    if (date.length == 0)
	{
		return DT;
	}
		
	var timePartNotSelected = false;
	if (hour.length == 0)
	{
		timePartNotSelected = true;
	}
	else if (minute.length == 0)
	{
		timePartNotSelected = true;
	}
		
	if (timePartNotSelected)
	{
		if (isStartDate)
		{
			DT = date + " 12:00 AM";
		}
		else
		{
			DT = date + " 11:59 PM";
		}			
	}
	else
	{
		hour = pad(hour, 2, "0", STR_PAD_LEFT);
		minute = pad(minute, 2, "0", STR_PAD_LEFT);
		DT = date + " " + hour + ":" + minute + " " + ampm;
	}		
		
	return DT;
}


function ConvertFromSQLiteDateToJSDate(strDateTime) 
{
    var strDate = strDateTime.split(' ')[0];
    var strTime = strDateTime.split(' ')[1];
    var arrDate = strDate.split('-');
    var arrTime = strTime.split(':');
    return new Date(arrDate[0], arrDate[1]-1, arrDate[2], arrTime[0], arrTime[1], arrTime[2]).format("web");
}

function ConvertFromSQLiteDateToJSDateObject(strDateTime) 
{
    var strDate = strDateTime.split(' ')[0];
    var strTime = strDateTime.split(' ')[1];
    var arrDate = strDate.split('-');
    var arrTime = strTime.split(':');
    return new Date(arrDate[0], arrDate[1]-1, arrDate[2], arrTime[0], arrTime[1], arrTime[2]);
}

//to do, use pad function [Istvan]
function toTwoCharacters(number) 
{
    return (number.toString().length == 2) ? number.toString() : '0' + number.toString();
}


function ConvertFromJSDateToSQLiteDate(date) 
{
    var strYear = date.getFullYear();
    var strMonth = toTwoCharacters(date.getMonth() + 1);
    var strDate = toTwoCharacters(date.getDate());
    var strHour = toTwoCharacters(date.getHours());
    var strMinute = toTwoCharacters(date.getMinutes());
    var strSecond = toTwoCharacters(date.getSeconds());
    return strYear + '-' + strMonth + '-' + strDate + ' ' + strHour + ':' + strMinute + ':' + strSecond;
}


function AddSecondsToDate(stringDate, noOfSeconds)
{   
    var t = new Date(stringDate);  
 
    t.setTime(t.getTime() + noOfSeconds*1000);
 
    return t;
}


function TruncateSecondsFromDate(stringDate)
{
    var t = new Date(stringDate);  
 
    t.setTime(t.getTime() - t.getSeconds()*1000);
 
    return t;
}

function ConvertArrayToSqlliteParam(arr)
{	
	if (arr == null)
	{
		alert("Commands array is null!");	
	}

	var sqliteParam = "";
	
	for(i=0;i<arr.length;i++)
	{
		sqliteParam = sqliteParam + arr[i] + ",";
	}
	if (sqliteParam.length != 0)
	{
		//remove last comma
		sqliteParam = sqliteParam.substring(0, sqliteParam.length-1);

		sqliteParam = "(" + sqliteParam + ")";
	}
	else
	{
		sqliteParam = null;
	}		
	return sqliteParam;
}


function ClearList(controlName) {
   var lst = document.getElementById(controlName);

   if (lst.length == 0){
        return;
   }
   
   for (i = lst.length; i >= 0; i--) {
      lst[i] = null;
   }
}


function IfNullStr(val)
{
	return val == "NULL" ? NAString : val;
}

//returns the index of an element in an array, -1 otherwise
function IndexInArray(arr, val)
{
    for (var i=0; i<arr.length; i++) 
    {
        if(arr[i] == val)
        { 
            return i;
        }
    }
    return -1;
}

//returns the index of an element in an array, -1 otherwise
function AddValueToArray(arr, val) {
    if (IndexInArray(arr, val) == -1) {
        arr.push(val);
    }
}

//returns the index of an element in an array, -1 otherwise
function GetPageParamValue(paramName) {
    var url = parent.document.URL;
    var nameIndex = url.indexOf(paramName);

    if (nameIndex < 0) {
        return null;
    }
    
    var beginIndex = nameIndex + paramName.length + 1;
    var endIndex = url.indexOf('&', beginIndex);

    if (endIndex < 0)
        endIndex = url.length;

    return url.substring(beginIndex, endIndex);
}

//returns the index of an element in an array, -1 otherwise
function IsParameter(paramName) {
    var url = parent.document.URL;
    var nameIndex = url.indexOf(paramName);

    if (nameIndex < 0) {
        return false;
    }
    
    return true;
}

String.prototype.trim = function () {
    return this.replace(/^\s*/, "").replace(/\s*$/, "");
}

String.prototype.trimAll = function () {
    if (this == null) {
        return null;
    }
    var trimString = "";
    for (var i=0; i<this.length; i++) {
        if (this.substr(i, 1) != " ") {
            trimString = trimString + this.substr(i, 1);
        }
    }
    return trimString;
}

function HexToDec(hexNumber) {
    return parseInt(hexNumber, 16);
}

function DecToHex(decNumber) {
    return decNumber.toString(16).toUpperCase();
}

function GetAsciiString(hexString) {
    var asciiString = "";
    
    if (hexString == null) {
        return null;
    }
    if (hexString.length % 2 != 0) {
        hexString = hexString + "0";
    }
    for (var i=0; i<hexString.length; i=i+2) {
        var intByte = HexToDec(hexString.substring(i, i+2));
        if (intByte >= 32 && intByte < 127) {
            asciiString = asciiString + String.fromCharCode(intByte);
        } else {
            return asciiString;
        }
    }
    return asciiString;
}

function GetHexString(asciiString) {
    var hexString = "";
    
    if (asciiString == null) {
        return null;
    }
    for (var i=0; i<asciiString.length; i++) {
        hexString = hexString + DecToHex(asciiString.charCodeAt(i));
    }
    return hexString;
}

function ReopenDatabase(service, openMode) {
    var response;
    
    if (!service) {
        return false;
    }
    
    try  {
        service.sqldal.close({});
    } catch(e) {
	    HandleException(e, "Error closing database connection!");
	    return false;
    }
    try {
        response = service.sqldal.open({dbFile:"/tmp/Monitor_Host.db3", mode:openMode});
        if(!response) {
            alert("Database cannot be opened in WRITE mode!")
            return false;
        }
    } catch (e) {
        HandleException(e, "Unexpected error opening database!");
        return false;
    }
    return true;
}

function GetUTCDate()
{
    d = new Date();
    return new Date(d.getTime() + d.getTimezoneOffset() * 60000);
}


function getFileName(path)
{
    return path.split('\\').pop().split('/').pop();
}

function GetChannelFormatName(formatCode) {
    switch (formatCode) {
        case 0: return "UInt8";
        case 1: return "UInt16";
        case 2: return "UInt32";
        case 3: return "Int8";
        case 4: return "Int16";
        case 5: return "Int32";
        case 6: return "Float32";
        default: return formatCode;
    }
}


function sleep(millis) {
	var date = new Date();
	var curDate = null;
	
	do { curDate = new Date(); } 
	while(curDate-date < millis);
} 

function roundNumber(num, dec) {
	var result = Math.round(num*Math.pow(10,dec))/Math.pow(10,dec);
	return result;
}

function PopupValue(divId, title, content){
	document.getElementById("popupTitle").innerHTML = title;
	document.getElementById("popupContent").innerHTML = content;
    Popup.showModal(divId);
}


function GetHelp(popup, contentBurst, reference, position, width, height) {
    if (popup.div != null && 
        popup.div.style.display != "none" &&
        popup.reference == reference) {
        popup.hide();
        return;
    }
    popup.content = contentBurst; 
    popup.reference = reference;
    popup.width = width;
    popup.height = height;
    popup.position = position;
    popup.show();
}


function SetPager() {
    var anchorFirst = document.getElementById("anchorFirst");
    var anchorPrev = document.getElementById("anchorPrev");
    var anchorNext = document.getElementById("anchorNext");
    var anchorLast = document.getElementById("anchorLast");
    var spnPageNumber = document.getElementById("spnPageNumber");
    var spnTotalNoOfRows = document.getElementById("spnTotalNoOfRows");
    
    if (TotalNoOfRows > 0) {    	
        spnPageNumber.innerHTML = CurrentPage + "/" + TotalPages;
        spnPageNumber.style.color = "#FFFFFF";
        spnTotalNoOfRows.innerHTML = " out of total " + TotalNoOfRows;        
        if (CurrentPage > 1) {
            anchorFirst.className = "white";
            anchorFirst.href = "javascript:PageNavigate(1);";
            
            anchorPrev.className = "white";
            anchorPrev.href = "javascript:PageNavigate(" + (CurrentPage - 1) + ");";
        } else {
            anchorFirst.className = "tabLink";
            anchorPrev.className = "tabLink";   
        }
        
        if (CurrentPage < TotalPages) {
            anchorNext.className = "white";
            anchorNext.href = "javascript:PageNavigate(" + (CurrentPage * 1 + 1) + ");";
            
            anchorLast.className = "white";
            anchorLast.href = "javascript:PageNavigate(" + TotalPages + ");";
        } else {
            anchorNext.className = "tabLink";
            anchorLast.className = "tabLink";   
        }   
    } else {
        spnPageNumber.innerHTML = "";
        spnTotalNoOfRows.innerHTML = "";
        
        anchorFirst.className = "tabLink";
        anchorPrev.className = "tabLink";
        anchorNext.className = "tabLink";
        anchorLast.className = "tabLink";
    }        
}

function CalculateDuration(time){
	var startSeconds = ConvertFromSQLiteDateToJSDateObject(time).getTime()
	var endSeconds   = GetUTCDate().getTime();
	
	var hh = pad((endSeconds - startSeconds)/3600, 2, "0", STR_PAD_LEFT);
	var	mm = pad((endSeconds - startSeconds)/60,   2, "0", STR_PAD_LEFT);
	var	ss = pad((endSeconds - startSeconds)%60,   2, "0", STR_PAD_LEFT);
	return hh + ":" + mm + ":" + ss;
}

function loadjscssfile(){
	var mcsTheme = GetMcsTheme();
	var fileref=document.createElement("link")
	fileref.setAttribute("rel", "stylesheet")
	fileref.setAttribute("type", "text/css")
	var file;
	if (mcsTheme == 0){
		file = "styles/default.css"
	} else {
		if (mcsTheme == 1) {
			file = "styles/theme1.css"
		} else {
			if (mcsTheme == 2) {
				file = "styles/theme2.css"
			} else {
				file = "styles/default.css"
			}
		}
	}		
	fileref.setAttribute("href",file);	
	document.getElementsByTagName('head')[0].appendChild(fileref);
}

function GetMcsTheme(){
	var mcsTheme = ReadCookie("MCS_THEME");
	if (mcsTheme == null) {
		var myQuery = " SELECT Value FROM Properties WHERE Key = 'ColorTheme' ";
		var result;	
		try {
			var service = new jsonrpc.ServiceProxy(serviceURL, ["sqldal.execute"]);
			result = service.sqldal.execute({query:myQuery});
		} catch(e) {
			return 0; //default theme
		}
		if (result.length != 0) {			
			mcsTheme = result[0][0];
		} else {			
			mcsTheme = 0; //default theme
		}
	}	
	return mcsTheme;
}	
 
function SetMcsTheme(mcsTheme){
	CreateCookie("MCS_THEME", mcsTheme, 365);	
	var myQuery = " REPLACE INTO Properties(Key, Value) VALUES('ColorTheme', " + mcsTheme + ")";
	var result;	
	try {
		var service = new jsonrpc.ServiceProxy(serviceURL, ["sqldal.execute"]);
		result = service.sqldal.execute({mode : "write", query : myQuery});
	} catch(e) {
		HandleException(e, "Unexpected error running SetMcsTheme !");		
	}
}

function ElementExists(el, ar){
	for (var i=0; i<ar.length; i++){
		if (el.toString() == ar[i].toString()){
			return true;
		};
	};
	return false;
};