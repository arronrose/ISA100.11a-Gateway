

DELETE FROM Properties WHERE `Key`='Version';
REPLACE INTO Properties(`Key`, Value) VALUES('SchemaVersion', '2.0.50');
REPLACE INTO Properties(`Key`, Value) VALUES('DataVersion', '2.0.50');


CREATE TABLE Dashboard(
SlotNumber       integer,
DeviceID         integer,
ChannelNo        integer,
GaugeType        integer,
MinValue         real,
MaxValue         real);

