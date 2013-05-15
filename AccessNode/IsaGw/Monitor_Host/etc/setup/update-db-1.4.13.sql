ALTER TABLE DeviceBatteryStatistics RENAME TO DBS_TMP;

DROP TABLE IF EXISTS DeviceBatteryStatistics;

CREATE TABLE DeviceBatteryStatistics
(
	DeviceID INTEGER NOT NULL,
	ReadingTime DATETIME NOT NULL,
	TransmissionsCount INTEGER NOT NULL,
	ReceptionsCount INTEGER NOT NULL,
	AbandonedReceptions INTEGER NOT NULL,
	SleepsCount INTEGER NOT NULL,
	ConsumedBatteryPeriod INTEGER NOT NULL,		
	ConsumedBatteryEnergy INTEGER NOT NULL,
	BatteryLevel INTEGER NOT NULL,		
	RemainingBatteryHours INTEGER NOT NULL	
);

INSERT INTO DeviceBatteryStatistics
(DeviceID, ReadingTime, TransmissionsCount, ReceptionsCount, AbandonedReceptions, SleepsCount, ConsumedBatteryPeriod, ConsumedBatteryEnergy, BatteryLevel, RemainingBatteryHours)
 SELECT DeviceID, ReadingTime, TransmissionsCount, ReceptionsCount, AbandonedReceptions, SleepsCount, 0, BatteryValue, BatteryLevel, 0 
 FROM DBS_TMP;
 
DROP TABLE IF EXISTS DBS_TMP;						

--update version in Properties table
DELETE FROM Properties WHERE Key='Version'; 
REPLACE INTO Properties(Key, Value) VALUES('SchemaVersion', '1.4.13');
REPLACE INTO Properties(Key, Value) VALUES('DataVersion', '1.4.13');