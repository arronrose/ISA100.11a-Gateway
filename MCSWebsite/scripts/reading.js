function Reading()
{
    this.DeviceID = null;
    this.ReadingTime = null;
    this.ChannelNumber = null;
    this.Value = null;
    this.ReadingType = null;
    this.Address128 = null;
    this.Address64 = null;
    this.ChannelName = null;
    this.UnitOfMeasurement = null;    
}


//Reading Types
var RT_OnDemand = 0;
var RT_PublishSubscribe = 1;

function GetReadingTypeName(readingType)
{
	switch (readingType)
	{
		case RT_OnDemand:
			return "On Demand";
		case RT_PublishSubscribe:
			return "Publish/Subscribe";
		default:
			return NAString;
	}
}
