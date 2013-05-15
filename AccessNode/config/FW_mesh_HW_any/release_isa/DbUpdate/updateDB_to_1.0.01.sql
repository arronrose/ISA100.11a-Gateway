DELETE FROM Properties WHERE `Key`='Version';
REPLACE INTO Properties(`Key`, Value) VALUES('SchemaVersion', '1.0.01');
REPLACE INTO Properties(`Key`, Value) VALUES('DataVersion', '1.0.01');


ALTER TABLE Readings DROP COLUMN Status;
ALTER TABLE Readings ADD COLUMN Status INTEGER;


