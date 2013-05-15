var methods = ["sqldal.execute", "user.logout"];

function AppendWhereCondition(whereClause, whereCondition) {
	if (whereClause.length == 0) {
		whereClause = whereCondition;
	} else {			
		whereClause = whereClause + " AND " + whereCondition;
	}
	return whereClause;
}

function GetReadingsCount(deviceId, channelNo, readingType) {
	if (deviceId != null && deviceId.length == 0) {
		deviceId = null;
	}
	if (channelNo != null && channelNo.length == 0)	{
		channelNo = null;
	}
	if (readingType != null && readingType.length == 0)	{
		readingType = null;
	}
	
	var whereClause = "";
    whereClause = " WHERE R.ReadingTime > '1970-01-01 00:00:00' ";
	
	if (deviceId != null) {			
		whereClause = whereClause + " AND (d.DeviceID = " + deviceId + ")";
	}
	if (readingType != null) {			
		whereClause = whereClause + " AND (r.ReadingType = " + readingType + ")";
	}
	if (channelNo != null) {			
		whereClause = whereClause + " AND (dc.ChannelNo = " + channelNo + ")";
	}
							                
	var myQuery =   "   SELECT  COUNT(*) " +
					"   FROM    DeviceReadings r " +
					"	        INNER JOIN Devices d ON d.DeviceID = r.DeviceID " +
					"	        INNER JOIN DeviceChannels dc ON d.DeviceID = dc.DeviceID AND dc.ChannelNo = r.ChannelNo " 
					+ whereClause;
      
    var result;  
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetReadingsCount !");
        return null;
    }
	
	if (result.length != 0) {    	
		return result[0][0];
	} else {			
        return 0;
	}	
}
 
function GetReadings(deviceId, channelNo, readingType, pageNo, pageSize, rowCount, forExport) {
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
	if (channelNo != null && channelNo.length == 0)	{
		channelNo = null;
	}
	if (readingType != null && readingType.length == 0)	{
		readingType = null;
	}
	
    var whereClause = "";
    /// work around for MH
    //if (whereClause.length > 0) {
        whereClause = " WHERE R.ReadingTime > '1970-01-01 00:00:00' " + whereClause + " ";
    //}
    
	if (deviceId != null) {			
		whereClause = whereClause +  " AND (d.DeviceID = " + deviceId + ")";
	}
	if (readingType != null) {			
		whereClause = whereClause + " AND (r.ReadingType = " + readingType + ")";
	}
	if (channelNo != null) {			
		whereClause = whereClause + " AND (dc.ChannelNo = " + channelNo + ")";
	}
                   
    var myQuery =   "   SELECT  d.Address64, r.ReadingTime, dc.ChannelName, r.Value, dc.UnitOfMeasurement," +
                    "           CASE r.ReadingType " +
                    "                WHEN 0 THEN 'On Demand' " +
                    "                WHEN 1 THEN 'Publish/Subscribe' " +
                    "                ELSE 'N/A' " +
                    "           END  AS ReadingType  " +
				    "   FROM    DeviceReadings r " + 
				    "	        INNER JOIN Devices d ON d.DeviceID = r.DeviceID " +
				    "	        INNER JOIN DeviceChannels dc ON d.DeviceID = dc.DeviceID AND dc.ChannelNo = r.ChannelNo	"
				    + whereClause  +
				    "   ORDER BY r.ReadingTime DESC " +
				    "   LIMIT " + pageSize + " OFFSET " + rowOffset;
    if (forExport) {
        myQuery = " SELECT 'EUI-64 Address', 'Timestamp', 'Channel Name', 'Value', 'Unit Of Measurement', 'Reading type' UNION ALL " + myQuery;        
        return myQuery;
    }
	
	var result;
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetReadings !");
        return null;
    }
	
	var readings = new Array();		
	if (result.length != 0) {
	    for (i=0;i<result.length;i++) {
			var reading = new Reading();
			reading.Address64 = result[i][0];	
			reading.ReadingTime = result[i][1];
			reading.ChannelName = result[i][2];
    		reading.Value = result[i][3];
			reading.UnitOfMeasurement = result[i][4];
			reading.ReadingType = result[i][5];
			
			if (i % 2 == 0) {
                reading.cellClass = "tableCell2";
            } else {   
                reading.cellClass = "tableCell";
            }
			readings[i] = reading;
		}		
	}
	readings.readings = readings;
	return readings;
}
