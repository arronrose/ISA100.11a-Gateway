ALTER TABLE DeviceLinks ADD CreatedTime DATETIME DEFAULT '2008-01-01' NOT NULL;
ALTER TABLE DeviceLinks ADD LinkStatus INTEGER DEFAULT -1 NOT NULL;
						
ALTER TABLE DeviceBatteryStatistics ADD BatteryLevel INTEGER DEFAULT 0 NOT NULL; 	

UPDATE Properties SET Value='1.3.10.1' WHERE Key='Version';