USE Monitor_Host;


DELETE FROM Devices WHERE DeviceTypeID in (1, 2, 3);
INSERT INTO Devices(DeviceTypeID, Address128, Address64, DeviceStatus, DeviceLevel)
  VALUES(2, '::', '600D:BEEF:600D:BEEF', 0, -1);


DROP TABLE Commands;
CREATE TABLE Commands
(
	CommandID INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,
	DeviceID INTEGER NOT NULL,
	CommandCode INTEGER NOT NULL,
	CommandStatus INTEGER NOT NULL,
	TimePosted DATETIME NOT NULL,
	TimeResponsed DATETIME,
	ErrorCode INTEGER NOT NULL DEFAULT 0,
	ErrorReason VARCHAR(256),
	Response VARCHAR(256),
	Generated INTEGER DEFAULT 0 NOT NULL
);


DELETE FROM Properties WHERE `Key`='Version';
REPLACE INTO Properties(`Key`, Value) VALUES('SchemaVersion', '14.1.1');
REPLACE INTO Properties(`Key`, Value) VALUES('DataVersion', '14.1.1');

grant all privileges on *.* to 'nisa100'@'127.0.0.1' identified by 'demo' with grant option;