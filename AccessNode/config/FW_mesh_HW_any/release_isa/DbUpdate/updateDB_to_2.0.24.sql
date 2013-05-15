
DELETE FROM Properties WHERE `Key`='Version';
REPLACE INTO Properties(`Key`, Value) VALUES('SchemaVersion', '2.0.24');
REPLACE INTO Properties(`Key`, Value) VALUES('DataVersion', '2.0.24');


DROP TABLE IF EXISTS FirmwareDownloads;

CREATE TABLE FirmwareDownloads
(
    DeviceID                INTEGER,
    FwType                  INTEGER,            /* Managed by MCS : 10-Device,  0-Acquistion Board, 3-BBR */  
    FwStatus                INTEGER,            /* Set from MCS to:   1-InProgress,  2-CancelInProgress, 
      												  Updated by MH to:   3-Cancelled, 4-Completed, 5-Failed */
    CompletedPercent     	INTEGER NULL,       /* calculated & updated by MH */
    DownloadSize            INTEGER NULL,       /* calculated MH */
    Speed                   INTEGER NULL,       /* calculated & updated by MH in packets/min*/
    AvgSpeed                INTEGER NULL,       /* calculated & updated by MH in packets/min */
    StartedOn               DATETIME,           /* calculated by MCS when creating the record*/
    CompletedOn             DATETIME NULL,		/* calculated by MH */
    PRIMARY KEY (DeviceID, FwType, StartedOn)
);


