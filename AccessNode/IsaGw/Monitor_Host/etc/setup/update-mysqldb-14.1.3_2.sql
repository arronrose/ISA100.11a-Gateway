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

DELETE FROM Devices;
INSERT INTO Devices(DeviceTypeID, Address128, Address64, DeviceStatus, DeviceLevel)
  VALUES(2, '::', '600D:BEEF:600D:BEEF', 0, -1);
  
DELETE FROM DeviceTypeChannels;
/*
for echech device type, currently we have 10=routing, 11=non routing
*/
/*Field Router */
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,1,'UAP_DATA_ANALOG1',0,0,0,0,6,2,129,1,0,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,2,'UAP_DATA_ANALOG2',0,0,0,0,6,2,129,2,0,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,3,'UAP_DATA_ANALOG3',0,0,0,0,6,2,129,3,0,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,4,'UAP_DATA_ANALOG4',0,0,0,0,6,2,129,4,0,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,5,'UAP_DATA_INPUT_TEMP',0,0,0,0,6,2,129,5,0,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,6,'UAP_DATA_INPUT_HUMIDITY',0,0,0,0,6,2,129,6,0,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,7,'UAP_DATA_INPUT_DEWPOINT',0,0,0,0,6,2,129,7,0,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,8,'UAP_DATA_INPUT_BATTERY',0,0,0,0,6,2,129,8,0,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,9,'UAP_DATA_DIGITAL1',0,0,0,0,0,2,129,9,1,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,10,'UAP_DATA_DIGITAL2',0,0,0,0,0,2,129,10,1,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,11,'UAP_DATA_DIGITAL3',0,0,0,0,0,2,129,11,1,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,12,'UAP_DATA_DIGITAL4',0,0,0,0,0,2,129,12,1,0,1,'N/A', 1);
/* Full ISA Radio Interface */
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,1,'DATA_ANALOG_1',0,0,0,0,6,2,3,1,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,2,'DATA_ANALOG_2',0,0,0,0,6,2,3,2,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,3,'DATA_ANALOG_3',0,0,0,0,6,2,3,3,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,4,'DATA_ANALOG_4',0,0,0,0,6,2,3,4,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,5,'DATA_DIGITAL4',0,0,0,0,6,2,3,21,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,6,'DATA_DIGITAL4',0,0,0,0,0,2,3,22,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,7,'DATA_DIGITAL4',0,0,0,0,0,2,3,23,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,8,'DATA_DIGITAL4',0,0,0,0,0,2,3,24,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,9,'DATA_WRITE_1',0,0,0,0,6,2,3,31,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,10,'DATA_WRITE_2',0,0,0,0,6,2,3,32,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,11,'DATA_WRITE_3',0,0,0,0,6,2,3,33,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,12,'DATA_WRITE_4',0,0,0,0,6,2,3,34,0,0,0,'N/A', 2);

/*Non-routing*/
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,1,'UAP_DATA_ANALOG1',0,0,0,0,6,2,129,1,0,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,2,'UAP_DATA_ANALOG2',0,0,0,0,6,2,129,2,0,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,3,'UAP_DATA_ANALOG3',0,0,0,0,6,2,129,3,0,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,4,'UAP_DATA_ANALOG4',0,0,0,0,6,2,129,4,0,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,5,'UAP_DATA_INPUT_TEMP',0,0,0,0,6,2,129,5,0,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,6,'UAP_DATA_INPUT_HUMIDITY',0,0,0,0,6,2,129,6,0,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,7,'UAP_DATA_INPUT_DEWPOINT',0,0,0,0,6,2,129,7,0,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,8,'UAP_DATA_INPUT_BATTERY',0,0,0,0,6,2,129,8,0,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,9,'UAP_DATA_DIGITAL1',0,0,0,0,0,2,129,9,1,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,10,'UAP_DATA_DIGITAL2',0,0,0,0,0,2,129,10,1,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,11,'UAP_DATA_DIGITAL3',0,0,0,0,0,2,129,11,1,0,1,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,12,'UAP_DATA_DIGITAL4',0,0,0,0,0,2,129,12,1,0,1,'N/A', 1);
/* Full ISA100 Radio Interface */
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,1,'DATA_ANALOG_1',0,0,0,0,6,2,3,1,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,2,'DATA_ANALOG_2',0,0,0,0,6,2,3,2,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,3,'DATA_ANALOG_3',0,0,0,0,6,2,3,3,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,4,'DATA_ANALOG_4',0,0,0,0,6,2,3,4,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,5,'DATA_DIGITAL4',0,0,0,0,0,2,3,21,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,6,'DATA_DIGITAL4',0,0,0,0,0,2,3,22,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,7,'DATA_DIGITAL4',0,0,0,0,0,2,3,23,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,8,'DATA_DIGITAL4',0,0,0,0,0,2,3,24,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,9,'DATA_WRITE_1',0,0,0,0,6,2,3,31,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,10,'DATA_WRITE_2',0,0,0,0,6,2,3,32,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,11,'DATA_WRITE_3',0,0,0,0,6,2,3,33,0,0,0,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,12,'DATA_WRITE_4',0,0,0,0,6,2,3,34,0,0,0,'N/A', 2);


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


INSERT INTO Properties(`Key`, Value) VALUES('SchemaVersion', '14.1.3');
INSERT INTO Properties(`Key`, Value) VALUES('DataVersion', '14.1.3');
 