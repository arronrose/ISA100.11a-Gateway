var methods = ["sqldal.execute","sqldal.close","sqldal.open","user.logout","file.remove","file.create","file.exists","user.isValidSession"];

function GetFirmwareCount() {	
	var myQuery = "SELECT COUNT(*) FROM Firmwares";
    var result;
 
    try  {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);       
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "GetFirmwareCount statement failed to execute successfully");
        return null;
    }
		
	if (result.length != 0) {
		return result[0][0];
	} else {
		return 0;
	}
}

function GetFirmwareFilename(filename) {	
	var myQuery = "SELECT COUNT(*) FROM Firmwares WHERE FileName = '" + filename + "'";
    var result;
 
    try  {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);       
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "GetFirmwareCount statement failed to execute successfully");
        return null;
    }
		
	if (result.length != 0) {
		return result[0][0];
	} else {
		return 0;
	}
}

function GetFirmwaresPage(pageNo, pageSize, rowCount, forExport) {	
    var rowOffset = 0;   
   	if (rowCount != 0 && forExport == false) {
		var maxAvailablePageNo = Math.ceil(rowCount/pageSize);
		
		if (pageNo > maxAvailablePageNo) {
			pageNo = maxAvailablePageNo;
		}   
	    TotalPages = maxAvailablePageNo;			
        rowOffset = (pageNo - 1) * pageSize;
    }	
	var myQuery =	"	SELECT	FirmwareID, FileName, Version, Description, UploadDate, FirmwareType, UploadStatus " +
					"	FROM	Firmwares " +
					"	ORDER BY UploadDate DESC " +
					"	LIMIT " + pageSize + " OFFSET " + rowOffset;
    if (forExport) {
        myQuery =	" SELECT 'FirmwareID', 'FileName', 'Version', 'Description', 'UploadDate', 'FirmwareType', 'UploadStatus'UNION ALL " + myQuery;
        return myQuery;
    }
						
    var result;
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);     
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "GetFirmwares statement failed to execute successfully");
        return null;
    }  
			
	if (result.length != 0) {
		var firmwares = Array();	
		for (i=0;i<result.length;i++) {
			var firmware = new Firmware();
			firmware.FirmwareID = result[i][0];			
			firmware.FileName = "&nbsp;<a href='fwdetails.html?firmwareId=" + firmware.FirmwareID + "'>" + result[i][1] + "</a>";			
			firmware.Version = result[i][2];
			firmware.Description= result[i][3];			
			firmware.UploadDate = result[i][4];
			firmware.FirmwareTypeID = result[i][5];
			firmware.UploadStatusID = result[i][6];
			firmware.FirmwareTypeName = GetFirmwareTypeName(result[i][5]);
			firmware.UploadStatusName = GetUploadStatusName(result[i][6]);
			if (firmware.Description.length > 20){
				firmware.PopupLink = "<a href='javascript:PopupValue(\"modalPopup\", \"Firmware Description\", \"" + firmware.Description + "\")'>" + firmware.Description.substring(0,20) + "...</a>";
			} else {
				firmware.PopupLink = firmware.Description; 
			};
			if (firmware.Version.length > 20){
				firmware.VersionPopupLink = "<a href='javascript:PopupValue(\"modalPopup\", \"Firmware Version\", \"" + firmware.Version + "\")'>" + firmware.Version.substring(0,20) + "...</a>";
			} else {
				firmware.VersionPopupLink = firmware.Version; 
			};
			firmware.DeleteAction = "<a href='javascript:DeleteFWFile("+firmware.FirmwareID +","+firmware.UploadStatusID+")'><img src='styles/images/delete.gif' /></a>"			
			
			if (i % 2 == 0) {
                firmware.cellClass = "tableCell";
            } else {   
                firmware.cellClass = "tableCell2";
            }
			firmwares[i] = firmware;
		}				
		firmwares.firmwares = firmwares;			
		return firmwares;
	} else {		
		return null;
	} 	
}

function GetFirmwareDetails(firmwareId) {	
	var myQuery =	"	SELECT  FirmwareID, FileName, Version, Description, UploadDate, FirmwareType, UploadStatus " +
					"	FROM	Firmwares " +
					"	WHERE	FirmwareID = " + firmwareId ;
	var result;

    try  {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);     
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "GetFirmwareDetails statement failed to execute successfully");
        return null;
    }  
	
	if (result.length != 0) {
		var firmware = new Firmware();
		firmware.FirmwareID = result[0][0];
		firmware.FileName = result[0][1];
		firmware.Version = result[0][2];
		firmware.Description = result[0][3];
		firmware.UploadDate = result[0][4];
		firmware.FirmwareTypeID = result[0][5];
		firmware.UploadStatusID = result[0][6];
		firmware.FirmwareTypeName = GetFirmwareTypeName(result[0][5]);
		firmware.UploadStatusName = GetUploadStatusName(result[0][6]);
		return firmware;
	} else {		
		return null;
	}
}	


function AddFirmware(firmware) {
	var myQuery =	" INSERT INTO Firmwares (FileName, Version, Description, UploadDate, FirmwareType) " +
					" VALUES ('" + firmware.FileName + "', '" + firmware.Version + "', '" + firmware.Description + "', '" + ConvertFromJSDateToSQLiteDate(firmware.UploadDate) + "', " + firmware.FirmwareType + ") ";	
    try  {
        var service = new jsonrpc.ServiceProxy(serviceURL, methods);     
        var result = service.sqldal.execute({mode: "write", query: myQuery});
    } catch(e) {
        HandleException(e, "AddFirmware statement failed to execute successfully");
        return null;
    } 
	
	var lastFirmareIDQuery = " SELECT last_insert_rowid() ";
	var resultID;

    try  {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);     
       resultID = service.sqldal.execute({mode: "read", query: lastFirmareIDQuery});
    } catch(e) {
        HandleException(e, "AddFirmware statement failed to obtain LastInserted FirmwareID");
        return null;
    }  			
	
	if (resultID.length != 0) {
		return resultID[0][0];
	} else {
		return	null;
	}	
}


function UpdateFirmware(firmwareId, description) {
	var myQuery = "	UPDATE Firmwares SET Description = '" + description + "' WHERE FirmwareID = " + firmwareId;

    try {
        var service = new jsonrpc.ServiceProxy(serviceURL, methods);
        var result = service.sqldal.execute({mode : "write", query : myQuery});
    } catch(e) {
        HandleException(e, "UpdateFirmware statement failed to execute successfully");
        return null;
    }        
}


function DeleteFirmware(firmwareId) {
	var myQuery = " DELETE FROM Firmwares WHERE FirmwareID = " + firmwareId;

    try {
        var service = new jsonrpc.ServiceProxy(serviceURL, methods); 
        var result = service.sqldal.execute({mode: "write", query: myQuery});
    } catch(e) {
        HandleException(e, "DeleteFirmware statement failed to execute successfully");
        return null;
    }  
}


function GetFirmwareFiles(firmwareTypes, uploadStatuses) {	
	var myQuery; 
	var whereClause = "";
	
    if (firmwareTypes) {
        whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " FirmwareType IN " + ConvertArrayToSqlliteParam(firmwareTypes) + " "
    }
    if (uploadStatuses) {
        whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " UploadStatus IN " + ConvertArrayToSqlliteParam(uploadStatuses) + " "
    }
	if (whereClause != ""){
		whereClause = " WHERE " + whereClause;
	}		
	
	myQuery = "	SELECT	FileName, Version, FirmwareID FROM Firmwares " + whereClause + " ORDER BY FileName";
	
	var result; 
    try {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);     
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "GetFirmwaresFiles statement failed to execute successfully");
        return null;
    }  

	if (result.length != 0)	{
		var firmwares = Array();	
		for (i=0;i<result.length;i++) {
			var firmware = new Object();
			firmware.FileName = result[i][0];
			firmware.Version = result[i][1];
			firmware.FirmwareID = result[i][2];
			firmwares[i] = firmware;
		}				
		firmwares.firmwares = firmwares;			
		return firmwares;
	} else {		
		return null;
	} 	
}

function GetFwDownloadCount(dvAddress64, fwType, fwDownloadStatus){
	var whereClause = "";

	if (dvAddress64) {
       whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " D.Address64 LIKE '%" + dvAddress64 + "%' ";
    }
    if (fwType) {
       whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " FD.FwType = " + fwType + " ";
    }
    if (fwDownloadStatus) {
        whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " FD.FwStatus = " + fwDownloadStatus + " ";
    }
    if (whereClause != "") {    
    	whereClause = " WHERE " + whereClause;
    }
	
	var myQuery = "SELECT COUNT(*) FROM FirmwareDownloads FD INNER JOIN Devices D ON FD.DeviceID = D.DeviceID " + whereClause;
    var result;
 
    try  {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);       
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "GetFwDownloadCount statement failed to execute successfully");
        return null;
    }
		
	if (result.length != 0) {
		return result[0][0];
	} else {
		return 0;
	}	
}

function GetFwDownloadPage(dvAddress64, fwType, fwDownloadStatus, pageNo, pageSize, rowCount, forExport, orderBy, orderDirection){
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
   	
	var whereClause = "";

	if (dvAddress64) {
       whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " D.Address64 LIKE '%" + dvAddress64 + "%' ";
    }
    if (fwType) {
       whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " FD.FwType = " + fwType + " ";
    }
    if (fwDownloadStatus) {
        whereClause = whereClause + (whereClause == "" ? " " : " AND ") + " FD.FwStatus = " + fwDownloadStatus + " ";
    }
    if (whereClause != "") {    
    	whereClause = " WHERE " + whereClause;
    }
	var orderbyClause = " ORDER BY ";
	if (orderBy == 1) {
		orderbyClause = orderbyClause + " D.Address64 ";	
	} else {
		if (orderBy == 2) {
			orderbyClause = orderbyClause + " FD.StartedOn ";
		} else {
			if (orderBy == 3) {
				orderbyClause = orderbyClause + " FD.CompletedOn ";
			} else {
				orderbyClause = orderbyClause + " D.Address64 ";
			}			
		}					
	}
	orderbyClause = orderbyClause + orderDirection;	
	if (forExport) {
	        myQuery = 	" SELECT 'EUI-64 Address64', 'Type', 'Status', 'Completed(%)', 'Avg Speed (hh:mm:ss)', 'Remaining (hh:mm:ss)', 'Duration (hh:mm:ss)', 'Started On'	" +
	        			" UNION ALL " +
				        " SELECT D.Address64," +
				         " CASE FD.FwType " +
				         	" WHEN " + FT_AquisitionBoard + " THEN '" + GetFirmwareTypeName(FT_AquisitionBoard) + "' " +
				         	" WHEN " + FT_BackboneRouter + " THEN '" + GetFirmwareTypeName(FT_BackboneRouter) + "' " +
				         	" WHEN " + FT_Device + " THEN '" + GetFirmwareTypeName(FT_Device) + "' " +
				         " END AS FwType, " +	
				         " CASE FD.FwStatus " +
				         	" WHEN " + FDS_InProgress + " THEN '" + GetFirmwareDownloadStatus(FDS_InProgress) + "' " +
				         	" WHEN " + FDS_Canceling + " THEN '" + GetFirmwareDownloadStatus(FDS_Canceling) + "' " +
				         	" WHEN " + FDS_Completed + " THEN '" + GetFirmwareDownloadStatus(FDS_Completed) + "' " +
				         	" WHEN " + FDS_Failed + " THEN '" + GetFirmwareDownloadStatus(FDS_Failed) + "' " +
				         	" WHEN " + FDS_Cancelled + " THEN '" + GetFirmwareDownloadStatus(FDS_Cancelled) + "' " +
				         " END AS FwStatus, "+	
				         " ifnull(FD.CompletedPercent,0), " +
				         " ifnull(FD.AvgSpeed,'" + NAString +"'), "+
				         /*" ifnull(FD.Speed,'" + NAString +"'), "+*/
				         " CASE WHEN cast((FD.Downloadsize-FD.Downloadsize*FD.CompletedPercent/100)/FD.Speed*60/3600 as int) || ':' || cast((FD.Downloadsize-FD.Downloadsize*FD.CompletedPercent/100)/FD.Speed % 60 as int) || ':' || cast(((FD.Downloadsize-FD.Downloadsize*FD.CompletedPercent/100.0)/FD.Speed*60) % 60 as int) is null " +
				         	" THEN '" + NAString + "'" +
				         	" ELSE cast((FD.Downloadsize-FD.Downloadsize*FD.CompletedPercent/100)/FD.Speed*60/3600 as int) || ':' || cast((FD.Downloadsize-FD.Downloadsize*FD.CompletedPercent/100)/FD.Speed % 60 as int) || ':' || cast(((FD.Downloadsize-FD.Downloadsize*FD.CompletedPercent/100.0)/FD.Speed*60) % 60 as int) " +
				         " END AS RemainingTime, " +	
				         " CASE FD.FwStatus " +
				         	" WHEN " + FDS_InProgress + " THEN " +
				         			" (select cast((strftime('%s','now') - strftime('%s',FD.StartedOn))/3600 as varchar) || ':' ||cast(((strftime('%s','now') - strftime('%s',FD.StartedOn)) % 3600)/60 as varchar) || ':' || cast((strftime('%s','now') - strftime('%s',FD.StartedOn)) % 60 as varchar)) " +
				         	" ELSE CASE WHEN FD.CompletedOn is null " +
				         			   " THEN '" + NAString + "' " + 
				         			   " ELSE " + " (select cast((strftime('%s',FD.CompletedOn) - strftime('%s',FD.StartedOn))/3600 as varchar) || ':' ||cast(((strftime('%s',FD.CompletedOn) - strftime('%s',FD.StartedOn)) % 3600)/60 as varchar) || ':' || cast((strftime('%s',FD.CompletedOn) - strftime('%s',FD.StartedOn)) % 60 as varchar)) " +				         			   
				         		  " END " +
				         " END AS Duration, " +
				         " FD.StartedOn " +
					  " FROM FirmwareDownloads FD " +
					  " INNER JOIN Devices D ON FD.DeviceID = D.DeviceID " + 
					  whereClause;
	        return myQuery;
	} else {
		myQuery = " SELECT D.Address64," +
				         " CASE FD.FwType " +
				         	" WHEN " + FT_AquisitionBoard + " THEN '" + GetFirmwareTypeName(FT_AquisitionBoard) + "' " +
				         	" WHEN " + FT_BackboneRouter + " THEN '" + GetFirmwareTypeName(FT_BackboneRouter) + "' " +
				         	" WHEN " + FT_Device + " THEN '" + GetFirmwareTypeName(FT_Device) + "' " +
				         " END AS FwType, " +	
				         " CASE FD.FwStatus " +
				         	" WHEN " + FDS_InProgress + " THEN '" + GetFirmwareDownloadStatus(FDS_InProgress) + "' " +
				         	" WHEN " + FDS_Canceling + " THEN '" + GetFirmwareDownloadStatus(FDS_Canceling) + "' " +
				         	" WHEN " + FDS_Completed + " THEN '" + GetFirmwareDownloadStatus(FDS_Completed) + "' " +
				         	" WHEN " + FDS_Failed + " THEN '" + GetFirmwareDownloadStatus(FDS_Failed) + "' " +
				         	" WHEN " + FDS_Cancelled + " THEN '" + GetFirmwareDownloadStatus(FDS_Cancelled) + "' " +
				         " END AS FwStatus, " +	
				         " ifnull(FD.CompletedPercent,0), " +
				         " ifnull(FD.AvgSpeed,'" + NAString +"'), " +
				         /*" ifnull(FD.Speed,'" + NAString +"'), " +*/
				         " CASE WHEN cast((FD.Downloadsize-FD.Downloadsize*FD.CompletedPercent/100)/FD.Speed*60/3600 as int) || ':' || cast((FD.Downloadsize-FD.Downloadsize*FD.CompletedPercent/100)/FD.Speed%60 as int) || ':' || cast(((FD.Downloadsize-FD.Downloadsize*FD.CompletedPercent/100.0)/FD.Speed*60)%60 as int) is null " +
				         	" THEN '" + NAString + "'" +
				         	" ELSE cast((FD.Downloadsize-FD.Downloadsize*FD.CompletedPercent/100)/FD.Speed*60/3600 as int) || ':' || cast((FD.Downloadsize-FD.Downloadsize*FD.CompletedPercent/100)/FD.Speed%60 as int) || ':' || cast(((FD.Downloadsize-FD.Downloadsize*FD.CompletedPercent/100.0)/FD.Speed*60)%60 as int) " +
				         " END AS RemainingTime, " +	
				         " CASE FD.FwStatus " +
				         	" WHEN " + FDS_InProgress + " THEN " +
				         			" (select cast((strftime('%s','now') - strftime('%s',FD.StartedOn))/3600 as varchar) || ':' ||cast(((strftime('%s','now') - strftime('%s',FD.StartedOn))%3600)/60 as varchar) || ':' || cast((strftime('%s','now') - strftime('%s',FD.StartedOn))%60 as varchar)) " +
				         	" ELSE CASE WHEN FD.CompletedOn is null " +
				         			   " THEN '" + NAString + "' " + 
				         			   " ELSE " + " (select cast((strftime('%s',FD.CompletedOn) - strftime('%s',FD.StartedOn))/3600 as varchar) || ':' ||cast(((strftime('%s',FD.CompletedOn) - strftime('%s',FD.StartedOn))%3600)/60 as varchar) || ':' || cast((strftime('%s',FD.CompletedOn) - strftime('%s',FD.StartedOn))%60 as varchar)) " +				         			   
				         		  " END " +	   
				         " END AS Duration, "+	
				         " FD.StartedOn, " +				         				         				        
				         " FD.DeviceID, " +
				         " FD.FwType, " +
				         " FD.FwStatus " +
				  " FROM FirmwareDownloads FD " +
				  " INNER JOIN Devices D ON FD.DeviceID = D.DeviceID " + 
				  whereClause +  
				  orderbyClause +  
	              " LIMIT " + pageSize + " OFFSET " + rowOffset;
	};
	var result;  
    try  {
       var service = new jsonrpc.ServiceProxy(serviceURL, methods);       
       result = service.sqldal.execute({query: myQuery});
    } catch(e) {
        HandleException(e, "GetFwDownloadPage statement failed to execute successfully");
        return null;
    }

	if (result.length != 0) {
		var fwDownloads = Array();			
		for (i=0;i<result.length;i++) {
			var fwDownload = new FwDownload();
			fwDownload.Address64 = result[i][0];
			fwDownload.Type = result[i][1];
			fwDownload.Status = result[i][2];			
			fwDownload.DivName = "<div id=\"holder"+i + "\" style=\"border:solid 0px #000000\"/>"; 
			fwDownload.Completed = result[i][3];
			fwDownload.AvgSpeed = result[i][4];
			/*fwDownload.CrtSpeed = result[i][5];*/
			fwDownload.RemainingTime = result[i][5];
			fwDownload.Duration = result[i][6];
			fwDownload.StartedOn = result[i][7];			
			fwDownload.DeviceID = result[i][8];
			fwDownload.FwType = result[i][9];						
			fwDownload.FwStatus = result[i][10];			
						
			if (fwDownload.FwStatus == FDS_InProgress){
				fwDownload.CancelIcon = "<a href='javascript:CancelFWDownload("+fwDownload.DeviceID+",\""+fwDownload.Address64+"\","+fwDownload.FwType+",\""+fwDownload.StartedOn+"\","+fwDownload.FwStatus+")' title='Cancel firware download'>"
				fwDownload.CancelIcon = fwDownload.CancelIcon + "<img src='styles/images/cancel.png'/></a>" 
			} else {
				fwDownload.CancelIcon = "<a href='javascript:DeleteFwDownload("+fwDownload.DeviceID+","+fwDownload.FwType+",\""+fwDownload.StartedOn+"\")' title='Delete record'>"
				fwDownload.CancelIcon = fwDownload.CancelIcon + "<img src='styles/images/delete.gif'/></a>"
			}							
			//fwDownload.CompletedOn = result[i][12];
			if (i % 2 == 0) {
		 		fwDownload.cellClass = "tableCell";
            } else {   
            	fwDownload.cellClass = "tableCell2";
            }
			fwDownloads[i] = fwDownload;
		}				
		fwDownloads.fwDownloads = fwDownloads;			
		return fwDownloads;
	} else {		
		return null;
	}
}

function AddFwDownload(deviceId, fwType) {
	var myQuery =	" INSERT INTO FirmwareDownloads(DeviceID,FwType,FwStatus,CompletedPercent,DownloadSize,Speed,AvgSpeed,StartedOn,CompletedOn) " +
					" VALUES ("  + deviceId + ", " + fwType + ", " + FDS_InProgress + ", null, null, null, null, datetime('now'), null) ";	
    try  {
        var service = new jsonrpc.ServiceProxy(serviceURL, methods);     
        var result = service.sqldal.execute({mode: "write", query: myQuery});
    } catch(e) {
        HandleException(e, "AddFwDownload statement failed to execute successfully");
    } 	
}

function UpdateFwDownload(deviceId, fwType, startedOn){
	var myQuery =	" UPDATE FirmwareDownloads " +
					" SET FwStatus = " + FDS_Canceling + 
					" WHERE DeviceID = " + deviceId + " AND FwType = " + fwType + " AND StartedOn = '" + startedOn + "' ";					
	try  {
		var service = new jsonrpc.ServiceProxy(serviceURL, methods);     
		var result = service.sqldal.execute({mode: "write", query: myQuery});
	} catch(e) {
		HandleException(e, "UpdateFwDownload statement failed to execute successfully");
	} 		
}

function RemoveFwDownload(deviceId, fwType, startedOn){
	var myQuery =	" DELETE FROM FirmwareDownloads " +					
					" WHERE DeviceID = " + deviceId + " AND FwType = " + fwType + " AND StartedOn = '" + startedOn + "' ";					
	try  {
		var service = new jsonrpc.ServiceProxy(serviceURL, methods);     
		var result = service.sqldal.execute({mode: "write", query: myQuery});
	} catch(e) {
		HandleException(e, "RemoveFwDownload statement failed to execute successfully");
	} 		
}

function objDevice(){
	this.DeviceID = null;
	this.Address64 = null;
	this.HasFwInProgress = null;
}

function GetDevicesStatus(devices){
	var myQuery = " SELECT D.DeviceID, D.Address64, ifnull(F.FwInProgress,0) "+
			 	  " FROM Devices D " +
	      		  " LEFT OUTER JOIN (SELECT DeviceID, COUNT(*) AS FwInProgress FROM FirmwareDownloads WHERE FwStatus IN (" + FDS_InProgress + "," + FDS_Canceling + ") GROUP BY DeviceID ) F ON D.DeviceID = F.DeviceID "+
	      		  " WHERE D.DeviceID IN " + ConvertArrayToSqlliteParam(devices);
	//document.write(myQuery);return;
	try  {
		var service = new jsonrpc.ServiceProxy(serviceURL, methods);     
		var result = service.sqldal.execute({mode: "read", query: myQuery});
		var devices = [];
		for(var i=0; i<result.length; i++){			
			var dev = new objDevice();
			dev.DeviceID = result[i][0];
			dev.Address64 = result[i][1];
			dev.HasFwInProgress = result[i][2];
			devices[i] = dev;
		}				
		return devices; 
	} catch(e) {
		HandleException(e, "GetDevicesStatus statement failed to execute successfully");
	} 		
}