DELETE FROM Properties WHERE `Key`='Version';
REPLACE INTO Properties(`Key`, Value) VALUES('SchemaVersion', '2.0.17');
REPLACE INTO Properties(`Key`, Value) VALUES('DataVersion', '2.0.17');


/*CLIENT/SERVER*/

/*---command.deviceID=DeviceID---*/
INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
VALUES(1, 'Read Value', 1230, 'Committed Burst', 1);

INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
SELECT 10, 'ISACSRequest', 1230, 'Committed Burst', 1;


/*---command.deviceID=sm---*/
INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
VALUES(119, 'Get Contracts and Routes', 1230, 'Committed Burst', 1);

INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
SELECT 108,'Reset Device', 1230, 'Committed Burst', 1;


INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
SELECT 104, 'Firmware Update', 1230, 'Committed Burst', 1;


INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
VALUES(103, 'Get Radio FW Version', 1230, 'Committed Burst', 1);

INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
VALUES(105, 'Get Firmware Update Status', 1230, 'Committed Burst', 1);

INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
VALUES(106, 'Cancel Firmware Update', 1230, 'Committed Burst', 1);

INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
VALUES(123, 'Get Channels Statistics', 1230, 'Committed Burst', 1);


/*BULK*/

/*---command.deviceID=sm---*/
INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
SELECT 110, 'Firmware Upload', 1230, 'Committed Burst', 1;


DELETE FROM CommandSet WHERE CommandCode = 122;
INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
VALUES(122, 'Cancel Update Sensor Board Firmware', 1210, 'Target Device ID', 1);
INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
VALUES(122, 'Cancel Update Sensor Board Firmware', 1230, 'Committed Burst', 1);

/*---command.deviceID=gw---*/
DELETE FROM CommandSet WHERE CommandCode = 121;
INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
SELECT 121, 'Update Sensor Board Firmware', 1200, 'Target Device ID', 1
UNION
SELECT 121, 'Update Sensor Board Firmware', 1201, 'File Name', 1 
UNION
SELECT 121, 'Update Sensor Board Firmware', 1202, 'port', 1
UNION
SELECT 121, 'Update Sensor Board Firmware', 1203, 'objID', 1
UNION
SELECT 121, 'Update Sensor Board Firmware', 1230, 'Committed Burst', 1;







