var methods = ["sqldal.execute", 
               "sqldal.close", 
			   "sqldal.open", 
			   "user.logout", 
			   "file.remove", 
			   "file.create", 
			   "file.exists", 
			   "user.isValidSession"];

function GetDeviceInformation(deviceId) {
    if (deviceId == null || deviceId.length == 0) {
        return null;
    }
    
    var myQuery = " SELECT  Address64, Address128, DeviceRole, DeviceStatus, " +
                  "         (SELECT MAX(R.ReadingTime) FROM DeviceReadings R WHERE R.DeviceID = D.DeviceID AND ReadingTime > '1970-01-01 00:00:00') AS LastRead, " +
                  "         PowerSupplyStatus, Manufacturer, Model, Revision, SubnetID, " +
                  "         DPDUsTransmitted, DPDUsReceived, DPDUsFailedTransmission, DPDUsFailedReception " +
                  " FROM    Devices D " +
                  "         INNER JOIN DevicesInfo I ON D.DeviceID = I.DeviceID " +
               	  " WHERE   D.DeviceID = " + deviceId ;
                   
    var result;
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running get device information !");
        return null;
    }
        
    if	(result != ''){   	
	    var dev = new Device();
	    dev.Address64 = result[0][0];
	    dev.Address128 = result[0][1];
	    dev.DeviceRoleID = result[0][2];
	    dev.DeviceRole = GetDeviceRole(result[0][2]);
	    dev.DeviceStatus = result[0][3];
	    dev.LastRead = (result[0][4] != 'NULL') ? result[0][4] : NAString ;
	    dev.PowerSupplyStatus = result[0][5] != "NULL" ? result[0][5] : NAString;
	    var devString = GetAsciiString(result[0][6]);
	    dev.Manufacturer = (result[0][6] != "NULL") ? (devString != "") ? devString : NAString : NAString;
	    devString = GetAsciiString(result[0][7]);
	    dev.Model = (result[0][7] != "NULL") ? (devString != "") ? devString : NAString : NAString;
	    devString = GetAsciiString(result[0][8]);
	    dev.Revision = (result[0][8] != "NULL") ? (devString != "") ? devString : NAString : NAString;
	    dev.SubnetID = result[0][9] != "NULL" ? result[0][9] : NAString;
	    dev.DPDUsTransmitted = result[0][10] != "NULL" ? result[0][10] : NAString;
	    dev.DPDUsReceived = result[0][11] != "NULL" ? result[0][11] : NAString;
	    dev.DPDUsFailedTransmission = result[0][12] != "NULL" ? result[0][12] : NAString;
	    dev.DPDUsFailedReception = result[0][13] != "NULL" ? result[0][13] : NAString;
	    return dev;
    } else {
    	return null;
    }	    
}


function GetDeviceProcessValuesCount(deviceId) {
    if (deviceId == null || deviceId.length == 0) {
        return null;
    }
    var myQuery = " SELECT  COUNT(*) " +
                  " FROM    DeviceChannels " +
               	  " WHERE   DeviceID = " + deviceId ;
    var result;
    
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running get device process values !");
        return;
    }

	if (result.length != 0) {
		return result[0][0];
	} else {			
	    return null;
    }
}

function GetDeviceProcessValues(deviceId, pageSize, pageNo, rowCount) {
    if (deviceId == null || deviceId.length == 0) {
        return null;
    }
    
   	if (rowCount != 0) {
	   var maxAvailablePageNo = Math.ceil(rowCount/pageSize);
	
	   if (pageNo > maxAvailablePageNo) {
	       pageNo = maxAvailablePageNo;
	   }
        //global var
        TotalPages = maxAvailablePageNo;
        rowOffset = (pageNo - 1) * pageSize;
    }
      
    var myQuery = " SELECT  C.ChannelName, C.UnitOfMeasurement, C.ChannelFormat, C.SourceTSAPID, C.SourceObjID, C.SourceAttrID, C.SourceIndex1, C.SourceIndex2, C.ChannelNo, C.DeviceID, D.ChannelNo " +
                  " FROM    DeviceChannels C " +
                  " LEFT OUTER JOIN Dashboard D ON C.ChannelNo = D.ChannelNo " +		
               	  " WHERE   C.DeviceID = " + deviceId +
               	  " LIMIT " + pageSize + " OFFSET " + rowOffset;
    var result;
     
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running get device process values !");
        return null;
    }
    
    for (i=0;i<result.length; i++) {
        result[i].ChannelName  = result[i][0];
        result[i].UnitOfMeasurement  = result[i][1];
        result[i].ChannelFormat =  GetChannelFormatName(result[i][2]);
        result[i].SourceTSAPID  = result[i][3];
        result[i].SourceObjID   = result[i][4];
        result[i].SourceAttrID  = result[i][5];
        result[i].SourceIndex1  = result[i][6];
        result[i].SourceIndex2  = result[i][7];
        result[i].ChannelNo		= result[i][8];              
        result[i].DeviceID		= result[i][9];
        
        if (result[i][10] == "NULL") {
        	result[i].ATD  = "<a href='adddevicetodashboard.html?channelNo=" + result[i].ChannelNo + "?deviceId=" + result[i].DeviceID + "?callerId=1'><img src='styles/images/deviceAdd.png' title='Add to dashboard'></a>";	
        } else {
        	result[i].ATD = "";
        };        	        
        if (i % 2 == 0) {
            result[i].cellClass = "tableCell";
        } else {   
            result[i].cellClass = "tableCell2";
        };
    }    
    result.processvalues = result;
    return result;
}

function GetDeviceCount(showDevicesFilter, euiAddress, deviceTag, revision, deviceTypes, level) {
    var whereClause = "";
    switch (showDevicesFilter) {
        case 0: whereClause = " DeviceStatus >= " + DS_JoinedAndConfigured + " "; break;
        case 1: whereClause = " DeviceStatus < " + DS_JoinedAndConfigured + " AND DeviceStatus >= " + DS_SecJoinRequestReceived + " "; break;
        case 2: whereClause = " DeviceStatus < " + DS_SecJoinRequestReceived + " "; break; 
        default: ;
    }
    if (euiAddress) {
       whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " Address64 LIKE '%" + euiAddress + "%' ";
    }
    if (deviceTag) {
       whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " DeviceTag LIKE '%" + deviceTag + "%' ";
    }
    if (revision) {
        whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " Revision = '" + revision + "' ";
    }
	if (deviceTypes){		
		 whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " DeviceRole IN " + ConvertArrayToSqlliteParam(deviceTypes) + " ";
	}
    if (whereClause != "") {    
        whereClause = " WHERE " + whereClause;
    }

    var myQuery = "";
    if (revision) {
    	myQuery = "SELECT D.DeviceID FROM Devices D INNER JOIN DevicesInfo I ON D.DeviceID = I.DeviceID " + whereClause;
    } else {
    	myQuery = "SELECT DeviceID FROM Devices " + whereClause;
    };
    
    var result;
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query:myQuery});       
    } catch(e) {
        HandleException(e, "Unexpected error running get device count !");
        return 0;
    }  
    var noOfDevices = 0;
    if (result!= null){
        if (level != null && level != ""){
        	var tmp = GetDevicesLevels();
        	for(var i=0; i<result.length; i++){
        		if (tmp[result[i][0]] == Number(level)){
        			noOfDevices++;
        		}
        	}
        } else {
        	noOfDevices = result.length;
        }    	
    }   
    return noOfDevices;
}


function GetDevicePage(pageSize, pageNo, rowCount, showDevicesFilter, sortBy, sortOrder, euiAddress, deviceTag, revision, deviceTypes) {
    var rowOffset = 0;
    TotalPages = 0;
    
   	if (rowCount != 0) {
		var maxAvailablePageNo = Math.ceil(rowCount/pageSize);
		
		if (pageNo > maxAvailablePageNo) {
			pageNo = maxAvailablePageNo;
		}
        //global var
	    TotalPages = maxAvailablePageNo;
        rowOffset = (pageNo - 1) * pageSize;
    }
 	
    var whereClause = "";
    switch (showDevicesFilter) {
        case 0: whereClause = " DeviceStatus >= " + DS_JoinedAndConfigured + " "; break;
        case 1: whereClause = " DeviceStatus < " + DS_JoinedAndConfigured + " AND DeviceStatus >= " + DS_SecJoinRequestReceived + " "; break;
        case 2: whereClause = " DeviceStatus < " + DS_SecJoinRequestReceived + " "; break; 
        default: ;
    }
    if (euiAddress) {
       whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " Address64 LIKE '%" + euiAddress + "%' ";
    }
    if (deviceTag) {
       whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " DeviceTag LIKE '%" + deviceTag + "%' ";
    }
    if (revision) {
        whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " Revision = '" + revision + "' ";
    }    
	if (deviceTypes){		
		 whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " DeviceRole IN " + ConvertArrayToSqlliteParam(deviceTypes) + " ";
	}    
    if (whereClause != "") {    
        whereClause = " WHERE " + whereClause;
    }
   
    var myQuery = " SELECT  D.DeviceID, Address64, Address128, DeviceStatus, R.LastRead AS LastReadTime, DeviceRole, Model, DeviceTag, I.Revision " +
                  " FROM    Devices D " +
                  "			INNER JOIN DevicesInfo I ON D.DeviceID = I.DeviceID " +
                  "         LEFT OUTER JOIN (SELECT DeviceID, max(ReadingTime) AS LastRead " + 
				  "                           FROM DeviceReadings WHERE ReadingTime > '1970-01-01 00:00:00' " +
				  "                           GROUP BY DeviceID) R ON D.DeviceID = R.DeviceID " + 
                  whereClause +
                  " ORDER BY " + GetOrderByColumnName(sortBy, sortOrder) + " LIMIT " + pageSize + " OFFSET " + rowOffset;
    var result;
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running get device page !");
        return null;
    }  
 
    var icons = GetCustomIconFiles();
    for (var i=0;i<result.length; i++) {
        result[i].DeviceID = result[i][0];        
        result[i].Address64String = result[i][1];
        result[i].Address64 = "<a href='deviceinformation.html?deviceId=" + result[i].DeviceID + "'>" + result[i][1]; + "</a>";       
        result[i].Address128 = result[i][2];        
        result[i].DeviceStatus = result[i][3];
        result[i].LastRead = result[i][4];
        result[i].LastRead = result[i].LastRead != "NULL" ? "<a href='readings.html?deviceId=" + result[i].DeviceID + "'>" + result[i].LastRead + "</a>" : NAString;
        var image = new Image();
        image.src = "styles/images/" + GetIconFileName(icons, result[i][6], result[i][5]);
        image.width =  ((image.width  > MAX_DEVICE_ICON_SIZE) ? MAX_DEVICE_ICON_SIZE : image.width); 
        image.height = ((image.height > MAX_DEVICE_ICON_SIZE) ? MAX_DEVICE_ICON_SIZE : image.height); 
        result[i].Icon = "<img src='" + image.src + "' width='" + image.width + "' height='" + image.height + "' border='0'></img>";
        
        result[i].CommandLink = (result[i].DeviceStatus >= DS_JoinedAndConfigured && result[i][5] != DT_SystemManager) ?
        						"<a href='devicecommands.html?deviceId=" + result[i].DeviceID + "'><img src='styles/images/execute.png' title='Run Command'></a>" : ""
        
        result[i].DeleteLink = (result[i].DeviceStatus < DS_JoinedAndConfigured) ? 
                                "<a href='javascript:DeleteDevice(" + result[i].DeviceID + ");'><img src='styles/images/delete.gif' title='Delete Device'></a>" : "";                               
                                
        result[i].Model = result[i][6] != "NULL" ? result[i][6] : "";
        var devString = GetAsciiString(result[i][6]);
        result[i].DeviceRole = GetDeviceRole(result[i][5]) + "/ " + devString;
        result[i].DeviceStatus = "<span style='color:"+ GetDeviceStatusColor(result[i].DeviceStatus) + "'><b>" +GetDeviceStatusName(result[i].DeviceStatus) + "</b></span>";  
        devString = GetAsciiString(result[i][7]); 
        result[i].DeviceTag = (devString != "") ? devString : NAString;        
        result[i].Revision = result[i][8]; 
        result[i].RevisionString = GetAsciiString(result[i][8]);  
        result[i].Checked = "<input type=\"checkbox\" id=\"chkDevice" + result[i].DeviceID + "\" style=\"vertical-align:sub;\" onclick = \"AddRemoveOneDevice()\"/>";                
        result[i].HeightSpacer = "<img src='styles/images/pixel.gif' height='31px' width='1px' border='0'>";
        result[i].DeviceTagAndRevision = result[i].DeviceTag + "/ " + result[i].RevisionString;
        if (i % 2 == 0) {
            result[i].cellClass = "tableCell";
        } else {   
            result[i].cellClass = "tableCell2";
        }        	   
    }    
    result.devices = result;    
    return result;
}


function DeleteDeviceAndData(deviceId) {
    var qArray = new Array();
    
    qArray[0] = "DELETE FROM CommandParameters WHERE CommandID IN (SELECT CommandID FROM Commands WHERE DeviceID = " + deviceId + ")";
    qArray[1] = "DELETE FROM Commands WHERE DeviceID = " + deviceId;
    qArray[2] = "DELETE FROM DeviceChannels WHERE DeviceID = " + deviceId;
    qArray[3] = "DELETE FROM DeviceHistory WHERE DeviceID = " + deviceId;
    qArray[4] = "DELETE FROM Commands WHERE DeviceID = " + deviceId;
    qArray[5] = "DELETE FROM DeviceReadings WHERE DeviceID = " + deviceId;
    qArray[6] = "DELETE FROM TopologyLinks WHERE FromDeviceID = " + deviceId + " OR ToDeviceID = " + deviceId;
    qArray[7] = "DELETE FROM TopologyGraphs WHERE FromDeviceID = " + deviceId + " OR ToDeviceID = " + deviceId;
    qArray[8] = "DELETE FROM RouteLinks WHERE DeviceID = " + deviceId;
    qArray[9] = "DELETE FROM RoutesInfo WHERE DeviceID = " + deviceId;    
    qArray[10] = "DELETE FROM AlertNotifications WHERE DeviceID = " + deviceId;        
    qArray[11] = "DELETE FROM AlertSubscriptionAlertSourceID WHERE DeviceID = " + deviceId;            
    qArray[12] = "DELETE FROM ChannelsStatistics WHERE DeviceID = " + deviceId;            
    qArray[13] = "DELETE FROM ContractElements WHERE DeviceID = " + deviceId + " OR SourceDeviceID = " + deviceId; 
    qArray[14] = "DELETE FROM Contracts WHERE DestinationDeviceID = " + deviceId + " OR SourceDeviceID = " + deviceId;
    qArray[15] = "DELETE FROM DeviceChannelsHistory WHERE DeviceID = " + deviceId ;
    qArray[16] = "DELETE FROM DeviceConnections WHERE DeviceID = " + deviceId ;
    qArray[17] = "DELETE FROM DeviceHealthHistory WHERE DeviceID = " + deviceId ;    
    qArray[18] = "DELETE FROM DeviceReadingsHistory WHERE DeviceID = " + deviceId ;        
    qArray[19] = "DELETE FROM DeviceScheduleLinks WHERE DeviceID = " + deviceId + " OR NeighborDeviceID = " + deviceId;
    qArray[20] = "DELETE FROM DeviceScheduleSuperframes WHERE DeviceID = " + deviceId;    
    qArray[21] = "DELETE FROM NeighborHealthHistory WHERE DeviceID = " + deviceId + " OR NeighborDeviceID = " + deviceId;
    qArray[22] = "DELETE FROM NetworkHealthDevices WHERE DeviceID = " + deviceId;    
    qArray[23] = "DELETE FROM DevicesInfo WHERE DeviceID = " + deviceId;
    qArray[24] = "DELETE FROM Devices WHERE DeviceID = " + deviceId;

    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       for(i=0; i<qArray.length; i++) {
            service.sqldal.execute({mode : "write", query: qArray[i]});
       }
    } catch(e) {
        HandleException(e, "Unexpected error running delete device !");
    } 
}			
//END DEVICE LIST FUNCTIONS


//DEVICE SETTINGS FUNCTIONS

function GetDeviceNeighbors(deviceId)
{
    var myQuery = " SELECT  Address64, " + 
				  "			CASE ClockSource " +
				  "				WHEN 0 THEN 'No' " +
				  "				WHEN 1 THEN 'Secondary' " +
				  "				WHEN 2 THEN 'Preferred' " +
				  "				ELSE '" + NAString + "' " +
				  "			END AS ClockSource, " +
				  "			SignalQuality " +
                  " FROM    TopologyLinks T " +
                  "         INNER JOIN Devices D ON T.ToDeviceID = D.DeviceID " +
                  " WHERE   FromDeviceID = " + deviceId +
                  " ORDER BY Address64 ";
   
    var result;
 
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running get device neighbors !");
        return;
    }  
    
    for (i=0;i<result.length; i++) {
        result[i].Address64 = result[i][0];
        result[i].IsClockSource = result[i][1];
        result[i].SignalQuality = " <span id=\"spnSignalQuality\" style=\"color:" + GetSignalQualityColor(result[i][2]) + "\">" + GetSignalQuality(result[i][2]) + "</span> (" + result[i][2] + ")";  
        
        if (i % 2 == 0) {
            result[i].cellClass = "tableCell2";
        } else {   
            result[i].cellClass = "tableCell";
        }
    }
    result.neighbors = result;
    return result;
}


function GetDeviceRoutes(deviceId) {
    var myQuery = " SELECT  R.RouteID, " + 
				  "			R.Alternative, " + 
				  "			CASE R.Alternative " + 
				  "				WHEN 0 THEN 'ContractID: ' || R.Selector " +
				  "				WHEN 1 THEN 'ContractID: ' || R.Selector " +
				  "				WHEN 2 THEN 'DestAddress: ' || (SELECT Address64 FROM Devices WHERE DeviceID = R.Selector) " +
				  "				WHEN 3 THEN '" + NAString + "' " +
				  "				ELSE R.Selector " +
				  "			END AS Selector, " +
				  "			R.FowardLimit, " +
                  "         CASE WHEN L.NeighbourID is null " +
                  "              THEN 'Graph: '|| L.GraphID " +
                  "              ELSE 'Node: '|| D.Address64 " +
                  "         END RouteElement " +
                  " FROM    RoutesInfo R " +
                  "         INNER JOIN RouteLinks L ON R.DeviceID = L.DeviceID AND R.RouteID = L.RouteID " +
                  "			LEFT OUTER JOIN Devices D ON L.NeighbourID = D.DeviceID " +
                  " WHERE   R.DeviceID = " + deviceId +
                  " ORDER BY L.RouteID, L.RouteIndex";    
    var result;
 
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running get device routes !");
        return;
    }  
    
    for (i=0;i<result.length; i++) {
        result[i].RouteId = result[i][0];
        result[i].Alternative = result[i][1];
        result[i].Selector = result[i][2] != "NULL" ? result[i][2] : NAString ;
        result[i].ForwardLimit = result[i][3] != "NULL" ? result[i][3] : NAString ;	
        result[i].RouteElement = result[i][4];
        
        if (i % 2 == 0) {
            result[i].cellClass = "tableCell2";
        } else {   
            result[i].cellClass = "tableCell";
        }
    }
    
    result.routes = result;
    return result;
}


function GetDeviceGraphs(deviceId) {
    var myQuery = " SELECT  GraphID, Address64 " +
                  " FROM    TopologyGraphs T " +
                  "         INNER JOIN Devices D ON T.ToDeviceID = D.DeviceID " +
                  " WHERE   FromDeviceID = " + deviceId + 
                  " ORDER BY GraphID, Address64 ";
    
    var result;
 
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);

       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running get device neighbors !");
        return;
    }  
    
    for (i=0;i<result.length; i++) {
        result[i].GraphId = result[i][0];
        result[i].Address64 = result[i][1];
        
        if (i % 2 == 0) {
            result[i].cellClass = "tableCell2";
        } else  {   
            result[i].cellClass = "tableCell";
        }
    }
    result.graphs = result;
    return result;
}

//END DEVICE SETTINGS FUNCTIONS

function GetIPAddressForGateway(deviceId) {
    var myQuery = "SELECT IP, Port FROM DeviceConnections WHERE DeviceID = " + deviceId;
    
    var result;
    
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running get IP address for gateway!");
        return;
    }  
    
    if (result.length > 0) {
        return result[0][0] + ":" + result[0][1];
    } else {
        return NAString;  
    }   
}

function GetDeviceListByType(deviceType, deviceStatus) {
    var	whereClause = "";

	if (deviceType != null)
	{		
		 whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " DeviceRole IN " + ConvertArrayToSqlliteParam(deviceType) + " ";
	}
	if (deviceStatus != null)
	{		
		 whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " DeviceStatus = " + deviceStatus + " ";
	}
	if (whereClause.length > 0)
	{
		whereClause = " WHERE " + whereClause;
	}

    var myQuery =   "   SELECT  DeviceID, " +
                    "           DeviceRole, " +
                    "           Address64, " +
                    "           Address128, " +
                    "           DeviceStatus " +
					"   FROM    Devices "
					+   whereClause + 
					"   ORDER BY Address64 ";
	var result;
    
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running get DeviceList by type!");
        return;
    }  
    
    for (i=0;i<result.length; i++) {        
        result[i].DeviceID = result[i][0];
        result[i].DeviceRole = GetDeviceRole(result[i][1]);
        result[i].Address64 = result[i][2];
		result[i].Address128 = result[i][3];        
    	result[i].DeviceStatus = result[i][4];
    }
    
    result.devices = result;
    return result;    
}

function GetSystemManagerDevice() {
    var result = GetDeviceListByType(new Array(DT_SystemManager.toString()), null);     
	if (result.devices.length == 0) {
		alert("No system manager present in the system!");
		return null;
	}	
	for(i=0; i<result.devices.length; i++) {
		if (result.devices[i].DeviceStatus >= DS_JoinedAndConfigured) {
			return result.devices[i];
		}
	}	
	return result.devices[0];
}

function GetGatewayDevice() {
    var result = GetDeviceListByType(new Array(DT_Gateway.toString()), null);     
	if (result.devices.length == 0) {
		alert("No gateway present in the system!");
		return null;
	}	
	return result.devices[0];
}


function GetDeviceDetails(deviceId) {				
    var myQuery = "   SELECT  Address128, " + 
                  "           Address64, " +
                  "           DeviceID, " +    
                  "           DeviceStatus, " +
                  "           DeviceRole " +
                  "   FROM    Devices " +
		          "   WHERE   DeviceID = " + deviceId ;

    var result;
    
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetDeviceDetails !");
        return;
    }  		
		
	if (result.length != 0) {		
	    var device = new Device();
		device.Address128 = result[0][0];
		device.Address64 = result[0][1];
		device.DeviceID = result[0][2];
		device.DeviceStatus = result[0][3];
		device.DeviceRole = GetDeviceRole(result[0][4]);
        return device;
    } else {
        return null;   
    }	
}


function DeleteDeviceHistory(deviceId) {
	var myQuery = "DELETE FROM DeviceHistory WHERE DeviceID = " + deviceId; 
					
    var result;
    
    try {
        var service = new jsonrpc.ServiceProxy(serviceURL, methods);
        result = service.sqldal.execute({mode : "write", query: myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running DeleteDeviceHistory !");
        return;
    }
}


function GetDeviceHistoryCount(deviceId, startTime, endTime, deviceStatus)
{
	var whereClause = " WHERE DeviceID = " + deviceId;

    if (startTime != null) {		
        startTime = TruncateSecondsFromDate(startTime);    
	    whereClause = whereClause + " AND Timestamp >= '" + ConvertFromJSDateToSQLiteDate(startTime) + "' ";
    }

    if (endTime != null) {		
        endTime = TruncateSecondsFromDate(AddSecondsToDate(endTime, 60));
        whereClause = whereClause + " AND Timestamp < '" + ConvertFromJSDateToSQLiteDate(endTime) + "' ";
    }

	if (deviceStatus != null) {
		whereClause = whereClause + " AND DeviceStatus = " + deviceStatus;
	}
	
	var myQuery = " SELECT COUNT(*) FROM DeviceHistory " + whereClause;	
				
    var result;
    
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetDeviceHistoryCount !");
        return;
    }  		
    
	if (result.length != 0) {
		return result[0][0];
	} else {			
	    return 0;
    }
}
	

function GetDeviceHistoryPage(deviceId, startTime, endTime, deviceStatus, sortByExpression, sortOrder, pageSize, pageNo, rowCount) {
    var rowOffset = 0;
    TotalPages = 0;
    
   	if (rowCount != 0) {
		var maxAvailablePageNo = Math.ceil(rowCount/pageSize);
		if (pageNo > maxAvailablePageNo) {
			pageNo = maxAvailablePageNo;
		}
        //global var
	    TotalPages = maxAvailablePageNo;
        rowOffset = (pageNo - 1) * pageSize;
    }

	var whereClause =  " WHERE DeviceID = " + deviceId;
    if (startTime != null) {		
        startTime = TruncateSecondsFromDate(startTime);    
	    whereClause = whereClause + " AND Timestamp >= '" + ConvertFromJSDateToSQLiteDate(startTime) + "' ";
    }
    if (endTime != null) {		
        endTime = TruncateSecondsFromDate(AddSecondsToDate(endTime, 60));
        whereClause = whereClause + " AND Timestamp < '" + ConvertFromJSDateToSQLiteDate(endTime) + "' ";
    }
	if (deviceStatus != null) {
		whereClause = whereClause + " AND DeviceStatus = " + deviceStatus + " ";
	}

	var myQuery =   "   SELECT  Timestamp, " + 
	                "           DeviceStatus " +
	                "   FROM    DeviceHistory "  
	                +   whereClause  +
	                "   ORDER BY " + GetOrderByColumnNameDeviceHistory(sortByExpression) + " " + sortOrder + 
	                "   LIMIT " + pageSize + " OFFSET " + rowOffset;		 

    var result;
    
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetDeviceHistoryPage !");
        return null;
    }  		

    var histArr = Array();
	if (result.length != 0) {
        for (i=0;i<result.length;i++) {
			var deviceHistory = new DeviceHistory();
			deviceHistory.Timestamp = result[i][0];
			deviceHistory.DeviceStatus = GetDeviceStatusName(result[i][1]);
			
			if (i % 2 == 0) {
                deviceHistory.cellClass = "tableCell";
            } else {   
                deviceHistory.cellClass = "tableCell2";
            }
			histArr[i] = deviceHistory;
		}			
	}
    histArr.registrationlogs = histArr;
    return histArr;
}


function GetOrderByColumnNameDeviceHistory(sortType)
{
	switch (sortType)
	{
		case 1:
			return	" Timestamp ";
		case 2:
			return	" CASE DeviceStatus " +
			
			        " WHEN " + DS_NotJoined + " THEN '" + GetDeviceStatusName(DS_NotJoined) + "' " +
					" WHEN " + DS_SecJoinRequestReceived + " THEN '" + GetDeviceStatusName(DS_SecJoinRequestReceived) + "' " +
					" WHEN " + DS_SecJoinResponseSent + " THEN '" + GetDeviceStatusName(DS_SecJoinResponseSent) + "' " +
					" WHEN " + DS_SmJoinReceived + " THEN '" + GetDeviceStatusName(DS_SmJoinReceived) + "' " +
					" WHEN " + DS_SmJoinResponseSent + " THEN '" + GetDeviceStatusName(DS_SmJoinResponseSent) + "' " +
					" WHEN " + DS_SmContractJoinReceived + " THEN '" + GetDeviceStatusName(DS_SmContractJoinReceived) + "' " +
					" WHEN " + DS_SmContractJoinResponseSent + " THEN '" + GetDeviceStatusName(DS_SmContractJoinResponseSent) + "' " +
					" WHEN " + DS_SecConfirmReceived + " THEN '" + GetDeviceStatusName(DS_SecConfirmReceived) + "' " +
					" WHEN " + DS_SecConfirmResponseSent + " THEN '" + GetDeviceStatusName(DS_SecConfirmResponseSent) + "' " +
					" WHEN " + DS_JoinedAndConfigured + " THEN '" + GetDeviceStatusName(DS_JoinedAndConfigured) + "' " +
					" ELSE 'Unknown' " +
					" END ";
  		default:
			return " Timestamp ";
	}
}

function GetOrderByColumnName(sortType, sortOrder)
{
	switch (sortType)
	{
		case 1:
		    return " Address64 " + sortOrder;
		case 2:
            return " Address128 " + sortOrder;
        case 3:
			return " CASE DeviceRole " +
				    " WHEN " + DT_SystemManager + " THEN '" + GetDeviceRole(DT_SystemManager) + "' " +
					" WHEN " + DT_Gateway + " THEN '" + GetDeviceRole(DT_Gateway) + "' " +
					" WHEN " + DT_BackboneRouter + " THEN '" + GetDeviceRole(DT_BackboneRouter) + "' " +
					" WHEN " + DT_Device + " THEN '" + GetDeviceRole(DT_Device) + "' " +
					" WHEN " + DT_DeviceNonRouting + " THEN '" + GetDeviceRole(DT_DeviceNonRouting) + "' " +
					" WHEN " + DT_DeviceIORouting + " THEN '" + GetDeviceRole(DT_DeviceIORouting) + "' " +										
					" WHEN " + DT_HartISAAdapter + " THEN '" + GetDeviceRole(DT_HartISAAdapter) + "' " +
					" WHEN " + DT_WirelessHartDevice + " THEN '" + GetDeviceRole(DT_WirelessHartDevice) + "' " +
					" ELSE 'Unknown' " +
				   " END " + sortOrder + ", Model " + sortOrder;
        case 4:
            return " DeviceTag " + sortOrder;            
        case 5:
            return " LastReadTime " + sortOrder;
  		default:
    		return " Address64 " + sortOrder;
	}
}


function GetRegisteredDeviceListByType(deviceType) {
    var	whereClause = "";

	if (deviceType != null) {		
		 whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " DeviceRole IN " + ConvertArrayToSqlliteParam(deviceType) + " ";
	}
    whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " DeviceStatus >= " + DS_JoinedAndConfigured + " ";

	if (whereClause.length > 0) {
		whereClause = " WHERE " + whereClause;
	}

    var myQuery =   "   SELECT  DeviceID, " +
                    "           DeviceRole, " +
                    "           Address64, " +
                    "           Address128, " +
                    "           DeviceStatus " +
					"   FROM    Devices "
					+   whereClause + 
					"   ORDER BY Address64 ";    
	var result;
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running get Registered DeviceList by type!");
        return;
    }  
    
    for (i=0;i<result.length; i++) {        
        result[i].DeviceID = result[i][0];
        result[i].DeviceRole = GetDeviceRole(result[i][1]);
        result[i].Address64 = result[i][2];
		result[i].Address128 = result[i][3];        
    	result[i].DeviceStatus = result[i][4];
    	result[i].DeviceRoleID = result[i][1];
    }
    
    result.devices = result;
    return result;    
}	

/** Custom Icons */
function GetCustomIconsList(savedIcons) {
    var myQuery =   "   SELECT  di.Model, d.DeviceRole " +
					"   FROM    Devices d INNER JOIN DevicesInfo di ON di.DeviceID = d.DeviceID " +
                    "   WHERE   di.Model NOT IN ('NULL', 'N/A')"; 
	var result;
    try  {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetCustomIconsPage to get models!");
        return;
    };       
    
    var icons = new Array();
    var j = 0;
    var iconFiles = GetCustomIconFiles();
    var arrElements = [];
    for (var i=0; i<result.length; i++) {    	
    	if (!ElementExists(result[i], arrElements)){
    		arrElements.push(result[i]);    	
	        var iconFile = GetIconFile(iconFiles, result[i][0], result[i][1]); 
	        if (savedIcons) {
	            if (iconFile != null) {
	                var objIcon = new Object();
	                objIcon.DeviceModel = result[i][0];
	                objIcon.DeviceRole = result[i][1];
	                objIcon.DeviceRoleName = GetDeviceRole(result[i][1]);
	                objIcon.ModelAscii = GetAsciiString(result[i][0]);
	                var image = new Image();
	                image.src = "styles/images/custom/" + getFileName(iconFile);
	                image.width =  ((image.width  > MAX_DEVICE_ICON_SIZE) ? MAX_DEVICE_ICON_SIZE : image.width); 
	                image.height = ((image.height > MAX_DEVICE_ICON_SIZE) ? MAX_DEVICE_ICON_SIZE : image.height); 
	                objIcon.Icon = "<img src='" + image.src + "' width='" + image.width + "' height='" + image.height + "' border='0'></img>";
	                objIcon.DelAction = "<a href='javascript:DeleteIcon(\"" + iconFile + "\");'><img src='styles/images/delete.gif'></a>";
	                objIcon.cellClass = (j % 2 == 0) ? "tableCell" : "tableCell2";
	                icons[j++] = objIcon;
	            };
	        } else {
	            if (iconFile == null) {
	                var objIcon = new Object();
	                objIcon.DeviceModel = result[i][0];
	                objIcon.DeviceRole = result[i][1];
	                objIcon.DeviceRoleName = GetDeviceRole(result[i][1]);
	                objIcon.ModelAscii = GetAsciiString(result[i][0]);
	                objIcon.Icon = null;
	                objIcon.DelAction = null;
	                objIcon.File = null;
	                objIcon.cellClass = null;
	                icons[j++] = objIcon;
	            };
	        };
    	};    
    };    
    icons.customicons = icons;
    if (icons == null || icons.length == 0) {
        return null;
    } else {
        return icons;    
    }
}

function GetCustomIconFiles() {
	var result;
    try  {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       
       result = service.file.exists({file : WebsitePath + "styles/images/custom/*.*"});
    } catch(e) {
        HandleException(e, "Unexpected error getting Icon Files!");
        return;
    } 
    return result;
}

function GetIconFile(iconsList, deviceModel, deviceRole) {
    if (iconsList == null) {
        return null;
    }
    var iconFileName = null;
    for (var i=0; i<iconsList.length; i++) {
        iconFileName = getFileName(iconsList[i]);
        if (iconFileName.split(".")[0] == deviceModel && iconFileName.split(".")[1] == deviceRole) {
            return iconsList[i];
        }
    }
}

function GetIconFileName(iconsList, deviceModel, deviceRole) {
    if (deviceModel != null && deviceModel != "" && deviceRole != null && deviceRole != "") {
        var modelIcon = GetIconFile(iconsList, deviceModel, deviceRole);
    	if (modelIcon != null) {
    		return "custom/" + deviceModel + "." + deviceRole + "." + modelIcon.split(".")[2];
        }
    }
    return GetDeviceRoleImage(deviceRole, 2);
}

function GetAllDevicesRevisions(deviceTypes){
	var myQuery = " SELECT Revision " +
				  "	FROM DevicesInfo I "+
				  	   " INNER JOIN Devices D ON D.DeviceID = I .DeviceID " +
				  " WHERE D.DeviceStatus >= " + DS_JoinedAndConfigured + " AND " + 
				        " D.DeviceRole IN " + ConvertArrayToSqlliteParam(deviceTypes) + " ";
	var result;
	try  {
	var service = new jsonrpc.ServiceProxy(serviceURL, methods);
		result = service.sqldal.execute({query: myQuery});
	} catch(e) {
		HandleException(e, "Unexpected error running GetAllDevicesRevisions!");
		return;
	}; 
	
	var arrElements = [];
	for (i = 0; i < result.length; i++) {
		result[i].RevisionHex    = result[i][0]; 
		result[i].RevisionString = GetAsciiString(result[i][0]);
		if (!ElementExists(result[i], arrElements)){
			arrElements.push(result[i]);
		};
    };      
    return arrElements; 
}

function GetDevicePageForFirmwareExecution(pageSize, pageNo, rowCount, euiAddress, deviceTag, revision, deviceTypes, level, orderBy, orderDirection) {
    var rowOffset = 0;
    TotalPages = 0;
    
   	if (rowCount != 0) {
		var maxAvailablePageNo = Math.ceil(rowCount/pageSize);
		
		if (pageNo > maxAvailablePageNo) {
			pageNo = maxAvailablePageNo;
		}
        //global var
	    TotalPages = maxAvailablePageNo;
        rowOffset = (pageNo - 1) * pageSize;
    }
 	
    var whereClause = " WHERE DeviceStatus >= " + DS_JoinedAndConfigured + " ";    
    
    if (euiAddress) {
       whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " Address64 LIKE '%" + euiAddress + "%' ";
    }
    if (deviceTag) {
       whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " DeviceTag LIKE '%" + deviceTag + "%' ";
    }
    if (revision) {
        whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " Revision = '" + revision + "' ";
    }    
	if (deviceTypes){		
		 whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " DeviceRole IN " + ConvertArrayToSqlliteParam(deviceTypes) + " ";
	}       
    var myQuery = " SELECT  D.DeviceID, D.Address64, D.DeviceTag, D.DeviceRole, I.Model, I.Revision " +
                  " FROM    Devices D " +
                  "			INNER JOIN DevicesInfo I ON D.DeviceID = I.DeviceID " 
                  + whereClause +
                  " ORDER BY Address64 " + orderDirection;// LIMIT " + pageSize + " OFFSET " + rowOffset;
   
    var result;
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetDevicePageForFirmwareExecution !");
        return null;
    }  
    
    var devLevels = GetDevicesLevels();    
    for (var i=0;i<result.length; i++) {    	    	    	
        result[i].DeviceID 	= result[i][0];        
        result[i].Address64	= result[i][1];
        result[i].DeviceTag = (GetAsciiString(result[i][2]) != "") ? GetAsciiString(result[i][2]) : NAString;
        result[i].DeviceRole = GetDeviceRole(result[i][3]) + "/" + GetAsciiString(result[i][4]);                  
        result[i].Revision = GetAsciiString(result[i][5]);        
        result[i].Level = devLevels[result[i][0]];
        result[i].Checked = "<input type=\"checkbox\" id=\"chkDevice" + result[i].DeviceID + "\" style=\"vertical-align:sub;\" onclick = \"AddRemoveOneDevice()\"/>";        
   };    	    	

   var filteredResult = [];
   if (level != null && level != ""){
	   for (var i=0; i<result.length; i++){
		   if (devLevels[result[i].DeviceID] == Number(level)){
			   filteredResult.push(result[i]);
		   };    		
	   };
	} else {
		filteredResult = result;	
	}    
       
    if (orderBy == 1){
    	var orderedResult = [];
    	var min;
    	if (orderDirection == OD_ASC){
        	for (var i=0; i<filteredResult.length; i++){
        		min = i;
        		for (j=i+1; j<filteredResult.length; j++){
        			if (filteredResult[j].Level < filteredResult[min].Level){
        				min=j;
        			};
        		};
        		if (i != min){
        			var swap = filteredResult[i];
        			filteredResult[i] = filteredResult[min];
        			filteredResult[min] = swap;
        		};
        	};            		
    	} else{
        	for (var i=0; i<filteredResult.length; i++){
        		max = i;
        		for (j=i+1; j<filteredResult.length; j++){
        			if (filteredResult[j].Level > filteredResult[max].Level){
        				max=j;
        			};
        		};
        		if (i != max){
        			var swap = filteredResult[i];
        			filteredResult[i] = filteredResult[max];
        			filteredResult[max] = swap;
        		};
        	};        
    	};
    };
    
    var requestedPage = [];
    requestedPage = filteredResult.slice(rowOffset, rowOffset+Number(pageSize));    
    for(var i=0; i<requestedPage.length; i++){
        if (i % 2 == 0) {
        	requestedPage[i].cellClass = "tableCell";
        } else {   
        	requestedPage[i].cellClass = "tableCell2";
        };        	       		    	
    }
    requestedPage.devices = requestedPage; 
	return requestedPage;
    
}


function GetDeviceChannels() {
    var myQuery = " SELECT D.Address64, D.DeviceID, C.ChannelName, C.ChannelFormat, C.ChannelNo " +
                  " FROM DeviceChannels C INNER JOIN Devices D ON C.DeviceID = D.DeviceID " +
                  " ORDER BY D.Address64, C.ChannelName ";	
    var result;     
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetDeviceChannels !");
        return null;
    }    
    for (i=0;i<result.length; i++) {
    	result[i].Address64 	= result[i][0];
    	result[i].DeviceID 		= result[i][1];
        result[i].ChannelName  	= result[i][2];       
        result[i].ChannelFormat = GetChannelFormatName(result[i][3]);        
        result[i].ChannelNo		= result[i][4];
    }    
    return result;
}

function GetDeviceWithChannels() {
    var myQuery = " SELECT D.Address64, D.DeviceID " +
                  " FROM DeviceChannels C INNER JOIN Devices D ON C.DeviceID = D.DeviceID " +
                  " LEFT OUTER JOIN Dashboard S ON C.ChannelNo = S.ChannelNo " +
                  " WHERE S.ChannelNo is null " +
                  " ORDER BY D.Address64 ";	
    var result;     
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetDeviceWithChannels !");
        return null;
    }    
    var arrElements = [];
    for (i = 0; i < result.length; i++) {
    	result[i].Address64 	= result[i][0];
    	result[i].DeviceID 		= result[i][1];
    	if (!ElementExists(result[i], arrElements)){
    		arrElements.push(result[i]);
    	};
    };    
    return arrElements;
}

function GetChannelsForDevice(deviceId) {
    var myQuery = " SELECT C.ChannelNo, C.ChannelName " +
                  " FROM DeviceChannels C " +
                  " LEFT OUTER JOIN Dashboard S ON C.ChannelNo = S.ChannelNo " +               
                  " WHERE C.DeviceID = " + deviceId + " AND S.ChannelNo is null "	
    var result;     
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetChannelsForDevice !");
        return null;
    }    
    for (i=0;i<result.length; i++) {
    	result[i].ChannelNo 	= result[i][0];
    	result[i].ChannelName	= result[i][1];
    }    
    return result;
}

function GetNoOfDevicesInDashboard(){
	var myQuery = " SELECT COUNT(*) FROM Dashboard ";
	var result;     
	try {
		var service = new jsonrpc.ServiceProxy(serviceURL, methods);
		result = service.sqldal.execute({query:myQuery});
	} catch(e) {
		HandleException(e, "Unexpected error running GetNoOfDevicesInDashboard !");
		return null;
	}
	result.Count = result[0][0];
	return result;	
}


function AddDeviceInDashboard(slotNumber, deviceId, channelNo, gaugeType, minValue, maxValue){				    		
	var myQuery =  " BEGIN TRANSACTION;" +
				   " UPDATE Dashboard SET SlotNumber = SlotNumber + 1 WHERE SlotNumber >= " + slotNumber + ";" +
				   " INSERT INTO Dashboard(SlotNumber, DeviceID, ChannelNo, GaugeType, MinValue, MaxValue)" +
				   " VALUES(" + slotNumber + ", " + deviceId + ", " + channelNo + ", " + gaugeType + ", " + minValue + ", " + maxValue + ");" +
				   " COMMIT; ";
	try {
		var service = new jsonrpc.ServiceProxy(serviceURL, methods);
		service.sqldal.execute({mode:"write", query:myQuery});
	} catch(e) {
		HandleException(e, "Unexpected error running AddDeviceInDashboard !");
		return false;
	}	
	return true;	
}


function GetDevicesForDashboard(){				    		
	var myQuery = " SELECT D.SlotNumber, D.DeviceID, D.ChannelNo, D.GaugeType, D.MinValue, D.MaxValue, "+
				  		 " R.Address64, I.Manufacturer, I.Model, C.ChannelName, strftime('%H'||':'||'%M'||':'||'%S',DR.ReadingTime), DR.Value, R.DeviceRole " +
				  "	FROM Dashboard D " +
				  		" INNER JOIN DeviceChannels C ON D.ChannelNo = C.ChannelNo " +
				  		" INNER JOIN Devices R ON D.DeviceID = R.DeviceID " +
				  		" INNER JOIN DevicesInfo I ON D.DeviceID = I.DeviceID " +
				  		" LEFT OUTER JOIN DeviceReadings DR ON C.ChannelNo = DR.ChannelNo " +
				  "	ORDER BY SlotNumber ASC ";
	var result; 
	try {
		var service = new jsonrpc.ServiceProxy(serviceURL, methods);
		result = service.sqldal.execute({mode:"write", query:myQuery});
	} catch(e) {
		HandleException(e, "Unexpected error running GetDevicesForDashboard !");
		return false;
	}			
	var devices = [{}];
	var icons = GetCustomIconFiles();
	for (var i=0; i<result.length; i++){
        var image = new Image();        
        image.src = "styles/images/" + GetIconFileName(icons, result[i][8], result[i][12]);
		devices[i] = {	SlotNumber: 	result[i][0],
						DeviceID:		result[i][1],
						ChannelNo:		result[i][2],
						GaugeType:		result[i][3],
						MinValue:		result[i][4],
						MaxValue:		result[i][5],
						Address64:		result[i][6],
						Manufacturer:	GetAsciiString(result[i][7]).trim(),
						Model:			GetAsciiString(result[i][8]).trim(),
						ChannelName:	result[i][9],
						ReadingTime:	result[i][10],
						Value:			result[i][11],
						DeviceRole:		result[i][12],
						Icon:			image.src,
						IconWidth: 		(image.width  > MAX_DEVICE_ICON_SIZE) ? MAX_DEVICE_ICON_SIZE : image.width,
						IconHeight:		(image.height > MAX_DEVICE_ICON_SIZE) ? MAX_DEVICE_ICON_SIZE : image.height}
	}			
	if (result.length != 0){
		return devices
	}  else {
		return 
	}
}

function RemoveDeviceFromDashboard(slotNumber){
	var myQuery = 	" BEGIN TRANSACTION; " +
					" DELETE FROM Dashboard WHERE SlotNumber = " + slotNumber + "; " +
					" UPDATE Dashboard SET SlotNumber = SlotNumber - 1 WHERE SlotNumber > " + slotNumber + ";" +
					" COMMIT; ";
	var result;
	try {
		var service = new jsonrpc.ServiceProxy(serviceURL, methods);
		result = service.sqldal.execute({mode:"write",query:myQuery});
	} catch(e) {
		HandleException(e, "Unexpected error running RemoveDeviceFromDashboard !");
		return false;
	}	
	return true;
}


function GetDashboardHashKey(){
	var myQuery =  " SELECT SlotNumber, ChannelNo FROM Dashboard ORDER BY SlotNumber ";
	var result;
	try {
		var service = new jsonrpc.ServiceProxy(serviceURL, methods);
		result = service.sqldal.execute({query:myQuery});
	} catch(e) {
		HandleException(e, "Unexpected error running GetDashboardHashKey !");
		return null;
	}	
	
	if (result != null) {
		var hashSlots = new Array();
		var hashChannels = new Array();
		for (var i=0; i< result.length; i++){
			hashSlots.push(result[i][0]);
			hashChannels.push(result[i][1]);
		}	
		return hashSlots.toString() + "," + hashChannels.toString();	
	} else {
		return null;
	}
}

function GetDevicesForTroubleshootingPage(){
	var myQuery =  " SELECT Address64, Address128, DeviceTag, DeviceID FROM Devices ORDER BY Address64 ";
	var result;
	try {
		var service = new jsonrpc.ServiceProxy(serviceURL, methods);
		result = service.sqldal.execute({query:myQuery});
	} catch(e) {
		HandleException(e, "Unexpected error running GetDevicesForTroubleshootingPage !");
		return null;
	}	
	for (var i=0; i<result.length; i++){
		result[i].Address64 = result[i][0];
		result[i].Address128 = result[i][1];
		result[i].DeviceTag = GetAsciiString(result[i][2]);
		result[i].DeviceID = result[i][3];
		result[i].Checked = "<input type=\"checkbox\" id=\"chkDevice" + result[i].DeviceID + "\" style=\"vertical-align:sub;\" onclick = \"CheckUncheckDevice("+result[i].DeviceID+",this.checked)\"/>";
		if (i % 2 == 0) {
            result[i].cellClass = "tableCell";
        } else {   
            result[i].cellClass = "tableCell2";
        };		
	}	
	
    result.devices = result;
    return result;    
}