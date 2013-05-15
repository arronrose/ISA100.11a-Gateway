var methods = ["sqldal.execute", "user.logout", "sqldal.getCsv", "user.isValidSession"];

var valueArray = new Array();

function AppendWhereCondition(whereClause, whereCondition) {
	if (whereClause.length == 0) {
		whereClause = whereCondition;
	} else {			
		whereClause = whereClause + " AND " + whereCondition;
	}
	return whereClause;
}


function GetAlertsCount(deviceId, category, priority, clasa, startDate, endDate) {
	if (deviceId != null && deviceId.length == 0) {
		deviceId = null;
	};
	if (category != null && category.length == 0)	{
		category = null;
	};
	if (priority != null && priority.length == 0)	{
		priority = null;
	};
	
	var whereClause = "";
	if (deviceId != null) {			
		whereClause = AppendWhereCondition(whereClause, "(d.DeviceID = " + deviceId + ")");
	};
	if (category != null) {			
		whereClause = AppendWhereCondition(whereClause, "(Category = " + category + ")");
	};
	if (clasa != null) {			
		whereClause = AppendWhereCondition(whereClause, "(Class = " + clasa + ")");
	};

	
	if (priority != null) {			
	    var interval = GetAlertPrioritiesIntervals(priority);   
		whereClause = AppendWhereCondition(whereClause, "(" + interval[0] + " <= Priority AND Priority <= " + interval[1] + ")");
	};
    if (startDate != null) {		
        startDate = TruncateSecondsFromDate(startDate);    
	    whereClause = AppendWhereCondition(whereClause, "(AlertTime >= '" + ConvertFromJSDateToSQLiteDate(startDate) + "')");
    };
    if (endDate != null) {		
        endDate = TruncateSecondsFromDate(AddSecondsToDate(endDate, 60));
        whereClause = AppendWhereCondition(whereClause, "(AlertTime < '" + ConvertFromJSDateToSQLiteDate(endDate) + "')");
    };
	if (whereClause.length > 0) {
		whereClause = " WHERE " + whereClause + " ";
	};
							                
	var myQuery =   "   SELECT  COUNT(*) " +
					"   FROM    AlertNotifications a " +
				    "	        INNER JOIN Devices d ON a.DeviceID = d.DeviceID "
					+ 	whereClause;
    var result;  
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetAlertsCount !");
        return null;
    }
	
	if (result.length != 0) {
		return result[0][0];		    
	} else {			
        return 0;
	}	
}

function GetAlertsPage(deviceId, category, priority, clasa, startDate, endDate, pageNo, pageSize, rowCount, forExport, sortBy, sortOrder) {
    var rowOffset = 0;  
   	if (rowCount != 0 && forExport == false) {
		var maxAvailablePageNo = Math.ceil(rowCount/pageSize);
			
		if (pageNo > maxAvailablePageNo) {
			pageNo = maxAvailablePageNo;
		}
        //global var
	    TotalPages = maxAvailablePageNo;
        rowOffset = (pageNo - 1) * pageSize;
    }
    
	if (deviceId != null && deviceId.length == 0) {
		deviceId = null;
	}
	if (category != null && category.length == 0)	{
		category = null;
	}
	if (priority != null && priority.length == 0)	{
		priority = null;
	}
	if (clasa != null && clasa.length == 0)	{
		clasa = null;
	}	

    var whereClause = "";
	if (deviceId != null) {			
		whereClause = AppendWhereCondition(whereClause, "(d.DeviceID = " + deviceId + ")");
	}
	if (priority != null) {			
	    var interval = GetAlertPrioritiesIntervals(priority);   
		whereClause = AppendWhereCondition(whereClause, "(" + interval[0] + " <= Priority AND Priority <= " + interval[1] + ")");
	}
	if (category != null) {			
		whereClause = AppendWhereCondition(whereClause, "(Category = " + category + ")");
	}	
	if (clasa != null) {			
		whereClause = AppendWhereCondition(whereClause, "(Class = " + clasa + ")");
	};
	
    if (startDate != null) {		
        startDate = TruncateSecondsFromDate(startDate);    
	    whereClause = AppendWhereCondition(whereClause, "(AlertTime >= '" + ConvertFromJSDateToSQLiteDate(startDate) + "')");
    };
    if (endDate != null) {		
        endDate = TruncateSecondsFromDate(AddSecondsToDate(endDate, 60));
        whereClause = AppendWhereCondition(whereClause, "(AlertTime < '" + ConvertFromJSDateToSQLiteDate(endDate) + "')");
    };	
	if (whereClause.length > 0)	{
		whereClause = " WHERE " + whereClause + " ";
	};

    var myQuery =   "   SELECT  d.Address64, AlertTime AS Time, " +
                    "   CASE Category " +
                    "     WHEN " + AC_Process                   + " THEN '" + GetAlertCategoryName(AC_Process) + "' " +
                    "     WHEN " + AC_DeviceDiagnostic          + " THEN '" + GetAlertCategoryName(AC_DeviceDiagnostic) + "' " +
                    "     WHEN " + AC_CommunicationDiagnostic   + " THEN '" + GetAlertCategoryName(AC_CommunicationDiagnostic) + "' " +
                    "     WHEN " + AC_Security                  + " THEN '" + GetAlertCategoryName(AC_Security) + "' " +
                    "     ELSE 'UNKNOWN' " +
                    "   END AS Category, " +
                    "   Priority || '-' || CASE " +
                    "     WHEN (" + AP_JournalOnly  + " <= Priority AND Priority < " + AP_Low           + ") THEN '" + GetAlertPriorityName(AP_JournalOnly) + "' " +
                    "     WHEN (" + AP_Low          + " <= Priority AND Priority < " + AP_Medium        + ") THEN '" + GetAlertPriorityName(AP_Low) + "' " +
                    "     WHEN (" + AP_Medium       + " <= Priority AND Priority < " + AP_High          + ") THEN '" + GetAlertPriorityName(AP_Medium) + "' " +
                    "     WHEN (" + AP_High         + " <= Priority AND Priority < " + AP_Urgent        + ") THEN '" + GetAlertPriorityName(AP_High) + "' " +
                    "     WHEN (" + AP_Urgent       + " <= Priority AND Priority <= " + AP_MaxUrgent    + ") THEN '" + GetAlertPriorityName(AP_Urgent) + "' " +
                    "     ELSE 'UNKNOWN' " +
                    "   END AS PriorityString, " +    
                    "   AlertData AS Value, " +
                    "   TsapID, " +
                    "   ObjID AS ObjectID, " +
                    "   CASE Class " +
                    "       WHEN " + ACL_Event + " THEN '" + GetAlertClassName(ACL_Event) + "' " +
                    "       WHEN " + ACL_Alarm + " THEN '" + GetAlertClassName(ACL_Alarm) + "' " +
                    "     ELSE 'UNKNOWN' " +
                    "   END AS Class, " +
                    "   CASE Class WHEN " + ACL_Event + " THEN '" + NAString + "' ELSE " +
                    "   CASE Direction " +                    
                    "       WHEN " + AD_AlarmEnded + " THEN '" + GetAlertDirection(AD_AlarmEnded) + "' " +
                    "       WHEN " + AD_AlarmBegan + " THEN '" + GetAlertDirection(AD_AlarmBegan) + "' " +
                    "     ELSE 'UNKNOWN' " +
                    "   END " +
                    "   END AS Direction, " +
                    "   AlertTimeMsec, " +
                    "   Type " +
				    "   FROM    AlertNotifications a" + 
				    "	        INNER JOIN Devices d ON a.DeviceID = d.DeviceID "
				    + whereClause  +
				    "   ORDER BY " + GetAlertOrderByColumnName(sortBy, sortOrder) + " " +
				    "   LIMIT " + pageSize + " OFFSET " + rowOffset;
          
    if (forExport) {
        myQuery = " SELECT 'EUI-64 Address', 'Time', 'Category', 'PriorityString', 'Value', 'TsapID', 'ObjectID', 'Class', 'Direction', 'AlertTimeMsec', 'Type' UNION ALL " + myQuery;       
        return myQuery;        
    }

	var result;
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetAlertsPage !");
        return null;
    }

	var alerts = new Array();		
	if (result.length != 0) {	    
	    for (i=0;i<result.length;i++) {
			var alert = new AlertLog();
			alert.Address64 = result[i][0];	
			alert.Time = result[i][1];
	    	alert.Category = result[i][2];
			alert.Priority = result[i][3];    		 
            alert.TsapID = result[i][5];
            alert.ObjectID = result[i][6];
            alert.Class = result[i][7];
            alert.Direction = result[i][8];
            alert.Type = result[i][10];
            if (result[i][4].length > 27){
            	alert.PopupLink = "<a href='javascript:PopupValue(\"modalPopup\", \"Alert Value\", \"" + result[i][4] + "\")'>" + result[i][4].substring(0,27) + "...</a>";
            	alert.AlertData = "";
	    	} else {
	    		alert.PopupLink = "";
	    		alert.AlertData = result[i][4].substring(0,27);
	    	}
			if (i % 2 == 0) {
                alert.cellClass = "tableCell2";
            } else {   
                alert.cellClass = "tableCell";
            }
			alerts[i] = alert;
		}		
	}
	alerts.alerts = alerts;
	return alerts;
}

function SaveAlertSubstription(processAlerts, deviceDiagnostic, communicationDiagnostic, securityAlerts){
    try {
      var myUpdate = "   UPDATE  AlertSubscriptionCategory " +
                     "   SET     CategoryProcess = " + processAlerts + ", " +
                     "           CategoryDevice =  " + deviceDiagnostic + ", " +
                     "           CategoryNetwork = " + communicationDiagnostic + ", " +
                     "           CategorySecurity = " + securityAlerts + " ";
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       var result = service.sqldal.execute({mode:"write",query:myUpdate});
       return true;       
    } catch(e) {
       HandleException(e, "Unexpected error running SaveAlertSubstription !");
       return false;       
    };
}

function GetAlertSubstription(){
    var myQuery =  "   SELECT  CategoryProcess, CategoryDevice, CategoryNetwork, CategorySecurity " +
                   "   FROM    AlertSubscriptionCategory ";
    var result;                    
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetAlertSubstription !");
        return null;
    };

	if (result.length != 0) {
	   result.CategoryProcess = result[0][0] == 1 ? true : false;
	   result.CategoryDevice = result[0][1] == 1 ? true : false;
	   result.CategoryNetwork = result[0][2] == 1 ? true : false;
	   result.CategorySecurity = result[0][3] == 1 ? true : false;
	}
	result.result = result;
	return result;    
}

function GetAlertOrderByColumnName(sortType, sortOrder)
{
	switch (sortType)
	{
		case 1:
		    return " d.Address64 " + sortOrder;
		case 2:
            return " AlertTime " + sortOrder + ", AlertTimeMsec " + sortOrder;
        case 3:    
            return " Category " + sortOrder;
        case 4:    
            return " Priority " + sortOrder;
        case 5:
            return " Class " + sortOrder;           
  		default:
    		return " d.Address64 " + sortOrder;
	}
}
