
function InitCustomSettings() {
    SetPageCommonElements();
    InitJSON();
    GetTheme();
    document.getElementById("btnAdd").disabled = false;        
    document.getElementById("spnMaxLogoSize").innerHTML = "Maximum logo size (Width x Height): " + MAX_DEVICE_LOGO_WIDTH + "x" + MAX_DEVICE_LOGO_HEIGHT + " pixels.";
    document.getElementById("spnMaxLogoFileSize").innerHTML = "Maximum logo file size: 10 Kb.";
}

function Add() {
    var logoFile = document.getElementById("logoFile").value;
    if (logoFile == null || logoFile == "") {
        alert("Please select an logo file!");
        return;
    }    
    var params = getFileName(logoFile); 
    document.form1.call1.value = jsonrpc.JSONRPCMethod("a").jsonRequest(1, "misc.fileUpload", {script: "/access_node/firmware/www/uploadLogo.sh", scriptParams: params});
    return 1;    
}

function operationDoneListener( text) {
	var spnOperationResultActivate = document.getElementById("spnOperationResultAdd");
	if (text.result) {
		DisplayOperationResult("spnOperationResultAdd", "Logo successfully added.");
    } else {    
        DisplayOperationResult("spnOperationResultAdd", "Add logo failed: "+ text.result);
	};	
}

function GetTheme(){
	var rbThemes = document.getElementsByName("rbThemes")
	var theme = GetMcsTheme();
	rbThemes[theme].checked = true;			
}

function SetTheme(){
	var rbThemes = document.getElementsByName("rbThemes")
	var theme = 0;
	if (rbThemes[0].checked){
		theme = 0;
	} else if (rbThemes[1].checked){
		theme = 1;
	} else if (rbThemes[2].checked){
		theme = 2;		
	};	
	SetMcsTheme(theme);
	document.location.reload(true);
} 
