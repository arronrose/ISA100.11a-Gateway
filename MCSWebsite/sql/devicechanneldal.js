var methods = ["sqldal.execute", "user.logout", "user.isValidSession"];

function GetChannelDetails(deviceID, channelNo) {
    var myQuery = "	SELECT * FROM DeviceChannels WHERE DeviceID = " + deviceID + " AND ChannelNo = "  + channelNo;

    var result;
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running get channel details!");
        return null;
    }
	return result;
}

function GetChannelsForDevice(deviceId) {
    var myQuery =   "   SELECT  ChannelNo, ChannelName " +
                    "   FROM    DeviceChannels  " +
                    "	WHERE	DeviceID = " + deviceId +
                    "   ORDER BY ChannelNo  "
    var result;
    try  {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running get all channels!");
        return null;
    }
    var channelColl = new Array();
    for(i = 0; i<result.length; i++) {
        var channel = new DeviceChannel();
    	channel.ChannelNo = result[i][0];
	    channel.ChannelName = result[i][1];
    	channelColl[i] = channel;
    }
    return channelColl;
}

function GetChannelsStatistics(deviceId) {
    var myQuery = " SELECT ByteChannelsArray FROM ChannelsStatistics WHERE DeviceID = " + deviceId;
    var result;
    try  {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running get channels statistics!");
        return null;
    }
    
    if (result.length == 0 || result[0][0] == "NULL") {        
    	return null;
    } else {		
		var statistics = new Array();
		for (var i=0; i<16; i++) {
			var cs = new ChannelStatistics();
			cs.ChannelNo = 11+i;
			if (result[0][0].substr(2*i, 2) != ''){								
				cs.Value = HexToDec(result[0][0].substr(2*i, 2));
			} else {
				cs.Value = NAString;
			}	
			if (i % 2 == 0) {
				cs.cellClass = "tableCell";
	        } else {   
	        	cs.cellClass = "tableCell2";
	        }
			statistics[i] = cs;
	    }		
		statistics.statistics = statistics;
		return statistics;		
    }
}
