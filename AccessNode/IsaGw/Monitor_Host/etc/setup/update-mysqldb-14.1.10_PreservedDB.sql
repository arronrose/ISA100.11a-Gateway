USE Monitor_Host;

DROP TABLE IF EXISTS RadioInterfaceTypes;

CREATE TABLE RadioInterfaceTypes (
  `ID` tinyint NOT NULL,
  `Name` varchar(64) NOT NULL,
  PRIMARY KEY  (`ID`)
);

INSERT INTO RadioInterfaceTypes (`ID`, `Name`)
VALUES (1, 'Simple Interface');

INSERT INTO RadioInterfaceTypes (`ID`, `Name`)
VALUES (2, 'Full ISA100 Interface');

/* new column for deviceReadings */
#DELETE FROM DeviceReadings;
ALTER TABLE DeviceReadings ADD COLUMN ValueStatus int(8) NULL;


DELETE FROM Properties WHERE `Key`='Version';
REPLACE INTO Properties(`Key`, Value) VALUES('SchemaVersion', '14.1.10');
REPLACE INTO Properties(`Key`, Value) VALUES('DataVersion', '14.1.10');

