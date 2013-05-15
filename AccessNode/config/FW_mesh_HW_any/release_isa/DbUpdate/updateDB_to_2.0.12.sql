DELETE FROM Properties WHERE `Key`='Version';
REPLACE INTO Properties(`Key`, Value) VALUES('SchemaVersion', '2.0.12');
REPLACE INTO Properties(`Key`, Value) VALUES('DataVersion', '2.0.12');


CREATE TABLE ChannelsStatistics
(
     DeviceID                 INTEGER NOT NULL,
     ByteChannelsArray        VARCHAR(32)          /*16 channels per device => hex_string_len=32*/
);


CREATE TABLE AlertNotifications
(
	AlertNotifID		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	DeviceID 			INTEGER NOT NULL,
	TsapID		 		INTEGER NOT NULL,
	ObjID		 		INTEGER NOT NULL,
	
	AlertTime 			DATETIME NOT NULL,
	AlertTimeMsec		INTEGER NOT NULL,
	
	Class				INTEGER NOT NULL,			-- 0 ? Event  1 ? Alarm 
	Direction 			INTEGER,					-- 0 ? Alarm ended  1 ? Alarm began, just ignore if class is event
	Category			INTEGER NOT NULL, 			-- 0 ? process,1 ? device 2 ? network 3 ? security
 	Type				INTEGER NOT NULL,			-- Application-specific
 	Priority			INTEGER NOT NULL,			-- Application-specific
 	AlertData			VARCHAR(128)				-- string hex  Application-specific

);	
	
-- should have one row
CREATE TABLE AlertSubscriptionCategory
(
	CategoryProcess		INTEGER,			-- 1 - subscribe, 0 - unscribe 	
	CategoryDevice		INTEGER,			-- 1 - subscribe, 0 - unscribe 	
	CategoryNetwork		INTEGER,			-- 1 - subscribe, 0 - unscribe 	
	CategorySecurity	INTEGER				-- 1 - subscribe, 0 - unscribe 	
);	

CREATE TABLE AlertSubscriptionAlertSourceID
(
	DeviceID 			INTEGER NOT NULL,
	TsapID		 		INTEGER NOT NULL,
	ObjID		 		INTEGER NOT NULL,
	Type				INTEGER NOT NULL			-- Application-specific
);



/*alert*/
INSERT INTO AlertSubscriptionCategory VALUES (1,1,1,1);


