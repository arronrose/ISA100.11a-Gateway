DELETE FROM Properties WHERE `Key`='Version';
REPLACE INTO Properties(`Key`, Value) VALUES('SchemaVersion', '2.0.15');
REPLACE INTO Properties(`Key`, Value) VALUES('DataVersion', '2.0.15');


ALTER TABLE DeviceChannelsHistory ADD COLUMN Timestamp DATETIME;


