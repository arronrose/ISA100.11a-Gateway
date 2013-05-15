USE Monitor_Host;

#HACK this file is to update correctly special chars like: celsius degree, ...

UPDATE DeviceTypeChannels SET ChannelName='Temperature (°C)' 
	WHERE ChannelNo=1 AND (DeviceTypeID=10 OR DeviceTypeID=11);
	
UPDATE DeviceTypeChannels SET ChannelName='Dew Point (°C)' 
	WHERE ChannelNo=3 AND (DeviceTypeID=10 OR DeviceTypeID=11);
 