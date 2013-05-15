ALTER TABLE DeviceTypeChannels RENAME TO DTC_TMP;
DROP TABLE IF EXISTS DeviceTypeChannels;
CREATE TABLE DeviceTypeChannels
(
	DeviceTypeID	INTEGER NOT NULL,
	ChannelNo	INTEGER NOT NULL, 
	ChannelName 	VARCHAR(100) NOT NULL, 
	MinValue 	INTEGER NOT NULL, 
	MaxValue 	INTEGER NOT NULL, 
	MinRawValue INTEGER NOT NULL,
	MaxRawValue INTEGER NOT NULL,	
	IsPredefined 	INTEGER NOT NULL,
	ChannelType INTEGER NOT NULL,
	Enabled INTEGER NOT NULL DEFAULT 1,
	UnitOfMeasurement VARCHAR(50) NOT NULL DEFAULT 'N/A'
);
INSERT INTO DeviceTypeChannels
(DeviceTypeID, ChannelNo, ChannelName, MinValue, MaxValue, MinRawValue,	MaxRawValue, IsPredefined, ChannelType,	Enabled, UnitOfMeasurement)
 SELECT DeviceTypeID, ChannelNo, ChannelName, MinValue, MaxValue, MinRawValue,	MaxRawValue, IsPredefined, ChannelType,	Enabled, 'N/A' 
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
	IsPredefined INTEGER NOT NULL,
	ChannelType INTEGER NOT NULL,
	Enabled INTEGER NOT NULL DEFAULT 1,
	UnitOfMeasurement VARCHAR(50) NOT NULL DEFAULT 'N/A'
);
INSERT INTO DeviceChannels
(DeviceID,	ChannelNo,	ChannelName, MinValue, MaxValue, MinRawValue, MaxRawValue, IsPredefined, ChannelType, Enabled, UnitOfMeasurement)
 SELECT DeviceID,	ChannelNo,	ChannelName, MinValue, MaxValue, MinRawValue, MaxRawValue, IsPredefined, ChannelType, Enabled, 'N/A' 
 FROM DC_TMP;

DROP TABLE IF EXISTS DC_TMP;



--update version in Properties table
DELETE FROM Properties WHERE Key='Version'; 
REPLACE INTO Properties(Key, Value) VALUES('SchemaVersion', '10.2.15');
REPLACE INTO Properties(Key, Value) VALUES('DataVersion', '10.2.15');