--update DeviceTypeChannels
UPDATE DeviceTypeChannels SET MinValue=4 WHERE ChannelNo=4;
UPDATE DeviceTypeChannels SET ChannelName='Analog Input RTD (Celsius)/Pressure (PSI)', MinValue=-100, MaxValue=300 WHERE ChannelNo=6;
UPDATE DeviceTypeChannels SET MaxValue=7.5 WHERE ChannelNo=7;
--update DeviceChannels
UPDATE DeviceChannels SET MinValue=4 WHERE ChannelNo=4;
UPDATE DeviceChannels SET ChannelName='Analog Input RTD(Celsius)/Pressure(PSI)', MinValue=-100, MaxValue=300 WHERE ChannelNo=6;
UPDATE DeviceChannels SET MaxValue=7.5 WHERE ChannelNo=7;