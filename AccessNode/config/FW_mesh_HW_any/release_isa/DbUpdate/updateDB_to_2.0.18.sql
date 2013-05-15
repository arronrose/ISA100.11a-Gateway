
DELETE FROM Properties WHERE `Key`='Version';
REPLACE INTO Properties(`Key`, Value) VALUES('SchemaVersion', '2.0.18');
REPLACE INTO Properties(`Key`, Value) VALUES('DataVersion', '2.0.18');


ALTER TABLE DeviceChannels ADD COLUMN WithStatus INTEGER;
ALTER TABLE DeviceChannelsHistory ADD COLUMN WithStatus INTEGER;


/*---command.deviceID=gw---*/
DELETE FROM CommandSet WHERE CommandCode = 121;
INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
SELECT 121, 'Update Sensor Board Firmware', 1200, 'Target Device ID', 1
UNION
SELECT 121, 'Update Sensor Board Firmware', 1201, 'File Name', 1 
UNION
SELECT 121, 'Update Sensor Board Firmware', 1202, 'TSAP-ID', 1
UNION
SELECT 121, 'Update Sensor Board Firmware', 1203, 'objID', 1
UNION
SELECT 121, 'Update Sensor Board Firmware', 1230, 'Committed Burst', 1;

