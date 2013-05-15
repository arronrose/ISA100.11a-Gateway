var methods = ["config.getGroupVariables",
               "isa100.mh.setPublisher",
               "isa100.mh.delPublisher",
               "misc.applyConfigChanges",
               "config.getConfig",
               "user.logout",
               "user.isValidSession"];

var CONTENT = null;

var helpPopup = new Popup();
    helpPopup.autoHide = false;
    helpPopup.offsetLeft = 10;
    helpPopup.constrainToScreen = true;
    helpPopup.position = 'above adjacent-right';
    helpPopup.style = {'border':'2px solid black','backgroundColor':'#EEE'};

var contentPublisher =                
        '<b>Publisher Format:</b> <i>{' + 
		'&lt;EUI64&gt;, &lt;CO_TSAP_ID&gt;, &lt;CO_ID&gt;, &lt;DATA_PERIOD&gt;,<br />' +
		'&lt;DATA_PHASE&gt;, &lt;DATA_STALELIMIT&gt;, &lt;DATA_CONTENTVERSION&gt;,<br />' +
		'&lt;INTERFACETYPE&gt;}</i><br />'+
		'&nbsp;&nbsp;&nbsp;&nbsp;<b>EUI64:</b> 8 bytes grouped by 2, hex represented separated by colons<br />' +
		'&nbsp;&nbsp;&nbsp;&nbsp;<b>CO_TSAP_ID:</b> the TSAP ID of the publishing device concentrator object,<br />' +
		'&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;integer, [1-15]<br />' +
		'&nbsp;&nbsp;&nbsp;&nbsp;<b>CO_ID:</b> Concentrator object ID of the publishing device, integer, [0-65535]<br />' + 
		'&nbsp;&nbsp;&nbsp;&nbsp;<b>DATA_PERIOD:</b> Subscription data period, integer, [-4, 32767];<br />' +
		'&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;if positive, represents seconds. If negative, represents fractions of a second:<br />' +
		'&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;1/(-&lt;DATA_PERIOD&gt;).<br />' + 
        '&nbsp;&nbsp;&nbsp;&nbsp;<b>DATA_PHASE:</b> Subscription data phase, integer, [0-100], percent of the period<br />' + 
        '&nbsp;&nbsp;&nbsp;&nbsp;<b>DATA_STALELIMIT:</b> Subscription stale limit, seconds, [0-255]<br />' + 
        '&nbsp;&nbsp;&nbsp;&nbsp;<b>DATA_CONTENTVERSION:</b> Content version from publish packets, integer, [0-255]<br />' + 
        '&nbsp;&nbsp;&nbsp;&nbsp;<b>INTERFACETYPE:</b> 1 - Full ISA, 2 - Simple ISA<br />' +                 
        '<br />';
var contentChannel =
        '<b>Channel Format:</b> <i>{' +   
        '&lt;TSAP_ID&gt;, &lt;OBJID&gt;, &lt;ATTRID&gt;, &lt;INDEX1&gt;, &lt;INDEX2&gt;,<br />' +
		'&lt;FORMAT&gt;, &lt;NAME&gt;, &lt;UNIT_OF_MEASUREMENT&gt;, &lt;WITHSTATUS&gt;}</i><br />' +
        '&nbsp;&nbsp;&nbsp;&nbsp;<b>TSAP_ID:</b> the TSAP ID of the publishing device channel <br />' +
		'&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(the actual value published),integer, [1-15]<br />' +
        '&nbsp;&nbsp;&nbsp;&nbsp;<b>OBJID:</b> Object ID of the publishing device channel (the actual value published), <br />' +
		'&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;integer, [0-65535]<br />' +
        '&nbsp;&nbsp;&nbsp;&nbsp;<b>ATTRID:</b> Attribute ID, integer, [0-4095]<br />' +
        '&nbsp;&nbsp;&nbsp;&nbsp;<b>INDEX1:</b> Index 1, integer, [0-32767]<br />' +
        '&nbsp;&nbsp;&nbsp;&nbsp;<b>INDEX2:</b> Index 2, integer, [0-32767]<br />' +
        '&nbsp;&nbsp;&nbsp;&nbsp;<b>FORMAT:</b> Data format. Accepted values: int8, uint8, int16, uint16, int32, uint32,<br />' +
		 '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;float<br />' +
        '&nbsp;&nbsp;&nbsp;&nbsp;<b>NAME:</b> Data name, string<br />' +
        '&nbsp;&nbsp;&nbsp;&nbsp;<b>UNIT_OF_MEASUREMENT:</b> Unit, string<br />' +
        '&nbsp;&nbsp;&nbsp;&nbsp;<b>WITHSTATUS:</b> 0-no Status, !=0 with_Status<br />';               
        '<br />';
var closeLink = 
        '&nbsp;&nbsp;<a href="#" onclick="helpPopup.hide();return false;"><b>Close</b></a>';


function InitMHConfigPage() {
    SetPageCommonElements();
    InitJSON();

    ReadConfigFromDisk();    
    ClearChannels();
    
    PopulatePublisherList();
}

function ReadConfigFromDisk() {
    try {    
        var service = new jsonrpc.ServiceProxy(serviceURL, methods);

        CONTENT = service.config.getConfig({configFile: FILE_MH_PUBLISHERS_CONF});
    } catch(e) {
        HandleException(e, "Error reading " + FILE_MH_PUBLISHERS_CONF + " file!");
    }
}
 
function DeletePublisher() {
    ClearOperationResults();

    var txtPublisher = document.getElementById("txtPublisher");
    if (confirm("Are you sure you want to delete the Publisher?")){      
        if (ValidatePublisher(txtPublisher, "Publisher")) {
            var eui64 = txtPublisher.value.split(",")[0];
            
            try {
                var service = new jsonrpc.ServiceProxy(serviceURL, methods);
                var response = service.isa100.mh.delPublisher({eui64 : eui64});
                    
                if(response) {
                    ReadConfigFromDisk();
                    
                    ClearChannels();
                    PopulatePublisherList();
                    DisplayOperationResult("spnOperationResultPublisher", "Publisher deleted successfully.");
                };
            } catch(e) {
                HandleException(e, "Publisher not deleted or not found !");
                return;
            };
        };
    };
}


function EditPublisher() {
    ClearOperationResults();    
    ClearChannels();
    
    var txtPublisher = document.getElementById("txtPublisher");
    txtPublisher.value = document.getElementById("lstPublishers").value;
    var group = txtPublisher.value.split(",");
    document.getElementById("txtChannelId").value = group[1].trim() + ",";
    
    //find the publisher and get the channels
    for(var i = 0; i < CONTENT.length; i++) {
        if (CONTENT[i].group == group[0].trim()) {
            var lstChannels = document.getElementById("lstChannels");
            for (var j = 0; j < CONTENT[i].variables.length - 1; j++) {
                if(CONTENT[i].variables[j+1].CHANNEL != null) {
                    var op = new Option(CONTENT[i].variables[j+1].CHANNEL, CONTENT[i].variables[j+1].CHANNEL);
                    if ( j % 2 == 1) {
                        op.className = "listAlternateItem";
                    }
                    lstChannels[lstChannels.length] = op;
                }
            }
            break;
        }
    }
}


function EditChannel() {
    ClearOperationResults();
    var lstChannels = document.getElementById("lstChannels");
    var txtChannel = document.getElementById("txtChannel");
    var txtChannelId = document.getElementById("txtChannelId");
    txtChannel.value = formatCSVString(lstChannels.value, 1);
}

function SavePublisher() {
    ClearOperationResults();

    var txtPublisher = document.getElementById("txtPublisher");
    var lstChannels = document.getElementById("lstChannels");
    
    if (ValidatePublisher(txtPublisher, "Publisher")) {
        var eui64 = txtPublisher.value.split(",")[0];
        var concentrator = txtPublisher.value.replace(eui64 + ",", "");

        var channels = Array();
        
        for (i = 0; i < lstChannels.length; i++) {
            channels.push(lstChannels[i].value);
        }
                
        try {
            var service = new jsonrpc.ServiceProxy(serviceURL, methods);
            var response = service.isa100.mh.setPublisher({eui64 : eui64, concentrator: concentrator, channels: channels});
            if(response) {
                ReadConfigFromDisk();
                ClearChannels();
                
                PopulatePublisherList();
                DisplayOperationResult("spnOperationResultPublisher", "Save completed successfully.");
            } else {
                alert ("Error saving Publisher !");
            }
        } catch(e) {
            HandleException(e, "Unexpected error saving Publisher !");
            return;
        }
    }
}


function PopulatePublisherList() {
    ClearPublishers();
    
    var lst = document.getElementById("lstPublishers");
    if (CONTENT != null) {
        for (i = 0; i < CONTENT.length; i++) {   
            if(CONTENT[i].group != null) {
                var op = new Option(CONTENT[i].group + ", " + CONTENT[i].variables[0].CONCENTRATOR, CONTENT[i].group + ", " + CONTENT[i].variables[0].CONCENTRATOR);
                if ( i % 2 == 1) {
                    op.className = "listAlternateItem";
                }
                lst[lst.length] = op;
            }
        }
    }
}


function ClearPublishers() {
    ClearList("lstPublishers");
    document.getElementById("txtPublisher").value = "";
}


function ClearChannels() {
   ClearList("lstChannels");
   document.getElementById("txtChannelId").value = "";    
   document.getElementById("txtChannel").value = "";    
}


function ChangeChannel() {
    ClearOperationResults();
    
    var lstChannels = document.getElementById("lstChannels");
    var txtChannelId = document.getElementById("txtChannelId");
    var txtChannel = document.getElementById("txtChannel");
    
    if(ValidateChannel(txtChannel, "Channel")) {
        var lstKey = new Array(lstChannels.length);
        for (var i=0; i<lstChannels.length; i++) {
			lstKey[i] = formatCSVString(lstChannels[i].value, 0, 2);
        }
        var valKey = formatCSVString(txtChannelId.value + txtChannel.value, 0, 2); 
        
        //check wether key exists already
        var index = IndexInArray(lstKey, valKey);
		var txtChannelValue = formatCSVString(txtChannelId.value + txtChannel.value);
        if (index == -1) {
            lstChannels[lstChannels.length] = new Option(txtChannelValue, txtChannelValue);
        } else {
            lstChannels[index] = new Option(txtChannelValue, txtChannelValue);
        }
        txtChannel.value = "";
    }
}


function DeleteChannel() {
    ClearOperationResults();
    
    var txtChannel = document.getElementById("txtChannel");
    var lstChannels = document.getElementById("lstChannels");
	for (var i = lstChannels.selectedIndex; i< lstChannels.length-1; i++) {
	    lstChannels[i] = new Option(lstChannels[i+1].value, lstChannels[i+1].value);
	}
	lstChannels[lstChannels.length-1] = null;
	txtChannel.value = "";
}


function Activate() {
    ClearOperationResults();
    
    try {
        var service = new jsonrpc.ServiceProxy(serviceURL, methods);
        var response = service.misc.applyConfigChanges({module: "MonitorHost"});
        if(response) {
            DisplayOperationResult("spnOperationResultActivate", "Activate completed successfully.");
        } else {
            alert ("Error activating Publisher list !");
        }
    } catch(e) {
        HandleException(e, "Unexpected error activating Publisher list !");
        return;
    }
}


function Download() {
    ClearOperationResults();
    try {    
        var service = new jsonrpc.ServiceProxy(serviceURL, methods);
        service.user.isValidSession();        
	    document.form1.call1.value = jsonrpc.JSONRPCMethod("a").jsonRequest(1, "misc.fileDownload", {file: FILE_MH_PUBLISHERS_CONF});
        return 1 ;
    } catch (e) {
        HandleException(e, 'Unexpected error downloading file !')
    };        
}


function Upload() {
    ClearOperationResults();
    var file = document.getElementById("mhFile");
    if (getFileName(file.value) != FILE_MH_PUBLISHERS_CONF)  {
        alert('The name of the uploaded file must be ' + FILE_MH_PUBLISHERS_CONF);
        return false; 
    } else {   
        if (confirm("The previous publisher list will be lost. Are you sure you want to replace the file?")){
            try {    
                var service = new jsonrpc.ServiceProxy(serviceURL, methods);
                service.user.isValidSession();        
                document.form1.call1.value = jsonrpc.JSONRPCMethod("a").jsonRequest(1,"misc.fileUpload", {file: FILE_MH_PUBLISHERS_CONF});
                return 1 ;
            } catch (e) {
                HandleException(e, 'Unexpected error uploading file !')
            };    
        } else {
            return false;
        };
    };    
}


function ClearOperationResults() {
    ClearOperationResult("spnOperationResultPublisher");
    ClearOperationResult("spnOperationResultActivate");
}


function ShowHideHelp(op) {
    var divHelp = document.getElementById("divHelp");
    
    if (op == "open") {
        divHelp.style.display = "";
    } else {
        divHelp.style.display = "none";
    }
}


function operationDoneListener(text) {
    if (text.error) {
        alert("Error downloading " + FILE_MH_PUBLISHERS_CONF + " file !");
	}
}

function formatCSVString(csvString, beginIndex, endIndex) {
    if (csvString == null)
       return null;
    var csvArray = csvString.split(",");
    beginIndex = (beginIndex != null && beginIndex <= csvArray.length) ? beginIndex : 0;
    endIndex = (endIndex != null && endIndex < csvArray.length) ? endIndex : csvArray.length-1;
    var csvResult = "";
    for (var i = beginIndex; i <= endIndex; i++){
       csvResult += csvArray[i].trim() + ((i < endIndex) ? ", " : "");
    }
    return csvResult;
}
