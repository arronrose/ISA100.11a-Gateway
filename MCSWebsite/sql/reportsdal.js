var methods = ["sqldal.execute", "user.logout"];

function GetNeighborsForScheduleReportLinksPage(deviceId, superframeId)
{        
    var whereClause = "	WHERE L.DeviceID = " + deviceId + " AND L.SuperframeID = " + superframeId;
    
 	var myQuery =	" SELECT CASE WHEN D.Address64 is null THEN 'FFFF:FFFF:FFFF:FFFF' ELSE D.Address64 END AS Address64, " +
 	                	   " L.NeighborDeviceID AS DeviceID" +
					" FROM DeviceScheduleLinks L " +
						   " LEFT OUTER JOIN Devices D ON L.NeighborDeviceID = D.DeviceID " 
					+	whereClause ;

    var result;    
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);   
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetNeighborsForScheduleReportLinksPage");
        return null;
    }    
    var arrElements = [];
	if (result.length != 0) {
		for (i=0;i<result.length;i++) {
			if (!ElementExists(result[i], arrElements)){
				result[i].Address64 = result[i][0];
				result[i].DeviceID = result[i][1];
				arrElements.push(result[i]);				
			};
		};
		return arrElements;
	} else {
		return null;
	};
}


function GetNeighborsHealth(deviceId, pageSize, pageNo, rowCount, sortType, sortOrder) {
    var rowOffset = 0;
    TotalPages = 0;
    
   	if (rowCount != 0) {
		var maxAvailablePageNo = Math.ceil(rowCount/pageSize);	
		if (pageNo > maxAvailablePageNo) {
			pageNo = maxAvailablePageNo;
		}
	    TotalPages = maxAvailablePageNo;		
        rowOffset = (pageNo - 1) * pageSize;
    }

    var myQuery =	"	SELECT	D.Address64, " +
					"			N.TimeStamp, " +
					"			N.LinkStatus, " +
					"			N.DPDUsTransmitted, " +
					"			N.DPDUsFailedTransmission, " +
					"			N.DPDUsReceived, " + 
					"			N.DPDUsFailedReception, " +
					"			N.SignalStrength, " + 
					"			N.SignalQuality " +
					"	FROM	NeighborHealthHistory N " +
					"			INNER JOIN Devices D ON N.NeighborDeviceID = D.DeviceID " +
					"	WHERE   N.DeviceID = " + deviceId + 
                    "     AND   N.TimeStamp IN (SELECT MAX(TimeStamp) FROM NeighborHealthHistory WHERE DeviceId = " + deviceId + ") " +
					"     ORDER BY " + GetOrderByColumnNameNeighborsHealth(sortType) + " " + sortOrder + 
					"	  LIMIT " + pageSize + " OFFSET " + rowOffset;
    var result;
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);   
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetNeighborsHealth");
        return null;
    }    
	if (result.length != 0) {
		for (i=0;i<result.length;i++) {
			result[i].NeighborAddress64 = result[i][0];
			result[i].TimeStamp = result[i][1];            
			result[i].LinkStatus = GetLinkStatusName(result[i][2]);
			result[i].DPDUsTransmitted = IfNullStr(result[i][3]);
			result[i].DPDUsFailedTransmission = IfNullStr(result[i][4]);
			result[i].DPDUsReceived = IfNullStr(result[i][5]);
			result[i].DPDUsFailedReception = IfNullStr(result[i][6]);
			
			result[i].TransmittedAndFailed = result[i].DPDUsTransmitted + "/" + result[i].DPDUsFailedTransmission;
			result[i].ReceivedAndFailed = result[i].DPDUsReceived + "/" + result[i].DPDUsFailedReception;
			
			result[i].SignalStrength = GetSignalStrength(result[i][7]);
    		result[i].SignalQuality = " <span id=\"spnSignalQuality\" style=\"color:" + GetSignalQualityColor(result[i][8]) + "\">" + GetSignalQuality(result[i][8]) + "</span> (" + result[i][8] + ")";
			
		    if (i % 2 == 0) {
                result[i].cellClass = "tableCell";
            } else {   
                result[i].cellClass = "tableCell2";
            }
		}
		result.neighborshealth = result;
		return result;
	} else {
		return null;
	}
}

function GetNeighborsHealthCount(deviceId) {
    var myQuery =	"	SELECT	COUNT(*) " +
					"	FROM	NeighborHealthHistory N INNER JOIN Devices D ON N.NeighborDeviceID = D.DeviceID " +
					"	WHERE   N.DeviceID = " + deviceId + 
                    "     AND   N.TimeStamp IN (SELECT MAX(TimeStamp) FROM NeighborHealthHistory WHERE DeviceId = " + deviceId + ")";

    var result; 
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);   
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetNeighborsHealthCount");
        return null;
    }    
	if (result.length != 0) {
		return result[0][0];
	} else {
		return null;
	}
}


/* Device Schedule Report */

function GetDeviceScheduleReportPage(deviceId, pageSize, pageNo, rowCount) {
    var rowOffset = 0;
    TotalPages = 0;
    
   	if (rowCount != 0) {
		var maxAvailablePageNo = Math.ceil(rowCount/pageSize);	
		if (pageNo > maxAvailablePageNo) {
			pageNo = maxAvailablePageNo;
		}
	    TotalPages = maxAvailablePageNo;		
        rowOffset = (pageNo - 1) * pageSize;
    }
        
	var myQuery =	"	SELECT	DeviceID, " +
					"			SuperframeID, " +
					"			NumberOfTimeSlots, " +
					"			StartTime, " +
					"			NumberOfLinks " +
					"	FROM	DeviceScheduleSuperframes " +
					"	WHERE	DeviceID = " + deviceId +
					"	ORDER BY StartTime DESC " +
					"	LIMIT " + pageSize + " OFFSET " + rowOffset;
    var result;    
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);   
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetDeviceScheduleReportPage");
        return null;
    }    
	if (result.length != 0)	{
		for (var i=0;i<result.length;i++) {
			result[i].DeviceID = result[i][0];
			result[i].SuperframeID = result[i][1];
			result[i].NumberOfTimeSlots = result[i][2];
			result[i].StartTime = result[i][3];
			if (result[i][4] > 0) {
                result[i].NumberOfLinks = "<a href='schedulereportlinks.html?deviceId=" + result[i].DeviceID + "&superframeId=" + result[i].SuperframeID + "'>" + result[i][4] + "</a>";
            } else {
                result[i].NumberOfLinks = "" + result[i][4];

            }
		    if (i % 2 == 0) {
                result[i].cellClass = "tableCell";
            } else {   
                result[i].cellClass = "tableCell2";
            }
		}
		result.schedulereport = result;
		return result;
	} else {
		return null;
	}
}

function GetDeviceScheduleReportCount(deviceId) {
	var myQuery =	"	SELECT	COUNT(*) " +
					"	FROM	DeviceScheduleSuperframes " +
					"	WHERE	DeviceID = " + deviceId ;
    var result;    
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);   
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetDeviceScheduleReportCount");
        return null;
    }    
	if (result.length != 0) {		
		return result[0][0];
	} else {
		return 0;
	}
}


function GetRFChannels() {
	var myQuery =	"	SELECT	ChannelNumber, " +
					"			ChannelStatus " +
					"	FROM	RFChannels " +
					"	ORDER BY ChannelNumber ASC ";
    var result;
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);   
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetRFChannels");
        return null;
    }    

	if (result.length != 0) {
		for (i=0;i<result.length;i++) {
			result[i].ChannelNumber = result[i][0];
			result[i].ChannelStatus = result[i][1];
		}
		result.devicerfchannels = result;
		return result;
	} else {
		return null;
	}
}

function GetDeviceScheduleReportLinksPage(deviceId, superframeId, neighborDeviceID, linkType, direction, pageSize, pageNo, rowCount) {
    var rowOffset = 0;
    TotalPages = 0;
    
   	if (rowCount != 0) {
		var maxAvailablePageNo = Math.ceil(rowCount/pageSize);	
		if (pageNo > maxAvailablePageNo) {
			pageNo = maxAvailablePageNo;
		}
	    TotalPages = maxAvailablePageNo;		
        rowOffset = (pageNo - 1) * pageSize;
    }
        
    var whereClause = "	WHERE	L.DeviceID = " + deviceId + " AND L.SuperframeID = " + superframeId;
    
    if (neighborDeviceID != null) {
		whereClause = whereClause + " AND L.NeighborDeviceID = " + neighborDeviceID;
    }
    if (linkType != null) {
		whereClause = whereClause + " AND L.LinkType = " + linkType + " ";
    } 
    if (direction != null) {
		whereClause = whereClause + " AND L.Direction = " + direction + " ";
    } 
	var myQuery =	"	SELECT	CASE WHEN D.Address64 is null THEN 'FFFF:FFFF:FFFF:FFFF' ELSE D.Address64 END AS Address64, " +
					"			L.SlotIndex, " +
					"			L.SlotLength, " +
					"			L.ChannelNumber, " +
					"			L.Direction, " +
					"			L.LinkType, " +
					"			L.LinkPeriod " +
					"	FROM	DeviceScheduleLinks L " +
					"			LEFT OUTER JOIN Devices D ON L.NeighborDeviceID = D.DeviceID " 
					+	whereClause +
					"	ORDER BY L.SlotIndex ASC " +
					"	LIMIT " + pageSize + " OFFSET " + rowOffset;

    var result;    
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);   
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetDeviceScheduleReportLinksPage");
        return null;
    }    
	if (result.length != 0) {
		for (i=0;i<result.length;i++) {
			result[i].NeighborAddress64 = result[i][0];
			result[i].SlotIndex = result[i][1];
			result[i].SlotLength = result[i][2];
			result[i].ChannelNumber = result[i][3];
			result[i].Direction = GetLinkDirectionName(result[i][4]);
			result[i].LinkType = GetLinkTypeName(result[i][5]);
			result[i].LinkPeriod = result[i][6];
		    if (i % 2 == 0) {
                result[i].cellClass = "tableCell";
            } else {   
                result[i].cellClass = "tableCell2";
            }
		}
		result.schedulereportlinks = result;
		return result;
	} else {
		return null;
	}
}


function GetDeviceScheduleReportLinksCount(deviceId, superframeId, neighborDeviceID, linkType, direction) {      
    var whereClause = "	WHERE	L.DeviceID = " + deviceId + " AND L.SuperframeID = " + superframeId;
    
    if (neighborDeviceID != null) {
		whereClause = whereClause + " AND L.NeighborDeviceID = " + neighborDeviceID;
    }
    if (linkType != null) {
		whereClause = whereClause + " AND L.LinkType = " + linkType + " ";
    } 
    if (direction != null) {
		whereClause = whereClause + " AND L.Direction = " + direction + " ";
    } 
	var myQuery =	"	SELECT	COUNT(*) " +
					"	FROM	DeviceScheduleLinks L " +
					"			LEFT OUTER JOIN Devices D ON L.NeighborDeviceID = D.DeviceID " 
					+	whereClause ;
    var result;    
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);   
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetDeviceScheduleReportLinksCount");
        return null;
    }    
	if (result.length != 0) {
		return result[0][0];
	} else {
		return null;
	}
}

/* Network Health Report */

function GetNetworkHealthReportHeader() {

	var myQuery =	"	SELECT	NetworkID, " +
					"			NetworkType, " +
					"			DeviceCount, " +
					"			StartDate, " +
					"			CurrentDate, " +
					"			DPDUsSent, " +
					"			DPDUsLost, " +
					"			GPDULatency, " +
					"			GPDUPathReliability, " +
					"			GPDUDataReliability, " +
					"			JoinCount " +
					"	FROM	NetworkHealth "; 
	var result;		
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);    
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetNetworkHealthReportHeader");
        return null;
    }		    
  
    if (result.length != 0)	{
		result.NetworkID = IfNullStr(result[0][0]);
		result.NetworkType = IfNullStr(result[0][1]);
		result.DeviceCount = IfNullStr(result[0][2]);
		result.StartDate = result[0][3];
		result.CurrentDate = result[0][4];
		result.DPDUsSent = IfNullStr(result[0][5]);
		result.DPDUsLost = IfNullStr(result[0][6]);
		result.GPDULatency = IfNullStr(result[0][7]);
		result.GPDUPathReliability = IfNullStr(result[0][8]);
		result.GPDUDataReliability = IfNullStr(result[0][9]);
		result.JoinCount = IfNullStr(result[0][10]);
		return result;
    } else {
		return null;
    }
}

function GetNetworkHealthReportPageCount() {      
 
	var myQuery =	"	SELECT	COUNT(*) " +
					"	FROM	NetworkHealthDevices N " +
					"			INNER JOIN Devices D ON N.DeviceID = D.DeviceID "  +
					"	ORDER BY D.Address64 ASC ";
    var result;    
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);   
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetNetworkHealthReportPageCount");
        return null;
    }    
	if (result.length != 0) {
		return result[0][0];
	} else {
		return 0;
	}
}

function GetNetworkHealthReportPage(pageSize, pageNo, rowCount) {
    var rowOffset = 0;
    TotalPages = 0;
    
   	if (rowCount != 0) {
		var maxAvailablePageNo = Math.ceil(rowCount / pageSize);	
		if (pageNo > maxAvailablePageNo) {
			pageNo = maxAvailablePageNo;
		}
	    TotalPages = maxAvailablePageNo;		
        rowOffset = (pageNo - 1) * pageSize;
    }
    
 	var myQuery =	"	SELECT	D.Address64, " +
					"			StartDate, " +
					"			CurrentDate, " +
					"			DPDUsSent, " +
					"			DPDUsLost, " +
					"			GPDULatency, " +
					"			GPDUPathReliability, " +
					"			GPDUDataReliability, " +
					"			JoinCount " +
					"	FROM	NetworkHealthDevices N " +
					"			INNER JOIN Devices D ON N.DeviceID = D.DeviceID "  +
					"	ORDER BY D.Address64 ASC " +
                    "	LIMIT " + pageSize + " OFFSET " + rowOffset;
	var result;		
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);    
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetNetworkHealthReportPage");
        return null;
    }		    
  
    if (result.length != 0)	{
		for(i=0;i<result.length;i++) {
			result[i].Address64 = IfNullStr(result[i][0]);
			result[i].StartDate = result[i][1];
			result[i].CurrentDate = result[i][2];
			result[i].DPDUsSent = IfNullStr(result[i][3]);
			result[i].DPDUsLost = IfNullStr(result[i][4]);
			result[i].GPDULatency = IfNullStr(result[i][5]);
			result[i].GPDUPathReliability = IfNullStr(result[i][6]);
			result[i].GPDUDataReliability = IfNullStr(result[i][7]);
			result[i].JoinCount = IfNullStr(result[i][8]);
		    if (i % 2 == 0) {
                result[i].cellClass = "tableCell";
            } else {   
                result[i].cellClass = "tableCell2";
            }
		}
		result.networkhealthdevices = result;
		return result;
    } else {
		return null;
    }
}

function GetOrderByColumnNameNeighborsHealth(sortType) {
	switch (sortType) {
   		case 1:
			return	" Address64 ";
		case 2:
			return	" Timestamp ";
  		default:
			return	" Address64 ";
	}
}
