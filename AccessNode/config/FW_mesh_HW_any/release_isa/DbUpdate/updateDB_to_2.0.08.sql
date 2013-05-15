DELETE FROM Properties WHERE `Key`='Version';
REPLACE INTO Properties(`Key`, Value) VALUES('SchemaVersion', '2.0.08');
REPLACE INTO Properties(`Key`, Value) VALUES('DataVersion', '2.0.08');


DELETE FROM CommandSet;

/*REPORTS*/

/*---command.deviceID=gw---*/
INSERT INTO CommandSet(CommandCode,CommandName, ParameterDescription,IsVisible)
VALUES(0, 'Request Topology', NULL, 1);

INSERT INTO CommandSet(CommandCode,CommandName,ParameterDescription,IsVisible)
VALUES(113, 'Network Health Report', NULL, 1);

INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
VALUES(114, 'Schedule Report', 61, 'Target Device ID', 1);

INSERT INTO CommandSet(CommandCode,CommandName,ParameterDescription,IsVisible)
VALUES(115, 'Neighbor Health Report', 'Target Device ID', 1);

INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
VALUES(116, 'Device Health Report', 63, 'Target Device ID', 1);

INSERT INTO CommandSet(CommandCode,CommandName,ParameterDescription,IsVisible)
VALUES(117, 'Network Resource Report', NULL, 1);

INSERT INTO CommandSet(CommandCode,CommandName, ParameterDescription,IsVisible)
VALUES(120, 'Get Devices List', NULL, 1);



/*CLIENT/SERVER*/

/*---command.deviceID=DeviceID---*/
INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
VALUES(1, 'Read Value', 10, 'Channel', 1);

INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
SELECT 10, 'ISACSRequest', 80, 'TSAPID', 1
UNION
SELECT 10, 'ISACSRequest', 81, 'RequestType', 1
UNION
SELECT 10, 'ISACSRequest', 82, 'ObjID', 1
UNION
SELECT 10, 'ISACSRequest', 83, 'AttributeID/MethodID', 1
UNION
SELECT 10, 'ISACSRequest', 84, 'AttributeIndex1', 1
UNION
SELECT 10, 'ISACSRequest', 85, 'AttributeIndex2', 1
UNION
SELECT 10, 'ISACSRequest', 86, 'DAtaBuffer', 1
UNION
SELECT 10, 'ISACSRequest', 87, 'ReadAsPublish', 1;


/*---command.deviceID=sm---*/
INSERT INTO CommandSet(CommandCode,CommandName,ParameterDescription,IsVisible)
VALUES(119, 'Get Contracts and Routes', NULL, 1);

INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
SELECT 108,'Reset Device',1080,'Target Device ID', 1
UNION
SELECT 108,'Reset Device',1081,'Restart Type', 1;

INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
SELECT 104, 'Firmware Update', 1040, 'Target Device ID', 1
UNION
SELECT 104, 'Firmware Update', 1041, 'File Name', 1;

INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
VALUES(103, 'Get Radio FW Version', 1030, 'Target Device ID', 1);

INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
VALUES(105, 'Get Firmware Update Status', 1050,'Target Device ID', 1);

INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
VALUES(106, 'Cancel Firmware Update', 1060, 'Target Device ID', 1);

INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
VALUES(123, 'Get Channels Statistics', 1220, 'Target Device ID', 1);


/*BULK*/

/*---command.deviceID=sm---*/
INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
SELECT 110, 'Firmware Upload', 1100, 'File Name', 1;


/*---command.deviceID=gw---*/
INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
SELECT 121, 'Sensor Board Firmware Update', 1200, 'Target Device ID', 1
UNION
SELECT 121, 'Sensor Board Firmware Update', 1201, 'File Name', 1 
UNION
SELECT 121, 'Sensor Board Firmware Update', 1202, 'port', 1
UNION
SELECT 121, 'Sensor Board Firmware Update', 1203, 'objID', 1;

INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
VALUES(122, 'Cancel Sensor Board Firmware Update', 1210, 'Target Device ID', 1);



/*PUBLIHING*/

/*---command.deviceID=DeviceID---*/
INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
SELECT 3, 'Publish/Subscribe', 31, 'Publish Frequency', 1
UNION
SELECT 3, 'Publish/Subscribe', 32, 'Publish Offset', 1      
UNION
SELECT 3, 'Publish/Subscribe', 35, 'Concentrator Object ID', 1
UNION
SELECT 3, 'Publish/Subscribe', 36, 'TSAP ID', 1
UNION
SELECT 3, 'Publish/Subscribe', 37, 'Data Stale Limit', 1;


/*LEASE*/

/*---command.deviceID=DeviceID---*/
INSERT INTO CommandSet(CommandCode,CommandName,ParameterDescription,IsVisible)
VALUES(100, 'Create Lease', NULL, 1);

/*---command.deviceID=gw---*/
INSERT INTO CommandSet(CommandCode,CommandName,ParameterCode,ParameterDescription,IsVisible)
VALUES(112, 'Delete Lease', 38, 'Lease ID', 1);




