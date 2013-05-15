USE Monitor_Host;


/* CREATE statement for DeviceReadings with new column Miliseconds and new primary key */
DROP TABLE DeviceReadings;
CREATE TABLE DeviceReadings (
  DeviceID int(11) NOT NULL,
  ReadingTime datetime NOT NULL,
  ChannelNo int(11) NOT NULL,
  `Value` varchar(50) NOT NULL,
  RawValue int(11) NOT NULL,
  ReadingType int(11) NOT NULL,
  Miliseconds smallint(6) NOT NULL default 0,
  PRIMARY KEY  (DeviceID,ReadingTime, ChannelNo,ReadingType, Miliseconds)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;


/* Remove existing channel mapping so the new channels for Simple API and Full ISA100 API will be added*/

DELETE FROM DeviceChannels;
#DELETE FROM DeviceReadings;
DELETE FROM DeviceTypeChannels WHERE DeviceTypeID in (10, 11);

/* Remove existing devices, otherwise new channel mapping is not inserted */

DELETE FROM Devices WHERE DeviceTypeID in (10, 11);

/* Add RadioInterfaceType column with values 1 for Simple Interface, 2 for Full ISA100 Interface */

ALTER TABLE DeviceTypeChannels ADD COLUMN RadioInterfaceType TINYINT NOT NULL DEFAULT 1;
ALTER TABLE DeviceChannels ADD COLUMN RadioInterfaceType TINYINT NOT NULL DEFAULT 1;

/* Recreate the primary key to include the RadioInterfaceType column, allowing the user to use 1 based index for each Radio API channel set */
DROP INDEX IX_DeviceChannels ON `DeviceChannels`;
CREATE UNIQUE INDEX IX_DeviceChannels ON `DeviceChannels` (`DeviceID`,`ChannelNo`,`RadioInterfaceType`);

DROP INDEX IX_DeviceTypeChannels ON `DeviceTypeChannels`;
CREATE UNIQUE INDEX IX_DeviceTypeChannels ON `DeviceTypeChannels` (`DeviceTypeID`,`ChannelNo`,`RadioInterfaceType`);

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


DELETE FROM Properties WHERE `Key`='Version';
REPLACE INTO Properties(`Key`, Value) VALUES('SchemaVersion', '14.1.0');
REPLACE INTO Properties(`Key`, Value) VALUES('DataVersion', '14.1.0');

grant all privileges on *.* to 'nisa100'@'%' identified by 'demo' with grant option;