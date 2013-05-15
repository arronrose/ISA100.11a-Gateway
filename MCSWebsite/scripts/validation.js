function ValidateRequired(control, fieldName) {
    if (control.value == null || control.value.trim() == "") {
        alert("Field '" + fieldName + "' is required!");
        control.focus();        
        return false;
    }
    return true;
}


function ValidateRequiredRadio(control, fieldName) {
    for (var i=0; i < control.length; i++) {
        if (control[i].checked) {
            return true;
        }
    }
    alert("Field '" + fieldName + "' is required!");
    return false;
}


function ValidateEUI64(control, fieldName) {
    var regex = new RegExp("[0-9a-fA-F]{16}");
    var result = regex.exec(control.value);
    if (result == null) {
        alert("Field '" + fieldName + "' is invalid!");
        control.focus();
        return false;
    } else {
        return true;
    }
}

/*  validateAsIP    - must be true when the control's value will be validated as numeric IP ex: (10.16.0.26)
                    - otherwise must be false
    validateAsHost  - must be true when the control's value will be validated as host IP ex: (yahoo.com)
                    - otherwise must be false
*/
function ValidateIP(control, fieldName, validateAsIP, validateAsHost) {

    var inputString = control.value;
    var rxIP   = /^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$/
    var rxHost = /^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]*[a-zA-Z0-9])\.)*([A-Za-z]|[A-Za-z][A-Za-z0-9\-]*[A-Za-z0-9])$/
    var result = false;
    
    if (validateAsIP && rxIP.test(inputString)){
        result = true;
    };

    if (validateAsHost && rxHost.test(inputString)){
        result = true;        
    };

    if (!result) {
        alert("Field '" + fieldName + "' is invalid!");
        control.focus();
    }    
    return result;    
}


function Validate16BHex(control, fieldName) {
    var regex = new RegExp("[0-9a-fA-F]{32}");
    var result = regex.exec(control.value);
    if (result == null) {
        alert("Field '" + fieldName + "' is invalid!");
        control.focus();
        return false;
    } else {
        return true;
    }
}

function ValidateDinamicHex(control, fieldName, len) {	
    var regex = new RegExp("[0-9a-fA-F]{"+len+"}");
    var result = regex.exec(control.value);
    if (result == null) {
        alert("Field '" + fieldName + "' is invalid!");
        control.focus();
        return false;
    } else {
        return true;
    }
}
function ValidateHex(control, fieldName) {
    var regex = new RegExp("[0-9a-fA-F]+");
    var result = regex.exec(control.value);
    if (result != control.value) {
        alert("Field '" + fieldName + "' is invalid!");
        control.focus();
        return false;
    } else {
        return true;
    }
}


function ValidateIPv6(control, fieldName) {
    var regex = new RegExp("([0-9a-fA-F]{4}:){7}[0-9a-fA-F]{4}");
    var result = regex.exec(control.value);
    if (result == null) {
        alert("Field '" + fieldName + "' is invalid!");
        control.focus();
        return false;
    } else {
        return true;
    }
}


function ValidateRange(control, fieldName, startRange, endRange){
    var val = control.value;

    var regex = new RegExp("-?[0-9]+");
    var result = regex.exec(val);
    
    if (result == null || result != val || val < startRange || endRange < val ) {
        alert("Field '" + fieldName + "' must be in [" + startRange + ", " + endRange + "] !");
        control.focus();
        return false;
    } else {
        return true;
    }	
}


function ValidatePosUShort(control, fieldName) {
    var val = control.value;

    var regex = new RegExp("[0-9]{1,5}");
    var result = regex.exec(val);
    
    if (result == null || result != val || val < 1 || val > 65535) {
        alert("Field '" + fieldName + "' is invalid!");
        control.focus();
        return false;
    } else {
        return true;
    }
}


function Validate4DigitHex(control, fieldName) {
    var val = control.value;

    var regex = new RegExp("[0-9a-fA-F]{4}");
    var result = regex.exec(val);
    
    if (result == null) {
        alert("Field '" + fieldName + "' is invalid!");
        control.focus();
        return false;
    } else {
        return true;
    }
}

/*
Validates a device row.
Format is: <EUI64>,<Key>,<Subnet>
<MAC> 8 bytes grouped by 2, hex represented separated by semi columns
<Key> 16 bytes hex represented separated by spaces
<Subnet> integer [0-65535]

Example: 6202:0304:0506:FC00,C0 C1 C2 C3 C4 C5 C6 C7 C8 C9 CA CB CC CD CE CF,3,10

<EUI64> is 6202:0304:0506:FC00
<Key> is C0 C1 C2 C3 C4 C5 C6 C7 C8 C9 CA CB CC CD CE CF
<Subnet> is 3
<Role> is 10
*/
function ValidateDevice(control, fieldName) {
    var validExpresion = false;
    if(!ValidateRequired(control, fieldName)) {
        return false;
    }          
    
    var regex = new Array();
    if (fieldName == "Device"){
        regex[0] = new RegExp("([0-9a-fA-F]{4}:){3}[0-9a-fA-F]{4},([0-9a-fA-F]{2} ){15}[0-9a-fA-F]{2},[0-9]+(,[0-9]+)?");
        regex[1] = new RegExp("([0-9a-fA-F]{4}:){3}[0-9a-fA-F]{4},([0-9a-fA-F]{2}){15}[0-9a-fA-F]{2},[0-9]+(,[0-9]+)?"); 
        regex[2] = new RegExp("([0-9a-fA-F]{4}:){3}[0-9a-fA-F]{4},'([0-9a-fA-F]{2} ){15}[0-9a-fA-F]{2}',[0-9]+(,[0-9]+)?");   
        regex[3] = new RegExp("([0-9a-fA-F]{4}:){3}[0-9a-fA-F]{4},'([0-9a-fA-F]{2}){15}[0-9a-fA-F]{2}',[0-9]+(,[0-9]+)?"); 

    } else {
        regex[0] = new RegExp("([0-9a-fA-F]{4}:){3}[0-9a-fA-F]{4},([0-9a-fA-F]{2} ){15}[0-9a-fA-F]{2},[0-9]+");
        regex[1] = new RegExp("([0-9a-fA-F]{4}:){3}[0-9a-fA-F]{4},([0-9a-fA-F]{2}){15}[0-9a-fA-F]{2},[0-9]+"); 
        regex[2] = new RegExp("([0-9a-fA-F]{4}:){3}[0-9a-fA-F]{4},'([0-9a-fA-F]{2} ){15}[0-9a-fA-F]{2}',[0-9]+");   
        regex[3] = new RegExp("([0-9a-fA-F]{4}:){3}[0-9a-fA-F]{4},'([0-9a-fA-F]{2}){15}[0-9a-fA-F]{2}',[0-9]+"); 
    };
        
    for(i=0;i<regex.length;i++)
    {   var result = regex[i].exec(control.value);                
        if (result != null && result[0] == control.value){
            validExpresion = true;
            break; 
        };
    }
    if (fieldName == "Device"){
        var regexint = Array();
        regexint[0] = new RegExp("([0-9a-fA-F]{4}:){3}[0-9a-fA-F]{4}[ ]?-[ ]?([0-9a-fA-F]{4}:){3}[0-9a-fA-F]{4},([0-9a-fA-F]{2} ){15}[0-9a-fA-F]{2},[0-9]+(,[0-9]+)?");
        regexint[1] = new RegExp("([0-9a-fA-F]{4}:){3}[0-9a-fA-F]{4}[ ]?-[ ]?([0-9a-fA-F]{4}:){3}[0-9a-fA-F]{4},([0-9a-fA-F]{2}){15}[0-9a-fA-F]{2},[0-9]+(,[0-9]+)?");        
        regexint[2] = new RegExp("([0-9a-fA-F]{4}:){3}[0-9a-fA-F]{4}[ ]?-[ ]?([0-9a-fA-F]{4}:){3}[0-9a-fA-F]{4},'([0-9a-fA-F]{2} ){15}[0-9a-fA-F]{2}',[0-9]+(,[0-9]+)?");
        regexint[3] = new RegExp("([0-9a-fA-F]{4}:){3}[0-9a-fA-F]{4}[ ]?-[ ]?([0-9a-fA-F]{4}:){3}[0-9a-fA-F]{4},'([0-9a-fA-F]{2}){15}[0-9a-fA-F]{2}',[0-9]+(,[0-9]+)?");
                
        for(j=0;j<regexint.length;j++)
        {
            resultInterval = regexint[j].exec(control.value);
            if (regexint[j].exec(control.value) && resultInterval[0] == control.value) {
                validExpresion = true;
                break;
            }
        };                
    }
    
    if (validExpresion == false){
        alert("Field " + fieldName + " is invalid!");
        control.focus();
        return false;
    };    

    var devComponents = control.value.split(",");
    var subnet = devComponents[2] * 1;
    var role = devComponents[3] * 1;
    
    if (subnet < 0 || subnet > 65535) {
        alert("SubnetID must be between 0 and 65535!");
        control.focus();
        return false;
    };  
    if (role < 0 || role > 65535) {
        alert("Role must be between 0 and 65535!");
        control.focus();
        return false;
    };
    return true;
}

function ValidateLoggingLevel(control, fieldName) {
     var val = control.value;

    var regex = new RegExp("DEBUG|INFO|WARN|ERROR");
    var result = regex.exec(val);
    
    if (result == null || result != val) {
        alert("Field '" + fieldName + "' is invalid!");
        control.focus();
        return false;
    } else {
        return true;
    }
}


function ValidateNumber(control, fieldName, minValue, maxValue) {
    var val = control.value;

    var regex = new RegExp("[0-9]+");
    var result = regex.exec(val);
    
    if (result == null || result != val || val < minValue || val > maxValue) {
        alert("Field '" + fieldName + "' is invalid!");  // Must be an integer value between " + minValue + " and " + maxValue + ".");
        control.focus();
        return false;
    }
    
    return true;
}

function ValidateNumberSet(control, fieldName, valuesList) {
    var val = control.value;
    var regex = new RegExp("[0-9]+");
    var result = regex.exec(val);    
       
    if (result == null || result != val) {
        alert("Field '" + fieldName + "' is invalid!");  // Must be an integer value between " + minValue + " and " + maxValue + ".");
        control.focus();
        return false;
    };
    var arrNumber =  valuesList.split(',');       
    var isValidValue = false;
    for (i=0; i<arrNumber.length ; i++){   
        if (arrNumber[i] == val) {
           isValidValue = true;
        };
    };   
    if (!isValidValue){
        alert("Field '" + fieldName + "' is invalid!");  // Must be an integer value between " + minValue + " and " + maxValue + ".");
        control.focus();
        return false;   
    };
    return true;
}

/* A row format is: <EUI64>, <CO_TSAP_ID>, <CO_ID>, <Data_Period>, <Data_Phase>, <Data_StaleLimit>, <Data_ContentVersion>, <interfaceType> */
function ValidatePublisher(control, fieldName) {
    if(!ValidateRequired(control, fieldName)) {
        return false;
    }
    
    var val = control.value;
    var regex = new RegExp("([0-9a-fA-F]{4}:){3}[0-9a-fA-F]{4}(,[ ]?[0-9]+)(,[ ]?-?[0-9]+)(,[ ]?-?[0-9]+)(,[ ]?[0-9]+)(,[ ]?[0-9]+)(,[ ]?[0-9]+)(,[ ]?[1-2]+)?");
    var result = regex.exec(val);
    if (result == null || result[0] != control.value) {
        alert("Field '" + fieldName + "' is invalid!");
        control.focus();
        return false;
    }
    var devComponents = val.split(",");
    
    var tsapid = devComponents[1] * 1;
    if (tsapid < 1 || tsapid > 15) {
        alert("CO_TSAP_ID is invalid!");
        control.focus();
        return false;
    }
    var dataphase = devComponents[2] * 1;
    if (dataphase < 0 || dataphase > 65535) {
        alert("CO_ID is invalid!");
        control.focus();
        return false;
    }
    var dataphase = devComponents[3] * 1;
    if (dataphase < -4 || dataphase > 32767) {
        alert("Data_Period is invalid!");
        control.focus();
        return false;
    }
    var dataphase = devComponents[4] * 1;
    if (dataphase < 0 || dataphase > 100) {
        alert("Data_Phase is invalid!");
        control.focus();
        return false;
    }
    var datastalelimit = devComponents[5] * 1;
    if (datastalelimit < 0 || datastalelimit > 255) {
        alert("Data_StaleLimit is invalid!");
        control.focus();
        return false;
    }
    var datacontentversion = devComponents[6] * 1;
    if (datacontentversion < 0 || datacontentversion > 255) {
        alert("Data_ContentVersion is invalid!");
        control.focus();
        return false;
    }
    return true;
}

//channel format <TSAP_ID>[0-15], <ObjID>[0-65535], <AttrID>[0-4095], <Index1>[0-32767], <Index2>[0-32767], <format>, <name>, <unit_of_measurement>, <withstatus> 
function ValidateChannel(control, fieldName) {
    if(!ValidateRequired(control, fieldName)) {
        return false;
    }
    
    var val = control.value;
    var regex = new RegExp("[0-9]+(,[ ]*[0-9]+)(,[ ]*[0-9]+)(,[ ]*[0-9]+)(,[ ]*'(int8|uint8|int16|uint16|int32|uint32|float)')(,[ ]*'[^\f\n\r\t\v]*'){2}(,[ ]*[-]?[0-9]*)?");   
    var result = regex.exec(val);
    if (result == null || result[0] != control.value) {
        alert("Field '" + fieldName + "' is invalid!");
        control.focus();
        return false;
    }
    
    var channelComponents = val.split(",");
/*    
    var tsapid = channelComponents[0] * 1;
    if (tsapid < 1 || tsapid > 15) {
        alert("TSAP_ID is invalid!");
        control.focus();
        return false;
    }
*/
    var objid = channelComponents[1] * 1;
    if (objid < 0 || objid > 65535) {
        alert("ObjID is invalid!");
        control.focus();
        return false;
    }
    var attrid = channelComponents[2] * 1;
    if (attrid < 0 || attrid > 4095) {
        alert("AttrID is invalid!");
        control.focus();
        return false;
    }
    var idxid1 = channelComponents[3] * 1;
    if (idxid1 < 0 || idxid1 > 32767) {
        alert("Index1 is invalid!");
        control.focus();
        return false;
    }
    return true;
}

//pref: Start, End - to do maybe, params are control names
function ValidateDateTime(txtDate, txtHour, txtMinute) {
    var date = document.getElementById(txtDate).value;
    if (date == "") {
        return true;   //empty date is accepted
    }
 
    //use the calendar control validator (throws alerts)
    var dateOk = f_tcalParseDate(date);
    if (dateOk == undefined) {
        document.getElementById(txtDate).focus();
        return false;
    }
    
    var hour = document.getElementById(txtHour).value;
    if (hour != "") {
        if (isNaN(hour) || hour < 1 || hour > 12) {   
            alert("Invalid hour: '" + hour +"'.");
            document.getElementById(txtHour).focus();
            return false;
        }
    }
    
    var minute = document.getElementById(txtMinute).value;
    if (minute != "") {
        if (isNaN(minute) || minute < 0 || minute > 59) {   
            alert("Invalid minute: '" + minute +"'.");
            document.getElementById(txtMinute).focus();
            return false;
        }
    }
    
    return true;
}

//format: <host_value>=<UnitId>,<EUI64>,<map_type>
function ValidateHost(control, fieldName) {
    if(!ValidateRequired(control, fieldName)) {
        return false;
    }
    
    var val = control.value;
    var regex = new RegExp("[0-9]+,[0-9a-fA-F]{16},[0-9]+");
    var result = regex.exec(val);
    
    if (result == null || result[0] != control.value) {
        alert("Field '" + fieldName + "' is invalid!");
        control.focus();
        return false;
    }
    return true;
}


//<register_value>=<start_address>,<word_count>,<TSAPID>,<ObjId>,<AttrId>,<Idx1>,<Idx2>,<MethId>[,<status_byte>]
function ValidateRegister(control, fieldName) {
    if(!ValidateRequired(control, fieldName)) {
        return false;
    }
    
    var val = control.value;
    var regex = new RegExp("([0-9]+,){2}[0-9a-fA-F]{16},([0-9]+,){5}[0-9]+(,yes|,no)?");
    var regex = new RegExp("([0-9]+,){2}");
    var result = regex.exec(val);
    
    if (result == null || result[0] != control.value) {
        alert("Field '" + fieldName + "' is invalid!");
        control.focus();
        return false;
    }
    var registerComps = val.split(",");
    var tsapid = registerComps[2] * 1;
    if (tsapid < 1 || tsapid > 15) {
        alert("TSAP_ID is invalid!");
        control.focus();
        return false;
    }
       
    return true;
}

//<register_value>=<start_address>,<word_count>,<EUI64>,<TSAPID>,<ObjId>,<AttrId>,<Idx1>,<Idx2>,<MethId>[, <status_byte>]
function ValidateRegisterServer(control, fieldName) {
    if(!ValidateRequired(control, fieldName)) {
        return false;
    }
    
    var val = control.value;
    var regex = new RegExp("([0-9]+,){2}([0-9a-fA-F]+,){1}([0-9]+,){5}([0-9]+){1}(,[0-2])?");
    var result = regex.exec(val);
    
    if (result == null || result[0] != control.value) {
        alert("Field '" + fieldName + "' is invalid!");
        control.focus();
        return false;
    }
    var registerComps = val.split(",");
    
    var tsapid = registerComps[3] * 1;
    if (tsapid < 1 || tsapid > 15) {
        alert("TSAP_ID is invalid!");
        control.focus();
        return false;
    }
       
    return true;
}
