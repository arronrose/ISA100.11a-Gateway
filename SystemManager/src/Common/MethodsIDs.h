/*
* Copyright (C) 2013 Nivis LLC.
* Email:   opensource@nivis.com
* Website: http://www.nivis.com
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, version 3 of the License.
* 
* Redistribution and use in source and binary forms must retain this
* copyright notice.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

/*
 * MethodsID.h
 * Contains IDs for all methods of ISA objects implemented by SM or by Devices.
 *
 *  Created on: Jun 17, 2008
 *      Author: catalin.pop
 */

#ifndef METHODSID_H_
#define METHODSID_H_

namespace Isa100 {

namespace Common {

namespace MethodsID {

namespace DLMOMethodID {
//enum DLMOMethodID {
//    SetDllTbl_Ch_Entry = 1,
//    SetDllTbl_TsTemplate_Receive_Entry = 2,
//    SetDllTbl_TsTemplate_Transmit_Entry = 3,
//    Set_DllTbl_Neighbor_Entry = 4,
//    Set_DllTbl_Superframe_Entry = 5,
//    Set_DllTbl_Graph_Entry = 6,
//    Set_DllTbl_Link_Entry = 7,
//    Set_DllTbl_Route_Entry = 8,
//    GetDllTbl_Ch_Entry = 9,
//    GetDllTbl_TsTemplate_Receive_Entry = 10,
//    GetDllTbl_TsTemplate_Transmit_Entry = 11,
//    Get_DllTbl_Neighbor_Entry = 12,
//    Get_DllTbl_Superframe_Entry = 13,
//    Get_DllTbl_Graph_Entry = 14,
//    Get_DllTbl_Link_Entry = 15,
//    Get_DllTbl_Route_Entry = 16,
//    Delete_DllTbl = 17,
//    Clear_DllTbl = 18,
//    Get_DllTbl_Statistics = 19,
//    SetDiagnosticsParameters = 20,
//    Set_Neighbor_Discovery_Pbl_Params = 21
//};

enum DLMOMethodID {
    Scheduled_Write = 1,
    Read_Row = 2,
    Write_Row = 3,
    Reset_Row = 4,
    Delete_Row = 5,
    Meta_Data_Attribute = 6
};
}

namespace DMOMethodID {
enum DMOMethodID {
//    Write_Row = 1,
//    Delete_Row = 2,
    Terminate_Contract = 1,
    Modify_Contract = 2,
    Proxy_System_Manager_Join = 3,
    Proxy_System_Manager_Contract = 4,
    Proxy_Security_Sym_Join = 5
};
}

namespace DMSOMethodID {
enum DMSOMethodID {
    System_Manager_Join = 1,       // device join request
    System_Manager_Contract = 2,   // device contract join request
};
}

namespace SCOMethodID {
enum SCOMethodID {
    Contract_Establishment_Modification_Renewal = 1,
    Contract_Termination_Deactivation_Reactivation = 2,
    NewPSMOJoinRequestForward = 0x91  // custom method id, notifies SCO by PSMO when it receives a sec join request
};
}

namespace TLMOMethodID {
enum TLMOMethodID {
    AddContract = 1, ModifyContract = 2, DeleteContract = 3
};
}

namespace NLMOMethodID {
enum NLMOMethodID {
	Set_row_RT = 1,
	Get_row_RT = 2,
	Delete_row_RT = 3,
	Set_row_ContractTable = 4,
	Get_row_ContractTable = 5,
	Delete_row_ContractTable = 6,
	Set_row_ATT = 7,
	Get_row_ATT = 8,
	Delete_row_ATT = 9
};
}

namespace DOMethodID {
enum DOMethodID {
    Configure_Object = 1,
    Get_Device_Statistics = 2
};
}

namespace DSOMethodID {
enum DSOMethodID {
    Read_Address_Row = 1
};
}

namespace PSMOMethodsId {
enum PSMOMethodsIdEnum {
    SECURITY_SYM_JOIN_REQUEST = 1, SECURITY_CONFIRM = 2, SECURITY_NEW_SESSION = 6,  PAYLOAD_ENCRYPT = 0x81
};
}

namespace DSMOMethodId {
enum DSMOMethodsIdEnum
{
	NEW_KEY = 1,
    DELETE_KEY = 2
};
}


namespace SMOMethodId {
enum SMOMethodIdEnum {
    GET_NETWORK_TOPOLOGY = 1,
    GET_CONTRACT_TABLE = 2,
    GET_ADDRESS_TABLE = 3,
    GET_PING_STATUS = 4,
//    GET_DEVICE_INFORMATION = 5,
    GENERATE_TOPOLOGY = 6,
    GET_TOPOLOGY_BLOCK = 7,
    GENERATE_DEVICE_LIST = 8,
    GET_DEVICE_LIST_BLOCK = 9,
    CONFIRM_DEVICE = 10,
    START_PUBLISH_CONFIG = 11,
    RESTART_DEVICE = 12,
    GENERATE_REPORT = 13,
    GET_BLOCK = 14,
    GET_CONTRACTS_AND_ROUTES = 15,
    GET_CCA_BACKOFF = 16
};
}

namespace BLOMethodId {
enum BLOMethodsIdEnum
{
	ProcessChannelDiagnostics = 1
};
}

} // namespace MethodsID

} // namespace Common

} // namespace Isa100

#endif /* METHODSID_H_ */
