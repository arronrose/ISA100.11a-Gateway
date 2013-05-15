function AlertLog() {
    this.DeviceID = null;
    this.Address64 = null;
    this.TsapID = null;
    this.ObjectID = null;
    this.Time = null;
    this.TimeMsec = null;
    this.Class = null;
    this.Direction = null;
    this.Category = null;
    this.Type = null;
    this.Priority = null;
    this.AlertData = null;
    this.PopupLink = null;    
}

// alert categories
var AC_DeviceDiagnostic = 0;
var AC_CommunicationDiagnostic = 1;
var AC_Security = 2;
var AC_Process = 3;

function GetAlertCategoryName(alertCategory) {
    switch (alertCategory) {
    case AC_Process:
        return "Process";
	case AC_DeviceDiagnostic:
		return "Device Diagnostic";
	case AC_CommunicationDiagnostic:
		return "Communication Diagnostic";	
	case AC_Security:
		return "Security";	
	default:
        return "UNKNOWN";
    }
}

function GetAlertCategoriesArray()
{
    return Array(AC_Process, AC_DeviceDiagnostic, AC_CommunicationDiagnostic, AC_Security);
}


// alert priorities
var AP_JournalOnly = 0; // 0-2
var AP_Low = 3;         // 3-5
var AP_Medium = 6;      // 6-8
var AP_High = 9;        // 9-11
var AP_Urgent = 12;     // 12-15
var AP_MaxUrgent = 15;

function GetAlertPriorityName(alertPriority) {
    if (AP_JournalOnly <= alertPriority && alertPriority < AP_Low) {
        return "Journal";	
    } else if (AP_Low <= alertPriority && alertPriority < AP_Medium) {
        return "Low";
    } else if (AP_Medium <= alertPriority && alertPriority < AP_High) {
        return "Medium";
    } else if (AP_High <= alertPriority && alertPriority < AP_Urgent) {
        return "High";
    } else if (AP_Urgent <= alertPriority && alertPriority <= AP_MaxUrgent) {
        return "Urgent";
    } else {
        return "UNKNOWN";
    };
}

function GetAlertPrioritiesArray()
{
    return Array(AP_JournalOnly, AP_Low, AP_Medium, AP_High, AP_Urgent);
}


function GetAlertPrioritiesIntervals(val){
    var interval = Array(2);
    if (AP_JournalOnly <= val && val < AP_Low) {
        interval[0] = AP_JournalOnly;
        interval[1] = AP_Low-1;	
    } else if (AP_Low <= val && val < AP_Medium) {
        interval[0] = AP_Low;
        interval[1] = AP_Medium-1;	
    } else if (AP_Medium <= val && val < AP_High) {
        interval[0] = AP_Medium;
        interval[1] = AP_High-1;	
    } else if (AP_High <= val && val < AP_Urgent) {
        interval[0] = AP_High;
        interval[1] = AP_Urgent-1;	
    } else if (AP_Urgent <= val && val <= AP_MaxUrgent) {
        interval[0] = AP_Urgent;
        interval[1] = AP_MaxUrgent;  
    } else {
        interval[0] = -100000;
        interval[1] = 100000;	
    };
    return interval;
}

// alert clases
var ACL_Event = 0;
var ACL_Alarm = 1;

function GetAlertClassName(val){
  switch (val) {
    case ACL_Event:
        return "Event";
	case ACL_Alarm:
		return "Alarm";
	default:
        return "UNKNOWN";
    }
}

function GetAlertClassArray(){
  return Array(ACL_Event, ACL_Alarm);   
}

//alert directions
var AD_AlarmEnded = 0;
var AD_AlarmBegan = 1;

function GetAlertDirection(val){
  switch (val) {
    case AD_AlarmEnded:
        return "Ended";
	case AD_AlarmBegan:
		return "Began";
	default:
        return "UNKNOWN";
    }
}       
