USE Monitor_Host;


DELETE FROM Users;
INSERT INTO Users(UserID, UserName, FirstName, LastName, Email, Password, Role) VALUES (1, "admin", "admin", "admin", "a@a.com", "5b1ea208c3d08373c1e554f12331020125ae52b3d01977461eb734ca80d71819", 0);

DELETE FROM DeviceTypes;

INSERT INTO DeviceTypes(DeviceTypeID, DeviceTypeName) VALUES(1, 'System Manager');
INSERT INTO DeviceTypes(DeviceTypeID, DeviceTypeName) VALUES(2, 'Gateway');
INSERT INTO DeviceTypes(DeviceTypeID, DeviceTypeName) VALUES(3, 'Backbone Router');
INSERT INTO DeviceTypes(DeviceTypeID, DeviceTypeName) VALUES(10, 'Field Router');
INSERT INTO DeviceTypes(DeviceTypeID, DeviceTypeName) VALUES(11, 'Non-Routing Device');
INSERT INTO DeviceTypes(DeviceTypeID, DeviceTypeName) VALUES(1000, 'HART Adapter Device');

DELETE FROM DeviceChannels;

DELETE FROM Devices;

INSERT INTO Devices(DeviceTypeID, Address128, Address64, DeviceStatus, DeviceLevel)
  VALUES(2, '::', '600D:BEEF:600D:BEEF', 0, -1);
  
DELETE FROM DeviceTypeChannels;
/*
for echech device type, currently we have 10=routing, 11=non routing
*/
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType) 
  VALUES(10, 1, 'Temperature', 0, 65535, -40, 123.8, 1, 1, 3, 1, 0, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType)  
  VALUES(10, 2, 'Humidity (%)', 0, 65535, 0, 100, 1, 1, 3, 2, 0, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType)  
  VALUES(10, 3, 'Dew Point', 0, 65535, -40, 123.8, 1, 1, 3, 3, 0, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType)  
  VALUES(10, 4, '4-20 mA', 0, 65535, 4, 20, 1, 1, 3, 4, 0, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType)  
  VALUES(10, 5, '0-5 V', 0, 65535, 0, 5, 1, 2, 4, 5, 0, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType)  
  VALUES(10, 6, 'Analog Input RTD (Celsius)/Pressure (PSI)', 0, 65535, -100, 300, 1, 2, 4, 6, 0, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType)  
  VALUES(10, 7, 'Battery Voltage (V)', 0, 65535, 0, 7.5, 1, 2, 4, 7, 0, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType)  
  VALUES(10, 8, 'Digital Input', 0, 1, 0, 1, 1, 2, 4, 8, 0, 1);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType)  
  VALUES(10, 9, 'Digital output', 0, 1, 0, 1, 1, 2, 4, 9, 0, 1);


INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType) 
  VALUES(11, 1, 'Temperature', 0, 65535, -40, 123.8, 1, 1, 3, 1, 0, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType)  
  VALUES(11, 2, 'Humidity (%)', 0, 65535, 0, 100, 1, 1, 3, 2, 0, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType)  
  VALUES(11, 3, 'Dew Point', 0, 65535, -40, 123.8, 1, 1, 3, 3, 0, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType)  
  VALUES(11, 4, '4-20 mA', 0, 65535, 4, 20, 1, 1, 3, 4, 0, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType)  
  VALUES(11, 5, '0-5 V', 0, 65535, 0, 5, 1, 2, 4, 5, 0, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType)  
  VALUES(11, 6, 'Analog Input RTD (Celsius)/Pressure (PSI)', 0, 65535, -100, 300, 1, 2, 4, 6, 0, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType)  
  VALUES(11, 7, 'Battery Voltage (V)', 0, 65535, 0, 7.5, 1, 2, 4, 7, 0, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType)  
  VALUES(11, 8, 'Digital Input', 0, 1, 0, 1, 1, 2, 4, 8, 0, 1);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType)  
  VALUES(11, 9, 'Digital output', 0, 1, 0, 1, 1, 2, 4, 9, 0, 1);


INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType) 
  VALUES(1000, 1, 'Primary Variable', 0, 0, 0, 0, 6, 1, 3, 1, 1, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType) 
  VALUES(1000, 2, 'Secondary Variable', 0, 0, 0, 0, 6, 1, 3, 2, 1, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType) 
  VALUES(1000, 3, 'Tertiary Variable', 0, 0, 0, 0, 6, 1, 3, 3, 1, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType) 
  VALUES(1000, 4, 'Quaternary Variable', 0, 0, 0, 0, 6, 1, 3, 4, 1, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType) 
  VALUES(1000, 5, 'Expanded Device Type', 0, 0, 0, 0, 2, 2, 4, 1, 1, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType) 
  VALUES(1000, 6, 'Device ID', 0, 0, 0, 0, 2, 2, 4, 1, 1, 0);

DELETE FROM Properties WHERE `Key`='Version';
REPLACE INTO Properties(`Key`, Value) VALUES('SchemaVersion', '14.0.0');
REPLACE INTO Properties(`Key`, Value) VALUES('DataVersion', '14.0.0');