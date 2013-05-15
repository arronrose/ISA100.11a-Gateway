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

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Adrian Simionescu
/// Date:         January 2008
/// Description:  This file holds definitions of the Network Layer Management Entity
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _NIVIS_NLME_H_
#define _NIVIS_NLME_H_

#include "config.h"
#include "dmap_dmo.h"
#include <sys/time.h>
#include <boost/shared_ptr.hpp>
#include <map>
#include <string.h>

typedef enum {
  NLME_ATRBT_ID_BBR_CAPABLE = 1,
  NLME_ATRBT_ID_DL_CAPABLE,
  NLME_ATRBT_ID_SHORT_ADDR,
  NLME_ATRBT_ID_IPV6_ADDR,
  NLME_ATRBT_ID_ROUTE_TABLE,
  NLME_ATRBT_ID_DEFAULT_ROUTE_FLAG,
  NLME_ATRBT_ID_DEFAULT_ROUTE_ID,
  NLME_ATRBT_ID_CONTRACT_TABLE,
  NLME_ATRBT_ID_ATT_TABLE,
  NLME_ATRBT_ID_MAX_NSDU_SIZE,
  NLME_ATRBT_ID_FRAG_REASSEMBLY_TIMEOUT,
  NLME_ATRBT_ID_FRAG_DATAGRAM_TAG,
  
  NLME_ATRBT_ID_ROUTE_TBL_META,
  NLME_ATRBT_ID_CONTRACT_TBL_META,
  NLME_ATRBT_ID_ATT_TBL_META,
  
  NLME_ATRBT_ID_ALERT_DESC,
  
  NLME_ATRBT_ID_NO
} NLME_ATTRIBUTE_ID;

typedef enum {
  NLME_ALERT_DEST_UNREACHABLE = 1,
  NLME_ALERT_FRAG_ERROR, 
  NLME_ALERT_REASS_TIMEOUT, 
  NLME_ALERT_HOP_LIMIT_REACHED, 
  NLME_ALERT_HEADER_ERROR, 
  NLME_ALERT_NO_ROUTE, 
  NLME_ALERT_OUT_OF_MEM, 
  NLME_ALERT_NPDU_LEN_TOO_LONG 
} NLME_ALERT_TYPE;

struct classcompIPv6 
{
	bool operator() (const uint8* lhs, const uint8* rhs) const
	{	
		return memcmp(lhs,rhs,sizeof(IPV6_ADDR)) < 0;
	}
};

typedef struct
{
    uint8 * m_pValue;
    uint8   m_ucSize;
    uint8   m_ucIndexSize;
    uint8   m_ucIsReadOnly;
} NLME_ATTRIBUTES_PARAMS;

typedef struct
{   
  IPV6_ADDR     m_aDestAddress; 
  IPV6_ADDR     m_aNextHopAddress;
  uint8         m_ucNWK_HopLimit;                        // Default value : 64
  uint8         m_bOutgoingInterface;                 // Valid values: 0-DL, 1-Backbone
}NLME_ROUTE_ATTRIBUTES;

typedef boost::shared_ptr<NLME_ROUTE_ATTRIBUTES> CNlmeRoutePtr;

typedef std::map<uint8*,CNlmeRoutePtr,classcompIPv6> CNlmeRoutesMap;

class NLME_CONTRACT_ATTRIBUTES
{ 
public:
  uint16          m_unContractID;
//  IPV6_ADDR       m_aSourceAddress; not used since I'm the source all time
  IPV6_ADDR       m_aDestAddress; 
  uint8           m_bPriority : 2;                          // Valid values: 00, 01, 10, 11
  uint8           m_bIncludeContractFlag : 1;               // 0 = Don't Include, 1 = Include
  uint8           m_bUnused : 5;             
  int32           m_nRTO;  // retry timer-out interval (units normally in seconds, temporary used with 1/8s)
  uint32          m_nSRTT; // smoothed round-trip time (units in 1/8 s)
  uint32          m_nRTTV; // round-trip time variation (units in 1/8 s)
  uint8           m_nCrtWndSize; // number of allowed simultaneously outstanding requests
  uint8           m_nInUseWndSize; // number of actual simultaneously outstanding requests
  uint8           m_nSuccessTransmissions; // number of consecutive round-trip communications
  uint8		  m_ucReqID;	// the ASL request ID counter for newly generated requests
  uint16          m_unAssignedMaxNSDUSize; //maximum NSDU supported in octets
  int16           m_nComittedBurst; //the long term rate that needs to be supported for client-server or source-sink messages;
				// Valid value set: positive values indicate APDUs per second, negative values indicate seconds per APDU
  int16           m_nExcessBurst; //the short term rate that needs to be supported for client-server or source-sink messages;
				// Valid value set: positive values indicate APDUs per second, negative values indicate seconds per APDU
  uint8           m_ucMaxSendWindow; // this is a duplicate of the DMO aperiodic contract bandwidth. All changes there MUST come here too!
  struct timeval  m_stSendNoEarlierThan; //timestamp of the earliest opportunity to send using this contract
  uint32	m_unPktCount;
};

typedef boost::shared_ptr<NLME_CONTRACT_ATTRIBUTES> CNlmeContractPtr;
//typedef boost::weak_ptr<NLME_CONTRACT_ATTRIBUTES> CNlmeContractWeakPtr;

typedef std::map<uint16,CNlmeContractPtr> CNlmeContractsMap;

typedef struct{
	struct{
		uint16	m_nContractId;	///< in network order
		uint8	m_aOwnIPv6[16];	///< IPv6 of the local device
	}__attribute__((packed))m_stIdx;
	struct{
		uint16	m_nContractId;	///< copy of m_nContractId
		uint8	m_aOwnIPv6[16];	///< copy of m_aOwnIPv6
		uint8	m_aPeerIPv6[16];///< IPv6 of the peer device
		uint8	m_cFlags;	///< b1b0 = priority, b3 = include contract flag
	}__attribute__((packed))m_stData;
}__attribute__((packed)) NLME_CONTRACT_ATTRIBUTES_FOR_SETROW;

typedef struct
{   
  IPV6_ADDR     m_aIPV6Addr;
  uint8         m_aShortAddress[2];
} NLME_ADDR_TRANS_ATTRIBUTES;

typedef boost::shared_ptr<NLME_ADDR_TRANS_ATTRIBUTES>  CNlmeAttPtr;




typedef std::map<uint8*,CNlmeAttPtr,classcompIPv6> CNlmeAttMap16toEntry;

struct classcompShortAddr 
{
	bool operator() (const uint8* lhs, const uint8* rhs) const
	{	
		return memcmp(lhs,rhs,2) < 0;
	}
};

typedef std::map<uint8*,CNlmeAttPtr,classcompShortAddr> CNlmeAttMap2toEntry;

typedef struct
{
  uint16 m_unFragDatagramTag;      
  uint8  m_ucFragReassemblyTimeout;                     // Valid value set: 1 - 64   
  uint8  m_ucEnableDefaultRoute;
  uint8  m_aDefaultRouteEntry[16];                         // Index into the RouteTable that is the default route 

  //uint16  m_nContractNo;
//  uint16  m_nAddrTransNo;
    

//  uint16  m_nRoutesNo;
//  NLME_ROUTE_ATTRIBUTES       m_aRoutesTable[MAX_ROUTES_NO];
	CNlmeRoutesMap				m_oNlmeRoutesMap;

	CNlmeContractsMap			m_oContractsMap;

  //NLME_CONTRACT_ATTRIBUTES    m_aContractTable[MAX_CONTRACT_NO];
  //NLME_ADDR_TRANS_ATTRIBUTES  m_aAddrTransTable[MAX_ADDR_TRANSLATIONS_NO];
	CNlmeAttMap16toEntry			m_oAttMap16toEntry;
	CNlmeAttMap2toEntry				m_oAttMap2toEntry;

  uint8  m_ucAlertDesc;
  
} NLME_ATRIBUTES;



    void NLME_Init( void );

    uint8 NLME_SetRow( uint8         p_ucAttributeID, 
                       uint32        p_ulTaiCutover, 
                       const uint8 * p_pData,
                       uint8         p_ucDataLen );
uint8 NLME_SetRowWithDMOContractParams( uint8         p_ucAttributeID,
                   uint32        p_ulTaiCutover,
                   const uint8 * p_pData,
                   uint8         p_ucDataLen,
                   uint16        p_unAssignedMaxNSDUSize,
                   uint8         p_ucWndLen,
                   int16         p_nComittedBurst,
                   int16         p_nExcessBurst );
    
    uint8 NLME_GetRow( uint8         p_ucAttributeID, 
                       const uint8 * p_pIdxData,
                       uint8         p_ucIdxDataLen,
                       uint8       * p_pData,
                       uint8       * p_pDataLen );
    
    uint8 NLME_DeleteRow( uint8         p_ucAttributeID, 
                          uint32        p_ulTaiCutover, 
                          const uint8 * p_pIdxData,
                          uint8         p_ucIdxDataLen );

    uint8 NLMO_Read( uint16 p_unAttrID, uint16 * p_punLen, uint8 * p_pucBuffer);  
	uint8 NLMO_Write( uint16 p_unAttrID, uint8 p_ucBufferSize, const uint8 * p_pucBuffer) ;

	uint8 NLME_DeleteContract(uint16 p_u16ContractID);
    uint8 NLME_delATT( const uint8 * p_pIdxData );

    NLME_CONTRACT_ATTRIBUTES * NLME_FindContract(  uint16 p_unContractID );
    uint8 NLME_AddDmoContract( const DMO_CONTRACT_ATTRIBUTE * p_pDmoContract );
    void NLME_Alert( uint8 p_ucAlertValue, const uint8 * p_pNpdu, uint8 p_ucNpduLen );
    

    NLME_ROUTE_ATTRIBUTES * NLME_FindDestinationRoute( uint8*   p_ucDestAddress );
    
    NLME_ADDR_TRANS_ATTRIBUTES * NLME_FindATTByShortAddr( const uint8 * p_pShortAddr );
    NLME_ADDR_TRANS_ATTRIBUTES * NLME_FindATT( const uint8 * p_pIpv6Addr );
	NLME_ROUTE_ATTRIBUTES * NLME_findRoute( const uint8*   p_ucDestAddress ) ;

	uint8 NLME_AddContract( uint16 p_u16ContractID, const uint8* p_pu8SrcAddress, const uint8* p_pu8DestAddress, 
								uint8 p_u8Priority, bool p_bIncludeContractFlag, uint16 p_unAssignedMaxNSDUSize, 
								uint8 p_ucWndLen, int16 p_nComittedBurst, int16 p_nExcessBurst );

	uint8 NLME_setContract( uint32 p_ulTaiCutover, const uint8 * p_pData, uint16 p_unAssignedMaxNSDUSize, uint8 p_ucWndLen, int16 p_nComittedBurst, int16 p_nExcessBurst );
	uint8 NLME_AddATT (uint8* p_pu8LongAddress, uint16 p_u16ShortAddress);


	uint8 NLME_setRoute( uint32 p_ulTaiCutover, const uint8 * p_pData );
	uint8 NLME_getRoute( const uint8 * p_pIdxData, uint8 * p_pData );
	uint8 NLME_delRoute( const uint8 * p_pIdxData );

	uint8 NLME_AddRoute( const uint8* p_pu8Destination, const uint8* p_pu8NextHop, uint8 p_u8HopLinit, int p_nInterface );

	void NLME_PrintContracts(void);
	void NLME_PrintATT(void);
	void NLME_PrintRoutes(void);

extern NLME_ATRIBUTES g_stNlme;

  #define g_unShortAddr   g_stDMO.m_unShortAddr
  #define g_aIPv6Address  g_stDMO.m_auc128BitAddr


 

#endif // _NIVIS_NLME_H_
