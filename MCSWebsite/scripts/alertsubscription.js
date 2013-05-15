var methods = ["config.getGroupVariables",
               "config.setVariable",
               "file.create",
               "isa100.sm.getLogLevel",
               "isa100.sm.setLogLevel",
               "user.logout",
               "sqldal.getCsv",
               "sqldal.execute"];
               
function InitAlertSubscriptionPage() {
    SetPageCommonElements();
    InitJSON();
    GetData();
}
             
               
function SaveData() {

    var chkCommunicationDiagnostic = document.getElementById("chkCommunicationDiagnostic").checked;
    var chkSecurity = document.getElementById("chkSecurity").checked;	
   	var chkDeviceDiagnostic = document.getElementById("chkDeviceDiagnostic").checked;
   	var chkProcess = document.getElementById("chkProcess").checked;    	

    if (chkCommunicationDiagnostic == true){
        chkCommunicationDiagnostic = 1;
    } else {
        chkCommunicationDiagnostic = 0;
    };

    if (chkSecurity == true){
        chkSecurity = 1;
    } else{
        chkSecurity = 0;
    };

    if (chkDeviceDiagnostic == true){
        chkDeviceDiagnostic = 1;
    } else{
        chkDeviceDiagnostic = 0;
    };

    if (chkProcess == true){
        chkProcess = 1;
    } else {
        chkProcess = 0;
    };
    
    if (SaveAlertSubstription(chkProcess, chkDeviceDiagnostic, chkCommunicationDiagnostic, chkSecurity)){
        DisplayOperationResult("spnOperationResult","Save completed successfully.");
    } else {
        alert("Error saving alert subscription !")
    };       
}


function GetData() {

    var result = GetAlertSubstription();

    if (result.length != 0){    

       	document.getElementById("chkProcess").checked = result.CategoryProcess;    	
   	    document.getElementById("chkDeviceDiagnostic").checked = result.CategoryDevice;
        document.getElementById("chkCommunicationDiagnostic").checked = result.CategoryNetwork;
        document.getElementById("chkSecurity").checked = result.CategorySecurity;	
    }
}
