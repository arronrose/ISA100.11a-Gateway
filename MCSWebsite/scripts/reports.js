
/* Link Status for NeighborsHealthReport*/
var LS_Unavailable = 0;
var LS_Available = 1;

function GetLinkStatusName(linkStatus)
{
	switch (linkStatus)
	{
		case LS_Unavailable:
		  return "Unavailable";
		case LS_Available:
		  return "Available";
		default:
		  return linkStatus;
	}
}

/* Link Direction for ScheduleLinkReport*/
var SLD_Reception = 0;
var SLD_Transmission = 1;

function GetLinkDirectionName(linkDirection)
{
	switch (linkDirection)
	{
		case SLD_Reception:
		  return "Reception";
		case SLD_Transmission:
		  return "Transmission";
		default:
		  return linkDirection;
	}
}

/* Lynk Type for ScheduleLinkReport*/

var SLT_AperiodicDataCommunication = 0;
var SLT_AperiodicManagementCommunication = 1;
var SLT_PeriodicDataCommunication = 2;
var SLT_PeriodicManagementCommunication = 3;

function GetLinkTypeName(linkType)
{
	switch (linkType)
	{
		case SLT_AperiodicDataCommunication:
		  return "Aperiodic Data Communication";
		case SLT_AperiodicManagementCommunication:
		  return "Aperiodic Management Communication";
		case SLT_PeriodicDataCommunication:
		  return "Periodic Data Communication";
		case SLT_PeriodicManagementCommunication:
		  return "Periodic Management Communication";
		default:
		  return linkType;
	}
}


function GetSignalQuality(val){

    if ( val == 0 ) 
        {return NAString};
    if ( 1 <= val && val <= 63 )
        {return 'Poor'};
    if ( 63 < val && val <= 127)
        {return 'Fair'};
    if (127 < val && val <= 191)
        {return 'Good'};
    if (191 < val && val <= 255)
        {return 'Excellent'}
    else
        {return 'Out of range'};
}

function GetSignalStrength(val){ 

    if (-192 <= val && val <= 63 ){
        return val;
    }
    else{
        return 'Out of range';
    };
}


function GetSignalQualityColor(val){

    if ( val == 0 ) 
        {return '#000000'};
    if ( 1 <= val && val <= 63 )
        {return '#FF7800'};
    if ( 63 < val && val <= 127)
        {return '#00B6FF'};
    if (127 < val && val <= 191)
        {return '#0000FF'};
    if (191 < val && val <= 255)
        {return '#007700'}
    else
        {return '#FF0000'};
}