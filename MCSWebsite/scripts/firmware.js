function Firmware()
{
	this.FirmwareID = null;
	this.FileName = null;
	this.Version = null;
	this.Description = null;
	this.UploadDate = null;
	this.FirmwareTypeID = null;
	this.FirmwareTypeName = null;
	this.UploadStatusID = null;
	this.UploadStatusName = null;
	this.DeleteAction = null;
	this.PopupLink = null;
}

function FwDownload(){
	this.Address64 = null;
	this.Type = null;
	this.Status = null;			
	this.Completed = null;
	this.AvgCrtSpeed = null;
	this.RemainingTime = null;
	this.Duration = null;
	this.StartedOn = null;
	this.CompletedOn = null;
	this.FwCommandID = null;
	this.CancelIcon = null;
	this.cellClass = null;
} 

/* Firmware Download Status*/
var FDS_InProgress = 1;
var FDS_Canceling = 2;
var FDS_Cancelled = 3;
var FDS_Completed = 4;
var FDS_Failed = 5;

function GetFirmwareDownloadStatus(fwDownloadStatus) 
{
	switch (fwDownloadStatus)
	{
		case  FDS_InProgress:
			return "In Progress";
		case  FDS_Canceling:
			return "Canceling";
		case  FDS_Cancelled:
			return "Canceled";
		case  FDS_Completed:
			return "Completed";
		case  FDS_Failed:
			return "Failed";
		default:			
			return "Unkonwn - " + fwDownloadStatus;
	}
}


/* Firmware Types*/
var FT_AquisitionBoard = 0;
var FT_BackboneRouter = 3;
var FT_Device = 10;
	
function GetFirmwareTypeName(firmwareType) 
{
	var firmwareTypeName = null;	
	switch (firmwareType)
	{
		case  FT_BackboneRouter:
			firmwareTypeName = "Backbone Router";
			break;
		case  FT_Device:
			firmwareTypeName = "Device";
			break;
		case  FT_AquisitionBoard:
			firmwareTypeName = "Acquisition Board";
			break;
		default:
			firmwareTypeName = "Unkonwn - " + firmwareType;
			break;
	}
	return firmwareTypeName;
}

/* Upload Status */

var US_New = 0;
var US_SuccessfullyUploaded = 1;
var US_Uploading = 2;
var US_WaitRetrying = 3;
var US_Failed = 4;

	
function GetUploadStatusName(uploadStatus)
{
	var uploadStatusName = null;	
	switch (uploadStatus)
	{
		case US_New:
			uploadStatusName = "New";
			break;
		case US_Uploading:
			uploadStatusName = "Uploading";
			break;
		case US_SuccessfullyUploaded:
			uploadStatusName = "Successfully Uploaded";
			break;
		case US_WaitRetrying:
			uploadStatusName = "Wait-Retrying";
			break;
		case US_Failed:
			uploadStatusName = "Failed";
			break;			
		default:
			uploadStatusName = "Unkonwn - " + uploadStatus;
			break;
	}
	return uploadStatusName;
}