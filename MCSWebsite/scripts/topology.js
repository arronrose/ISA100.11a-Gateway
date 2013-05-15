var methods = ["sqldal.execute", "user.logout", "file.remove", "file.create", "file.exists"];

/** CONSTANTS ******************************************************/
// used colors
var grayColor  = "#EEEEEE";
var blueColor  = "#0000FF";
var redColor   = "#FF0000";
var greenColor = "#00FF00";
var whiteColor = "#FFFFFF";
var blackColor = "#000000";

// Legend colors
var LinkColor = "#000000";
var SecondaryClockSourceColor = blueColor;
var SecondarySignalQualitiColor = "#0000BB";
var PreferredClockSourceColor = redColor;
var PreferredSignalQualitiColor = "#AE0000";
var PeriodicContractColor =  "#04B404";
var AperiodicContractColor = "#FF0000";
 

// graph parameters
var CellWidth = 50;
var CellWidthDefault = 50;
var CellWidthMin = 25;
var CellWidthMax = 75;

var SequenceOffset = 20;
var SequenceOffsetDefault = 20;
var SequenceOffsetMin = 0;
var SequenceOffsetMax = 40;

var GraphMaxLevelIndex = 5;
var GraphMaxLevel = 3;
var PaperWidth = 960;
var PaperHeight = 500;

var CLOCKSOURCE_NONE = 0;
var CLOCKSOURCE_SECONDARY = 1;
var CLOCKSOURCE_PREFERRED = 2;

// LEVEL parameters
var LevelSequence = new Array(10); 
LevelSequence[0] = [1];
LevelSequence[1] = [1];
LevelSequence[2] = [1, 2];
LevelSequence[3] = [2, 1, 3];
LevelSequence[4] = [2, 4, 1, 3];
LevelSequence[5] = [3, 1, 5, 4, 2];
LevelSequence[6] = [3, 1, 6, 5, 2, 4];
LevelSequence[7] = [4, 1, 7, 5, 3, 6, 2];
LevelSequence[8] = [4, 1, 8, 3, 5, 6, 2, 7];
LevelSequence[9] = [5, 1, 9, 4, 6, 2, 7, 8, 3];

var LevelTopBorder = 2;
var LevelBottomBorder = 2;
var LevelLeftBorder = 50;
var LevelRightBorder = 30;
var LevelSeparatorSize = 20;

// ICON parameters
var IconMargin = 2; // px
var IconLabelHeight = 12; // px

/** GLOBAL VARIABLES ***********************************************/
// topology structures
var LEVELS = [];
var DEVICES = [];
var DEVICE_INDEX = [];
var LINKS = [];
var CONTRACTS = [];

var SelectedDeviceId = null;
var SelectedDeviceIdOld = null;
var ContractSelected = new Array(null, null);
var DragingProcess = false;

var ShowAllLinks = false;
var ShowSignalQuality = false;
var ShowSecondaryCS = false;
var ShowPreferredCS = true;
var ShowNormalLink = false;
var ShowPER = false;
var ShowMode = 1;
var LineShape = "C";
var DeviceChanged = false;

var LastRefreshedString;
var LastRefreshedDate;
var RefreshDeviceInfoActive = false;

var paper = null;
var DeviceLinks = [];
var DeviceNodes = [];
var DeviceLevels = [];
var isDrag = false;
var isClickedOnly = false;


/** Device events **************************************************/
// OnDrag
var Event_DragDevice = function (e) {
    UnTip();
    this.dx = e.clientX;
    this.dy = e.clientY;
    isDrag = this;
    this.animate({"fill-opacity" : .5}, 500);
    e.preventDefault && e.preventDefault();
};

document.onmousemove = function (e) {
    e = e || window.event;
    if (isDrag) {
        var x = isDrag.getBBox().x, dx = e.clientX - isDrag.dx
        var y = isDrag.getBBox().y, dy = e.clientY - isDrag.dy
        x = x + dx;
        y = y + dy;
        if ((x > LEVELS[isDrag.level].X + 1 && x + isDrag.getBBox().width < LEVELS[isDrag.level].X + LEVELS[isDrag.level].Width - 1) &&
            (y > LEVELS[isDrag.level].Y + 2 && y + isDrag.getBBox().height < LEVELS[isDrag.level].Y + LEVELS[isDrag.level].Height - 1)) {
            isDrag.translate(dx, dy);
            if(isDrag.text && isDrag.image) {
                isDrag.text.translate(dx, dy);
                isDrag.image.translate(dx, dy);
            }
            for (var i=0; i<DeviceLinks.length; i++) {
                paper.connection(DeviceLinks[i]);
            }
            paper.safari();
            isDrag.dx = e.clientX;
            isDrag.dy = e.clientY;
        }
    }
    isClickedOnly = false;
};


document.onmouseup = function () {
    isDrag && isDrag.animate({"fill-opacity" : 0.10}, 500);
    isDrag = false;
};


document.onmousedown = function () {
    isClickedOnly = true;
    
};


// Click 
var Event_ClickOnDevice = function (e) {
    if (!isClickedOnly)
        return;
    for (var i=0; i<DEVICES.length; i++) {    	
        if (DeviceNodes[i].rect.id == this.id) {
        	if (ShowMode == 1){
	        	if (!document.getElementById("chkSecondaryCS").checked &&
	                !document.getElementById("chkPreferredCS").checked &&
	                !document.getElementById("chkLink").checked)
	            {
	        		document.getElementById("chkLink").checked = true; 
	                ShowNormalLink = true;
	            }
        	}	
            if (SelectedDeviceId != DEVICES[i].DeviceID) {
            	this.attr({"stroke-width":3});            	            	
            	SelectDevice(DEVICES[i].DeviceID, true, null);
            } else {
            	this.attr({"stroke-width":1});
            	SelectDevice(null, true, null);            	
            }            
        } else {
            DeviceNodes[i].rect.attr({"stroke-width":1});
        }
    }
}


// Hover start
var Event_TipDevice = function (e) {
    for (var i=0; i<DEVICES.length; i++) {
        if (DeviceNodes[i].rect.id == this.id) {
            var tooltipText = 
                "Address64:&nbsp;<b>" +  DEVICES[i].Address64 + "</b>" + 
                "<br /> Role:&nbsp;<b>" + GetDeviceRole(DEVICES[i].DeviceRole) + "</b>" + 
                "<br /> SubnetId:&nbsp;<b>" + DEVICES[i].SubnetID + "</b>" + 
                "<br /> DeviceTag:&nbsp;<b>" + DEVICES[i].DeviceTag + "</b>" +
                "<br /> Manufacturer:&nbsp;<b>" + DEVICES[i].Manufacturer + "</b>" + 
                "<br /> Model:&nbsp;<b>" + DEVICES[i].Model + "</b>";
            Tip(tooltipText);
            return;
        }
    }
}


// Hover end
var Event_UnTipDevice = function (e) {
    UnTip();
}


/** METHODS ********************************************************/

function DrawTopology(drawDevices, drawLevels) {
    // init draw area size
    PaperWidth =  LevelLeftBorder + GraphMaxLevelIndex * CellWidth + LevelRightBorder;
    PaperWidth = PaperWidth < 940 ? 940 : PaperWidth;
    PaperHeight = PaperHeight < 480 ? 480 : PaperHeight;
    if (drawLevels) {
        if (paper == null) {
            paper = Raphael("holder", PaperWidth, PaperHeight);
        } else {
            paper.setSize(PaperWidth, PaperHeight);
        }
    }
    if (drawLevels) {
        RemoveLevels();
        DrawLevels();
    }
    if (drawDevices) {
        RemoveDevices();
        DrawDevices();
    }

    RemoveLinks();
    var lstContracts = document.getElementById("lstContracts");
	var selectedItem = 0; 
	if (lstContracts.selectedIndex < 0) {
		lstContracts.selectedIndex = 0;
	} else {
		selectedItem = lstContracts[lstContracts.selectedIndex].value;
	}
	DrawLegend(ShowMode);
    switch (ShowMode) {
    case 1: 
    	DrawLinks(); 
        break;
    case 2: 
        if (selectedItem == "A" || selectedItem == "I" || selectedItem == "O") {
        	DrawContracts();
        } else {
        	DrawContract(null, null, null);
        }
        break;
    default: ;
    }
    ShowSelectedDevice();
};


/* Get the value of X coordinate */
function GetRect(deviceId) {
    if (DEVICES == null || DEVICES.length == 0 || DEVICE_INDEX[deviceId] == null) 
        return null;

    return DeviceNodes[DEVICE_INDEX[deviceId]].rect;
}


/* Get the value of X coordinate */
function GetX(deviceId) {
    if (DEVICES == null || DEVICES.length == 0 || DEVICE_INDEX[deviceId] == null)
        return 0;
		
	 return DEVICES[DEVICE_INDEX[deviceId]].X;
}


/* Get the value of Y coordinate */
function GetY(deviceId) {
    if (DEVICES == null || DEVICES.length == 0 || DEVICE_INDEX[deviceId] == null)
        return 0;
		
	return DEVICES[DEVICE_INDEX[deviceId]].Y;
}


/* Get the maximum value of X coordinate */
function GetMaxLevelIndex() {
    if (DEVICES == null || DEVICES.length == 0) 
        return 0;
		
    var maxInt = 0;
    for (var i=0; i<DEVICES.length; i++) {
        if (DEVICES[i].DeviceLevelIndex > maxInt) {
            maxInt = DEVICES[i].DeviceLevelIndex;
        }
    }
    return maxInt;
}


/* Get the maximum value of Y coordinate */
function GetMaxLevel() {
    if (DEVICES == null || DEVICES.length == 0)
        return 0;
		
    var maxInt = -1;
    for (var i=0; i<DEVICES.length; i++) {
        if (DEVICES[i].DeviceLevel > maxInt) {
            maxInt = DEVICES[i].DeviceLevel;
        }
    }
    return maxInt;
}


function GetSequenceIndex(maxLevelIndex) {
    var levelIndex = 1;
    if (maxLevelIndex <= 2 ) {
        levelIndex = 1;
    } else if (maxLevelIndex == 3) {
        levelIndex = 2;
    } else if (maxLevelIndex >=  3 && maxLevelIndex <=  5) {
        levelIndex = 3;
    } else if (maxLevelIndex >=  6 && maxLevelIndex <= 10) {
        levelIndex = 4;
    } else if (maxLevelIndex >= 11 && maxLevelIndex <= 15) {
        levelIndex = 5;
    } else if (maxLevelIndex >= 16 && maxLevelIndex <= 20) {
        levelIndex = 6;
    } else if (maxLevelIndex >= 21 && maxLevelIndex <= 25) {
        levelIndex = 7;
    } else if (maxLevelIndex >= 26 && maxLevelIndex <= 30) {
        levelIndex = 8;
    } else {
        levelIndex = 9;
    }
    return levelIndex;
}


function GetLevelOfsset(level, levelIndex) {
    if (LEVELS == null || LEVELS.length == 0)
       return;

    return LevelSequence[LEVELS[level].SequenceIndex]
	                    [(levelIndex - 1)% LevelSequence[LEVELS[level].SequenceIndex].length] * SequenceOffset;
}


function GetMaxIconHeight(level) {
    if (DEVICES == null || DEVICES.length == 0)
        return 0;
		
    var maxHeight = 0;
    for (var i=0; i<LEVELS[level].DeviceList.length; i++) {
        var height = GetImage(DEVICES[DEVICE_INDEX[LEVELS[level].DeviceList[i]]].ModelHex,
					          DEVICES[DEVICE_INDEX[LEVELS[level].DeviceList[i]]].DeviceRole).height;
        if (height > maxHeight) {
            maxHeight = height + IconMargin + IconLabelHeight;
        }
    }
    return maxHeight;
}


function GetLevelHeight(seqIndex, level) {
    return SequenceOffset * (LevelSequence[seqIndex].length + 1) 
        + GetMaxIconHeight(level)
        + LevelTopBorder
        + LevelBottomBorder;
}


/* Get the maximum value of Index for a level */
function GetMaxIndexLvl(level) {
    if (DEVICES == null || DEVICES.length == 0) 
        return 0;

	if (level == 0 && LEVELS[level].DeviceList.length == 2) {
		DEVICES[DEVICE_INDEX[LEVELS[level].DeviceList[0]]].DeviceLevelIndex = 1;
        DEVICES[DEVICE_INDEX[LEVELS[level].DeviceList[2]]].DeviceLevelIndex = 2;
		return 2;
	}

    var maxInt = 0;
    for (var i=0; i<LEVELS[level].DeviceList.length; i++) {
        if (DEVICES[DEVICE_INDEX[LEVELS[level].DeviceList[i]]].DeviceLevelIndex > maxInt) {
            maxInt = DEVICES[DEVICE_INDEX[LEVELS[level].DeviceList[i]]].DeviceLevelIndex;
        }
    }
    return maxInt;
}


function ScaleLevelIndex(ox, level) {
    if (LEVELS == null || LEVELS.length == 0) 
        return 0;
        
    if (LEVELS[level].ScaleFactor > 1) {
        return ox * LEVELS[level].ScaleFactor - (LEVELS[level].ScaleFactor - 1) / 2;
    } else {
        return ox;
    }
}


function GetImage(deviceModel, deviceRole) {
    var imgObject = new Image();
    imgObject.src = "styles/images/" + GetIconFileName(deviceModel, deviceRole);
    return {src: imgObject.src,
            height:((imgObject.height > MAX_DEVICE_ICON_SIZE || imgObject.height <= 0) ? MAX_DEVICE_ICON_SIZE : imgObject.height), 
            width: ((imgObject.width  > MAX_DEVICE_ICON_SIZE || imgObject.width  <= 0) ? MAX_DEVICE_ICON_SIZE : imgObject.width)};
}


function DrawLevels() {
    if (DEVICES == null || DEVICES.length == 0 || LEVELS == null || LEVELS.length ==0)
        return;
        
    for (var i = 0; i <= GraphMaxLevel; i++) {
        DeviceLevels[i] = {level: paper.rect(LEVELS[i].X, LEVELS[i].Y, LEVELS[i].Width, LEVELS[i].Height).attr({stroke:grayColor, fill:grayColor})};
        DeviceLevels[i].text = paper.text(LEVELS[i].X + 25, LEVELS[i].Y + 10, "LEVEL " + i);
        DeviceLevels[i].text.attr({font:'12px Fontin-Sans, Arial', fill:blackColor});
    }
}


function GetDeviceLevel(deviceID) {
    var level = 0;
	
	if (LINKS == null)
	   return level;

    var deviceRole = -1;
	var index = 0;
	
	var historyLinks = []; // for checking infinit loops
	for (var i = 0; i < LINKS.length; i++) 
		historyLinks[i] = {direct: false, reverse: false};
	   
    while (deviceRole != DT_BackboneRouter) {
		index = LINKS.length;
        for (var i = 0; i < LINKS.length; i++) {
            if (LINKS[i].FromDeviceID == deviceID && LINKS[i].ClockSource == CLOCKSOURCE_PREFERRED) {
                level++;
                if (LINKS[i].ToDeviceRole == DT_BackboneRouter) {
					return level;
				} else {
					if (historyLinks[i].direct) // There is an infinite loop between the device and backbone and therefore there is no connection between the device and backbone
					   return 0;
                    deviceID = LINKS[i].ToDeviceID;
					deviceRole = LINKS[i].ToDeviceRole;
					historyLinks[i].direct = true;
					break;
				}
            } else 
			if (LINKS[i].ToDeviceID == deviceID && LINKS[i].ClockSource2 == CLOCKSOURCE_PREFERRED && LINKS[i].Bidirectional) {
                level++;
				if (LINKS[i].FromDeviceRole == DT_BackboneRouter) {
					return level;
				} else {
                    if (historyLinks[i].reverse) // There is an infinite loop between the device and backbone and therefore there is no connection between the device and backbone
                       return 0;
                    deviceID = LINKS[i].FromDeviceID;
                    deviceRole = LINKS[i].FromDeviceRole;
					historyLinks[i].reverse = true;
					break;					
				}
				
			} else {
				index--;
			}
		}
		if (index <= 0) // There is no direct link between current device and Backbone router. So, there is no link between specified device and Backbone router.
            return 0;
    }
    return 0;
}


function SetDeviceLevel() {
	var maxLevel = 1;
    for (var i = 0; i < DEVICES.length; i++) {
        DEVICES[i].DeviceLevel = (DEVICES[i].DeviceRole == DT_Gateway || 
		                          DEVICES[i].DeviceRole == DT_SystemManager || 
								  DEVICES[i].DeviceRole == DT_BackboneRouter) ? 0 : GetDeviceLevel(DEVICES[i].DeviceID);
		if (maxLevel < DEVICES[i].DeviceLevel) {
			maxLevel = DEVICES[i].DeviceLevel;
		}
    }
	
	// each device that has level 0 (not set) will be moved to last level. This happens when we do not have enough information about links.
	for (var i = 0; i < DEVICES.length; i++) {
		if (DEVICES[i].DeviceLevel == 0 &&
            DEVICES[i].DeviceRole != DT_Gateway &&
            DEVICES[i].DeviceRole != DT_SystemManager &&
            DEVICES[i].DeviceRole != DT_BackboneRouter) {
            DEVICES[i].DeviceLevel = maxLevel;
		}
	}
	
	var levelIndex = [];
	for (var i = 0; i <= maxLevel; i++) {
		levelIndex[i] = 0;
	}

	for (var i = 0; i < DEVICES.length; i++) {
		switch (DEVICES[i].DeviceRole) {
            case DT_Gateway:        DEVICES[i].DeviceLevelIndex = 1; break;
            case DT_BackboneRouter: DEVICES[i].DeviceLevelIndex = 2; break;
            case DT_SystemManager:  DEVICES[i].DeviceLevelIndex = 3; break;
            default:                DEVICES[i].DeviceLevelIndex = ++levelIndex[DEVICES[i].DeviceLevel];
		}             
	}
}

function SetLevels() {
	if (DEVICES == null || DEVICES.length == 0)
        return;
	   
    GraphMaxLevelIndex = GetMaxLevelIndex(); 
    GraphMaxLevel = GetMaxLevel();

    for (var i = 0; i < DEVICES.length; i++) {
		DEVICE_INDEX[DEVICES[i].DeviceID] = i;
		if (LEVELS[DEVICES[i].DeviceLevel] == null || 
		    LEVELS[DEVICES[i].DeviceLevel].DeviceList == null) {
            LEVELS[DEVICES[i].DeviceLevel] = { DeviceList: [DEVICES[i].DeviceID] };
		} else {
            LEVELS[DEVICES[i].DeviceLevel].DeviceList[LEVELS[DEVICES[i].DeviceLevel].DeviceList.length] = DEVICES[i].DeviceID;
		}
	}

    var lastLevelPosition = LevelSeparatorSize;
    var seqIndex = 0;
	
    for (var l = 0; l < LEVELS.length; l++) {
        seqIndex = GetSequenceIndex(LEVELS[l].DeviceList.length);
        LEVELS[l] = { X:             0,
                      Y:             lastLevelPosition,
                      Width:         LevelLeftBorder + GraphMaxLevelIndex * CellWidth + LevelRightBorder,
                      Height:        GetLevelHeight(seqIndex, l),
                      SequenceIndex: seqIndex,
                      ScaleFactor:   GraphMaxLevelIndex / ((l == 0) ? 3 : LEVELS[l].DeviceList.length),
					  DeviceList:    LEVELS[l].DeviceList}
        lastLevelPosition += LEVELS[l].Height + LevelSeparatorSize;
	}
    PaperHeight = lastLevelPosition + LevelSeparatorSize;
}


function DrawLinks() {
    if (LINKS == null || LINKS.length == 0) 
        return;

    if (!document.getElementById("chkShowLinks").checked)
        return;
    var linkColor = blackColor;
    for (var i=0; i<LINKS.length; i++) {
        if (SelectedDeviceId != null &&
            (SelectedDeviceId != LINKS[i].FromDeviceID && 
             SelectedDeviceId != LINKS[i].ToDeviceID)) {
            continue;
        }
        var rectFrom = GetRect(LINKS[i].FromDeviceID); 
        var rectTo = GetRect(LINKS[i].ToDeviceID);
        if (rectFrom != null && rectTo != null) {
			var conn, offset1 = 0; offset2 = 0, signalQuality = null;
			if (ShowSecondaryCS && LINKS[i].ClockSource  == CLOCKSOURCE_SECONDARY || 
			    ShowPreferredCS && LINKS[i].ClockSource  == CLOCKSOURCE_PREFERRED ||
			    ShowSecondaryCS && LINKS[i].ClockSource2 == CLOCKSOURCE_SECONDARY || 
				ShowPreferredCS && LINKS[i].ClockSource2 == CLOCKSOURCE_PREFERRED) {
				if (LINKS[i].Bidirectional) {
					offset1 = -2; offset2 = 2
				}
                linkColor = (LINKS[i].ClockSource == CLOCKSOURCE_SECONDARY) ? SecondaryClockSourceColor : 
				            (LINKS[i].ClockSource == CLOCKSOURCE_PREFERRED) ? PreferredClockSourceColor : LinkColor;
                if (linkColor != LinkColor) {
    				signalQuality = ShowSignalQuality ? LINKS[i].SignalQuality : null;
    				conn = paper.connection(rectFrom, rectTo, linkColor, true, false, null, offset1, signalQuality);
                    DeviceLinks.push(conn);
                } 
				if (LINKS[i].Bidirectional) {
					linkColor = (LINKS[i].ClockSource2 == CLOCKSOURCE_SECONDARY) ? SecondaryClockSourceColor : 
					            (LINKS[i].ClockSource2 == CLOCKSOURCE_PREFERRED) ? PreferredClockSourceColor : LinkColor;
					if (linkColor != LinkColor) {
	                    signalQuality = ShowSignalQuality ? LINKS[i].SignalQuality2 : null;
						conn = paper.connection(rectTo, rectFrom, linkColor, true, false, null, offset2, signalQuality);
						DeviceLinks.push(conn);
					}
				}      
			}  
			if (ShowNormalLink){
				signalQuality = ShowSignalQuality ? LINKS[i].SignalQuality : null;
                conn = paper.connection(rectFrom, rectTo, LinkColor, true, LINKS[i].Bidirectional, null, null, signalQuality);
                DeviceLinks.push(conn);
			}
        }
    }
}


function DrawContracts() {
    if (CONTRACTS == null || CONTRACTS.length == 0)
        return;

    var PeriodicContractColors  = ["#347C2C", "#437C17", "#41A317", "#4AA02C"];//, "#00FF00", "#006600", "#009900", "#00CC00"];
    var AperiodicContractColors = ["#E41B17", "#F62817", "#E42217", "#C11B17"];//, "#FF0000", "#660000", "#990000", "#CC0000"];

    var linesSpace = 3;
    var contracts = [];
    var contractId = null, k = 0;
    // get contracts list
    for (var i=0; i<CONTRACTS.length; i++) {
        if (contractId == null || contractId != CONTRACTS[i].ContractID) {
            contractId = CONTRACTS[i].ContractID;
            contracts[k++] = {ContractID: CONTRACTS[i].ContractID, ServiceType: CONTRACTS[i].ServiceType};
        } 
    }
    // draw contract
    for (var k=0; k<contracts.length; k++) {
        var contractColor = (contracts[k].ServiceType == CT_Periodic) ? 
            PeriodicContractColors[contracts[k].ContractID % PeriodicContractColors.length] : 
            AperiodicContractColors[contracts[k].ContractID % AperiodicContractColors.length];
        var contractOffset = (contracts[k].ContractID % PeriodicContractColors.length - 2) * linesSpace;
        DrawContract(contracts[k].ContractID, contractColor, contractOffset);
    }
}


function DrawContract(contractId, contractColor, contractOffset) {
    if (CONTRACTS == null || CONTRACTS.length == 0) 
        return;
        
    var rectFrom = null, rectTo = null;
    if (contractColor == null)
        contractColor = (CONTRACTS[0].ServiceType == CT_Periodic) ? PeriodicContractColor : AperiodicContractColor;
    if (contractOffset == null)
        contractOffset = 0;

    for (var i=0; i<CONTRACTS.length; i++) {
        if (contractId == null || contractId == CONTRACTS[i].ContractID) {
            if (CONTRACTS[i].Idx == 0) {
                rectFrom = GetRect(CONTRACTS[i].SourceDeviceID);
            }
            rectTo = GetRect(CONTRACTS[i].DeviceID);
            if (rectFrom != null && rectTo != null) {
                DeviceLinks.push(paper.connection(rectFrom, rectTo, contractColor, true, false, null, contractOffset));
            }
            rectFrom = rectTo;
        }
    }
}


function ShowSecondaryCSChanged() {
    ShowSecondaryCS = document.getElementById("chkSecondaryCS").checked;
    CheckShowAllLinks();
    DrawTopology(false, false);
}

function ShowPreferredCSChanged() {
    ShowPreferredCS = document.getElementById("chkPreferredCS").checked;
    CheckShowAllLinks();
    DrawTopology(false, false);
}

function ShowNormalLinkChanged() {
	ShowNormalLink = document.getElementById("chkLink").checked;
	CheckShowAllLinks();
    DrawTopology(false, false);
}

function CheckShowAllLinks(){	
	if (!document.getElementById("chkSecondaryCS").checked &&
		!document.getElementById("chkPreferredCS").checked &&
		!document.getElementById("chkLink").checked)
	{
		document.getElementById("chkShowAllLinks").checked = false; 
		ShowAllLinks = false;
	} else if (SelectedDeviceId == null) {
		document.getElementById("chkShowAllLinks").checked = true; 
		ShowAllLinks = true;		
	}
}

// Draw the legend of the topology 
function DrawLegend(mode) {
	var divLegend = document.getElementById("divLegend");
	switch (mode) {
    case 1: 
        divLegend.innerHTML =
            "<span class=\"labels\"><b>Links legend:</b></span><br />" +
            "<input type='checkbox' id='chkLink' " + ((ShowNormalLink) ? "checked='checked'" : "") + " onclick='ShowNormalLinkChanged();'/><span style=\"border-style:solid;border-width:1px;background:" + LinkColor + "\">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span> Link<br />" +
            "<input type='checkbox' id='chkSecondaryCS' " + ((ShowSecondaryCS) ? "checked='checked'" : "") + " onclick='ShowSecondaryCSChanged();' /><span style=\"border-style:solid;border-width:1px;background:" + SecondaryClockSourceColor + "\">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span> Secondary ClockSource<br />" +
            "<input type='checkbox' id='chkPreferredCS' " + ((ShowPreferredCS) ? "checked='checked'" : "") + " onclick='ShowPreferredCSChanged();' /><span style=\"border-style:solid;border-width:1px;background:" + PreferredClockSourceColor + "\">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span> Preferred ClockSource<br />" +
			"<img id='imgRefresh' src='styles/images/transparentpixel.gif' style='width:16px; height:16px;'/>&nbsp;<input type='button' id='btnGetPER' value='Get PER for selected device' onclick='ShowPERChanged();' class='buttonList' style='cursor:hand'/></div>";
        break;
	case 2:
        divLegend.innerHTML =
            "<span class=\"labels\"><b>Contracts legend:</b></span><br />" +
            "<span style=\"border-style:solid;border-width:1px;background:" + PeriodicContractColor + "\">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span> Periodic Contract<br />" +
            "<span style=\"border-style:solid;border-width:1px;background:" + AperiodicContractColor + "\">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span> Aperiodic Contract";
		break;
    default: ;
	}
	
}


// Detects the width and height for an image (assigned to a device)
function GetIconSize(iconSource) {
    var image = new Image();
    image.src = iconSource;
    return {height:((image.height > MAX_DEVICE_ICON_SIZE || image.height <= 0) ? MAX_DEVICE_ICON_SIZE : image.height), 
            width: ((image.width  > MAX_DEVICE_ICON_SIZE || image.width <= 0) ? MAX_DEVICE_ICON_SIZE : image.width)};
}


// Draw a device on the screen
function DrawDevice(x, y, iconSrc, label, deviceRole, level) {
    var color = GetDeviceRoleColor(deviceRole); 
    var icon = GetIconSize(iconSrc);
    var i = paper.image(iconSrc, x, y, icon.width, icon.height);
    var t = paper.text(i.attrs.x + Math.ceil(icon.width/2)-1, i.attrs.y + icon.height + IconLabelHeight - IconMargin, label);
    t.attr({font:'9px Fontin-Sans, Arial', fill:blackColor});
    var r = paper.rect(i.attrs.x - IconMargin, 
                       i.attrs.y - IconMargin,
                       i.attrs.width + 2*IconMargin,
					   i.attrs.height + IconLabelHeight + 2*IconMargin, 
					   2);
    r.attr({fill:color, stroke:color, "fill-opacity":0.10, "stroke-width":1});
    r.node.style.cursor = "hand";
    r.mousedown(Event_DragDevice);
    r.click(Event_ClickOnDevice);
    r.hover(Event_TipDevice, Event_UnTipDevice);
    r.image = i;
    r.text = t;
    r.level = level;
    r.toFront();
    return r
}


// Draw all devices from the topology
function DrawDevices() {
    if (DEVICES == null || DEVICES.length == 0)
        return;
		
    for (var i = 0; i < DEVICES.length; i++) {
        if (DEVICES[i].X > 0 && DEVICES[i].Y > 0) {
            DeviceNodes[i] = {rect: DrawDevice(DEVICES[i].X, 
			                                   DEVICES[i].Y, 
                                               DEVICES[i].Icon, 
											   DEVICES[i].Label, 
                                               DEVICES[i].DeviceRole, 
											   DEVICES[i].DeviceLevel/*-1*/)}; 
        }
    }
}


// Refresh device object on the screen
function RefreshDevice(deviceRect) {
    var d = (new Date()).getTime();
    var imgAttrs = deviceRect.image.attrs;
    imgAttrs.src = imgAttrs.src.split("?")[0] + "?d=" + d;
    var icon = GetIconSize(imgAttrs.src);
    imgAttrs.width = icon.width;
    imgAttrs.height = icon.height;
    var txtAttrs = deviceRect.text.attrs;
    txtAttrs.x = imgAttrs.x + Math.ceil(icon.width / 2) - 1;
    txtAttrs.y = imgAttrs.y + icon.height + IconLabelHeight - IconMargin;
    var rectAttrs = deviceRect.attrs;
    rectAttrs.x = imgAttrs.x - IconMargin;
    rectAttrs.y = imgAttrs.y - IconMargin;
    rectAttrs.width = imgAttrs.width + 2 * IconMargin;
    rectAttrs.height = imgAttrs.height + IconLabelHeight + 2 * IconMargin;
}


// refresh image source for each device in topology
function RefreshDevices () {
    if (DeviceNodes == null || DeviceNodes.length == 0)
        return;
    
    for (var i=0; i<DeviceNodes.length; i++) {
        if (DeviceNodes[i] != null) {
            var imgAttrs = DeviceNodes[i].rect.image.attrs;
            if ((imgAttrs.width <= 0 || imgAttrs.height <= 0) || 
			    (imgAttrs.width == MAX_DEVICE_ICON_SIZE && imgAttrs.height == MAX_DEVICE_ICON_SIZE)) {
                RefreshDevice(DeviceNodes[i].rect);
            }
        }
    }
}


// Clean all graphic objects for links. It's necessary when you want to refresh the topology.
function RemoveLinks() {
    if (DeviceLinks == null || DeviceLinks.length == 0)
        return;
    
    for (var i=0; i<DeviceLinks.length; i++) {
        DeviceLinks[i].line.remove();
        DeviceLinks[i].arrow && DeviceLinks[i].arrow.remove();
        DeviceLinks[i].label && DeviceLinks[i].label.remove();
    }
    DeviceLinks = [];
}


// Clean all graphic objects for devices. It's necessary when you want to refresh the topology.
function RemoveDevices() {
    if (DeviceNodes == null || DeviceNodes.length == 0)
        return;
    
    for (var i=0; i<DeviceNodes.length; i++) {
        DeviceNodes[i].rect.image.remove();
        DeviceNodes[i].rect.text.remove();
        DeviceNodes[i].rect.remove();
    }
    DeviceNodes = [];
}


// Clean all graphic objects for levels. It's necessary when you want to refresh the topology.
function RemoveLevels() {
    if (DeviceLevels == null || DeviceLevels.length == 0)
        return;

    for (var i=0; i<DeviceLevels.length; i++) {
        DeviceLevels[i].level.remove();
        DeviceLevels[i].text.remove();
    }
    DeviceLevels = [];
}


// Translate device position into screen coordinates
function SetScreenCoordinates() {
    if (DEVICES == null || DEVICES.length == 0)
        return;

    for (var i = 0; i < DEVICES.length; i++) {
        var icon = GetIconSize(DEVICES[i].Icon);
        if (DEVICES[i].DeviceLevelIndex > 0 && DEVICES[i].DeviceLevel >= 0) {
            DEVICES[i].X = Math.floor(LevelLeftBorder + CellWidth * (ScaleLevelIndex(DEVICES[i].DeviceLevelIndex, DEVICES[i].DeviceLevel/*-1*/) - 1));
            DEVICES[i].Y = Math.floor(LEVELS[DEVICES[i].DeviceLevel/*-1*/].Y + GetLevelOfsset(DEVICES[i].DeviceLevel/*-1*/, DEVICES[i].DeviceLevelIndex));
        } else {
            DEVICES[i].X = 0;
            DEVICES[i].Y = 0;
        }
    }
}


// Select a device 
function SelectDevice(deviceId, draw, prevAction) {
    var chkShow = document.getElementById("chkShow");
    var chkShowAllLinks = document.getElementById("chkShowAllLinks");
	var chkShowSignalQuality = document.getElementById("chkShowSignalQuality")
    var lstContracts = document.getElementById("lstContracts");
    if (prevAction != "DevicesChanged") {
        document.getElementById("lstDevices").selectedIndex = 0;
    }
	var prevRole = GetDeviceRoleById(SelectedDeviceId);
    SelectedDeviceId = deviceId;
    switch (ShowMode) {
    case 1: /* Links */
        chkShowAllLinks.checked = (SelectedDeviceId) ? false : true;
        ShowAllLinks = (SelectedDeviceId) ? false : true;
		chkShowSignalQuality.checked = ShowSignalQuality;
        // remove contract selection
        lstContracts.selectedIndex = 0;
        document.getElementById("tblContractDetail").innerHTML = "<span class='labels'><b>Contract&nbsp;details:</b></span>";
        break;
    case 2: /* Contracts */
        chkShowAllLinks.checked = false;
        ShowAllLinks = false;
		chkShowSignalQuality.checked = false;
		ShowSignalQuality = false;
        var prevOption = lstContracts.selectedIndex;
        PopulateContractsTable(); 
        // remove contract selection
        var devRole = GetDeviceRoleById(deviceId);
        if (devRole != DT_Gateway && 
		    devRole != DT_BackboneRouter && 
			devRole != DT_SystemManager &&
            (prevOption == 1 || prevOption == 2 || prevOption == 3)) {
			if (prevRole == DT_Gateway || 
			    prevRole == DT_BackboneRouter || 
				prevRole == DT_SystemManager) {
				lstContracts.selectedIndex = 0;
                CONTRACTS = null;
			} else {
	            lstContracts.selectedIndex = prevOption;
	            CONTRACTS = GetContractsElements(deviceId, prevOption);
	            draw = true;
			}
        } else {
            lstContracts.selectedIndex = 0;
            CONTRACTS = null;
        }
        document.getElementById("tblContractDetail").innerHTML = "<span class='labels'><b>Contract&nbsp;details:</b></span>";
        break;
    default: ;
    }
	// select device in combobox
	var lstDevices = document.getElementById("lstDevices");
	for (var i=0; i<lstDevices.length; i++) {
	    if (lstDevices[i].value == SelectedDeviceId) {
	        lstDevices.selectedIndex = i;
	        break;
	    }
	}
    if (draw) {
        DrawTopology(false, false);
    }
}


// Makes the selected device more visible 
function ShowSelectedDevice() {
    if (DEVICES == null || DEVICES.length == 0)
        return;

    for (var i=0; i<DEVICES.length; i++) {
        if (SelectedDeviceId == DEVICES[i].DeviceID) {
            DeviceNodes[i].rect.attr({"stroke-width":3});
        } else {
            DeviceNodes[i].rect.attr({"stroke-width":1});
        }
    }
}


// Set device coordinates
function SetDevicePosition(deviceId, x, y) {
    if (DEVICES == null || DEVICES.length == 0)
        return;

	DEVICES[DEVICE_INDEX[deviceId]].X = x;
    DEVICES[DEVICE_INDEX[deviceId]].Y = y;
}


function ZoomInTopologyW() {
    var btnZoomInW = document.getElementById("btnZoomInW");
    var btnZoomOutW = document.getElementById("btnZoomOutW")
    if (CellWidth < CellWidthMax) {
        CellWidth += 5;
        SetLevels(); 
        SetScreenCoordinates();
        DrawTopology(true, true);
        if (btnZoomOutW.disabled) {
            btnZoomOutW.disabled = false;
        }
    }
    if (CellWidth >= CellWidthMax) {
        btnZoomInW.disabled = true;
    }
}


function ZoomOutTopologyW() {
    var btnZoomInW = document.getElementById("btnZoomInW");
    var btnZoomOutW = document.getElementById("btnZoomOutW")
    if (CellWidth > CellWidthMin) {
        CellWidth -= 5;
        SetLevels(); 
        SetScreenCoordinates();
        DrawTopology(true, true);
        if (btnZoomInW.disabled) {
            btnZoomInW.disabled = false;
        }
    }
    if (CellWidth <= CellWidthMin) {
        btnZoomOutW.disabled = true;
    }
}


function ZoomInTopologyH() {
    var btnZoomInH = document.getElementById("btnZoomInH");
    var btnZoomOutH = document.getElementById("btnZoomOutH")
    if (SequenceOffset < SequenceOffsetMax) {
        SequenceOffset += 5;
        SetLevels();
        SetScreenCoordinates();
        DrawTopology(true, true);
        if (btnZoomOutH.disabled) {
            btnZoomOutH.disabled = false;
        }
    }
    if (SequenceOffset >= SequenceOffsetMax) {
        btnZoomInH.disabled = true;
    }
}


function ZoomOutTopologyH() {
    var btnZoomInH = document.getElementById("btnZoomInH");
    var btnZoomOutH = document.getElementById("btnZoomOutH")
    if (SequenceOffset > SequenceOffsetMin) {
        SequenceOffset -= 5;
        SetLevels();
        SetScreenCoordinates();
        DrawTopology(true, true);
        if (btnZoomInH.disabled) {
            btnZoomInH.disabled = false;
        }
    }
    if (SequenceOffset <= SequenceOffsetMin) {
        btnZoomOutH.disabled = true;
    }
}


function ZoomFitToWindow() {
    var paperWidth = PaperWidth;
    var paperHeight = PaperHeight;
    var topologyDiv = document.getElementById("networkTopology");
    var divWidth = parseInt(topologyDiv.style.width);
    var divHeight = parseInt(topologyDiv.style.height);
    var wasZoomed = false;

    CellWidth = CellWidthDefault;
    paperWidth = LevelLeftBorder + GraphMaxLevelIndex * CellWidth + LevelRightBorder;
    SequenceOffset = SequenceOffsetDefault;
    SetLevels();
    paperHeight = 0;
    for (var i = 0; i < LEVELS.length; i++) {
        paperHeight += LEVELS[i].Height + ((i < LEVELS.length - 1) ? LevelSeparatorSize : 0);
    }
    
    while (divWidth < paperWidth || divHeight < paperHeight) {
		var widthZoomed = false, heightZoomed = false;
		if (CellWidth > CellWidthMin && divWidth < paperWidth) {
			CellWidth -= 5;
            paperWidth = LevelLeftBorder + GraphMaxLevelIndex * CellWidth + LevelRightBorder;
			widthZoomed = true;
		}
        if (SequenceOffset > SequenceOffsetMin && divHeight < paperHeight) {
            SequenceOffset -= 5;
            heightZoomed = true;
        }
		
        if (widthZoomed || heightZoomed) {
            SetLevels();
			paperHeight = 0;
			for (var i = 0; i < LEVELS.length; i++) {
				paperHeight += LevelSeparatorSize + LEVELS[i].Height;
			}
			wasZoomed = true;
		} else {
			break;
		}
    }
    
    if (wasZoomed) {
        SetScreenCoordinates();
        DrawTopology(true, true);
        document.getElementById("btnZoomOutH").disabled = (SequenceOffset <= SequenceOffsetMin);
        document.getElementById("btnZoomOutW").disabled = (CellWidth <= CellWidthMin);
    }
}


function ZoomNormalSize() {
    document.getElementById("btnZoomInH").disabled = false;
    document.getElementById("btnZoomOutH").disabled = false;
    document.getElementById("btnZoomInW").disabled = false;
    document.getElementById("btnZoomOutW").disabled = false;
    SequenceOffset = SequenceOffsetDefault;
    CellWidth = CellWidthDefault;
    SetLevels(); 
    SetScreenCoordinates();
    DrawTopology(true, true);
}


function SetDefaultWidth() {
    document.getElementById("btnZoomInW").disabled = false;
    document.getElementById("btnZoomOutW").disabled = false;
    CellWidth = CellWidthDefault;
    SetLevels(); 
    SetScreenCoordinates();
    DrawTopology(true, true);
}


function SetDefaultHeight() {
    document.getElementById("btnZoomInH").disabled = false;
    document.getElementById("btnZoomOutH").disabled = false;
    SequenceOffset = SequenceOffsetDefault;
    SetLevels(); 
    SetScreenCoordinates();
    DrawTopology(true, true);
}


function RefreshTopology() {
    if (AddGetTopologyCommand() != null && AddGetContractsAndRoutes() != null) {
        AutoRefresh();    
    }
}


function RefreshTopologyGraph() {
    DEVICES = GetTopology();
	if (DEVICES != null) {
        LINKS = GetTopologyLinks();
	    SetDeviceLevel();
        SetLevels(); 
        //SetDeviceOrder();       
        SetScreenCoordinates();
        if (SelectedDeviceId == null)
            SelectedDeviceId = GetDeviceID(DT_BackboneRouter);
    }
    PopulateDevicesList();
    SelectDevice(SelectedDeviceId, false, null);

    document.getElementById("chkShowLinks").checked = true;
    document.getElementById("chkShowAllLinks").checked = false;
    ShowAllLinks = false;
	document.getElementById("chkShowSignalQuality").checked = false;
	ShowSignalQuality = false;
    document.getElementById("chkCurveLines").checked = true;
    LineShape = "C";
    ShowModeChanged(1, false); // Show Links
    
    DrawTopology(true, true);
}


function PopulateDevicesList() {
    if (DEVICES == null || DEVICES.length == 0)
        return;

    var lstDevices = document.getElementById("lstDevices");
    ClearList("lstDevices");
    lstDevices.options[0] = new Option("<None>", "N");
    var sortedDevices = SortDevicesList();
    for (var i=0; i<DEVICES.length; i++) {
        var devOptions = sortedDevices[i].split(",");
        lstDevices.options[i+1] = new Option(devOptions[0], devOptions[1]);
    }
}


function DevicesChanged() {
    var lstDevices = document.getElementById("lstDevices");
    var deviceId = lstDevices[lstDevices.selectedIndex].value;
	deviceId = (lstDevices[lstDevices.selectedIndex].value == "N") ? null : deviceId;
	
	if (!document.getElementById("chkSecondaryCS").checked &&
		 !document.getElementById("chkPreferredCS").checked &&
		 !document.getElementById("chkLink").checked)
	{
		 document.getElementById("chkLink").checked = true; 
		 ShowNormalLink = true;
	} 
	
    SelectDevice(deviceId, true, "DevicesChanged");
}

/* Init page */
function InitTopologyPage() {
    SetPageCommonElements();
	InitJSON();
    
    DEVICES = GetTopology();
	if (DEVICES != null) {
        LINKS = GetTopologyLinks();
	    SetDeviceLevel();
        SetLevels(); 
        //SetDeviceOrder();
        SetScreenCoordinates();
    }
    PopulateDevicesList();
    SelectDevice(GetDeviceID(DT_BackboneRouter), false, null);
	
    document.getElementById("chkShowLinks").checked = true;
    document.getElementById("chkShowAllLinks").checked = false;
    ShowAllLinks = false;
	document.getElementById("chkShowSignalQuality").checked = false;
	ShowSignalQuality = false;
    document.getElementById("chkCurveLines").checked = true;
    LineShape = "C";
    ShowModeChanged(1, false); // Show Links
 
    DrawTopology(true, true);
    
    SetRefreshLabel();
    setInterval("DisplaySecondsAgo()", 1000);      

    // refresh images at every 10 seconds
    RefreshDevices();
}


function AutoRefresh() {
    var LastCommand1 = GetLastCommand(CTC_RequestTopology);
    var LastCommand2 = GetLastCommand(CTC_GetContractsAndRoutes);
    if ((LastCommand1.CommandStatus == CS_New || LastCommand1.CommandStatus == CS_Sent) &&
        (LastCommand2.CommandStatus == CS_New || LastCommand2.CommandStatus == CS_Sent)){                   
        RefreshDeviceInfoActive = true;                 
    } else {
        if (LastCommand1.CommandStatus == CS_Failed && LastCommand2.CommandStatus == CS_Failed) {
            SetRefreshLabel();
        } else {  // at least one has a response            
            RefreshDeviceInfoActive = false;            
            var LastResponse = (LastCommand1.CommandStatus == CS_Responded ? LastCommand1.TimeResponded : LastCommand2.TimeResponded)
            LastRefreshedDate   = ConvertFromSQLiteDateToJSDateObject(LastResponse);
            LastRefreshedString = LastResponse;
        }
		RefreshTopologyGraph();
    }
    if (RefreshDeviceInfoActive) {      
        setTimeout(AutoRefresh, RefreshInterval);
    }
}


function SetRefreshLabel(){   
    var LastCommand1 = GetLastCommandResponded(CTC_RequestTopology);
    var LastCommand2 = GetLastCommandResponded(CTC_GetContractsAndRoutes);
    //both are null
    if (LastCommand1 == null && LastCommand2 == null) {         
        RefreshDeviceInfoActive = false;
        LastRefreshedString = NAString;
        return ;
    };
    var LastResponse;
    //both are not null
    if (LastCommand1 != null && LastCommand2 != null) {
       LastResponse = (LastCommand1.TimeResponded > LastCommand2.TimeResponded ? LastCommand1.TimeResponded : LastCommand2.TimeResponded);
    } else { //only one is null
       LastResponse = (LastCommand1 != null ? LastCommand1.TimeResponded : LastCommand2.TimeResponded);
    };
    // set the label
    LastRefreshedDate   = ConvertFromSQLiteDateToJSDateObject(LastResponse);
    LastRefreshedString = LastResponse; 
}

function GetDeviceID(role) {
    if (DEVICES == null || DEVICES.length == 0)
        return;

    for (var i=0; i<DEVICES.length; i++) {
        if (DEVICES[i].DeviceRole == role) {
            return DEVICES[i].DeviceID;
        }
    }
    return null;
}


function GetDeviceRoleById(deviceId) {
    if (deviceId == null || 
	    DEVICE_INDEX[deviceId] == null || 
		DEVICES == null)
        return null;
    return DEVICES[DEVICE_INDEX[deviceId]].DeviceRole;
}

function ShowChanged(mode, draw) {
	ShowModeChanged(mode, false);
	SelectDevice(SelectedDeviceId, draw, null);
}

function ShowModeChanged(mode, draw) {
    var chkShow = document.getElementById("chkShow");
    var chkShowAllLinks = document.getElementById("chkShowAllLinks");
    var spnShowAllLinks = document.getElementById("spnShowAllLinks");
	var chkShowSignalQuality = document.getElementById("chkShowSignalQuality");
	var spnShowSignalQuality = document.getElementById("spnShowSignalQuality");
    var spnContract = document.getElementById("spnContract");
    var lstContracts = document.getElementById("lstContracts");
    ShowMode = mode;
    switch (ShowMode) {
    case 1: // Links
        // show ShowAllLinks chekbox disabled
        chkShowAllLinks.disabled = false;
        spnShowAllLinks.style.display = "";
        chkShowAllLinks.checked = false;
        ShowAllLinks = false;
		// show ShowSignalQuality checkbox disabled
		chkShowSignalQuality.disabled = false;
		spnShowSignalQuality.style.display = "";
		chkShowSignalQuality.checked = false;
        // hide Contracts combobox
        spnContract.style.display = "none";
        lstContracts.style.display = "none";
        document.getElementById("tblContractDetail").innerHTML = "<span class='labels'><b>Contract&nbsp;details:</b></span>";
        // set Backbone Router as selected device
        if (SelectedDeviceId == null) {
            SelectedDeviceId = GetDeviceID(DT_BackboneRouter);
        }
        break;
    case 2: // Contracts
        // remove contract selection
        ContractSelected[0] = null;
        ContractSelected[1] = null;
        CONTRACTS = null; //GetContractsElements(SelectedDeviceId);
        // hide ShowAllLinks chekbox
        chkShowAllLinks.disabled = true;
        chkShowAllLinks.checked = false;
        ShowAllLinks = false;
		// hide ShowSignalQuality checkbox
		chkShowSignalQuality.disabled = true;
		chkShowSignalQuality.checked = false;
		ShowSignalQuality = false;
        // show Contracts combobox
        spnContract.style.display = "";
        lstContracts.style.display = "";
        document.getElementById("tblContractDetail").innerHTML = "<span class='labels'><b>Contract&nbsp;details:</b></span>";
        // set Backbone Router as selected device
        if (SelectedDeviceId == null) {
            SelectedDeviceId = GetDeviceID(DT_SystemManager);
        }
        // get contracts for Backbone Router
        PopulateContractsTable();
        break;
    default: ;
    }
    if (draw) {
        DrawTopology(false, false);
    }
}


function ShowAllLinksChanged() {
    ShowAllLinks = document.getElementById("chkShowAllLinks").checked;
    if (!document.getElementById("chkSecondaryCS").checked &&
    	!document.getElementById("chkPreferredCS").checked &&
    	!document.getElementById("chkLink").checked)
    {
    	document.getElementById("chkLink").checked = true; 
    	ShowNormalLink = true;
    } 
    if (ShowAllLinks) {
        // remove device selection
		SelectedDeviceIdOld = SelectedDeviceId;
        SelectedDeviceId = null;
    } else {
        // set Backbone Router as selected device
		if (SelectedDeviceIdOld != null) {
			SelectedDeviceId = SelectedDeviceIdOld;
		} else {
			SelectedDeviceId = GetDeviceID(DT_BackboneRouter);
		}
    }
    //document.getElementById("tblContractDetail").innerHTML = "<span class='labels'><b>Contract&nbsp;details:</b></span>";
    SelectDevice(SelectedDeviceId, false, null);
    DrawTopology(false, false);
}

function ShowSignalQualityChanged() {
	ShowSignalQuality = document.getElementById("chkShowSignalQuality").checked;
	DrawTopology(false, false);
}

var MaxNoOfRetryOnNeighborHealthResponse = 3;
var RetryIntervalForNeighborHealthResponse = 5000;
var CurrentRetryNoOnNeighborHealthResponse = 0;

// Refresh neighbour health information
function AddNeighborHealthReportCommand(deviceId) {
    var systemManager = GetSystemManagerDevice();
    if (systemManager == null) {
        return null;
    }
    if (systemManager.DeviceStatus < DS_JoinedAndConfigured) {
       alert("System Manager not registered !");
       return null;
    }   
    var params = Array(1);
    var cmdParam = new CommandParameter();
    cmdParam.ParameterCode = CPC_NeighborHealthReport_DeviceID;
    cmdParam.ParameterValue = deviceId;
    params[0] = cmdParam;
    
    var cmd = new Command();
    cmd.DeviceID = systemManager.DeviceID;
    cmd.CommandTypeCode = CTC_NeighborHealthReport;
    cmd.CommandStatus = CS_New;
    cmd.TimePosted = ConvertFromJSDateToSQLiteDate(GetUTCDate());
    cmd.ParametersDescription = GetParameterName(Number(cmd.CommandTypeCode), Number(cmdParam.ParameterCode)) + ": " + DEVICES[DEVICE_INDEX[deviceId]].Address64;           
    return AddCommand(cmd, params);        
}

function AutoRefreshNeighborHealth() {	
	if (CurrentRetryNoOnNeighborHealthResponse < MaxNoOfRetryOnNeighborHealthResponse){		
	    var LastCommand = GetLastCommand(CTC_NeighborHealthReport, CPC_NeighborHealthReport_DeviceID, SelectedDeviceId);
	    if (LastCommand.CommandStatus == CS_New || LastCommand.CommandStatus == CS_Sent){	    	
	    	CurrentRetryNoOnNeighborHealthResponse++;	    	
	    	setTimeout(AutoRefreshNeighborHealth, RetryIntervalForNeighborHealthResponse);
	    } else {	    	
	    	LINKS = null;
	    	LINKS = GetTopologyLinks();
	    	RemoveLinks();
	    	DrawLinks();
	    	ShowRefreshInProgress(false);
	    };                     	         
	}    
}

function ShowRefreshInProgress(show) {
    if (show) {
        document.getElementById("imgRefresh").src = "styles/images/loader_arrows.gif";
    } else {
        document.getElementById("imgRefresh").src = "styles/images/transparentpixel.gif";
    }
}

function ShowPERChanged() {
	if (SelectedDeviceId == null) {
		alert("There is no selected device!");
		return;
	}
	if (!ShowSignalQuality) {
		alert("[Show signal quality] checkbox is not selected!");
		return;
	}

    if (!ShowPreferredCS && !ShowSecondaryCS) {
        alert("Neither preferred nor secondary clock-source is seleceted!");
        return;
    }

    if (AddNeighborHealthReportCommand(SelectedDeviceId) != null) {
		ShowRefreshInProgress(true);
		CurrentRetryNoOnNeighborHealthResponse = 0;
        AutoRefreshNeighborHealth();
    }
//    DrawTopology(false, false);
}


function LineShapeChanged() {
    LineShape = (document.getElementById("chkCurveLines").checked) ? "C" : "L";
    DrawTopology(false, false);
}


function PopulateContractsTable() {
    var lstContracts = document.getElementById("lstContracts");   
    var contractsData = GetContracts(SelectedDeviceId);
    ClearList("lstContracts");
    var devRole = GetDeviceRoleById(SelectedDeviceId);
    lstContracts.options[0] = new Option("<None>", "N");
    if (contractsData != null) {
	    var idx = 1;
	    if (devRole != DT_Gateway && 
	        devRole != DT_BackboneRouter &&
	        devRole != DT_SystemManager) {
	        lstContracts.options[1] = new Option("<All>", "A");
	        lstContracts.options[2] = new Option("<Inbound>", "I");
	        lstContracts.options[3] = new Option("<Outbound>", "O");
	        idx = 4;
	    }
        for(var i = 0; i < contractsData.length; i++) {
            lstContracts.options[i+idx] = new Option(contractsData[i].SourceDestination, 
                contractsData[i].ContractID + "," + contractsData[i].SourceDeviceID);
        }
    }
}


function ContractChanged() {
    // display selected contract
    var lstContracts = document.getElementById("lstContracts");
    var contractKey = lstContracts[lstContracts.selectedIndex].value;
    if (contractKey == "N" || contractKey == "A" || contractKey == "I" || contractKey == "O") {
        document.getElementById("tblContractDetail").innerHTML = "<span class='labels'><b>Contract&nbsp;details:</b></span>";
        switch (contractKey) {
        case "A": 
            CONTRACTS = GetContractsElements(SelectedDeviceId, 1);
            break;
        case "I":
            CONTRACTS = GetContractsElements(SelectedDeviceId, 2);
            break;
        case "O":
        	CONTRACTS = GetContractsElements(SelectedDeviceId, 3);
        	break;
        default:
        	CONTRACTS = null;
        }
    } else {
        var contractId = contractKey.split(",");
        ContractSelected[0] = contractId[0];
        ContractSelected[1] = contractId[1];
        var contractDetail = GetContractDetails(contractId[0], contractId[1]);
        if (contractDetail) {
            document.getElementById("tblContractDetail").innerHTML = 
            "<span class='labels'><b>Contract&nbsp;details:</b></span><br />" +
            "<span class='labels'>Contract&nbsp;ID:</span>" + contractDetail.ContractID + ", " +
            "<span class='labels'>Service&nbsp;Type:</span>" + GetContractTypeName(contractDetail.ServiceType) + ", " +
			contractDetail.SrcDeviceAddress64 + "/" + contractDetail.SourceSAP + " -> " +
			contractDetail.DstDeviceAddress64 + "/" + contractDetail.DestinationSAP + ", " +
            "<span class='labels'>Activation&nbsp;Time:</span>" + contractDetail.ActivationTime + ", " +
            "<span class='labels'>Expiration&nbsp;Time:</span>" + contractDetail.ExpirationTime + ", " +
            "<span class='labels'>Priority:</span>" + contractDetail.Priority + ", " +
            "<span class='labels'>NSDU&nbsp;Size:</span>" + contractDetail.NSDUSize + ", " +
            "<span class='labels'>Reliability:</span>" + contractDetail.Reliability + ", " +
            ((contractDetail.ServiceType == CT_Periodic) ? 
                "<span class='labels'>Period:</span>" + contractDetail.Period + ", " +
                "<span class='labels'>Phase:</span>" + contractDetail.Phase + ", " +
                "<span class='labels'>Deadline:</span> " + contractDetail.Deadline :
                "<span class='labels'>Comitted&nbsp;Burst:</span>" + contractDetail.ComittedBurst + ", " +
                "<span class='labels'>Excess&nbsp;Burst:</span>" + contractDetail.ExcessBurst + ", " +
                "<span class='labels'>MaxSendWindow:</span> " + contractDetail.MaxSendWindow);
        } else {
            document.getElementById("tblContractDetail").innerHTML = "";
        }
        CONTRACTS = GetContractElements(contractId[0], contractId[1]);
    }
    DrawTopology(false, false);
}


var CT_Periodic = 0;
var CT_Aperiodic = 1


function GetContractTypeName(ctValue) {
    switch (ctValue) {
		case CT_Periodic:
		    return "Periodic";
		case CT_Aperiodic:
			return "Aperiodic";
		default:
			return NAString;
    }
}


var CP_NetworkControl = 3;
var CP_RealTimeBuffer = 2;
var CP_RealTimeSequential = 1;
var CP_BestEffortQueued = 0;


function GetContractPriorityName(cpValue) {
    switch (cpValue) {
		case CP_NetworkControl:
		    return "Network Control";
		case CP_RealTimeBuffer:
			return "Real Time Buffer";
		case CP_RealTimeSequential:
			return "Real Time Sequential";
		case CP_BestEffortQueued:
			return "Best Effort Queued";
		default:
			return NAString;
    }
}


var RN_RetransmitLow = 0;
var RN_RetransmitMedium = 2;
var RN_RetransmitHigh = 4;
var RN_DoNotRetransmitLow = 1;
var RN_DoNotRetransmitMedium = 3;
var RN_DoNotRetransmitHigh = 5;


function GetContractReliabilityName(rnValue) {
    switch (rnValue) {
		case RN_RetransmitLow:
		    return "Retransmit Low";
		case RN_RetransmitMedium:
			return "Retransmit Medium";
		case RN_RetransmitHigh:
			return "Retransmit High";
		case RN_DoNotRetransmitLow:
			return "DoNotRetransmit Low";
		case RN_DoNotRetransmitMedium:
			return "DoNotRetransmit Medium";
		case RN_DoNotRetransmitHigh:
			return "DoNotRetransmit High";			
		default:
			return NAString;
    }
}


Raphael.fn.connection = function (obj1, obj2, line, oriented, bidirectional, bg, offset, label) {
    if (offset == null) offset = 0;
    if (obj1.line && obj1.from && obj1.to) {
        line = obj1;
        obj1 = line.from;
        obj2 = line.to;
        oriented = line.oriented;
        bidirectional = line.bidirectional;
        offset = line.offset;
		label = line.label;
    }
    var bb1 = obj1.getBBox();
    var bb2 = obj2.getBBox();
    var p = [
        {x : bb1.x + bb1.width / 2 + offset, y : bb1.y - 1},
        {x : bb1.x + bb1.width / 2 + offset, y : bb1.y + bb1.height + 1},
        {x : bb1.x - 1, y : bb1.y + bb1.height / 2 + offset},
        {x : bb1.x + bb1.width + 1, y : bb1.y + bb1.height / 2 + offset},
        {x : bb2.x + bb2.width / 2 + offset, y : bb2.y - 1},
        {x : bb2.x + bb2.width / 2 + offset, y : bb2.y + bb2.height + 1},
        {x : bb2.x - 1, y : bb2.y + bb2.height / 2 + offset},
        {x : bb2.x + bb2.width + 1, y : bb2.y + bb2.height / 2 + offset}];

    var d = {};
    var dis = [];
    for (var i = 0; i < 4; i ++ ) {
        for (var j = 4; j < 8; j ++ ) {
            var dx = Math.abs(p[i].x - p[j].x);
            var dy = Math.abs(p[i].y - p[j].y);
            if ((i == j - 4) || 
                (((i != 3 && j != 6) || p[i].x < p[j].x) && 
                 ((i != 2 && j != 7) || p[i].x > p[j].x) && 
                 ((i != 0 && j != 5) || p[i].y > p[j].y) && 
                 ((i != 1 && j != 4) || p[i].y < p[j].y))) {
                dis.push(dx + dy);
                d[dis[dis.length - 1]] = [i, j];
            }
        }
    }
    if (dis.length == 0) {
        var res = [0, 4];
    } else {
        var res = d[Math.min.apply(Math, dis)];
    }
    
    var x1 = p[res[0]].x;
    var y1 = p[res[0]].y;
    var x4 = p[res[1]].x;
    var y4 = p[res[1]].y;
	
    var dx, dy, x2, y2, x3, y3, path, arrow, xt, yt, text;
    var arrowP1, arrowP2;
    if (LineShape == "C") {
        dx = Math.max(Math.abs(x1 - x4) / 2, 10);
        dy = Math.max(Math.abs(y1 - y4) / 2, 10);
        x2 = [x1, x1, x1 - dx, x1 + dx][res[0]].toFixed(3);
        y2 = [y1 - dy, y1 + dy, y1, y1][res[0]].toFixed(3);
        x3 = [0, 0, 0, 0, x4, x4, x4 - dx, x4 + dx][res[1]].toFixed(3);
        y3 = [0, 0, 0, 0, y1 + dy, y1 - dy, y4, y4][res[1]].toFixed(3);
        path = ["M", x1.toFixed(3), y1.toFixed(3), "C", x2, y2, x3, y3, x4.toFixed(3), y4.toFixed(3)].join(",");
    } else {
    	path = ["M", x1.toFixed(3), y1.toFixed(3), "L", x4.toFixed(3), y4.toFixed(3)].join(","); 
        if (oriented) {
        	arrowP1 = GetArrowPoints(x1, y1, x4, y4);
        	if (bidirectional) {
        		arrowP2 = GetArrowPoints(x4, y4, x1, y1);
        		arrow = ["M", arrowP1.x1, arrowP1.y1, "L", x4.toFixed(3), y4.toFixed(3), "L", arrowP1.x2, arrowP1.y2, "L", arrowP1.x1, arrowP1.y1,
        		         "M", arrowP2.x1, arrowP2.y1, "L", x1.toFixed(3), y1.toFixed(3), "L", arrowP2.x2, arrowP2.y2, "L", arrowP2.x1, arrowP2.y1].join(",");
        	} else {
        		arrow = ["M", arrowP1.x1, arrowP1.y1, "L", x4.toFixed(3), y4.toFixed(3), "L", arrowP1.x2, arrowP1.y2, "L", arrowP1.x1, arrowP1.y1].join(",");
        	}
    	}
    }

    /** CONSTANTS */
    var lineSizePercent = 0.1;
    var lineSizeMin = 7;
	
	/** For label offset */
	if (label != null) {
		var theta = Math.abs((y4 - y1) / (x4 - x1 == 0 ? 0.1 : x4 - x1));
		if (theta <= 2) {
			dxt = (bidirectional) ? 0 : 4*(offset==0 ? 1 : offset); 
			dyt = 4 * (offset==0 ? 1 : offset);
		} else {
			dxt = 4 * (offset==0 ? 1 : offset); 
			dyt = (bidirectional) ? 0 : 4*(offset==0 ? 1 : offset);
		}
/*
            var dm = Math.sqrt((x4-x1)*(x4-x1)+(y4-y1)*(y4-y1))
            var st = pt.y/dm;
            var ct = pt.x/dm;
            var dt = 5;
            var xt = pt.x-dt*st;
            var yt = pt.y+dt*ct;
            if (x4<x1) yt += 12;		

*/	}

    var p1, p2;
    if (line && line.line) {
        line.bg && line.bg.attr({path : path});
        var l = line.line.attr({path : path});
        var curveLength = l.getTotalLength();
		if (LineShape == "C") {
			if (oriented) {
                var arcLength = Math.floor(curveLength * lineSizePercent);
                arcLength = (arcLength < lineSizeMin) ? lineSizeMin : arcLength;
				var p1 = l.getPointAtLength(curveLength - arcLength);
				arrowP1 = GetArrowPoints(p1.x, p1.y, x4, y4);
				if (bidirectional) {
					var p2 = l.getPointAtLength(arcLength);
					arrowP2 = GetArrowPoints(p2.x, p2.y, x1, y1);
					arrow = ["M", arrowP1.x1, arrowP1.y1, "L", x4.toFixed(3), y4.toFixed(3), "L", arrowP1.x2, arrowP1.y2, "L", arrowP1.x1, arrowP1.y1, 
					         "M", arrowP2.x1, arrowP2.y1, "L", x1.toFixed(3), y1.toFixed(3), "L", arrowP2.x2, arrowP2.y2, "L", arrowP2.x1, arrowP2.y1].join(",");
				} else {
					arrow = ["M", arrowP1.x1, arrowP1.y1, "L", x4.toFixed(3), y4.toFixed(3), "L", arrowP1.x2, arrowP1.y2, "L", arrowP1.x1, arrowP1.y1].join(",");
				}
			}
		}
		var pt;
		if (label != null) {
			pt = l.getPointAtLength(Math.floor(curveLength / 2));
			line.label.attr({x: pt.x - dxt, y: pt.y - dyt});
			
		}
        line.arrow.attr({path : arrow});
    } else {
        var color = typeof line == "string" ? line : "#000";
		var b = bg && bg.split && this.path(path).attr({stroke : bg.split("|")[0], fill : "none", "stroke-width" : bg.split("|")[1] || 3});
		var l = this.path(path).attr({stroke : color, fill : "none"});
        var curveLength = l.getTotalLength();
        if (LineShape == "C") {
            if (oriented) {
				var arcLength = Math.floor(curveLength * lineSizePercent);
				arcLength = (arcLength < lineSizeMin) ? lineSizeMin : arcLength;
                var p1 = l.getPointAtLength(curveLength - arcLength);
                arrowP1 = GetArrowPoints(p1.x, p1.y, x4, y4);
                if (bidirectional) {
                    var p2 = l.getPointAtLength(arcLength);
                    arrowP2 = GetArrowPoints(p2.x, p2.y, x1, y1);
                    arrow = ["M", arrowP1.x1, arrowP1.y1, "L", x4.toFixed(3), y4.toFixed(3), "L", arrowP1.x2, arrowP1.y2, "L", arrowP1.x1, arrowP1.y1, 
                             "M", arrowP2.x1, arrowP2.y1, "L", x1.toFixed(3), y1.toFixed(3), "L", arrowP2.x2, arrowP2.y2, "L", arrowP2.x1, arrowP2.y1].join(",");
                }
                else {
                    arrow = ["M", arrowP1.x1, arrowP1.y1, "L", x4.toFixed(3), y4.toFixed(3), "L", arrowP1.x2, arrowP1.y2, "L", arrowP1.x1, arrowP1.y1].join(",");
                }
            }
		}
		var pt, lb = null;
		if (label != null) {
			pt = l.getPointAtLength(Math.floor(curveLength / 2));		
			var signalQualityColor;
			switch (line){
				case PreferredClockSourceColor: signalQualityColor = PreferredSignalQualitiColor; break;
				case SecondaryClockSourceColor: signalQualityColor = SecondarySignalQualitiColor; break;
				default: signalQualityColor = LinkColor;
			}			
			lb = this.text(pt.x - dxt, pt.y - dyt, label).attr({"stroke": signalQualityColor, font: '9px Fontin-Sans, Arial', "stroke-width": 0.5});			
		}
        return {
            bg: b,
            line: l,
            arrow: this.path(arrow).attr({stroke: color, fill : color}),
            from: obj1,
            to: obj2,
            oriented: oriented,
            bidirectional: bidirectional,
            offset: offset,
			label: lb
			};
    }
};


function AddGetTopologyCommand() {
	var systemManager = GetSystemManagerDevice();
	if (systemManager == null) {
		return null;
	}
	if (systemManager.DeviceStatus < DS_JoinedAndConfigured) {
	   alert("System Manager not registered !");
	   return null;
    }   
	var cmd = new Command();
	cmd.DeviceID = systemManager.DeviceID;
	cmd.CommandTypeCode = CTC_RequestTopology;
	cmd.CommandStatus = CS_New;
	cmd.TimePosted = ConvertFromJSDateToSQLiteDate(GetUTCDate());
	cmd.ParametersDescription = "";
			
	return AddCommand(cmd, "");
}


function AddGetContractsAndRoutes() {
    var systemManager = GetSystemManagerDevice();
    if (systemManager == null) {
        return null;
    }
    if (systemManager.DeviceStatus < DS_JoinedAndConfigured) {
       alert("System Manager not registered !");
       return null;
    }   
    var cmd = new Command();
    cmd.DeviceID = systemManager.DeviceID;
    cmd.CommandTypeCode = CTC_GetContractsAndRoutes;
    cmd.CommandStatus = CS_New;
    cmd.TimePosted = ConvertFromJSDateToSQLiteDate(GetUTCDate());
    cmd.ParametersDescription = "";
            
    return AddCommand(cmd, "");
}


function DisplaySecondsAgo() {
    if (RefreshDeviceInfoActive){
        document.getElementById("spnRefreshDate").innerHTML = LastRefreshedString +  (LastRefreshedString != NAString ? "  (" + Math.ceil((GetUTCDate().getTime() - LastRefreshedDate.getTime()) / 1000) + " seconds ago)" : "") + ' Refreshing...';
    } else {
        document.getElementById("spnRefreshDate").innerHTML = LastRefreshedString +  (LastRefreshedString != NAString ? "  (" + Math.ceil((GetUTCDate().getTime() - LastRefreshedDate.getTime()) / 1000) + " seconds ago)" : ""); 
    };
}


function SortDevicesList() {
	if (DEVICES == null) 
	   return;
    var sortedDevices = new Array(DEVICES.length);
    for (var i=0; i<DEVICES.length; i++) {
        sortedDevices[i] = DEVICES[i].Address64 + "," + DEVICES[i].DeviceID;
    }
    return sortedDevices.sort();
}


function GetArrowPoints(x, y, xx, yy ) {
  var arrowWidth = 3.0; // change these two values
  var theta = 0.323;    // to change the size of the head of the arrow
  var xPoints = [];
  var yPoints = [];
  var vecLine = [];
  var vecLeft = [];
  var fLength;
  var th;
  var ta;
  var baseX, baseY;

  xPoints[0] = xx;
  yPoints[0] = yy;

  // build the line vector
  vecLine[0] = xPoints[0] - x;
  vecLine[1] = yPoints[0] - y;

  // build the arrow base vector - normal to the line
  vecLeft[0] = -vecLine[1];
  vecLeft[1] = vecLine[0];

  // setup length parameters
  fLength = Math.sqrt(vecLine[0] * vecLine[0] + vecLine[1] * vecLine[1]) ;
  th = arrowWidth / (2.0 * fLength);
  ta = arrowWidth / (2.0 * (Math.tan(theta) / 2.0) * fLength);

  // find the base of the arrow
  baseX = xPoints[0] - ta * vecLine[0];
  baseY = yPoints[0] - ta * vecLine[1];

  // build the points on the sides of the arrow
  xPoints[1] = baseX + th * vecLeft[0];
  yPoints[1] = baseY + th * vecLeft[1];
  xPoints[2] = baseX - th * vecLeft[0];
  yPoints[2] = baseY - th * vecLeft[1];
  
  return {x1:xPoints[1], y1:yPoints[1], x2:xPoints[2], y2:yPoints[2]};
}  

function GetDeviceLevelAvarage(deviceId, level) {
    if (LINKS == null || LINKS.length ==0)
       return;
       
    var sumParentDevices = 0;
    var sumCount = 0;
    for (var i=0; i<LINKS.length; i++) {
        if (LINKS[i].FromDeviceID == deviceId) {
            if (DEVICES[DEVICE_INDEX[LINKS[i].FromDeviceID]].DeviceLevel == level) {
                sumParentDevices += DEVICES[DEVICE_INDEX[LINKS[i].FromDeviceID]].DeviceLevelIndex;
                sumCount++;
            }
        }
    }
    return sumParentDevices/sumCount;
}

function SetDeviceOrderByParents() {
	if (DEVICES == null || DEVICES.length == 0 || LEVELS == null || LEVELS.length == 0)
	   return;
	   
	for (var j=1; j<LEVELS.length; j++) {
        for (var i=0; i<LEVELS[j].DeviceList.length; i++) {
            DEVICES[DEVICE_INDEX[LEVELS[j].DeviceList[i]]].AvgLevelIndex = GetDeviceLevelAvarage(LEVELS[j].DeviceList[i], j);
		}
    }
}

function SetDeviceOrder() {
	if (DEVICES == null || DEVICES.length == 0)
	   return;

	SetDeviceOrderByParents();
    for (var i=0; i<DEVICES.length-1; i++) {
        var min = i;
        for (var j = i+1; j<DEVICES.length; j++) {
            if (DEVICES[j].AvgLevelIndex < DEVICES[min].AvgLevelIndex) {
                min = j;
            }
        }
        if (i != min) {
            var swapIndex = DEVICES[i].DeviceLevelIndex;
            DEVICES[i].DeviceLevelIndex = DEVICES[min].DeviceLevelIndex;
            DEVICES[min].DeviceLevelIndex = swapIndex;
            var swapDEVICE = DEVICES[i]; 
			DEVICE_INDEX[DEVICES[i].DeviceID] = min;
			DEVICE_INDEX[DEVICES[min].DeviceID] = i;
        }
    }
}

var MYDEVICES = [];
var MYLINKS = [];

function GetDevicesLevels(){
	MYDEVICES = null;	
	MYDEVICES = GetTopology();
		
	MYLINKS = null;
	MYLINKS = GetTopologyLinks();
	
	var tmp = [];	
	for(var i=0; i< MYDEVICES.length; i++){
		tmp[MYDEVICES[i].DeviceID] = GetNodeLevel(MYDEVICES[i].DeviceID); 
	}

	return tmp;
}

function GetMaxDeviceLevel(){
	MYDEVICES = null;	
	MYDEVICES = GetTopology();
		
	MYLINKS = null;
	MYLINKS = GetTopologyLinks();
	
	var maxLevel = 0;		
	if (MYDEVICES != null){
		for(var i=0; i< MYDEVICES.length; i++){
			var tmp = GetNodeLevel(MYDEVICES[i].DeviceID);
			if (maxLevel < tmp){
				maxLevel = tmp;
			}; 
		};
	};
	return maxLevel;	
}

function GetNodeLevel(deviceID) {
    var level = 0;
	
	if (MYLINKS == null)
	   return level;

    var deviceRole = -1;
	var index = 0;
	
	var historyLinks = []; // for checking infinit loops
	if (MYLINKS != null){
		for (var i = 0; i < MYLINKS.length; i++) 
			historyLinks[i] = {direct: false, reverse: false};		
	}
	   
    while (deviceRole != DT_BackboneRouter) {
		index = MYLINKS.length;
        for (var i = 0; i < MYLINKS.length; i++) {
            if (MYLINKS[i].FromDeviceID == deviceID && MYLINKS[i].ClockSource == CLOCKSOURCE_PREFERRED) {
                level++;
                if (MYLINKS[i].ToDeviceRole == DT_BackboneRouter) {
					return level;
				} else {
					if (historyLinks[i].direct) // There is an infinite loop between the device and backbone and therefore there is no connection between the device and backbone
					   return 0;
                    deviceID = MYLINKS[i].ToDeviceID;
					deviceRole = MYLINKS[i].ToDeviceRole;
					historyLinks[i].direct = true;
					break;
				}
            } else 
			if (MYLINKS[i].ToDeviceID == deviceID && MYLINKS[i].ClockSource2 == CLOCKSOURCE_PREFERRED && MYLINKS[i].Bidirectional) {
                level++;
				if (MYLINKS[i].FromDeviceRole == DT_BackboneRouter) {
					return level;
				} else {
                    if (historyLinks[i].reverse) // There is an infinite loop between the device and backbone and therefore there is no connection between the device and backbone
                       return 0;
                    deviceID = MYLINKS[i].FromDeviceID;
                    deviceRole = MYLINKS[i].FromDeviceRole;
					historyLinks[i].reverse = true;
					break;					
				}
				
			} else {
				index--;
			}
		}
		if (index <= 0) // There is no direct link between current device and Backbone router. So, there is no link between specified device and Backbone router.
            return 0;
    }
    return 0;
}