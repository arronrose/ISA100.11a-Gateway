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
 * EvaluationSignal.h
 *
 *  Created on: Sep 23, 2009
 *      Author: Catalin Pop
 */

#ifndef EVALUATIONSIGNAL_H_
#define EVALUATIONSIGNAL_H_

namespace NE {

namespace Common {

#define MASK_ROLE_ACTIVATION            0x0001
#define MASK_ROUTES_EVAL                0x0002
#define MASK_GRAPHS_EVAL                0x0004
#define MASK_GARBAGE                    0x0008
#define MASK_CONTRACT_EXPIRE            0x0010
#define MASK_REMOVE_ACCELERATED_LINKS   0x0020
#define MASK_DIRTY_CONTRACTS   			0x0040
#define MASK_HRCO_RECONFIGURE_PARENT    0x0080
#define MASK_IGNORED_DEVICES            0x0100
#define MASK__BETTER_PARENT             0x0200
#define MASK_UDO_CONTRACTS              0x0400
#define MASK_CLIENT_SERVER_CONTRACTS    0x0800
#define MASK__DIRTY_EDGES               0x1000
#define MASK__BLACKLIST_CHECK           0x2000
#define MASK__ROUTERS_ADVERTISE_CHECK   0x4000
#define MASK__FAST_DISCOVERY_CHECK      0x8000

typedef Uint16 EvaluationSignal;

inline bool isRoleActivationSignal(EvaluationSignal signal){return signal & MASK_ROLE_ACTIVATION;}
inline bool isGraphRedundancySignal(EvaluationSignal signal){return signal & MASK_GRAPHS_EVAL;}
inline bool isRoutesEvaluationSignal(EvaluationSignal signal){return signal & MASK_ROUTES_EVAL;}
inline bool isGarbageCollectionSignal(EvaluationSignal signal){return signal & MASK_GARBAGE;}
inline bool isCheckExpiredContractsSignal(EvaluationSignal signal){return signal & MASK_CONTRACT_EXPIRE;}
inline bool isRemoveAccLinksSignal(EvaluationSignal signal){return signal & MASK_REMOVE_ACCELERATED_LINKS;}
inline bool isDirtyContractsSignal(EvaluationSignal signal){return signal & MASK_DIRTY_CONTRACTS;}
inline bool isUdoContractsSignal(EvaluationSignal signal){return signal & MASK_UDO_CONTRACTS;}
inline bool isClientServerContractsSignal(EvaluationSignal signal){return signal & MASK_CLIENT_SERVER_CONTRACTS;}
inline bool isHRCOReconfigureSignal(EvaluationSignal signal){return signal & MASK_HRCO_RECONFIGURE_PARENT;}
inline bool isIgnoredDevicesSignal(EvaluationSignal signal){return signal & MASK_IGNORED_DEVICES;}
inline bool isFindBetterParentSignal(EvaluationSignal signal){return signal & MASK__BETTER_PARENT;}
inline bool isDirtyEdgesSignal(EvaluationSignal signal){return signal & MASK__DIRTY_EDGES;}
inline bool isBlackListCheckSignal(EvaluationSignal signal){return signal & MASK__BLACKLIST_CHECK;}
inline bool isRoutersAdvertiseCheckSignal(EvaluationSignal signal){return signal & MASK__ROUTERS_ADVERTISE_CHECK;}
inline bool isFastDiscoveryCheckSignal(EvaluationSignal signal){return signal & MASK__FAST_DISCOVERY_CHECK;}


inline void setRoleActivationSignal(EvaluationSignal& signal){ signal = signal | MASK_ROLE_ACTIVATION;}
inline void setGraphRedundancySignal(EvaluationSignal& signal){signal = signal | MASK_GRAPHS_EVAL;}
inline void setRoutesEvaluationSignal(EvaluationSignal& signal){signal = signal | MASK_ROUTES_EVAL;}
inline void setGarbageCollectionSignal(EvaluationSignal& signal){signal = signal | MASK_GARBAGE;}
inline void setCheckExpiredContractsSignal(EvaluationSignal& signal){signal = signal | MASK_CONTRACT_EXPIRE;}
inline void setRemoveAccLinksSignal(EvaluationSignal& signal){signal = signal | MASK_REMOVE_ACCELERATED_LINKS;}
inline void setDirtyContractsSignal(EvaluationSignal& signal){signal = signal | MASK_DIRTY_CONTRACTS;}
inline void setUdoContractsSignal(EvaluationSignal& signal){signal = signal | MASK_UDO_CONTRACTS;}
inline void setClientServerContractsSignal(EvaluationSignal& signal){signal = signal | MASK_CLIENT_SERVER_CONTRACTS;}
inline void setHRCOReconfigureSignal(EvaluationSignal& signal){signal = signal | MASK_HRCO_RECONFIGURE_PARENT;}
inline void setIgnoredDevicesSignal(EvaluationSignal& signal){signal = signal | MASK_IGNORED_DEVICES;}
inline void setFindBetterParentSignal(EvaluationSignal& signal){signal = signal | MASK__BETTER_PARENT;}
inline void setDirtyEdgesSignal(EvaluationSignal& signal){signal = signal | MASK__DIRTY_EDGES;}
inline void setBlackListCheckSignal(EvaluationSignal& signal){signal = signal | MASK__BLACKLIST_CHECK;}
inline void setRoutersAdvertiseCheckSignal(EvaluationSignal& signal){signal = signal | MASK__ROUTERS_ADVERTISE_CHECK;}
inline void setFastDiscoveryCheckSignal(EvaluationSignal& signal){signal = signal | MASK__FAST_DISCOVERY_CHECK;}



}

}

#endif /* EVALUATIONSIGNAL_H_ */
