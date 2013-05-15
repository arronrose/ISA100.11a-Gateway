var methods = ["sqldal.execute", "user.logout", "file.remove", "file.create", "file.exists"];

function GetTopology() {
	var myQuery =	"	SELECT	D.DeviceID, " +
					"			D.Address64, " +
					"			D.DeviceRole, " +
					"			D.DeviceLevel, " +
					"			I.SubnetID, " +
					"			I.Model, " +
					"			d.DeviceTag, " +
					"			I.Manufacturer " +
					"	FROM	Devices D " +
					"			INNER JOIN DevicesInfo I ON D.DeviceID = I.DeviceID " +
					"	WHERE	DeviceStatus >= " + DS_JoinedAndConfigured + " AND " +
					"			(SubnetID IS NULL OR SubnetID >= 0) " + //DeviceRole NOT IN (" + DT_SystemManager + ", " + DT_Gateway + ") " +
					"	ORDER BY I.SubnetID ASC, DeviceLevel ASC";

	var result;
	try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error on getting topology");
        return null;
    }			

    var asciiString;
    var icons = GetCustomIconFiles();
    if (result.length != 0) {
		for (var i=0; i<result.length; i++) {
			result[i].DeviceID = result[i][0];
			result[i].Address64 = result[i][1];
			result[i].DeviceRole = result[i][2];
			result[i].SubnetID = result[i][4];
			result[i].X = 0;
			result[i].Y = 0;
			switch (result[i].DeviceRole) {
            case DT_Gateway: 
                result[i].Label = "GW"; break;
            case DT_BackboneRouter: 
                result[i].Label = "BBR"; break;
            case DT_SystemManager: 
                result[i].Label = "SM"; break;
            default: 
                result[i].Label = result[i][1].substr(result[i][1].length - 4, 4);
            }             
			result[i].ModelHex = result[i][5];
			asciiString = GetAsciiString(result[i][5]);
			result[i].Model = result[i][5] != null ? asciiString != "" ? asciiString : NAString : NAString;
			asciiString = GetAsciiString(result[i][6]);
			result[i].DeviceTag = result[i][6] != null ? asciiString != "" ? asciiString : NAString : NAString;
			asciiString = GetAsciiString(result[i][7]);
			result[i].Manufacturer = result[i][7] != null ? asciiString != "" ? asciiString : NAString : NAString;
			result[i].Icon = "styles/images/" + GetIconFileName(icons, result[i][5], result[i][2]);
		}
		result.devices = result;
		return result
    } else {
		return null;
    }
}

function GetTopologyLinks() {
	var myQuery = "SELECT T.FromDeviceID, T.ToDeviceID,  T.ClockSource,  T.SignalQuality,  DT.DeviceRole ToDeviceRole , " +
                  "       ROUND(100.0 * N.DPDUsFailedTransmission / (N.DPDUsFailedTransmission + N.DPDUsTransmitted), 1) Per " + 
                  "  FROM TopologyLinks T " + 
                  "       LEFT JOIN NeighborHealthHistory N ON N.DeviceID = T.FromDeviceID AND N.NeighborDeviceID = T.ToDeviceID AND N.LinkStatus = 1 " + 
                  "             AND N.Timestamp = (SELECT MAX(Timestamp) FROM NeighborHealthHistory WHERE LinkStatus = 1 AND DeviceID = T.FromDeviceID) " + 
                  "       INNER JOIN Devices DF ON T.FromDeviceID = DF.DeviceID " +  
                  "       INNER JOIN Devices DT ON T.ToDeviceID = DT.DeviceID " + 	
                  " WHERE DF.DeviceStatus >= " + DS_JoinedAndConfigured + " AND " + 
                  "       DT.DeviceStatus >= " + DS_JoinedAndConfigured + " ";
				//"       DF.DeviceRole NOT IN (" + DT_SystemManager + ", " + DT_Gateway + ") AND DT.DeviceRole NOT IN (" + DT_SystemManager + ", " + DT_Gateway + ") "

	var result;
	try {
        var service = new jsonrpc.ServiceProxy(serviceURL, methods);
        result = service.sqldal.execute({query: myQuery});
    } catch (e) {
        HandleException(e, "Unexpected error on getting topology links");
        return null;
    }
	var links = [{FromDeviceID: -1, ToDeviceID: -1}];
	var idx = 0;
    if (result.length != 0) {
		for (var i = 0; i < result.length; i++) {
            var dupLink = false;
            var dupIdx = -1;
            for (var j = 0; j < links.length; j++) {
                if ((result[i][0] == links[j].FromDeviceID && result[i][1] == links[j].ToDeviceID) ||
                    (result[i][0] == links[j].ToDeviceID && result[i][1] == links[j].FromDeviceID)) {
                    dupLink = true;
                    dupIdx = j;
                }
            }
            if (dupLink) {
                links[dupIdx].Bidirectional = true;
				links[dupIdx].ClockSource2 = result[i][2];
				links[dupIdx].SignalQuality2 = "" + result[i][3] + ((result[i][2] == 1 || result[i][2] == 2) ? " / " + ((result[i][5] == 'NULL') ? 0 : result[i][5] + "%") : "");
				links[dupIdx].FromDeviceRole = result[i][4];
            } else {
                links[idx++] = {FromDeviceID: result[i][0], 
				                ToDeviceID: result[i][1], 
								Bidirectional: false, 
								ClockSource: result[i][2], 
								ClockSource2: 0,
								SignalQuality: "" + result[i][3] + ((result[i][2] == 1 || result[i][2] == 2) ? " / " + ((result[i][5] == 'NULL') ? 0 : result[i][5] + "%") : ""),
								SignalQuality2: "",
								ToDeviceRole: result[i][4]};
            }
		}
		return links
    } else {
		return null;
    }					
}

function GetContracts(deviceId) { 
    if (deviceId == null)
       return null;

    var myQuery = " SELECT c.ContractID, " +
                  "        c.SourceDeviceID, " +
                  "        CASE s.DeviceRole " +
                  "             WHEN " + DT_Gateway + " THEN 'GW' " +
                  "             WHEN " + DT_BackboneRouter + " THEN 'BBR' " +
                  "             WHEN " + DT_SystemManager + " THEN 'SM' " +
                  "             ELSE s.Address64 END AS SourceAddress64, " +
                  "        c.SourceSAP, " +
                  "        CASE d.DeviceRole " +
                  "             WHEN " + DT_Gateway + " THEN 'GW' " +
                  "             WHEN " + DT_BackboneRouter + " THEN 'BBR' " +
                  "             WHEN " + DT_SystemManager + " THEN 'SM' " +
                  "             ELSE d.Address64 END AS DestinationAddress64, " +
                  "        c.DestinationSAP " +    
                  "   FROM Contracts c " +
                  "        INNER JOIN Devices s ON c.SourceDeviceID = s.DeviceID AND s.DeviceStatus >= " + DS_JoinedAndConfigured + " " +
                  "        INNER JOIN Devices d ON c.DestinationDeviceID = d.DeviceID AND d.DeviceStatus >= " + DS_JoinedAndConfigured + " " +
                  "  WHERE c.SourceDeviceID = " + deviceId + " OR " +
				  "        c.DestinationDeviceID = " + deviceId + " " +
                  "  ORDER BY SourceAddress64, c.ContractID";
    var result;
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetContracts");
        return null;
    }
	if (result.length != 0) {
	    for (i=0;i<result.length;i++) {
            result[i].ContractID = result[i][0];
            result[i].SourceDeviceID = result[i][1];
            result[i].SourceDestination = result[i][0] + ": " + 
			             result[i][2] + "/" + result[i][3] + " -> " + 
			             result[i][4] + "/" + result[i][5];
			if (i % 2 == 0) {
                result[i].cellClass = "tableCell2";
            } else {
                result[i].cellClass = "tableCell";
            }
		}
	   result.contracts = result;
	   return result;    
	} else {
	   return null;
	}
}


function GetContractDetails(contractId, deviceId) {
    if (deviceId == null || contractId == null)  {
        alert("Invalid deviceId or contractId for GetContractDetails");
        return null;
    }
    var myQuery = " SELECT  c.ContractID, " +
                  "         c.ServiceType, " +
                  "         c.ActivationTime, " +
                  "         s.Address64 SrcDeviceAddress64, " +
                  "         c.SourceSAP, " +
                  "         d.Address64 DstDeviceAddress64, " +
                  "         c.DestinationSAP, " +
                  "         c.ExpirationTime, " +
                  "         c.Priority, " +
                  "         c.NSDUSize, " +
                  "         c.Reliability, " +
                  "         c.Period, " +
                  "         c.Phase, " +
                  "         c.ComittedBurst, " +
                  "         c.ExcessBurst, " +
                  "         c.Deadline, " +
                  "         c.MaxSendWindow " +
                  " FROM    Contracts c " +
                  "         INNER JOIN Devices s ON s.DeviceID = c.SourceDeviceID" +
                  "         INNER JOIN Devices d ON d.DeviceID = c.DestinationDeviceID" +
                  " WHERE   c.SourceDeviceID = " + deviceId + " AND " +
                  "         c.ContractID = " + contractId;
                        
    var result;
     
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetContractDetails");
        return null;
    }
	if (result.length != 0) {
		var contract = new Object();
        contract.ContractID = result[0][0];
        contract.ServiceType = result[0][1];
        contract.ActivationTime = result[0][2];
        contract.SrcDeviceAddress64 = result[0][3];
        contract.SourceSAP = result[0][4];
        contract.DstDeviceAddress64 = result[0][5];
        contract.DestinationSAP = result[0][6];
        contract.ExpirationTime = result[0][7];
        contract.Priority = GetContractPriorityName(result[0][8]);
        contract.NSDUSize = result[0][9];
        contract.Reliability = GetContractReliabilityName(result[0][10]);
        contract.Period = result[0][11];
        contract.Phase = result[0][12];
        contract.ComittedBurst = result[0][13];
        contract.ExcessBurst = result[0][14];
        contract.Deadline = result[0][15];
        contract.MaxSendWindow = result[0][16];
	}
	return contract; 
}


function GetContractElements(contractId, deviceId) {
    if (deviceId == null)
       return null;

	var myQuery =	"	SELECT	C.SourceDeviceID, " +
					"			C.ServiceType, " +
					"			C.ContractID, " +
					"           E.Idx, " +
					"           E.DeviceID " +
					"	FROM	Contracts C" +
					"           INNER JOIN ContractElements E ON C.ContractID = E.ContractID AND C.SourceDeviceID = E.SourceDeviceID  " +
					"	WHERE	C.ContractID = " + contractId + " AND " + 
                    "           C.SourceDeviceID = " + deviceId + 
					"   ORDER BY C.SourceDeviceID ASC, C.ContractID ASC, E.Idx ASC ";
	var result;
	try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetContractsElements");
        return null;
    }			
    if (result.length != 0) {
		for(i=0;i<result.length;i++) {
			result[i].SourceDeviceID = result[i][0];
			result[i].ServiceType = result[i][1];
			result[i].ContractID = result[i][2];
			result[i].Idx = result[i][3];
			result[i].DeviceID = result[i][4];
		}
		result.contractLinks = result;
		return result
    } else {
		return null;
    }					
}


function GetContractsElements(deviceId, mode) {
	if (deviceId == null)
	   return null;
	   
	var myQuery =	"	SELECT	C.SourceDeviceID, " +
					"			C.ServiceType, " +
					"			C.ContractID, " +
					"           E.Idx, " +
					"           E.DeviceID " +
					"	FROM	Contracts C" +
					"           INNER JOIN ContractElements E ON C.ContractID = E.ContractID AND C.SourceDeviceID = E.SourceDeviceID " +
					"   WHERE ";
	myQuery	+= (mode == 2) ? " C.SourceDeviceID = " + deviceId + " " :
			   (mode == 3) ? " C.DestinationDeviceID = " + deviceId + " " :
				   			 " C.SourceDeviceID = " + deviceId + " OR DestinationDeviceID = " + deviceId + " ";
	myQuery	+= "   ORDER BY C.SourceDeviceID ASC, C.ContractID ASC, E.Idx ASC ";
	
	var result;
	try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetContractsElements");
        return null;
    }
    if (result.length != 0) {
		for(i=0;i<result.length;i++) {
			result[i].SourceDeviceID = result[i][0];
			result[i].ServiceType = result[i][1];
			result[i].ContractID = result[i][2];
			result[i].Idx = result[i][3];
			result[i].DeviceID = result[i][4];
		}
		result.contractLinks = result;
		return result
    } else {
		return null;
    }					
}


function GetLinkType(sourceDeviceId, destinationDeviceID) {
	var myQuery =	"	SELECT	S.DeviceRole, " +
					"			D.DeviceRole, " +
					"			C.ServiceType " +
					"	FROM	Contracts C " +
					"			INNER JOIN Devices S ON C.SourceDeviceID = S.DeviceID AND S.DeviceID = " + sourceDeviceId + " " +
					"			INNER JOIN Devices D ON C.DestinationDeviceID = D.DeviceID AND D.DeviceID = " + destinationDeviceID;
	var result;
	try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);
     
       result = service.sqldal.execute({query:myQuery});
    } catch(e) {
        HandleException(e, "Unexpected error running GetLinkType");
        return null;
    }

    var linkType = 0;     
	if (result.length != 0) {
		result.SourceRole = result[0][0];
		result.DestinationRole = result[0][1];
		result.ServiceType = result[0][2];

		if (result.ServiceType == CST_Periodic) {
			/* SOURCE = Device and DEST = GW*/
			if (IsFieldDevice(result.SourceRole) && result.DestinationRole == DT_Gateway) {
				linkType = linkType | LT_PublishSubscribe;
			};
			/* SOURCE = Device and DEST = Device/BBR */
			if (IsFieldDevice(result.SourceRole) && (result.DestinationRole == DT_BackboneRouter || IsFieldDevice(result.DestinationRole))) {
				linkType = linkType | LT_LocalLoop;
			};
			/* SOURCE = Device/BBR and DEST = SM */
			if ((result.SourceRole == DT_BackboneRouter || IsFieldDevice(result.SourceRole)) && (result.DestinationRole == DT_SystemManager)) {
				linkType = linkType | LT_PeriodicPublishing;
			}
		} else {	/*Aperiodic contract service type*/
			linkType = linkType | LT_Aperiodic;
		}
		return linkType;		
    } else {
		return null;
    }    
}

