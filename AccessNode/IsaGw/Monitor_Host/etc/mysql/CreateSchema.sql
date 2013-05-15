DROP DATABASE IF EXISTS Monitor_Host;
CREATE DATABASE Monitor_Host;
USE Monitor_Host;

/* create tables */
CREATE TABLE Devices
(
                DeviceID                             INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,
                DeviceRole                         	INTEGER NOT NULL,
                Address128                        	VARCHAR(50),
                Address64                           VARCHAR(50) NOT NULL,
		  		DeviceTag							VARCHAR(50) NOT NULL, 
                DeviceStatus                      	INTEGER NOT NULL,
                LastRead                             DATETIME,
                DeviceLevel                        INTEGER NOT NULL,
                RejoinCount 					INTEGER,
                PublishErrorFlag                INTEGER NOT NULL DEFAULT 0,
                PublishStatus 					INTEGER NOT NULL DEFAULT 0 
);

CREATE TABLE DevicesInfo
(
                DeviceID                              INTEGER PRIMARY KEY,
                DPDUsTransmitted         INTEGER,
                DPDUsReceived                               INTEGER,
                DPDUsFailedTransmission            INTEGER,
                DPDUsFailedReception                 INTEGER,
                PowerSupplyStatus        INTEGER,
                Manufacturer                    VARCHAR(50),
                Model                                   VARCHAR(30),
                Revision                               VARCHAR(20),
                SerialNo                               VARCHAR(20),
                SubnetID                             INTEGER NULL 
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
    SignalQuality INTEGER NOT NULL,
    ClockSource INTEGER NOT NULL
);
CREATE UNIQUE INDEX IX_TopologyLinks
ON TopologyLinks( FromDeviceID, ToDeviceID);

CREATE TABLE TopologyGraphs
(
    FromDeviceID INTEGER NOT NULL,
    ToDeviceID INTEGER NOT NULL,
    GraphID INTEGER NOT NULL
);
CREATE UNIQUE INDEX IX_TopologyGraphs
ON TopologyGraphs( FromDeviceID, ToDeviceID, GraphID);


CREATE TABLE DeviceReadings
(
	DeviceID INTEGER NOT NULL,
	ReadingTime DATETIME NOT NULL,
	ChannelNo INTEGER NOT NULL,
	Value VARCHAR(50) NOT NULL,
	RawValue INTEGER NOT NULL,
	ReadingType INTEGER NOT NULL,
	Miliseconds INTEGER NOT NULL default '0',
	ValueStatus  INTEGER NULL,
	PRIMARY KEY  (DeviceID, ReadingTime, ChannelNo, ReadingType, Miliseconds)
);
CREATE INDEX IX_DeviceReadings USING BTREE
ON DeviceReadings(DeviceID, ChannelNo, ReadingTime);

CREATE TABLE DeviceReadingsHistory
(
	DeviceID INTEGER NOT NULL,
	ReadingTime DATETIME NOT NULL,
	ChannelNo INTEGER NOT NULL,
	Value VARCHAR(50) NOT NULL,
	RawValue INTEGER NOT NULL,
	ReadingType INTEGER NOT NULL,
	Miliseconds INTEGER NOT NULL default '0',
	ValueStatus  INTEGER NULL
);
CREATE INDEX IX_DeviceReadingsHistory USING BTREE
ON DeviceReadingsHistory(DeviceID, ChannelNo, ReadingTime);


CREATE TABLE Commands
(
	CommandID INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,
	DeviceID INTEGER NOT NULL,
	CommandCode INTEGER NOT NULL,
	CommandStatus INTEGER NOT NULL,
	TimePosted DATETIME NOT NULL,
	TimeResponsed DATETIME,
	ErrorCode INTEGER NOT NULL DEFAULT 0,
	ErrorReason VARCHAR(50),
	Response VARCHAR(50),
	Generated INTEGER DEFAULT 0 NOT NULL,
	ParametersDescription VARCHAR(1024) NULL
);
CREATE INDEX IX_Commands
ON Commands( DeviceID, CommandCode, TimePosted);


CREATE TABLE CommandParameters
(
	CommandID INTEGER NOT NULL,
	ParameterCode INTEGER NOT NULL,
	ParameterValue VARCHAR(50)
);
CREATE INDEX IX_CommandParameters
ON CommandParameters( CommandID, ParameterCode);


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

-- Key must be enclosed between `` because is it considered a keyword
CREATE TABLE Properties
(
	`Key` VARCHAR(50) NOT NULL,
 	Value VARCHAR(50)
);
CREATE UNIQUE INDEX IX_Properties 
ON Properties(`Key`); 


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

CREATE TABLE DeviceConnections
(
  DeviceID INTEGER NOT NULL,
  IP VARCHAR(50) NOT NULL,
  Port INTEGER NOT NULL    
);
CREATE UNIQUE INDEX IX_DeviceConnections
ON DeviceConnections(DeviceID);


CREATE TABLE Contracts
(
ContractID INTEGER NOT NULL,
ServiceType INTEGER NOT NULL,
ActivationTime DATETIME,
SourceDeviceID INTEGER NOT NULL,    
SourceSAP INTEGER,
DestinationDeviceID INTEGER NOT NULL,
DestinationSAP INTEGER,
ExpirationTime DATETIME,
Priority INTEGER,
NSDUSize INTEGER,
Reliability INTEGER,
Period INTEGER NULL,
Phase INTEGER NULL,
ComittedBurst INTEGER NULL,
ExcessBurst INTEGER NULL,
Deadline INTEGER,
MaxSendWindow INTEGER,
PRIMARY KEY(ContractID, SourceDeviceID)
);

CREATE TABLE RoutesInfo
(
DeviceID    INTEGER NOT NULL,
RouteID     INTEGER NOT NULL,
Alternative INTEGER,
FowardLimit INTEGER,
Selector    INTEGER,
SrcAddr     INTEGER,
PRIMARY KEY(DeviceID,RouteID)
);

CREATE TABLE RouteLinks
(
DeviceID    INTEGER NOT NULL,
RouteID     INTEGER NOT NULL,
RouteIndex  INTEGER NOT NULL,
NeighbourID INTEGER DEFAULT NULL,
GraphID     INTEGER DEFAULT NULL,
PRIMARY KEY(DeviceID, RouteID, RouteIndex)
);

CREATE TABLE ContractElements (
  ContractID          INTEGER NOT NULL,
  SourceDeviceID      INTEGER NOT NULL,
  Idx                 INTEGER NOT NULL,
  DeviceID            INTEGER NOT NULL,
  PRIMARY KEY (ContractID, SourceDeviceID, Idx)
);


CREATE TABLE RFChannels
(
ChannelNumber INTEGER PRIMARY KEY,
ChannelStatus INTEGER
);


CREATE TABLE DeviceScheduleSuperframes
(
DeviceID INTEGER NOT NULL,
SuperframeID INTEGER NOT NULL,
NumberOfTimeSlots INTEGER,
StartTime DATETIME,
NumberOfLinks INTEGER,
PRIMARY KEY (DeviceID, SuperframeID)
);


CREATE TABLE DeviceScheduleLinks
(
DeviceID INTEGER NOT NULL,
SuperframeID INTEGER NOT NULL,
NeighborDeviceID INTEGER NOT NULL,
SlotIndex INTEGER,
LinkPeriod INTEGER,
SlotLength INTEGER,
ChannelNumber INTEGER,
Direction INTEGER,
LinkType INTEGER
);



CREATE TABLE NetworkHealth
(
NetworkID INTEGER,
NetworkType INTEGER,
DeviceCount INTEGER,
StartDate DATETIME,
CurrentDate DATETIME,
DPDUsSent INTEGER,
DPDUsLost INTEGER,
GPDULatency INTEGER,
GPDUPathReliability INTEGER,
GPDUDataReliability INTEGER,
JoinCount INTEGER
);


CREATE TABLE NetworkHealthDevices
(
DeviceID INTEGER PRIMARY KEY, 
StartDate DATETIME,
CurrentDate DATETIME,
DPDUsSent INTEGER, 
DPDUsLost INTEGER, 
GPDULatency INTEGER, 
GPDUPathReliability INTEGER, 
GPDUDataReliability INTEGER, 
JoinCount INTEGER
);

CREATE TABLE DeviceHealthHistory
(
DeviceID INTEGER,
Timestamp DATETIME,
DPDUsTransmitted INTEGER,
DPDUsReceived INTEGER,
DPDUsFailedTransmission INTEGER,
DPDUsFailedReception INTEGER,
PRIMARY KEY(DeviceID ,Timestamp )
);

CREATE TABLE NeighborHealthHistory
(
DeviceID INTEGER,
Timestamp DATETIME,
NeighborDeviceID INTEGER,
LinkStatus INTEGER,
DPDUsTransmitted INTEGER,
DPDUsReceived INTEGER,
DPDUsFailedTransmission INTEGER,
DPDUsFailedReception INTEGER,
SignalStrength INTEGER,
SignalQuality INTEGER,
PRIMARY KEY(DeviceID, Timestamp, NeighborDeviceID )
);


CREATE TABLE CommandSet
(
CommandCode INTEGER NOT NULL,
CommandName VARCHAR(50) NOT NULL,
ParameterCode INTEGER NOT NULL DEFAULT -1,
ParameterDescription VARCHAR(100) NULL,
IsVisible INTEGER DEFAULT 1,
PRIMARY KEY  (CommandCode,ParameterCode)
);

CREATE TABLE DeviceChannels
(
	DeviceID 			INTEGER NOT NULL,
	ChannelNo 			INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,
	ChannelName 		VARCHAR(128) NOT NULL DEFAULT 'N/A',
	UnitOfMeasurement 	VARCHAR(64) NOT NULL DEFAULT 'N/A',
	ChannelFormat		INTEGER NOT NULL, 
	SourceTSAPID 		INTEGER NOT NULL,
	SourceObjID 		INTEGER NOT NULL,
	SourceAttrID 		INTEGER NOT NULL,
	SourceIndex1 		INTEGER NOT NULL,
	SourceIndex2 		INTEGER NOT NULL,
	WithStatus 			INTEGER
);

CREATE TABLE DeviceChannelsHistory
(
	DeviceID 			INTEGER NOT NULL,
	ChannelNo 			INTEGER NOT NULL PRIMARY KEY,
	ChannelName 		VARCHAR(128) NOT NULL DEFAULT 'N/A',
	UnitOfMeasurement 	VARCHAR(64) NOT NULL DEFAULT 'N/A',
	ChannelFormat		INTEGER NOT NULL, 
	SourceTSAPID 		INTEGER NOT NULL,
	SourceObjID 		INTEGER NOT NULL,
	SourceAttrID 		INTEGER NOT NULL,
	SourceIndex1 		INTEGER NOT NULL,
	SourceIndex2 		INTEGER NOT NULL,
	WithStatus 			INTEGER,
	Timestamp	  		DATETIME
);


CREATE TABLE ISACSConfirmDataBuffer
(
	DeviceID INTEGER NOT NULL,
	Timestamp  DATETIME,
	Miliseconds INTEGER NOT NULL default '0',
	TSAPID INTEGER NOT NULL,
	RequestType INTEGER NOT NULL,
	ObjectID INTEGER NOT NULL,
	ObjResourceID INTEGER NOT NULL,   					-- AttributeID or MethodID in function of requesttype
	AttrIndex1 INTEGER NOT NULL DEFAULT -1,		 	-- only for read requests (-1 for execute requests)
	AttrIndex2 INTEGER NOT NULL DEFAULT -1,			-- only for read requests (-1 for execute requests)
	DataBuffer VARCHAR(64) NOT NULL,					-- data buffer in which is stored a hexstring 'AB9423...'
	PRIMARY KEY  (DeviceID, Timestamp, Miliseconds)
);
CREATE INDEX IX_ISACSConfirmDataBuffer
ON ISACSConfirmDataBuffer( DeviceID, TSAPID, RequestType, ObjectID, ObjResourceID);


CREATE TABLE AlertNotifications
(
	AlertNotifID		INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,
	DeviceID 			INTEGER NOT NULL,
	TsapID		 		INTEGER NOT NULL,
	ObjID		 		INTEGER NOT NULL,
	
	AlertTime 			DATETIME NOT NULL,
	AlertTimeMsec		INTEGER NOT NULL,
	
	Class				INTEGER NOT NULL,			-- 0 ? Event  1 ? Alarm 
	Direction 			INTEGER,					-- 0 ? Alarm ended  1 ? Alarm began, just ignore if class is event
	Category			INTEGER NOT NULL, 			-- 0 ? process,1 ? device 2 ? network 3 ? security
 	Type				INTEGER NOT NULL,			-- Application-specific
 	Priority			INTEGER NOT NULL,			-- Application-specific
 	AlertData			VARCHAR(128)				-- string hex  Application-specific

);	
	
-- should have one row
CREATE TABLE AlertSubscriptionCategory
(
	CategoryProcess		INTEGER,			-- 1 - subscribe, 0 - unscribe 	
	CategoryDevice		INTEGER,			-- 1 - subscribe, 0 - unscribe 	
	CategoryNetwork		INTEGER,			-- 1 - subscribe, 0 - unscribe 	
	CategorySecurity	INTEGER				-- 1 - subscribe, 0 - unscribe 	
);	

CREATE TABLE AlertSubscriptionAlertSourceID
(
	DeviceID 			INTEGER NOT NULL,
	TsapID		 		INTEGER NOT NULL,
	ObjID		 		INTEGER NOT NULL,
	Type				INTEGER NOT NULL			-- Application-specific
);

CREATE TABLE ChannelsStatistics
(
     DeviceID                 INTEGER NOT NULL,
     ByteChannelsArray        VARCHAR(32)          /*16 channels per device => hex_string_len=32*/
);

CREATE TABLE FirmwareDownloads
(
    DeviceID                INTEGER,
    FwType                  INTEGER,            /* Managed by MCS : 10-Device,  0-Acquistion Board, 3-BBR */  
    FwStatus                INTEGER,            /* Set from MCS to:   1-InProgress,  2-CancelInProgress, 
      												  Updated by MH to:   3-Cancelled, 4-Completed, 5-Failed */
    CompletedPercent     	INTEGER NULL,       /* calculated & updated by MH */
    DownloadSize            INTEGER NULL,       /* calculated MH */
    Speed                   INTEGER NULL,       /* calculated & updated by MH in packets/min*/
    AvgSpeed                INTEGER NULL,       /* calculated & updated by MH in packets/min */
    StartedOn               DATETIME,           /* calculated by MCS when creating the record*/
    CompletedOn             DATETIME NULL,		/* calculated by MH */
    PRIMARY KEY (DeviceID, FwType, StartedOn)
);

CREATE TABLE Dashboard(
SlotNumber       integer,
DeviceID         integer,
ChannelNo        integer,
GaugeType        integer,
MinValue         real,
MaxValue         real);

