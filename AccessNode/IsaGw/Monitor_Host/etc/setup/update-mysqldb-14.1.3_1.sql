DROP DATABASE IF EXISTS Monitor_Host;
CREATE DATABASE Monitor_Host;
USE Monitor_Host;

DROP TABLE IF EXISTS DeviceTypes;
DROP TABLE IF EXISTS DeviceTypeChannels;
DROP TABLE IF EXISTS ConfiguratedDevices;

DROP TABLE IF EXISTS Devices;
DROP TABLE IF EXISTS DeviceCapabilities;
DROP TABLE IF EXISTS DeviceChannels;
DROP TABLE IF EXISTS DeviceHistory;
DROP TABLE IF EXISTS DeviceReadings;
DROP TABLE IF EXISTS DeviceLinks;
DROP TABLE IF EXISTS DeviceLinkParameters;
DROP TABLE IF EXISTS TopologyLinks;
DROP TABLE IF EXISTS TopologyGraphs;
DROP TABLE IF EXISTS DeviceBatteryStatistics;
DROP TABLE IF EXISTS DeviceReadings;

DROP TABLE IF EXISTS Commands;
DROP TABLE IF EXISTS CommandParameters;

DROP TABLE IF EXISTS Firmwares;

DROP TABLE IF EXISTS DeviceInformation_ChannelHoppings;
DROP TABLE IF EXISTS DeviceInformation_Superframes;
DROP TABLE IF EXISTS DeviceInformation_Neighbours;
DROP TABLE IF EXISTS DeviceInformation_Graphs;
DROP TABLE IF EXISTS DeviceInformation_Links;
DROP TABLE IF EXISTS DeviceInformation_Routes;DROP TABLE IF EXISTS DeviceInformation_ReceiveTemplates;
DROP TABLE IF EXISTS DeviceInformation_TransmitTemplates;
DROP TABLE IF EXISTS Users;
DROP TABLE IF EXISTS Companies;
DROP TABLE IF EXISTS LocalControlLoops;

DROP TABLE IF EXISTS DeviceConnections;
DROP TABLE IF EXISTS Properties;
CREATE TABLE DeviceTypes
(
	DeviceTypeID 	INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,
	DeviceTypeName 	VARCHAR(50) NOT NULL,
	CompanyID INTEGER NOT NULL DEFAULT -1,
  	IconFile VARCHAR(100),
  	SensorType INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE ConfiguratedDevices
(
	ID INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,
	Address64 VARCHAR(50) NOT NULL,       
  SensorTypeID INTEGER NOT NULL           
);
CREATE TABLE DeviceTypeChannels
(
	DeviceTypeID	INTEGER NOT NULL,
	ChannelNo		INTEGER NOT NULL, 
	ChannelName 	VARCHAR(100) NOT NULL,
	MinValue 		INTEGER NOT NULL, 
	MaxValue 		INTEGER NOT NULL, 
	MinRawValue 	INTEGER NOT NULL,
	MaxRawValue 	INTEGER NOT NULL,	
	ChannelData		INTEGER NOT NULL DEFAULT 2,
	MappedTSAPID 	INTEGER NOT NULL,
	MappedObjectID 	INTEGER NOT NULL,
	MappedAttributeID 	INTEGER NOT NULL,
	ChannelType 	INTEGER NOT NULL,
	IsPredefined 	INTEGER NOT NULL,
	Enabled 		INTEGER NOT NULL DEFAULT 1,
	UnitOfMeasurement VARCHAR(50) NOT NULL DEFAULT 'N/A',
	RadioInterfaceType TINYINT NOT NULL DEFAULT 1
);
CREATE TABLE Devices
(
	DeviceID INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,
	DeviceTypeID INTEGER NOT NULL,
	SensorTypeID INTEGER NOT NULL DEFAULT -1,
	DeviceImplementationType INTEGER NOT NULL DEFAULT 0,
	Address128 VARCHAR(50),
	Address64 VARCHAR(50) NOT NULL,
	DeviceStatus INTEGER NOT NULL,
	LastRead DATETIME,
	DeviceLevel INTEGER NOT NULL,
	LastDeviceUpdated DATETIME,
	RejoinCount INTEGER 
);

CREATE TABLE DeviceCapabilities
(
  DeviceID INTEGER NOT NULL,
  AquisitionBoardType INTEGER NOT NULL,
  PowerType INTEGER NOT NULL,
  AquisitionBoardVersion INTEGER
);
CREATE TABLE DeviceChannels
(
	DeviceID INTEGER NOT NULL,
	ChannelNo INTEGER NOT NULL,
	ChannelName VARCHAR(100) NOT NULL,
	MinValue INTEGER NOT NULL,
	MaxValue INTEGER NOT NULL,
	MinRawValue INTEGER NOT NULL,
	MaxRawValue INTEGER NOT NULL,
	ChannelData		INTEGER NOT NULL DEFAULT 2, 
	MappedTSAPID 	INTEGER NOT NULL,
	MappedObjectID 	INTEGER NOT NULL,
	MappedAttributeID 	INTEGER NOT NULL,
	
	ChannelType 	INTEGER NOT NULL,
	IsPredefined INTEGER NOT NULL,
	Enabled INTEGER NOT NULL DEFAULT 1,
	UnitOfMeasurement VARCHAR(50) NOT NULL DEFAULT 'N/A',
	LastPSCommandID INTEGER,
	RadioInterfaceType TINYINT NOT NULL DEFAULT 1
);

CREATE TABLE DeviceHistory
(
	HistoryID	INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,
	DeviceID 	INTEGER NOT NULL,
	Timestamp	DATETIME NOT NULL,
	DeviceStatus	INTEGER NOT NULL
);

CREATE TABLE TopologyLinks
(
    FromDeviceID INTEGER NOT NULL,
    ToDeviceID INTEGER NOT NULL,
    SignalQuality INTEGER NOT NULL
);

CREATE TABLE TopologyGraphs
(
    FromDeviceID INTEGER NOT NULL,
    ToDeviceID INTEGER NOT NULL,
    GraphID INTEGER NOT NULL
);

CREATE TABLE DeviceLinks
(
    LinkID INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,
    FromDeviceID INTEGER NOT NULL,
    FromChannel INTEGER NOT NULL,    
    ToDeviceID INTEGER NOT NULL,
    LinkType INTEGER NOT NULL,
    CreatedTime DATETIME NOT NULL,
	LinkStatus INTEGER NOT NULL
);

CREATE TABLE DeviceLinkParameters
(
  LinkID INTEGER NOT NULL,
  ParameterCode INTEGER NOT NULL,
  ParameterValue VARCHAR(50)
);

CREATE TABLE DeviceReadings
(
	DeviceID INTEGER NOT NULL,
	ReadingTime DATETIME NOT NULL,
	ChannelNo INTEGER NOT NULL,
	Value VARCHAR(50) NOT NULL,
	RawValue INTEGER NOT NULL,
	ReadingType INTEGER NOT NULL,
	Miliseconds smallint(6) NOT NULL default '0',
	PRIMARY KEY  (DeviceID, ReadingTime, ChannelNo, ReadingType, Miliseconds)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;



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
	BatteryLevel INTEGER NOT NULL,
	RemainingBatteryHours INTEGER NOT NULL		
);

CREATE TABLE Commands
(
	CommandID INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,
	DeviceID INTEGER NOT NULL,
	CommandCode INTEGER NOT NULL,
	CommandStatus INTEGER NOT NULL,
	TimePosted DATETIME NOT NULL,
	TimeResponsed DATETIME,
	ErrorCode INTEGER NOT NULL DEFAULT 0,
	ErrorReason VARCHAR(256),
	Response VARCHAR(256),
	Generated INTEGER DEFAULT 0 NOT NULL
);

CREATE TABLE CommandParameters
(
	CommandID INTEGER NOT NULL,
	ParameterCode INTEGER NOT NULL,
	ParameterValue VARCHAR(50)
);
CREATE TABLE Firmwares
(
   FirmwareID INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,
   FileName VARCHAR(250) NOT NULL,
   Version VARCHAR(11) NOT NULL,
   Description VARCHAR(1000),
   UploadDate DATETIME,
   FirmwareType INTEGER NOT NULL,
   UploadStatus INTEGER NOT NULL DEFAULT 0,   
   UploadRetryCount INTEGER NOT NULL DEFAULT 0,   
   LastFailedUploadTime DATETIME
);
CREATE TABLE DeviceInformation_ChannelHoppings
(
	DeviceID INTEGER NOT NULL,
	ChannelHoppingID INTEGER NOT NULL,
	ChannelIDs  VARCHAR(256)
);

CREATE TABLE DeviceInformation_Superframes
(
	DeviceID INTEGER NOT NULL,
	SuperframeID INTEGER NOT NULL,
	ChannelHoppingID INTEGER NOT NULL,
	ChannelHoppingOffset INTEGER NOT NULL,
	Priority INTEGER NOT NULL,
	SuperframeLength INTEGER NOT NULL
);
CREATE TABLE DeviceInformation_Neighbours
(
	DeviceID INTEGER NOT NULL,
	NeighbourAddress16 INTEGER NOT NULL,
	GroupCode INTEGER NOT NULL,
	IsClockSource BOOLEAN NOT NULL
);
CREATE TABLE DeviceInformation_Graphs
(
	DeviceID INTEGER NOT NULL,
	GraphID INTEGER NOT NULL,
	NeighbourAddresses16 VARCHAR(256)
);

CREATE TABLE DeviceInformation_Links
(
	DeviceID INTEGER NOT NULL,
	LinkID INTEGER NOT NULL,
	SuperframeID INTEGER NOT NULL,
	Type INTEGER NOT NULL,
	NeighbourType INTEGER NOT NULL,
	GraphType INTEGER NOT NULL,
	ScheduleType INTEGER NOT NULL,
	ChannelType INTEGER NOT NULL,
	PriorityType INTEGER NOT NULL,
	Neighbor INTEGER NOT NULL,
	GraphID INTEGER NOT NULL,
	Schedule INTEGER NOT NULL,
	ChannelOffset INTEGER NOT NULL,
	Priority INTEGER NOT NULL
);
#Option must be enclosed between `` because is it considered a keyword
CREATE TABLE DeviceInformation_Routes
(
	DeviceID INTEGER NOT NULL,
	RouteID INTEGER NOT NULL,
	`Option` INTEGER NOT NULL,
	PayloadSize INTEGER NOT NULL,
	RouteAddresses16 VARCHAR(256),
 	Selector INTEGER NOT NULL
);

CREATE TABLE DeviceInformation_ReceiveTemplates
(
	DeviceID INTEGER NOT NULL,
	ReceiveTemplateID INTEGER NOT NULL,
	EnableReceiver INTEGER NOT NULL,
	Timeout INTEGER NOT NULL,
	ACKEarliest INTEGER NOT NULL,
	ACKLatest INTEGER NOT NULL,
	ACKRef INTEGER NOT NULL,
	RespectBoundary INTEGER NOT NULL
);

CREATE TABLE DeviceInformation_TransmitTemplates
(
	DeviceID INTEGER NOT NULL,
	TransmitTemplateID INTEGER NOT NULL,
	XMITEarliest INTEGER NOT NULL,
	XMITLatest INTEGER NOT NULL,
	ACKEarliest INTEGER NOT NULL,
	ACKLatest INTEGER NOT NULL,
	ACKRefEnd INTEGER NOT NULL,
	CheckCCA INTEGER NOT NULL,
	KeepListening INTEGER NOT NULL	
);
#Key must be enclosed between `` because is it considered a keyword
CREATE TABLE Properties
(
	`Key` VARCHAR(50) NOT NULL,
 	Value VARCHAR(50)
);

CREATE TABLE Users
(
	UserID	 		INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,
	UserName 		VARCHAR(50) NOT NULL,
	FirstName 		VARCHAR(50) NOT NULL,
	LastName 		VARCHAR(50) NOT NULL,
	Email 			VARCHAR(50) NOT NULL,
	Password 		VARCHAR(250) NOT NULL,
	Role			INTEGER
);

CREATE TABLE Companies
(
	CompanyID INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,  
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

CREATE UNIQUE INDEX IX_DeviceConnections
ON DeviceConnections(DeviceID);

CREATE UNIQUE INDEX IX_DeviceCapabilities
ON DeviceCapabilities( DeviceID);

CREATE UNIQUE INDEX IX_DeviceTypeChannels 
ON DeviceTypeChannels (DeviceTypeID, ChannelNo, RadioInterfaceType);

CREATE UNIQUE INDEX IX_DeviceChannels 
ON DeviceChannels (DeviceID, ChannelNo, RadioInterfaceType);

CREATE UNIQUE INDEX IX_TopologyLinks
ON TopologyLinks( FromDeviceID, ToDeviceID);

CREATE UNIQUE INDEX IX_TopologyGraphs
ON TopologyGraphs( FromDeviceID, ToDeviceID, GraphID);

CREATE UNIQUE INDEX IX_DeviceLinks
ON DeviceLinks( FromDeviceID, ToDeviceID, LinkType, FromChannel);

CREATE INDEX IX_Commands
ON Commands( DeviceID, CommandCode, TimePosted);

CREATE INDEX IX_DeviceReadings
ON DeviceReadings(DeviceID, ChannelNo, ReadingTime);

CREATE INDEX IX_DeviceBatteryStatistics
ON DeviceReadings(DeviceID, ReadingTime);

CREATE INDEX IX_CommandParameters
ON CommandParameters( CommandID, ParameterCode);

CREATE UNIQUE INDEX IX_Properties 
ON Properties(`Key`); 