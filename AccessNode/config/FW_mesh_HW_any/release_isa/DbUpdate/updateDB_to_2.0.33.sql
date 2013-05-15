
DELETE FROM Properties WHERE `Key`='Version';
REPLACE INTO Properties(`Key`, Value) VALUES('SchemaVersion', '2.0.33');
REPLACE INTO Properties(`Key`, Value) VALUES('DataVersion', '2.0.33');


UPDATE Users SET Password = 'lm4E2SVQDmw17Y+oAo8z+jvbncs=' WHERE UserName = 'admin';

