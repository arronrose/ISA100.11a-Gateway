function SetDeviceTabs(deviceId, deviceStatus, deviceRoleID, activeTabNumber) {
    var tab0 = document.getElementById("tab0");
    var tab1 = document.getElementById("tab1");
    var tab2 = document.getElementById("tab2");
    var tab3 = document.getElementById("tab3");
    var tab4 = document.getElementById("tab4");
    var tab5 = document.getElementById("tab5");
    var tab6 = document.getElementById("tab6");

    var tab0Visible = true;
    var tab1Visible = (deviceStatus >= DS_JoinedAndConfigured && deviceRoleID != DT_SystemManager && deviceRoleID != DT_Gateway);
    var tab2Visible = true;    
    var tab3Visible = (deviceStatus >= DS_JoinedAndConfigured && deviceRoleID != DT_SystemManager && deviceRoleID != DT_Gateway);
    var tab4Visible = (deviceStatus >= DS_JoinedAndConfigured && deviceRoleID != DT_SystemManager && deviceRoleID != DT_Gateway); 
    var tab5Visible = (deviceStatus >= DS_JoinedAndConfigured && deviceRoleID != DT_SystemManager && deviceRoleID != DT_Gateway);
    var tab6Visible = (deviceStatus >= DS_JoinedAndConfigured && deviceRoleID != DT_SystemManager);
    
    if (activeTabNumber == 0) {
        tab0.className = "selectedTabButton";
        tab0.innerHTML = "Information";
    } else if (tab0Visible){
        tab0.className = "tabButton";
        tab0.innerHTML = "<a href='deviceinformation.html?deviceId=" + deviceId + "' class='tabLink'>Information</a>";
        tab0.disabled = false;
        tab0.style.cursor = "hand";
    } else {
    	tab0.className = "tabButton";    
    	tab0.innerHTML = "Information";
    	tab0.disabled = true;    	
    	tab0.style.cursor = "none";
    }
    
    if (activeTabNumber == 1) {
        tab1.className = "selectedTabButton";
        tab1.innerHTML = "Settings";
    } else if (tab1Visible) {
        tab1.className = "tabButton";
        tab1.innerHTML = "<a href='devicesettings.html?deviceId=" + deviceId + "' class='tabLink'>Settings</a>";
        tab1.disabled = false;
        tab1.style.cursor = "hand";
    } else {
    	tab1.className = "tabButton";    
    	tab1.innerHTML = "Settings";
    	tab1.disabled = true;    	
    	tab1.style.cursor = "none";
    }
    
    if (activeTabNumber == 2) {
        tab2.className = "selectedTabButton";
        tab2.innerHTML = "Registration Log";
    } else if (tab2Visible) {
        tab2.className = "tabButton";
        tab2.innerHTML = "<a href='registrationlog.html?deviceId=" + deviceId + "' class='tabLink'>Registration Log</a>";
        tab2.disabled = false;
        tab2.style.cursor = "hand";
    } else {
    	tab2.className = "tabButton";    
    	tab2.innerHTML = "Registration Log";
    	tab2.disabled = true;
    	tab2.style.cursor = "none";
    }
    
    if (activeTabNumber == 3) {
        tab3.className = "selectedTabButton";
        tab3.innerHTML = "Neighbors Health";
    } else if (tab3Visible) {
        tab3.className = "tabButton";
        tab3.innerHTML = "<a href='neighborshealth.html?deviceId=" + deviceId + "' class='tabLink'>Neighbors Health</a>";
        tab3.disabled = false;
        tab3.style.cursor = "hand";
    } else {
    	tab3.className = "tabButton";    	
    	tab3.innerHTML = "Neighbors Health";
    	tab3.disabled = true;
    	tab3.style.cursor = "none";
    }
    
    if (activeTabNumber == 4) {
        tab4.className = "selectedTabButton";
        tab4.innerHTML = "Schedule Report";
    } else if (tab4Visible) {
        tab4.className = "tabButton";
        tab4.innerHTML = "<a href='schedulereport.html?deviceId=" + deviceId + "' class='tabLink'>Schedule Report</a>";
        tab4.disabled = false;   
        tab4.style.cursor = "hand";
    } else {
    	tab4.className = "tabButton";    
    	tab4.innerHTML = "Schedule Report";
    	tab4.disabled = true;    	
    	tab4.style.cursor = "none";
    }
    
    if (activeTabNumber == 5) {
        tab5.className = "selectedTabButton";
        tab5.innerHTML = "Channels Statistics";
    } else if (tab5Visible) {
        tab5.className = "tabButton";
        tab5.innerHTML = "<a href='channelsstatistics.html?deviceId=" + deviceId + "' class='tabLink'>Channels Statistics</a>";
        tab5.disabled = false;
        tab5.style.cursor = "hand";
    } else {
    	tab5.className = "tabButton";
    	tab5.innerHTML = "Channels Statistics";
    	tab5.disabled = true;    	
    	tab5.style.cursor = "none";
    }

    if (activeTabNumber == 6) {
        tab6.className = "selectedTabButton";
        tab6.innerHTML = "Run Commands";
    } else if (tab6Visible) {
        tab6.className = "tabButton";
        tab6.innerHTML = "<a href='devicecommands.html?deviceId=" + deviceId + "' class='tabLink'>Run Commands</a>";
        tab6.disabled = false;
        tab6.style.cursor = "hand";
    } else {
    	tab6.className = "tabButton";
    	tab6.innerHTML = "Run Commands";
    	tab6.disabled = true;
    	tab6.style.cursor = "none";
    }        
}