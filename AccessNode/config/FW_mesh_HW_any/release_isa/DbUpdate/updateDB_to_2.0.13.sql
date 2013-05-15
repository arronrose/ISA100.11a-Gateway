DELETE FROM Properties WHERE `Key`='Version';
REPLACE INTO Properties(`Key`, Value) VALUES('SchemaVersion', '2.0.13');
REPLACE INTO Properties(`Key`, Value) VALUES('DataVersion', '2.0.13');

ALTER TABLE DeviceScheduleLinks ADD COLUMN LinkPeriod INTEGER;



