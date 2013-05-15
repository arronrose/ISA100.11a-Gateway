USE Monitor_Host;

/* create new index for DeviceReadings */
DELIMITER $$ 

DROP PROCEDURE IF EXISTS `sp_DropIndex` $$ 
CREATE PROCEDURE `sp_DropIndex` (tblSchema VARCHAR(64),tblName VARCHAR(64),ndxName VARCHAR(64)) 
BEGIN 

    DECLARE IndexColumnCount INT; 
    
    SELECT COUNT(1) INTO IndexColumnCount 
    FROM information_schema.statistics 
    WHERE table_schema = tblSchema 
    AND table_name = tblName 
    AND index_name = ndxName; 

    IF IndexColumnCount > 0 THEN 
	DROP INDEX `IX_DeviceReadings` on `DeviceReadings`;
    END IF; 

END $$ 

DELIMITER ; 

CALL sp_DropIndex('Monitor_Host', 'DeviceReadings ', 'IX_DeviceReadings');

CREATE INDEX IX_DeviceReadings USING BTREE ON DeviceReadings(DeviceID, ChannelNo, ReadingTime);

DROP PROCEDURE IF EXISTS `sp_DropIndex`;



/* Remove existing channel mapping so the new channels for Simple API and Full ISA100 API will be added*/

DELETE FROM DeviceChannels;
DELETE FROM DeviceTypeChannels WHERE DeviceTypeID in (10, 11);

DELETE FROM Devices;
ALTER TABLE Devices ADD COLUMN SubnetID INT NULL;

INSERT INTO Devices(DeviceTypeID, Address128, Address64, DeviceStatus, DeviceLevel)
  VALUES(2, '::', '600D:BEEF:600D:BEEF', 0, -1);
 

/*Field Router */
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,1,'UAP_DATA_ANALOG1',0,0,0,0,6,2,129,1,0,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,2,'UAP_DATA_ANALOG2',0,0,0,0,6,2,129,2,0,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,3,'UAP_DATA_ANALOG3',0,0,0,0,6,2,129,3,0,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,4,'UAP_DATA_ANALOG4',0,0,0,0,6,2,129,4,0,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,5,'UAP_DATA_INPUT_TEMP',0,0,0,0,6,2,129,5,0,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,6,'UAP_DATA_INPUT_HUMIDITY',0,0,0,0,6,2,129,6,0,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,7,'UAP_DATA_INPUT_DEWPOINT',0,0,0,0,6,2,129,7,0,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,8,'UAP_DATA_INPUT_BATTERY',0,0,0,0,6,2,129,8,0,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,9,'UAP_DATA_DIGITAL1',0,0,0,0,0,2,129,9,1,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,10,'UAP_DATA_DIGITAL2',0,0,0,0,0,2,129,10,1,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,11,'UAP_DATA_DIGITAL3',0,0,0,0,0,2,129,11,1,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,12,'UAP_DATA_DIGITAL4',0,0,0,0,0,2,129,12,1,0,0,'N/A', 1);
/* Full ISA Radio Interface */
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,1,'DATA_ANALOG_1',0,0,0,0,6,2,3,1,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,2,'DATA_ANALOG_2',0,0,0,0,6,2,3,2,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,3,'DATA_ANALOG_3',0,0,0,0,6,2,3,3,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,4,'DATA_ANALOG_4',0,0,0,0,6,2,3,4,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,5,'DATA_DIGITAL1',0,0,0,0,6,2,3,16,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,6,'DATA_DIGITAL2',0,0,0,0,0,2,3,17,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,7,'DATA_DIGITAL3',0,0,0,0,0,2,3,18,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,8,'DATA_DIGITAL4',0,0,0,0,0,2,3,19,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,9,'DATA_WRITE_1',0,0,0,0,6,2,3,31,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,10,'DATA_WRITE_2',0,0,0,0,6,2,3,32,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,11,'DATA_WRITE_3',0,0,0,0,6,2,3,33,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (10,12,'DATA_WRITE_4',0,0,0,0,6,2,3,34,0,0,1,'N/A', 2);

/*Non-routing*/
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,1,'UAP_DATA_ANALOG1',0,0,0,0,6,2,129,1,0,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,2,'UAP_DATA_ANALOG2',0,0,0,0,6,2,129,2,0,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,3,'UAP_DATA_ANALOG3',0,0,0,0,6,2,129,3,0,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,4,'UAP_DATA_ANALOG4',0,0,0,0,6,2,129,4,0,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,5,'UAP_DATA_INPUT_TEMP',0,0,0,0,6,2,129,5,0,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,6,'UAP_DATA_INPUT_HUMIDITY',0,0,0,0,6,2,129,6,0,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,7,'UAP_DATA_INPUT_DEWPOINT',0,0,0,0,6,2,129,7,0,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,8,'UAP_DATA_INPUT_BATTERY',0,0,0,0,6,2,129,8,0,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,9,'UAP_DATA_DIGITAL1',0,0,0,0,0,2,129,9,1,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,10,'UAP_DATA_DIGITAL2',0,0,0,0,0,2,129,10,1,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,11,'UAP_DATA_DIGITAL3',0,0,0,0,0,2,129,11,1,0,0,'N/A', 1);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,12,'UAP_DATA_DIGITAL4',0,0,0,0,0,2,129,12,1,0,0,'N/A', 1);
/* Full ISA100 Radio Interface */
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,1,'DATA_ANALOG_1',0,0,0,0,6,2,3,1,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,2,'DATA_ANALOG_2',0,0,0,0,6,2,3,2,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,3,'DATA_ANALOG_3',0,0,0,0,6,2,3,3,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,4,'DATA_ANALOG_4',0,0,0,0,6,2,3,4,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,5,'DATA_DIGITAL1',0,0,0,0,0,2,3,16,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,6,'DATA_DIGITAL2',0,0,0,0,0,2,3,17,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,7,'DATA_DIGITAL3',0,0,0,0,0,2,3,18,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,8,'DATA_DIGITAL4',0,0,0,0,0,2,3,19,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,9,'DATA_WRITE_1',0,0,0,0,6,2,3,31,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,10,'DATA_WRITE_2',0,0,0,0,6,2,3,32,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,11,'DATA_WRITE_3',0,0,0,0,6,2,3,33,0,0,1,'N/A', 2);
INSERT INTO DeviceTypeChannels (`DeviceTypeID`,`ChannelNo`,`ChannelName`,`MinValue`,`MaxValue`,`MinRawValue`,`MaxRawValue`,`ChannelData`,`MappedTSAPID`,`MappedObjectID`,`MappedAttributeID`,`ChannelType`,`IsPredefined`,`Enabled`,`UnitOfMeasurement`, `RadioInterfaceType`)
VALUES (11,12,'DATA_WRITE_4',0,0,0,0,6,2,3,34,0,0,1,'N/A', 2);


DELETE FROM Properties WHERE `Key`='Version';
REPLACE INTO Properties(`Key`, Value) VALUES('SchemaVersion', '14.1.09');
REPLACE INTO Properties(`Key`, Value) VALUES('DataVersion', '14.1.09');

