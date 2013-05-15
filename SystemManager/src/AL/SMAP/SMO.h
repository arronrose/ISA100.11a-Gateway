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

/**
 * @author sorin.bidian, catalin.pop, flori.parauan, Beniamin Tecar
 */
#ifndef SMO_H_
#define SMO_H_

#include "ASL/ASLStatistics.h"
#include "AL/Isa100Object.h"
#include "Common/NEException.h"
#include "Common/NETypes.h"
#include "Model/Device.h"
#include "Model/Routing/RouteTypes.h"


namespace Isa100 {
namespace AL {
namespace SMAP {

#define MAX_HEADER_SIZE 26  // as computed by Mihai Buha
struct HandlerGenerator {
        /**
         * Unique generated handler.
         */
        static Uint32 newHandler;

        /**
         * Generates a unique handler.
         */
        static Uint32 createHandler();
};

/**
 * System Monitoring Object (SMO)
 * @author  Ioan v. Pocol, sorin.bidian, catalin.pop, flori.parauan, Beniamin Tecar
 * @version 1.0, 03.04.2008
 * @version 2.0, D2A
 * @version 3.0, New SystemManager Design - October 2009
 */
class SMO: public Isa100Object {

        LOG_DEF("I.A.S.SMO");

    public:
        static const Uint8 RESTART_TYPE = 13;

    private:

        BytesPointer executeRequestParameters;

        time_t lastCheckRejoinsTime;

        /**
         * Holds a mapping between a unique generated handler and a corresponding generated report.
         */
        typedef std::map<Uint32, BytesPointer> ReportsMap;
        ReportsMap reportsMap;

        /**
         * Used to hold a flag for each generated report, reflecting whether the report has been completely read.
         * Each flag is set to 'true' when the last block of the corresponding report is read.
         */
        typedef std::map<Uint32, bool> ReportsReadFlags;
        ReportsReadFlags reportsReadFlags;

    public:

        void indicateExecute(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        SMO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine);

        virtual ~SMO();

        void execute(Uint32 currentTime){}

        Isa100::AL::ObjectID::ObjectIDEnum getObjectID() const{
            return Isa100::AL::ObjectID::ID_SMO;
        }

        bool expectIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        bool expectConfirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);

    private:

        void confirmWrite(Isa100::ASL::Services::PrimitiveConfirmationPointer confirmation);

        /**
         * This method generates any combination of Device list / Topology / Schedule / Device Health / Neighbor health / Network health reports.
         * Each report will get it�s own unique handle.
         */
        BytesPointer generateReport(BytesPointer parameters);

        /**
         * The method is executed repeatedly (with different offset and size parameters) after a generateReport call,
         * until the whole report structure is transferred. The handler is used to identify what structure to return.
         */
        void getBlock(BytesPointer parameters);

        /**
         * Returns the contracts and routes tables for all the devices.
         */
        void getContractsAndRoutes();

        /**
         * Returns the CCA backoff on all 16 channels of the device specified as parameter.
         */
        void getCCABackoff(BytesPointer parameters);

        /**
         * Process the reset of device request; forward the reset request to the target device
         */
        void resetDevice(BytesPointer parameters);

        /**
         * Writes device's contracts and routes information in the stream.
         */
        void marshallDevice(Device* device, NetworkOrderStream& devicesStream, int& devicesCount);

        void marshallContract(const PhyContract& contract, NE::Misc::Marshall::OutputStream& outStream, int& contractsCount);
        void marshallRoute(const PhyRoute& route, Uint16 subnetID, Address64& ownerDevice, NE::Misc::Marshall::OutputStream& outStream);

        /**
         * Determines whether there are reports that have not been read completely yet.
         */
        bool existUnreadBlocks();
};

}
}
}

#endif /*SMO_H_*/
