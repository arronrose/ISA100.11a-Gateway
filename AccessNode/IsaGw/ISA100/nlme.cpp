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
/// Author:       Nivis LLC, Adrian Simionescu, Ion Ticus
/// Date:         January 2008
/// Description:  Implements Network Layer Management Entity of ISA100 standard
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "provision.h"
#include "nlme.h"
#include "string.h"
#include "sfc.h"
#include "dmap.h"
#include "dmap_armo.h"
#include "porting.h"

#include <arpa/inet.h>

 NLME_ATRIBUTES g_stNlme;

 
#if defined( BACKBONE_SUPPORT )
 const uint8  c_ucBackboneCapable = 1;
#else
 const uint8  c_ucBackboneCapable = 0;
#endif

 const uint8  c_ucDL_Capable = 1;
 const uint16 c_unMaxNsduSize = ((MAX_DATAGRAM_SIZE & 0xFF) << 8)  | (MAX_DATAGRAM_SIZE >> 8);

 const NLME_ATTRIBUTES_PARAMS c_aAttributesParams[ NLME_ATRBT_ID_NO - 1 ] =
 {
     // NLME_ATRBT_ID_BBR_CAPABLE
     {(uint8*)&c_ucBackboneCapable, sizeof(c_ucBackboneCapable), 0,  1 },   

     // NLME_ATRBT_ID_DL_CAPABLE
     {(uint8*)&c_ucDL_Capable, sizeof(c_ucDL_Capable), 0, 1 },             

     // NLME_ATRBT_ID_SHORT_ADDR
     {(uint8*)&g_unShortAddr, sizeof(g_unShortAddr), 0, 1 },  

     // NLME_ATRBT_ID_IPV6_ADDR
     {(uint8*)g_aIPv6Address, sizeof(g_aIPv6Address), 0, 1 },  

     // NLME_ATRBT_ID_ROUTE_TABLE
     { NULL, 16+16+1+1, 16, 0 },

     // NLME_ATRBT_ID_DEFAULT_ROUTE_FLAG
     {(uint8*)&g_stNlme.m_ucEnableDefaultRoute, sizeof(g_stNlme.m_ucEnableDefaultRoute), 0, 0 },  
     
    //  NLME_ATRBT_ID_DEFAULT_ROUTE_ID,
     {(uint8*)&g_stNlme.m_aDefaultRouteEntry, sizeof(g_stNlme.m_aDefaultRouteEntry), 0, 0 },  

    //  NLME_ATRBT_ID_CONTRACT_TABLE,
     { NULL, 35, 2+16, 0 },

    //  NLME_ATRBT_ID_ATT_TABLE,
     { NULL, 18, 16, 0 },

    //  NLME_ATRBT_ID_MAX_NSDU_SIZE,
     {(uint8*)&c_unMaxNsduSize, sizeof(c_unMaxNsduSize), 0, 1 },             

    //  NLME_ATRBT_ID_FRAG_REASSEMBLY_TIMEOUT,
     {(uint8*)&g_stNlme.m_ucFragReassemblyTimeout, sizeof(g_stNlme.m_ucFragReassemblyTimeout), 0, 0 },  

    //  NLME_ATRBT_ID_FRAG_DATAGRAM_TAG,
     {(uint8*)&g_stNlme.m_unFragDatagramTag, sizeof(g_stNlme.m_unFragDatagramTag), 0, 1 },  

     //  NLME_ATRBT_ID_ROUTE_TBL_META,
     { NULL,    4, 0, 1 },  

     //  NLME_ATRBT_ID_CONTRACT_TBL_META,
     { NULL,  4, 0, 1 },  

     //  NLME_ATRBT_ID_ATT_TBL_META,
     { NULL, 4, 0, 1 },  

     //  NLME_ATRBT_ID_ALERT_DESC,
     { NULL, 2, 0, 1 }  
};



  uint8 NLME_setATT( uint32 p_ulTaiCutover, const uint8 * p_pData );
  
  uint8 NLME_getContract( const uint8 * p_pIdxData, uint8 * p_pData  );
  uint8 NLME_getATT( const uint8 * p_pIdxData, uint8 * p_pData  );
  
  uint8 NLME_delContract( const uint8 * p_pIdxData );
//uint8 NLME_delATT( const uint8 * p_pIdxData );

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Reset NLME attributes
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
 void NLME_Init( void )
 {
	g_stNlme.m_unFragDatagramTag = 0;
	g_stNlme.m_ucFragReassemblyTimeout = 60;
	g_stNlme.m_ucEnableDefaultRoute = 1;
    memset( g_stNlme.m_aDefaultRouteEntry, 0,   sizeof(g_stNlme.m_aDefaultRouteEntry));
    
	g_stNlme.m_oNlmeRoutesMap.clear();
	g_stNlme.m_oContractsMap.clear();
	g_stNlme.m_oAttMap16toEntry.clear();
	g_stNlme.m_oAttMap2toEntry.clear();

    
 }

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Set a network layer SMIB row or MIB 
/// @param  p_ucAttributeID - attribute ID
/// @param  p_ulTaiCutover - TAI cutover (ignored)
/// @param  p_pData - value if MIB or index+index+value if SMIB
/// @param  p_ucDataLen - p_pData size
/// @return SFC code
/// @remarks
///      Access level: user level
//////////////////////////////////////////////////////////////////////////////////////////////////// 
uint8 NLME_SetRow( uint8         p_ucAttributeID, 
                   uint32        p_ulTaiCutover, 
                   const uint8 * p_pData,
                   uint8         p_ucDataLen )
{
	return NLME_SetRowWithDMOContractParams(p_ucAttributeID, p_ulTaiCutover, p_pData, p_ucDataLen, MAX_DATAGRAM_SIZE, 0, 0, 0);
}

uint8 NLME_SetRowWithDMOContractParams( uint8         p_ucAttributeID,
                   uint32        p_ulTaiCutover,
                   const uint8 * p_pData,
                   uint8         p_ucDataLen,
                   uint16        p_unAssignedMaxNSDUSize,
		   uint8	 p_ucWndLen,
                   int16         p_nComittedBurst,
                   int16         p_nExcessBurst )
{
    if( (--p_ucAttributeID) >= (NLME_ATRBT_ID_NO-1) )
    {
		LOG_ISA100(LOGLVL_ERR, "ERROR: NLME_SetRowWithDMOContractParams: AttributeID=%d invalid", p_ucAttributeID);
         return SFC_INVALID_ATTRIBUTE;
    }

    if( c_aAttributesParams[p_ucAttributeID].m_ucIsReadOnly )
    {
		LOG_ISA100(LOGLVL_ERR, "ERROR: NLME_SetRowWithDMOContractParams: AttributeID=%d read only", p_ucAttributeID);
        return SFC_READ_ONLY_ATTRIBUTE;
    }

    if( p_ucDataLen != (c_aAttributesParams[p_ucAttributeID].m_ucSize + c_aAttributesParams[p_ucAttributeID].m_ucIndexSize) )
    {
		LOG_ISA100(LOGLVL_ERR, "ERROR: NLME_SetRowWithDMOContractParams: AttributeID=%d len=%d invalid, should be %d + %d ", p_ucAttributeID, p_ucDataLen, 
			c_aAttributesParams[p_ucAttributeID].m_ucSize,  c_aAttributesParams[p_ucAttributeID].m_ucIndexSize);
        return SFC_INVALID_SIZE;
    }

    switch( p_ucAttributeID + 1 )
    {
        case NLME_ATRBT_ID_ROUTE_TABLE:    
			return NLME_setRoute( p_ulTaiCutover, p_pData );
        case NLME_ATRBT_ID_CONTRACT_TABLE: 
			if( p_ucDataLen != sizeof(NLME_CONTRACT_ATTRIBUTES_FOR_SETROW))
			{
				LOG_ISA100(LOGLVL_ERR,"PROGRAMMER ERROR: NLME_setContract() called with buffer of %d bytes instead of %d", p_ucDataLen, sizeof(NLME_CONTRACT_ATTRIBUTES_FOR_SETROW));
			}
			if (p_unAssignedMaxNSDUSize <= 8+4+4) // //TL header(8), security header(4), mic(4)
			{
				LOG_ISA100(LOGLVL_ERR,"PROGRAMMER ERROR: NLME_setContract() p_unAssignedMaxNSDUSize=%d <= %d", p_unAssignedMaxNSDUSize, 8+4+4);
			}
			return NLME_setContract( p_ulTaiCutover, p_pData, p_unAssignedMaxNSDUSize, p_ucWndLen, p_nComittedBurst, p_nExcessBurst );
        case NLME_ATRBT_ID_ATT_TABLE:      
			return NLME_setATT( p_ulTaiCutover, p_pData );
        case NLME_ATRBT_ID_ALERT_DESC:     
                        DMAP_WriteCompressAlertDescriptor( &g_stNlme.m_ucAlertDesc, p_pData, p_ucDataLen );
                        break;
	default:
			LOG_ISA100(LOGLVL_INF, "NLME_SetRowWithDMOContractParams: AttributeID=%d MIB", p_ucAttributeID);
                        memcpy( c_aAttributesParams[p_ucAttributeID].m_pValue, p_pData, p_ucDataLen );
    }

    return SFC_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Get a network layer SMIB row or MIB
/// @param  p_ucAttributeID - attribute ID
/// @param  p_pIdxData - index to identify SMIB row (ignored if MIB)
/// @param  p_ucIdxDataLen - size of index
/// @param  p_pData - value if MIB or index+value if SMIB
/// @param  p_pDataLen - p_pData size
/// @return SFC code
/// @remarks
///      Access level: user level
//////////////////////////////////////////////////////////////////////////////////////////////////// 
uint8 NLME_GetRow( uint8         p_ucAttributeID, 
                   const uint8 * p_pIdxData,
                   uint8         p_ucIdxDataLen,
                   uint8       * p_pData,
                   uint8       * p_pDataLen )
{
    if( (--p_ucAttributeID) >= (NLME_ATRBT_ID_NO-1) )
    {
         return SFC_INVALID_ATTRIBUTE;
    }

    *p_pDataLen = c_aAttributesParams[p_ucAttributeID].m_ucSize;

    if( p_ucIdxDataLen != c_aAttributesParams[p_ucAttributeID].m_ucIndexSize )
    {
        return SFC_INCONSISTENT_CONTENT;
    }

	uint16_t u16Tmp;
    switch( p_ucAttributeID + 1 )
    {
        case NLME_ATRBT_ID_ROUTE_TABLE:    return NLME_getRoute( p_pIdxData, p_pData );
        case NLME_ATRBT_ID_CONTRACT_TABLE: return NLME_getContract( p_pIdxData, p_pData );
        case NLME_ATRBT_ID_ATT_TABLE:      return NLME_getATT( p_pIdxData, p_pData );
    
    case NLME_ATRBT_ID_ROUTE_TBL_META: 

				u16Tmp = g_stNlme.m_oNlmeRoutesMap.size();
				
                  p_pData[0] = u16Tmp >> 8;        
                  p_pData[1] = u16Tmp; 
                  p_pData[2] = (uint16_t)MAX_ROUTES_NO >>8;
                  p_pData[3] = (byte)MAX_ROUTES_NO;
                  break;
                  
    case NLME_ATRBT_ID_CONTRACT_TBL_META: 

				u16Tmp = g_stNlme.m_oContractsMap.size();
				p_pData[0] = u16Tmp >> 8;        
				p_pData[1] = u16Tmp; 

                p_pData[2] = (uint16_t)MAX_CONTRACT_NO >> 8;
                p_pData[3] = (byte)MAX_CONTRACT_NO;
                break;
                  
    case NLME_ATRBT_ID_ATT_TBL_META: 

				u16Tmp =  g_stNlme.m_oAttMap16toEntry.size();
				p_pData[0] = u16Tmp >> 8;        
				p_pData[1] = u16Tmp; 

                p_pData[2] = (uint16_t)MAX_ADDR_TRANSLATIONS_NO >> 8;
                p_pData[3] = (byte)MAX_ADDR_TRANSLATIONS_NO;
                break;
      
    case NLME_ATRBT_ID_ALERT_DESC:   
                  DMAP_ReadCompressAlertDescriptor( &g_stNlme.m_ucAlertDesc, p_pData, p_pDataLen ); 
                  break;
    
    default:
                  memcpy( p_pData, c_aAttributesParams[p_ucAttributeID].m_pValue, c_aAttributesParams[p_ucAttributeID].m_ucSize);            
    }
    return SFC_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Get a network layer non-indexed attribute (it is a wraper to the NLME_GetRow function)
/// @param  p_ucAttributeID - attribute ID
/// @param  p_punLen - pointer; in/out parameter. in - length of available buffer; out - actual size
///                     of read data
/// @param p_pucBuffer  - pointer to a buffer where to put read data
/// @return SFC code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 NLMO_Read( uint16 p_unAttrID, uint16 * p_punLen, uint8 * p_pucBuffer)
{
	//Claudiu Hobeanu: code written to be less intrusive in original device stack 
	if (	p_unAttrID == NLME_ATRBT_ID_ROUTE_TABLE 
		||	p_unAttrID == NLME_ATRBT_ID_ATT_TABLE
		||	p_unAttrID == NLME_ATRBT_ID_DEFAULT_ROUTE_FLAG
		||	p_unAttrID == NLME_ATRBT_ID_DEFAULT_ROUTE_ID
		) 
	{  
		LOG_ISA100(LOGLVL_INF, "NLMO_Read: RequestForLinuxBbr unAttrID=%d", p_unAttrID );
		g_nRequestForLinuxBbr = 1;
		g_nATT_TR_Changed = 1;
	} 

  if( NLME_ATRBT_ID_ROUTE_TABLE == p_unAttrID  
      || NLME_ATRBT_ID_CONTRACT_TABLE == p_unAttrID
      || NLME_ATRBT_ID_ATT_TABLE == p_unAttrID
      || NLME_ATRBT_ID_CONTRACT_TBL_META == p_unAttrID )
  {
      return SFC_INVALID_ATTRIBUTE; // only mib requests are accepted
  }
  
  uint8 ucSFC;
  uint8 ucLen = *p_punLen;
  
  ucSFC = NLME_GetRow( p_unAttrID, NULL, 0, &ucLen, p_pucBuffer);

  *p_punLen = ucSFC;
  
  return ucSFC;  
}

//#define NLMO_Write(p_unAttrID,p_ucBufferSize,p_pucBuffer)   
uint8 NLMO_Write( uint16 p_unAttrID, uint8 p_ucBufferSize, const uint8 * p_pucBuffer) 
{
	//Claudiu Hobeanu: code written to be less intrusive in original device stack 
	if (	p_unAttrID == NLME_ATRBT_ID_ROUTE_TABLE 
		||	p_unAttrID == NLME_ATRBT_ID_ATT_TABLE
		||	p_unAttrID == NLME_ATRBT_ID_DEFAULT_ROUTE_FLAG
		||	p_unAttrID == NLME_ATRBT_ID_DEFAULT_ROUTE_ID
		) 
	{  
		LOG_ISA100(LOGLVL_INF, "NLMO_Write: RequestForLinuxBbr unAttrID=%d", p_unAttrID );
		g_nRequestForLinuxBbr = 1;
		g_nATT_TR_Changed = 1;
	} 
	return NLME_SetRow( p_unAttrID, 0, p_pucBuffer, p_ucBufferSize ) ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Delete a network layer SMIB row
/// @param  p_ucAttributeID - attribute ID
/// @param  p_ulTaiCutover - ignored
/// @param  p_pIdxData - index to identify SMIB row 
/// @param  p_ucIdxDataLen - size of index
/// @return SFC code
/// @remarks
///      Access level: user level
//////////////////////////////////////////////////////////////////////////////////////////////////// 
uint8 NLME_DeleteRow( uint8         p_ucAttributeID, 
                      uint32        p_ulTaiCutover, 
                      const uint8 * p_pIdxData,
                      uint8         p_ucIdxDataLen )
{
	LOG_ISA100(LOGLVL_INF, "NLME_DeleteRow: AttributeID=%d TaiCutover=%d p_ucIdxDataLen=%d", p_ucAttributeID, p_ulTaiCutover, p_ucIdxDataLen);

    if( (--p_ucAttributeID) >= (NLME_ATRBT_ID_NO-1) )
    {
		 LOG_ISA100(LOGLVL_ERR, "ERROR: NLME_DeleteRow: invalid AttributeID=%d", p_ucAttributeID+1);
         return SFC_INVALID_ATTRIBUTE;
    }

    if( c_aAttributesParams[p_ucAttributeID].m_ucIndexSize ) // SMIB
    {
        if( p_ucIdxDataLen != c_aAttributesParams[p_ucAttributeID].m_ucIndexSize )
        {
			LOG_ISA100(LOGLVL_ERR, "ERROR: NLME_DeleteRow: inconsistent AttributeID_index=%d, table_size=%d param_size=%d", 
					c_aAttributesParams[p_ucAttributeID].m_ucIndexSize, p_ucAttributeID, p_ucIdxDataLen);
            return SFC_INCONSISTENT_CONTENT;
        }

        switch( p_ucAttributeID + 1 )
        {
        case NLME_ATRBT_ID_ROUTE_TABLE:    return NLME_delRoute( p_pIdxData );
        case NLME_ATRBT_ID_CONTRACT_TABLE: return NLME_delContract( p_pIdxData );
        case NLME_ATRBT_ID_ATT_TABLE:      return NLME_delATT( p_pIdxData );
        }
    }

    return SFC_INCOMPATIBLE_MODE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Claudiu Hobeanu
/// @name  NLME_AddRoute()
//////////////////////////////////////////////////////////////////////////////////////////////////// 
uint8 NLME_AddRoute( const uint8* p_pu8Destination, const uint8* p_pu8NextHop, uint8 p_u8HopLinit, int p_nInterface )
{
	LOG_ISA100(LOGLVL_INF, "NLME_AddRoute: dest=%s next=%s limit=%d if=%s", 
				GetHex(p_pu8Destination, sizeof(IPV6_ADDR)), GetHex(p_pu8NextHop,sizeof(IPV6_ADDR)),
				p_u8HopLinit, p_nInterface == OutgoingInterfaceBBR ? "BBR" : "DL");

	uint8 buffer[sizeof(IPV6_ADDR) + sizeof(NLME_ROUTE_ATTRIBUTES)]; // + index size

	memcpy(buffer, p_pu8Destination, sizeof(IPV6_ADDR));

	NLME_ROUTE_ATTRIBUTES* pRoute = (NLME_ROUTE_ATTRIBUTES*)(buffer + sizeof(IPV6_ADDR));

	memcpy(pRoute->m_aDestAddress, p_pu8Destination, sizeof(pRoute->m_aDestAddress));
	memcpy(pRoute->m_aNextHopAddress, p_pu8NextHop, sizeof(pRoute->m_aNextHopAddress));
	pRoute->m_ucNWK_HopLimit = p_u8HopLinit;
	pRoute->m_bOutgoingInterface = p_nInterface;

	return NLME_setRoute(0, buffer);	
}


////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @author NIVIS LLC, Ion Ticus
    /// @brief  Set a network layer route
    /// @param  p_ulTaiCutover - ignored
    /// @param  p_pData - index of replacement + new route entry
    /// @return SFC code
    /// @remarks
    ///      Access level: user level\n
    ///      Context: BBR only since devices don't use that table
//////////////////////////////////////////////////////////////////////////////////////////////////// 
uint8 NLME_setRoute( uint32 p_ulTaiCutover, const uint8 * p_pData )
{  
	if (memcmp(p_pData, p_pData + c_aAttributesParams[NLME_ATRBT_ID_ROUTE_TABLE - 1].m_ucIndexSize,  c_aAttributesParams[NLME_ATRBT_ID_ROUTE_TABLE - 1].m_ucIndexSize))
	{
		LOG_ISA100(LOGLVL_INF, "NLME_setRoute: out index != in index");
		NLME_delRoute(p_pData);
	}
	// that because ISA100 decides to send idx twice
	p_pData += c_aAttributesParams[NLME_ATRBT_ID_ROUTE_TABLE - 1].m_ucIndexSize;

	CNlmeRoutesMap::iterator it = g_stNlme.m_oNlmeRoutesMap.find((uint8*)p_pData);

	LOG_ISA100(LOGLVL_INF, "NLME_setRoute: dest=%s found=%d (usage:%d/%d)", GetHex(p_pData,sizeof(IPV6_ADDR)), 
				(it != g_stNlme.m_oNlmeRoutesMap.end()), g_stNlme.m_oNlmeRoutesMap.size(), MAX_ROUTES_NO );

    if( it == g_stNlme.m_oNlmeRoutesMap.end() ) // not found, add it
    {
		if( g_stNlme.m_oNlmeRoutesMap.size() >= MAX_ROUTES_NO )
		{
			LOG_ISA100(LOGLVL_ERR, "NLME_setRoute: ERROR: SFC_INSUFFICIENT_DEVICE_RESOURCES");
			return SFC_INSUFFICIENT_DEVICE_RESOURCES;
		}
	
		CNlmeRoutePtr pRoutePtr (new NLME_ROUTE_ATTRIBUTES);
		//
		memcpy( pRoutePtr.get(), p_pData, c_aAttributesParams[NLME_ATRBT_ID_ROUTE_TABLE - 1].m_ucSize ); 

		//NOTE: on insert first pointer should point inside second object 
		g_stNlme.m_oNlmeRoutesMap.insert (std::make_pair((uint8*)pRoutePtr.get(), pRoutePtr));
    }
	else
	{
		// copy as it is
		memcpy( it->second.get(), p_pData, c_aAttributesParams[NLME_ATRBT_ID_ROUTE_TABLE - 1].m_ucSize ); 			
	}       
    
    return SFC_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @author NIVIS LLC, Ion Ticus
    /// @brief  Get a network layer route
    /// @param  p_pIdxData - route index
    /// @param  p_pData - route entry
    /// @return SFC code
    /// @remarks
    ///      Access level: user level\n
    ///      Context: BBR only since devices don't use that table
//////////////////////////////////////////////////////////////////////////////////////////////////// 
uint8 NLME_getRoute( const uint8 * p_pIdxData, uint8 * p_pData )
{
    NLME_ROUTE_ATTRIBUTES * pRoute = NLME_findRoute(p_pIdxData);
    if( !pRoute )
    {
        return SFC_INVALID_ELEMENT_INDEX;
    }

    // copy as it is
    memcpy( p_pData, (uint8*)pRoute, c_aAttributesParams[NLME_ATRBT_ID_ROUTE_TABLE - 1].m_ucSize ); 
    return SFC_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @author NIVIS LLC, Ion Ticus
    /// @brief  Delete a network layer route
    /// @param  p_pIdxData - route index
    /// @return SFC code
    /// @remarks
    ///      Access level: user level\n
    ///      Context: BBR only since devices don't use that table
//////////////////////////////////////////////////////////////////////////////////////////////////// 
uint8 NLME_delRoute( const uint8 * p_pIdxData )
{
	CNlmeRoutesMap::iterator it = g_stNlme.m_oNlmeRoutesMap.find((uint8*)p_pIdxData);

	LOG_ISA100(LOGLVL_INF, "NLME_delRoute: dest=%s found=%d (usage:%d/%d)", GetHex(p_pIdxData,sizeof(IPV6_ADDR)), 
		(it != g_stNlme.m_oNlmeRoutesMap.end()), g_stNlme.m_oNlmeRoutesMap.size(), MAX_ROUTES_NO );

	if (it != g_stNlme.m_oNlmeRoutesMap.end())
	{
		g_stNlme.m_oNlmeRoutesMap.erase(it);
	}

    return SFC_SUCCESS;
}

 ////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Find a route on table
/// @param  p_ucDestAddress - IPv6 dest address (index on route table) 
/// @return pointer to route, null if not found
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
NLME_ROUTE_ATTRIBUTES * NLME_findRoute( const uint8*   p_ucDestAddress ) 
{
	CNlmeRoutesMap::iterator it = g_stNlme.m_oNlmeRoutesMap.find((uint8*)p_ucDestAddress);

	if (it == g_stNlme.m_oNlmeRoutesMap.end())
	{
		return NULL;
	}

	return it->second.get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Find the nwk route, otherwise use default
/// @param  p_ucDestAddress - IPv6 dest address (index on route table) 
/// @return pointer to route, null if not found
/// @remarks
///      Access level: user level
///      Obs: if returned index = g_ucRoutesNo means no route found
////////////////////////////////////////////////////////////////////////////////////////////////////
NLME_ROUTE_ATTRIBUTES * NLME_FindDestinationRoute( uint8*   p_ucDestAddress )
{
  NLME_ROUTE_ATTRIBUTES * pRoute = NLME_findRoute( p_ucDestAddress );

  if( !pRoute && g_stNlme.m_ucEnableDefaultRoute )
  {
        return NLME_findRoute( g_stNlme.m_aDefaultRouteEntry );
  }

  return pRoute;
}
  
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Claudiu Hobeanu
/// @name  NLME_AddContract()
//////////////////////////////////////////////////////////////////////////////////////////////////// 
uint8 NLME_AddContract( uint16 p_u16ContractID, const uint8* p_pu8SrcAddress, const uint8* p_pu8DestAddress, 
					   uint8 p_u8Priority, bool p_bIncludeContractFlag,	uint16 p_unAssignedMaxNSDUSize, 
					   uint8 p_ucWndLen, int16 p_nComittedBurst, int16 p_nExcessBurst )
{
	LOG_ISA100(LOGLVL_INF, "NLME_AddContract: id=%d dest=%s prio=%d includeFlag=%d maxNSDU=%d wndLen=%d commit=%d excess=%d",
							p_u16ContractID, GetHex(p_pu8DestAddress, 16), p_u8Priority, p_bIncludeContractFlag,
							p_unAssignedMaxNSDUSize, p_ucWndLen, p_nComittedBurst, p_nExcessBurst);

	NLME_CONTRACT_ATTRIBUTES_FOR_SETROW contract;

	memset(&contract, 0, sizeof(NLME_CONTRACT_ATTRIBUTES_FOR_SETROW));

	contract.m_stIdx.m_nContractId = contract.m_stData.m_nContractId = (uint16)htons(p_u16ContractID);
	memcpy(contract.m_stIdx.m_aOwnIPv6, p_pu8SrcAddress, sizeof(contract.m_stIdx.m_aOwnIPv6));
	memcpy(contract.m_stData.m_aOwnIPv6, p_pu8SrcAddress,sizeof(contract.m_stIdx.m_aOwnIPv6));
	memcpy(contract.m_stData.m_aPeerIPv6, p_pu8DestAddress, sizeof(contract.m_stData.m_aPeerIPv6));
	contract.m_stData.m_cFlags = ((uint8)p_u8Priority & 0x03) | (uint8)(p_bIncludeContractFlag ? 1 << 2 : 0);
	
	return NLME_setContract(0, (uint8*)&contract, p_unAssignedMaxNSDUSize, p_ucWndLen, p_nComittedBurst, p_nExcessBurst);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Set a network layer contract
/// @param  p_ulTaiCutover - ignored
/// @param  p_pData - index of replacement + new contract entry
/// @return SFC code
/// @remarks
///      Access level: user level
//////////////////////////////////////////////////////////////////////////////////////////////////// 
uint8 NLME_setContract( uint32 p_ulTaiCutover, const uint8 * p_pData, uint16 p_unAssignedMaxNSDUSize, uint8 p_ucWndLen, int16 p_nComittedBurst, int16 p_nExcessBurst )
{
	 // that because ISA100 decides to send idx twice
	//p_pData += c_aAttributesParams[NLME_ATRBT_ID_CONTRACT_TABLE - 1].m_ucIndexSize;

	NLME_CONTRACT_ATTRIBUTES_FOR_SETROW * pContractReq = ( NLME_CONTRACT_ATTRIBUTES_FOR_SETROW *)p_pData;
	if ( memcmp( pContractReq->m_stData.m_aOwnIPv6, g_aIPv6Address, 16 ) ) // Accept my TX contract only
	{
		LOG_ISA100(LOGLVL_ERR, "NLME_setContract: not for me %s", GetHex(pContractReq->m_stData.m_aOwnIPv6, sizeof(pContractReq->m_stData.m_aOwnIPv6)));
		return SFC_INVALID_ELEMENT_INDEX;
	}

	if (memcmp(p_pData, p_pData + c_aAttributesParams[NLME_ATRBT_ID_CONTRACT_TABLE - 1].m_ucIndexSize,  c_aAttributesParams[NLME_ATRBT_ID_CONTRACT_TABLE - 1].m_ucIndexSize))
	{
		LOG_ISA100(LOGLVL_INF, "NLME_setContract: out index != in index");
		NLME_delContract(p_pData);
	}

    uint16 u16ContractID = (((uint16)(p_pData[0])) << 8) | p_pData[1];
    
	CNlmeContractsMap::iterator it = g_stNlme.m_oContractsMap.find (u16ContractID);	

	LOG_ISA100(LOGLVL_INF, "NLME_setContract: contractID=%d found=%d", u16ContractID, (it != g_stNlme.m_oContractsMap.end()) );

	
	CNlmeContractPtr pContract;

	if (it == g_stNlme.m_oContractsMap.end())
	{
		if( g_stNlme.m_oContractsMap.size() >= MAX_CONTRACT_NO )
		{
			LOG_ISA100(LOGLVL_ERR, "ERROR: SFC_INSUFFICIENT_DEVICE_RESOURCES contract %d (usage:%d/%d)", u16ContractID, g_stNlme.m_oContractsMap.size(), MAX_CONTRACT_NO );
			return SFC_INSUFFICIENT_DEVICE_RESOURCES;
		}
		
		pContract.reset(new NLME_CONTRACT_ATTRIBUTES);

		pContract->m_ucReqID = random();
		g_stNlme.m_oContractsMap.insert(std::make_pair(u16ContractID,pContract));		
		LOG_ISA100(LOGLVL_INF, "NLME_setContract: Adding contract %d (usage:%d/%d)", u16ContractID, g_stNlme.m_oContractsMap.size(), MAX_CONTRACT_NO );
	}
	else
	{	pContract = it->second;   
	}
	uint8 savedReqId = pContract->m_ucReqID;
	memset(pContract.get(), 0, sizeof(NLME_CONTRACT_ATTRIBUTES));
	pContract->m_ucReqID = savedReqId;

    pContract->m_unContractID = u16ContractID;
    memcpy( pContract->m_aDestAddress, pContractReq->m_stData.m_aPeerIPv6, sizeof(pContractReq->m_stData.m_aPeerIPv6) ); 
    pContract->m_nRTO = (-p_nComittedBurst) << 1; // 2 * ComittedBurst

	if ( pContract->m_nRTO < 3)
	{
		pContract->m_nRTO = 3;
	}

	if (pContract->m_nRTO > 60)
	{
		pContract->m_nRTO = 60;
	}

    pContract->m_nSRTT = pContract->m_nRTTV = 0;
	pContract->m_unAssignedMaxNSDUSize = p_unAssignedMaxNSDUSize;
	pContract->m_ucMaxSendWindow = p_ucWndLen;
	pContract->m_nCrtWndSize = 1;
	pContract->m_nInUseWndSize = 0;
	pContract->m_nSuccessTransmissions = 0;
	pContract->m_nComittedBurst = p_nComittedBurst;
	pContract->m_nExcessBurst = p_nExcessBurst;
    
    return SFC_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Get a network layer contract
/// @param  p_pIdxData - contract index
/// @param  p_pData - contract entry
/// @return SFC code
/// @remarks
///      Access level: user level
//////////////////////////////////////////////////////////////////////////////////////////////////// 
uint8 NLME_getContract( const uint8 * p_pIdxData, uint8 * p_pData )
{
    NLME_CONTRACT_ATTRIBUTES * pContract;

    pContract = NLME_FindContract( (((uint16)(p_pIdxData[0])) << 8) | p_pIdxData[1]);    
    if( !pContract )
    {
        return SFC_INVALID_ELEMENT_INDEX;
    }

    *(p_pData++) = pContract->m_unContractID >> 8;
    *(p_pData++) = pContract->m_unContractID & 0xFF;
    
    memcpy( p_pData, g_aIPv6Address, 16 ); // Only my contracts     
    memcpy( p_pData+16, pContract->m_aDestAddress, c_aAttributesParams[NLME_ATRBT_ID_CONTRACT_TABLE - 1].m_ucSize-2-16 ); 
    
    if( c_aAttributesParams[NLME_ATRBT_ID_CONTRACT_TABLE - 1].m_ucSize != sizeof(NLME_CONTRACT_ATTRIBUTES_FOR_SETROW)-offsetof(NLME_CONTRACT_ATTRIBUTES_FOR_SETROW, m_stData))
    {
	LOG_ISA100(LOGLVL_ERR,"Programmer ERROR: NLME_getContract() delivering %d bytes instead of %d",
		c_aAttributesParams[NLME_ATRBT_ID_CONTRACT_TABLE - 1].m_ucSize, sizeof(NLME_CONTRACT_ATTRIBUTES_FOR_SETROW)-offsetof(NLME_CONTRACT_ATTRIBUTES_FOR_SETROW, m_stData));
    }
    return SFC_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Delete a network layer contract
/// @param  p_pIdxData - contract index
/// @return SFC code
/// @remarks
///      Access level: user level
//////////////////////////////////////////////////////////////////////////////////////////////////// 
uint8 NLME_delContract( const uint8 * p_pIdxData )
{
	uint16 u16ContractID = (((uint16)(p_pIdxData[0])) << 8) | p_pIdxData[1];

	CNlmeContractsMap::iterator it = g_stNlme.m_oContractsMap.find (u16ContractID);	

	LOG_ISA100(LOGLVL_INF, "NLME_delContract: contractID=%d found=%d", u16ContractID, (it != g_stNlme.m_oContractsMap.end()) );

	if (it != g_stNlme.m_oContractsMap.end())
	{
		g_stNlme.m_oContractsMap.erase(it);
	}

    return SFC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Claudiu Hobeanu
/// @name  NLME_DeleteContract()
//////////////////////////////////////////////////////////////////////////////////////////////////// 
uint8 NLME_DeleteContract(uint16 p_u16ContractID)
{
	LOG_ISA100(LOGLVL_INF, "NLME_DeleteContract: contractID=%d",p_u16ContractID);

	uint8 pu8Data[2];
	pu8Data[0] = (uint8)(p_u16ContractID >> 8);
	pu8Data[1] = (uint8)p_u16ContractID ;

	return NLME_delContract(pu8Data);
}


uint8 NLME_AddATT (uint8* p_pu8LongAddress, uint16 p_u16ShortAddress)
{
	uint8 pu8Buff[ 2 * sizeof(IPV6_ADDR) + 2];

	LOG_ISA100(LOGLVL_INF, "NLME_AddATT: long=%s short=%04X", GetHexC(p_pu8LongAddress,sizeof(IPV6_ADDR)), p_u16ShortAddress);

	memcpy(pu8Buff, p_pu8LongAddress,sizeof(IPV6_ADDR));
	memcpy(pu8Buff + sizeof(IPV6_ADDR), p_pu8LongAddress,sizeof(IPV6_ADDR));

	pu8Buff[ 2 * sizeof(IPV6_ADDR)] = p_u16ShortAddress >> 8;
	pu8Buff[ 2 * sizeof(IPV6_ADDR) + 1] = p_u16ShortAddress >> 8;

	return NLME_setATT (0, pu8Buff);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Set a network layer ATT entry
/// @param  p_ulTaiCutover - ignored
/// @param  p_pData - index of replacement + new ATT entry
/// @return SFC code
/// @remarks
///      Access level: user level
//////////////////////////////////////////////////////////////////////////////////////////////////// 
uint8 NLME_setATT( uint32 p_ulTaiCutover, const uint8 * p_pData )
{
	if (memcmp(p_pData, p_pData + c_aAttributesParams[NLME_ATRBT_ID_ATT_TABLE - 1].m_ucIndexSize,  c_aAttributesParams[NLME_ATRBT_ID_ATT_TABLE - 1].m_ucIndexSize))
	{
		LOG_ISA100(LOGLVL_INF, "NLME_setATT: out index != in index");
		NLME_delATT(p_pData);
	}
	// that because ISA100 decides to send idx twice
	p_pData += c_aAttributesParams[NLME_ATRBT_ID_ATT_TABLE - 1].m_ucIndexSize;

	NLME_ADDR_TRANS_ATTRIBUTES * pAttReq = (NLME_ADDR_TRANS_ATTRIBUTES *)p_pData;

	CNlmeAttMap2toEntry::iterator itShort = g_stNlme.m_oAttMap2toEntry.find(pAttReq->m_aShortAddress);
	CNlmeAttMap16toEntry::iterator itLong = g_stNlme.m_oAttMap16toEntry.find(pAttReq->m_aIPV6Addr);

	LOG_ISA100(LOGLVL_INF, "NLME_setATT: dest=%s foundLong=%d foundShort=%d", GetHexC(p_pData, sizeof(IPV6_ADDR)), 
		itLong != g_stNlme.m_oAttMap16toEntry.end(), itShort != g_stNlme.m_oAttMap2toEntry.end()  );


	if (itLong != g_stNlme.m_oAttMap16toEntry.end() && itShort != g_stNlme.m_oAttMap2toEntry.end())
	{
		if (itLong->second.get() == itShort->second.get())
		{
			return SFC_SUCCESS;
		}
		else
		{
			LOG_ISA100(LOGLVL_ERR,"ERROR: NLME_setATT: short and long addrs exist but in different entries!");
			return SFC_INCONSISTENT_CONTENT;
		}
	}

	if (itShort != g_stNlme.m_oAttMap2toEntry.end())
	{
		//itLong does not exist
		LOG_ISA100(LOGLVL_ERR,"ERROR: NLME_setATT: short addr exist in other entry!");
		return SFC_INCONSISTENT_CONTENT;
	}


	if (itLong != g_stNlme.m_oAttMap16toEntry.end())
	{
		NLME_delATT(p_pData);
	}

	if (g_stNlme.m_oAttMap16toEntry.size() >= MAX_ADDR_TRANSLATIONS_NO)
	{
		LOG_ISA100(LOGLVL_ERR,"ERROR: NLME_setATT: maxim size reached %d", MAX_ADDR_TRANSLATIONS_NO);
		return SFC_INSUFFICIENT_DEVICE_RESOURCES;
	}
	
	CNlmeAttPtr pAttPtr (new NLME_ADDR_TRANS_ATTRIBUTES);

	memcpy( pAttPtr.get(), p_pData, c_aAttributesParams[NLME_ATRBT_ID_ATT_TABLE - 1].m_ucSize ); 
	
	//NOTE: on insert first pointer should point inside second object 
	NLME_ADDR_TRANS_ATTRIBUTES * pAttSet = (NLME_ADDR_TRANS_ATTRIBUTES *) pAttPtr.get();

	g_stNlme.m_oAttMap16toEntry.insert(std::make_pair(pAttSet->m_aIPV6Addr,pAttPtr));   
	g_stNlme.m_oAttMap2toEntry.insert(std::make_pair(pAttSet->m_aShortAddress,pAttPtr));   
    
    return SFC_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Get a network layer ATT entry
/// @param  p_pIdxData - ATT index
/// @param  p_pData - ATT entry
/// @return SFC code
/// @remarks
///      Access level: user level
//////////////////////////////////////////////////////////////////////////////////////////////////// 
uint8 NLME_getATT( const uint8 * p_pIdxData, uint8 * p_pData )
{
    NLME_ADDR_TRANS_ATTRIBUTES * pAtt = NLME_FindATT( p_pIdxData );
    if( !pAtt )
    {
        return SFC_INVALID_ELEMENT_INDEX;
    }

// copy as it is
    memcpy( p_pData, (uint8*)pAtt, c_aAttributesParams[NLME_ATRBT_ID_ATT_TABLE - 1].m_ucSize ); 

    return SFC_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Delete a network layer ATT entry
/// @param  p_pIdxData - ATT index
/// @return SFC code
/// @remarks
///      Access level: user level
//////////////////////////////////////////////////////////////////////////////////////////////////// 
uint8 NLME_delATT( const uint8 * p_pIdxData )
{
	CNlmeAttMap16toEntry::iterator it = g_stNlme.m_oAttMap16toEntry.find((uint8*)p_pIdxData);

	LOG_ISA100(LOGLVL_INF, "NLME_delATT: dest=%s found=%d", GetHexC(p_pIdxData, 16), (it != g_stNlme.m_oAttMap2toEntry.end()) );

	if (it == g_stNlme.m_oAttMap16toEntry.end())
	{
		return SFC_INVALID_ELEMENT_INDEX;
	}

	NLME_ADDR_TRANS_ATTRIBUTES * pAtt = it->second.get();
	
	CNlmeAttMap2toEntry::iterator itShort = g_stNlme.m_oAttMap2toEntry.find(pAtt->m_aShortAddress); 
	
	if (itShort == g_stNlme.m_oAttMap2toEntry.end())
	{
		LOG_ISA100(LOGLVL_ERR, "NLME_delATT: ERROR: cannot find %s", GetHexC(pAtt->m_aShortAddress, 2));
	}
	else
	{
		g_stNlme.m_oAttMap2toEntry.erase(itShort);
	}
	
	g_stNlme.m_oAttMap16toEntry.erase(it);
    return SFC_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Adrian Simionescu
/// @brief  Find a contract on table
/// @param  p_unContractId - the contract id
/// @return contract index
/// @remarks
///      Access level: user level
///      Obs: if returned index = g_ucContractNo means no contract found
////////////////////////////////////////////////////////////////////////////////////////////////////
NLME_CONTRACT_ATTRIBUTES * NLME_FindContract (  uint16 p_unContractID )
{
	CNlmeContractsMap::iterator it = g_stNlme.m_oContractsMap.find (p_unContractID);	

	if (it == g_stNlme.m_oContractsMap.end())
	{
		return NULL;
	}

	return it->second.get();
}  



////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Add a DMO contract on NL contract table 
//          (keep consistency until ISA100 will decide to unify those 2 tables - not on ISA spec)
/// @param  p_pDmoContract - the DMO contract
/// @return SFC code
/// @remarks
///      Access level: user level\n
///      Obs: For a success result SM need to load DMO contract after NL contract 
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 NLME_AddDmoContract( const DMO_CONTRACT_ATTRIBUTE * p_pDmoContract )
{
	CNlmeContractsMap::iterator it = g_stNlme.m_oContractsMap.find (p_pDmoContract->m_unContractID);	

	if (it != g_stNlme.m_oContractsMap.end())
	{
		LOG_ISA100(LOGLVL_INF, "NLME_AddDmoContract: contractID=%d already exist -> set the DMO parameters", p_pDmoContract->m_unContractID  );
		CNlmeContractPtr pNLContract = it->second;
		pNLContract->m_unAssignedMaxNSDUSize = p_pDmoContract->m_unAssignedMaxNSDUSize;
		pNLContract->m_ucMaxSendWindow = p_pDmoContract->m_stBandwidth.m_stAperiodic.m_ucMaxSendWindow;
		pNLContract->m_nComittedBurst = p_pDmoContract->m_stBandwidth.m_stAperiodic.m_nComittedBurst;
		pNLContract->m_nExcessBurst = p_pDmoContract->m_stBandwidth.m_stAperiodic.m_nExcessBurst;
		pNLContract->m_nRTO = (-p_pDmoContract->m_stBandwidth.m_stAperiodic.m_nComittedBurst) << 1; // 2 * ComittedBurst
		if (pNLContract->m_nRTO < 3)  pNLContract->m_nRTO = 3;
		if (pNLContract->m_nRTO > 60) pNLContract->m_nRTO = 60;
		return SFC_SUCCESS;
	}

	if (g_stNlme.m_oContractsMap.size() >= MAX_CONTRACT_NO)
	{
		return SFC_INSUFFICIENT_DEVICE_RESOURCES;
	}

	LOG_ISA100(LOGLVL_INF, "NLME_AddDmoContract: Adding contract %d (usage:%d/%d)", p_pDmoContract->m_unContractID, g_stNlme.m_oContractsMap.size(), MAX_CONTRACT_NO );

	CNlmeContractPtr pContract(new NLME_CONTRACT_ATTRIBUTES);

	g_stNlme.m_oContractsMap.insert(std::make_pair(p_pDmoContract->m_unContractID,pContract));		

	pContract->m_unContractID = p_pDmoContract->m_unContractID;
	memcpy( pContract->m_aDestAddress, p_pDmoContract->m_aDstAddr128, sizeof(pContract->m_aDestAddress) );
	pContract->m_bPriority = p_pDmoContract->m_ucPriority;
	pContract->m_bIncludeContractFlag = 0; // missing info
	pContract->m_nRTO = (-p_pDmoContract->m_stBandwidth.m_stAperiodic.m_nComittedBurst) << 1; // 2 * ComittedBurst

	if (pContract->m_nRTO < 3)
	{
		pContract->m_nRTO = 3;
	}

	if (pContract->m_nRTO > 60)
	{
		pContract->m_nRTO = 60;
	}

	pContract->m_nSRTT = pContract->m_nRTTV = 0;
	pContract->m_unAssignedMaxNSDUSize = p_pDmoContract->m_unAssignedMaxNSDUSize;
	pContract->m_ucMaxSendWindow = p_pDmoContract->m_stBandwidth.m_stAperiodic.m_ucMaxSendWindow;
	pContract->m_nCrtWndSize = 1;
	pContract->m_nInUseWndSize = 0;
	pContract->m_nSuccessTransmissions = 0;
	pContract->m_ucReqID = random();
	pContract->m_nComittedBurst = p_pDmoContract->m_stBandwidth.m_stAperiodic.m_nComittedBurst;
	pContract->m_nExcessBurst = p_pDmoContract->m_stBandwidth.m_stAperiodic.m_nExcessBurst;

	return SFC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Find ATT entry using 16 bit short address (unique on subnet)
/// @return ATT entry if success, NULL if not found 
/// @remarks
///      Access level: Interrupt level or user level
///////////////////////////////////////////////////////////////////////////////////
NLME_ADDR_TRANS_ATTRIBUTES * NLME_FindATTByShortAddr ( const uint8 * p_pShortAddr )
{
	CNlmeAttMap2toEntry::iterator it = g_stNlme.m_oAttMap2toEntry.find((uint8*)p_pShortAddr);

	if (it == g_stNlme.m_oAttMap2toEntry.end())
	{
		return NULL;
	}
	return it->second.get();	
}

///////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Find ATT entry using IPV6 address (ATT index)
/// @return ATT entry if success, NULL if not found 
/// @remarks
///      Access level: Interrupt level or user level
///////////////////////////////////////////////////////////////////////////////////
NLME_ADDR_TRANS_ATTRIBUTES * NLME_FindATT ( const uint8 * p_pIpv6Addr )
{
	CNlmeAttMap16toEntry::iterator it = g_stNlme.m_oAttMap16toEntry.find((uint8*)p_pIpv6Addr);

	if (it == g_stNlme.m_oAttMap16toEntry.end())
	{
		return NULL;
	}
	return it->second.get();	
}

void NLME_Alert( uint8 p_ucAlertValue, const uint8 * p_pNpdu, uint8 p_ucNpduLen )
{
    if( g_stNlme.m_ucAlertDesc & 0x80 )
    {
        ALERT stAlert;
        
        uint8 ucAlertData[40];
        
        if( p_ucNpduLen >= sizeof(ucAlertData) - 2 )
        {
            p_ucNpduLen = sizeof(ucAlertData) - 2;
        }
        
        stAlert.m_ucPriority = g_stNlme.m_ucAlertDesc & 0x7F;
        stAlert.m_unDetObjTLPort = 0xF0B0; // TLME is DMAP port
        stAlert.m_unDetObjID = DMAP_NLMO_OBJ_ID; 
        stAlert.m_ucClass = ALERT_CLASS_EVENT; 
        stAlert.m_ucDirection = ALARM_DIR_IN_ALARM; 
        stAlert.m_ucCategory = ALERT_CAT_COMM_DIAG; 
        stAlert.m_ucType = 0; // NL_Dropped_PDU                     
        stAlert.m_unSize = p_ucNpduLen+2; 
        
        ucAlertData[0] = p_ucNpduLen+2;
        ucAlertData[1] = p_ucAlertValue;
        memcpy( ucAlertData+2, p_pNpdu, p_ucNpduLen );
        
        ARMO_AddAlertToQueue( &stAlert, ucAlertData );
        
    }
}

void NLME_PrintATT(void)
{
	CNlmeAttMap16toEntry::iterator it = g_stNlme.m_oAttMap16toEntry.begin();

	LOG_ISA100(LOGLVL_ERR, "  ATT TABLE    Long Addr/IPv6     Short Addr");
	LOG_ISA100(LOGLVL_ERR, "--------------------------------  ----------");
	for(; it != g_stNlme.m_oAttMap16toEntry.end(); it++)  // look in Address Translation Table
	{
		CNlmeAttPtr& pAtt = it->second;
		LOG_ISA100(LOGLVL_ERR,"%s        %s", GetHex(pAtt->m_aIPV6Addr, sizeof(pAtt->m_aIPV6Addr)), GetHex(pAtt->m_aShortAddress,2));
	}
	LOG_ISA100(LOGLVL_ERR, "--------------------------------  ----------");
}


void NLME_PrintRoutes(void)
{	char ipv61[40],ipv62[40];
	NLME_ROUTE_ATTRIBUTES * pRoute = NULL;

	if (g_stNlme.m_aDefaultRouteEntry && (pRoute = NLME_findRoute(g_stNlme.m_aDefaultRouteEntry)))
		LOG_ISA100(LOGLVL_ERR, "  ROUTE TABLE: Total %d, Default Route %s %s", g_stNlme.m_oNlmeRoutesMap.size(),
		pRoute->m_bOutgoingInterface == OutgoingInterfaceBBR ? "BBR" : "DL" , GetHex(g_stNlme.m_aDefaultRouteEntry, sizeof(g_stNlme.m_aDefaultRouteEntry)));
	else
		LOG_ISA100(LOGLVL_ERR, "  ROUTE TABLE: Total %d, default route NONE", g_stNlme.m_oNlmeRoutesMap.size());

	LOG_ISA100(LOGLVL_ERR, "        IPv6DstAddress                          IPv6NextHopAddress              TTL If");
	LOG_ISA100(LOGLVL_ERR, "--------------------------------------- --------------------------------------- --- --");
	CNlmeRoutesMap::iterator it = g_stNlme.m_oNlmeRoutesMap.begin();

	for( ; it != g_stNlme.m_oNlmeRoutesMap.end(); it++ )
	{
		CNlmeRoutePtr pRoutePtr = it->second;

		FormatIPv6( pRoutePtr->m_aDestAddress, ipv61);
		FormatIPv6(  pRoutePtr->m_aNextHopAddress, ipv62);
		LOG_ISA100(LOGLVL_ERR, "%s %s %3u %u", ipv61, ipv62,	pRoutePtr->m_ucNWK_HopLimit, pRoutePtr->m_bOutgoingInterface);
	}
	LOG_ISA100(LOGLVL_ERR, "--------------------------------------- --------------------------------------- --- --");
}



void NLME_PrintContracts(void)
{	long delaySec;
	char ipv6[40];
	
	MLSM_GetCrtTaiSec();
	LOG_ISA100(LOGLVL_ERR, "  NLME CONTRACT TABLE: Total %d,  SysMngContractID %d", g_stNlme.m_oContractsMap.size(), g_unSysMngContractID);
	LOG_ISA100(LOGLVL_ERR, "   ID         IPv6DstAddress                 RTry Wnd MaxNS CmtdBrst MaxWnd Delay");
	LOG_ISA100(LOGLVL_ERR, "----- --------------------------------------- --- --- ----- -------- ------ -----");

	for(CNlmeContractsMap::iterator it = g_stNlme.m_oContractsMap.begin(); it !=  g_stNlme.m_oContractsMap.end(); ++it )
	{
		CNlmeContractPtr pContract = it->second;

		FormatIPv6( pContract->m_aDestAddress, ipv6);
		delaySec = pContract->m_stSendNoEarlierThan.tv_sec - tvNow.tv_sec;
		if(delaySec < 0){
			delaySec = 0;
		}
		LOG_ISA100(LOGLVL_ERR, "%5u %s %3d %3u %5u %8d %6u %u", pContract->m_unContractID, ipv6,
			pContract->m_nRTO, pContract->m_nCrtWndSize,
			pContract->m_unAssignedMaxNSDUSize, pContract->m_nComittedBurst,
			pContract->m_ucMaxSendWindow, delaySec);
	}
	LOG_ISA100(LOGLVL_ERR, "----- --------------------------------------- --- --- ----- -------- ------ -----");
}
