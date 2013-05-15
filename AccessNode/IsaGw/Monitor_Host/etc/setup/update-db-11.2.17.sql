--create new tables
DROP TABLE IF EXISTS ConfiguratedDevices;
DROP TABLE IF EXISTS Users;
DROP TABLE IF EXISTS Companies;
DROP TABLE IF EXISTS LocalControlLoops;
DROP TABLE IF EXISTS DeviceConnections;

CREATE TABLE Users
(
	UserID	 		INTEGER NOT NULL PRIMARY KEY,
	UserName 		VARCHAR(50) NOT NULL,
	FirstName 		VARCHAR(50) NOT NULL,
	LastName 		VARCHAR(50) NOT NULL,
	Email 			VARCHAR(50) NOT NULL,
	Password 		VARCHAR(50) NOT NULL,
	Role			INTEGER
	/*
		0 - admin
		1 - power user
		2 - regular user
		*/
);

CREATE TABLE Companies
(
  CompanyID INTEGER NOT NULL PRIMARY KEY,  
  CompanyName VARCHAR(250) NOT NULL,
  LogoFile VARCHAR(100)
);

CREATE TABLE LocalControlLoops
(
	PublishDeviceID INTEGER NOT NULL,  
  	PublishChannelNo INTEGER NOT NULL,
	PublishFrequency INTEGER NOT NULL,
	PublishOffset INTEGER NOT NULL,
	SubscriberDeviceID INTEGER NOT NULL,
	LowThreshold INTEGER NOT NULL,
	HighThreshold INTEGER NOT NULL,
	LastCommandID INTEGER NOT NULL
);

CREATE TABLE DeviceConnections
(
  DeviceID INTEGER NOT NULL,
  IP VARCHAR(50) NOT NULL,
  Port INTEGER NOT NULL    
);

CREATE TABLE ConfiguratedDevices
(
	ID INTEGER NOT NULL PRIMARY KEY,
	Address64 VARCHAR(50) NOT NULL,       
  	SensorTypeID INTEGER NOT NULL           
);

--alter existing tables

ALTER TABLE DeviceTypes RENAME TO DT_TMP;
DROP TABLE IF EXISTS DeviceTypes;
CREATE TABLE DeviceTypes
(
	DeviceTypeID 	INTEGER NOT NULL PRIMARY KEY,
	DeviceTypeName 	VARCHAR(50) NOT NULL,
	CompanyID INTEGER NOT NULL DEFAULT -1,
  	IconFile VARCHAR(100),
  	SensorType INTEGER NOT NULL DEFAULT 0
  	/* 
  		0 - Standard -> type of radio
  		1 - Extended -> type of sensor
  	*/
);
INSERT INTO DeviceTypes
(DeviceTypeID, DeviceTypeName, CompanyID, IconFile, SensorType)
 SELECT DeviceTypeID, DeviceTypeName, -1, 'N/A', 0 FROM DT_TMP;
DROP TABLE IF EXISTS DT_TMP; 
 
ALTER TABLE DeviceTypeChannels RENAME TO DTC_TMP;
DROP TABLE IF EXISTS DeviceTypeChannels;
CREATE TABLE DeviceTypeChannels
(
	DeviceTypeID	INTEGER NOT NULL,
	ChannelNo		INTEGER NOT NULL, 
	ChannelName 	VARCHAR(100) NOT NULL,
	MinValue 		INTEGER NOT NULL, 
	MaxValue 		INTEGER NOT NULL, 
	MinRawValue 	INTEGER NOT NULL,
	MaxRawValue 	INTEGER NOT NULL,	
	ChannelData		INTEGER NOT NULL DEFAULT 2/* 0-uint8; 1-uint16; 2-uint32, 3-int8; 4-int16; 5-int32; 6-float*/, 
	MappedTSAPID 	INTEGER NOT NULL /*application identifier on ISA100 device, range [0..256]*/,
	MappedObjectID 	INTEGER NOT NULL /*object id from an application, range [0..65536]*/,
	MappedAttributeID 	INTEGER NOT NULL /*atrtribute id from an object, range [0..4096]*/,
	ChannelType 	INTEGER NOT NULL,
	IsPredefined 	INTEGER NOT NULL,
	Enabled 		INTEGER NOT NULL DEFAULT 1,
	UnitOfMeasurement VARCHAR(50) NOT NULL DEFAULT 'N/A'
);
INSERT INTO DeviceTypeChannels
(DeviceTypeID, ChannelNo, ChannelName, MinValue, MaxValue, MinRawValue,	MaxRawValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, ChannelType, IsPredefined, Enabled, UnitOfMeasurement)
 SELECT DeviceTypeID, ChannelNo, ChannelName, MinValue, MaxValue, MinRawValue,	MaxRawValue, 2, -1, -1, -1, ChannelType, IsPredefined, 1, 'N/A' 
 FROM DTC_TMP;
DROP TABLE IF EXISTS DTC_TMP;

ALTER TABLE DeviceChannels RENAME TO DC_TMP;
DROP TABLE IF EXISTS DeviceChannels;
CREATE TABLE DeviceChannels
(
	DeviceID INTEGER NOT NULL,
	ChannelNo INTEGER NOT NULL,
	ChannelName VARCHAR(100) NOT NULL,
	MinValue INTEGER NOT NULL,
	MaxValue INTEGER NOT NULL,
	MinRawValue INTEGER NOT NULL,
	MaxRawValue INTEGER NOT NULL,
	ChannelData		INTEGER NOT NULL DEFAULT 2/* 0-uint8; 1-uint16; 2-uint32, 3-int8; 4-int16; 5-int32; 6-float*/, 
	MappedTSAPID 	INTEGER NOT NULL /*application identifier on ISA100 device, range [0..256]*/,
	MappedObjectID 	INTEGER NOT NULL /*object id from an application, range [0..65536]*/,
	MappedAttributeID 	INTEGER NOT NULL /*atrtribute id from an object, range [0..4096]*/,	
	ChannelType 	INTEGER NOT NULL,
	IsPredefined INTEGER NOT NULL,
	Enabled INTEGER NOT NULL DEFAULT 1,
	UnitOfMeasurement VARCHAR(50) NOT NULL DEFAULT 'N/A',
	LastPSCommandID INTEGER /* Last command id for a (cancel) publish/subscribe command issued on this channel */
);
INSERT INTO DeviceChannels
(DeviceID, ChannelNo, ChannelName, MinValue, MaxValue, MinRawValue,	MaxRawValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, ChannelType, IsPredefined, Enabled, UnitOfMeasurement, LastPSCommandID)
 SELECT DeviceID, ChannelNo, ChannelName, MinValue, MaxValue, MinRawValue,	MaxRawValue, 1, -1, -1, -1, ChannelType, IsPredefined, 1, 'N/A' , -1
 FROM DC_TMP;
DROP TABLE IF EXISTS DC_TMP;

ALTER TABLE Devices RENAME TO D_TMP;
DROP TABLE IF EXISTS Devices;
CREATE TABLE Devices
(
	DeviceID INTEGER NOT NULL PRIMARY KEY,
	DeviceTypeID INTEGER NOT NULL,
	SensorTypeID INTEGER NOT NULL DEFAULT -1,
	DeviceImplementationType INTEGER NOT NULL DEFAULT 0,
	Address128 VARCHAR(50), /* The device IPv6 address*/
	Address64 VARCHAR(50) NOT NULL, /*The device MAC address*/
	DeviceStatus INTEGER NOT NULL, /* same with DeviceHistory.DeviceStatus */
	LastRead DATETIME,
	DeviceLevel INTEGER NOT NULL
	/*
	Devices are hirarchy organized:
		- the SM is on level = 0
		- GW, BBR, Monitoring App is on level 1
		- other devices are on level 2, 3 ...
	*/,
	LastDeviceUpdated DATETIME /* holds the timestamp when aproperty has been updated.*/,
	RejoinCount INTEGER 
);
INSERT INTO Devices
(DeviceID, DeviceTypeID, SensorTypeID, DeviceImplementationType, Address128, Address64,	DeviceStatus, LastRead, DeviceLevel, LastDeviceUpdated, RejoinCount)
 SELECT DeviceID, DeviceTypeID, -1, 0, Address128, Address64,	DeviceStatus, LastRead, DeviceLevel, LastDeviceUpdated, RejoinCount
 FROM D_TMP;
DROP TABLE IF EXISTS D_TMP;

ALTER TABLE DeviceBatteryStatistics RENAME TO DBS_TMP;
DROP TABLE IF EXISTS DeviceBatteryStatistics;
CREATE TABLE DeviceBatteryStatistics
(
	DeviceID INTEGER NOT NULL,
	ReadingTime DATETIME NOT NULL,
	CollectedPeriodInterval INTEGER NOT NULL,
	SuccessfulTransmissionsCount INTEGER NOT NULL,
	AbortedTransmissionsReceptions INTEGER NOT NULL,
	FailedTransmissions INTEGER NOT NULL,
	AverageRSSI INTEGER NOT NULL,
	DiscardedDLLMsgCount INTEGER NOT NULL,
	BatteryLevel INTEGER NOT NULL
		/*
		[0..100] - how much energy is available in battery
		101 - is in charging mode
		*/,
	RemainingBatteryHours INTEGER NOT NULL
		/*
		how much time the battery energy is available
		*/			
);
INSERT INTO DeviceBatteryStatistics
(DeviceID, ReadingTime, CollectedPeriodInterval, SuccessfulTransmissionsCount, AbortedTransmissionsReceptions, FailedTransmissions,	AverageRSSI, DiscardedDLLMsgCount, BatteryLevel, RemainingBatteryHours)
 SELECT DeviceID, ReadingTime, -1, -1, -1, -1, -1, -1, BatteryLevel, RemainingBatteryHours
 FROM DBS_TMP;
DROP TABLE IF EXISTS DBS_TMP;

ALTER TABLE Firmwares RENAME TO FW_TMP;
DROP TABLE IF EXISTS Firmwares;
CREATE TABLE Firmwares
(
   FirmwareID INTEGER NOT NULL PRIMARY KEY,
   FileName VARCHAR(250) NOT NULL,
   Version VARCHAR(11) NOT NULL,
   Description VARCHAR(1000),
   UploadDate DATETIME,
   FirmwareType INTEGER NOT NULL,   
   UploadStatus INTEGER NOT NULL DEFAULT(0)
   /*
   	0 = New
   	1 = SuccessfullyUploaded
   	2 = Uploading (Transfering)
   	3 = WaitRetring
   	4 = Failed
   */,   
   UploadRetryCount INTEGER NOT NULL DEFAULT(0),
   LastFailedUploadTime DATETIME
);
INSERT INTO Firmwares
(FirmwareID, FileName, Version, Description, UploadDate, FirmwareType, UploadStatus, UploadRetryCount, LastFailedUploadTime)
 SELECT FirmwareID, FileName, Version, Description, UploadDate, FirmwareType, 0, 0, '2008-01-01 12:00:00'
 FROM FW_TMP;
DROP TABLE IF EXISTS FW_TMP;

--create new indexes
CREATE UNIQUE INDEX IX_DeviceConnections
ON DeviceConnections(DeviceID);

--alter existing indexes
DROP INDEX IF EXISTS IX_CommandParameters;

CREATE INDEX IX_CommandParameters
ON CommandParameters( CommandID, ParameterCode);

--update DeviceTypes
UPDATE DeviceTypes SET DeviceTypeName='Field Router' WHERE DeviceTypeID=10;

--insert user
INSERT INTO Users(UserID, UserName, FirstName, LastName, Email, Password, Role) VALUES (1, "admin", "admin", "admin", "a@a.com", "5b1ea208c3d08373c1e554f12331020125ae52b3d01977461eb734ca80d71819", 0);
--insert hart type
INSERT INTO DeviceTypes(DeviceTypeID, DeviceTypeName) VALUES(1000, 'HART Adapter Device');

--insert hart variables
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType) 
  VALUES(1000, 1, 'Primary Variable', 0, 0, 0, 0, 6, 2, 4, 1, 1, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType) 
  VALUES(1000, 2, 'Secondary Variable', 0, 0, 0, 0, 6, 2, 4, 1, 1, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType) 
  VALUES(1000, 3, 'Tertiary Variable', 0, 0, 0, 0, 6, 2, 4, 1, 1, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType) 
  VALUES(1000, 4, 'Quaternary Variable', 0, 0, 0, 0, 6, 2, 4, 1, 1, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType) 
  VALUES(1000, 5, 'Expanded Device Type', 0, 0, 0, 0, 2, 2, 4, 1, 1, 0);
INSERT INTO DeviceTypeChannels(DeviceTypeID, ChannelNo, ChannelName, MinRawValue, MaxRawValue, MinValue, MaxValue, ChannelData, MappedTSAPID, MappedObjectID, MappedAttributeID, IsPredefined, ChannelType) 
  VALUES(1000, 6, 'Device ID', 0, 0, 0, 0, 2, 2, 4, 1, 1, 0);

--update existing channels
UPDATE DeviceTypeChannels SET ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=1
 WHERE ChannelNo=1 AND (DeviceTypeID=10 OR DeviceTypeID=11);
UPDATE DeviceTypeChannels SET ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=2
 WHERE ChannelNo=2 AND (DeviceTypeID=10 OR DeviceTypeID=11);
UPDATE DeviceTypeChannels SET ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=3
 WHERE ChannelNo=3 AND (DeviceTypeID=10 OR DeviceTypeID=11);   
UPDATE DeviceTypeChannels SET ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=4
 WHERE ChannelNo=4 AND (DeviceTypeID=10 OR DeviceTypeID=11);
UPDATE DeviceTypeChannels SET ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=5
 WHERE ChannelNo=5 AND (DeviceTypeID=10 OR DeviceTypeID=11);
UPDATE DeviceTypeChannels SET ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=6
 WHERE ChannelNo=6 AND (DeviceTypeID=10 OR DeviceTypeID=11);
UPDATE DeviceTypeChannels SET ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=7
 WHERE ChannelNo=7 AND (DeviceTypeID=10 OR DeviceTypeID=11);
UPDATE DeviceTypeChannels SET IsPredefined=1, ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=8
 WHERE ChannelNo=8 AND (DeviceTypeID=10 OR DeviceTypeID=11);
UPDATE DeviceTypeChannels SET IsPredefined=1, ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=9
 WHERE ChannelNo=9 AND (DeviceTypeID=10 OR DeviceTypeID=11);   
   
--update existing device channels  (ERROR: existing hart devices will be altered)  
UPDATE DeviceChannels SET ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=1
 WHERE ChannelNo=1;
UPDATE DeviceChannels SET ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=2
 WHERE ChannelNo=2;
UPDATE DeviceChannels SET ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=3
 WHERE ChannelNo=3;   
UPDATE DeviceChannels SET ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=4
 WHERE ChannelNo=4;
UPDATE DeviceChannels SET ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=5
 WHERE ChannelNo=5;
UPDATE DeviceChannels SET ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=6
 WHERE ChannelNo=6;
UPDATE DeviceChannels SET ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=7
 WHERE ChannelNo=7;
UPDATE DeviceChannels SET IsPredefined=1, ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=8
 WHERE ChannelNo=8;
UPDATE DeviceChannels SET IsPredefined=1, ChannelData=1, MappedTSAPID=2, MappedObjectID=4, MappedAttributeID=9
 WHERE ChannelNo=9;    
 
--update version in Properties table
DELETE FROM Properties WHERE Key='Version'; 
REPLACE INTO Properties(Key, Value) VALUES('SchemaVersion', '11.2.17');
REPLACE INTO Properties(Key, Value) VALUES('DataVersion', '11.3.18');