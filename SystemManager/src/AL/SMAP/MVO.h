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
 * MVO.h
 *
 *  Created on: Dec 16, 2008
 *      Author: mulderul(catalin.pop), beniamin.tecar
 */

#ifndef MVO_H_
#define MVO_H_

#include "AL/Isa100Object.h"
#include "AL/ObjectsIDs.h"
#include "Common/logging.h"
#include "Model/ModelPrinter.h"

namespace Isa100 {
namespace AL {
namespace SMAP {

namespace DebugCommands {
/**
 * Represents the possible commands that will be processed.
 */
enum DebugCommandsEnum {
    EXECUTE_METHOD = 8,
    READ_ATTRIBUTE = 10,
    WRITE_ALL_GRAPHS = 11,
    SM_CURRENT_TAI = 12,
    PRINT_GRAPHS_FROM_SUBNET = 13,
    PRINT_RESERVED_MNG_CHUNCKS_SUBNT = 14,
    PRINT_RESERVED_MNG_CHUNCKS_DVC = 15,
    PRINT_ATTRIBUTES_OF_DEVICE = 16,
    PRINT_ATTRIBUTES_OF_ALL_DEVICES = 17,
    PRINT_ENTITIES_OF_DEVICE = 18,
    PRINT_ENTITIES_OF_ALL_DEVICES = 19

};
}

class MVO;
typedef boost::shared_ptr<MVO> MVOPointer;

struct SentRequest {
        int requestId;

        Address128 deviceAddress128;
        Address64 deviceAddress64;

        Isa100::AL::ObjectID::ObjectIDEnum objectId;

        int methodId;

        int attributeID;

        SentRequest() {
            requestId = -1;
        }

        SentRequest(int requestId_, Address128 deviceAddress128_, Isa100::AL::ObjectID::ObjectIDEnum objectId_,
                    int methodId_) :
            requestId(requestId_), deviceAddress128(deviceAddress128_), objectId(objectId_), methodId(methodId_) {
        }
};



/**
 * This object will deal with one custom request that must be sent to a device.
 *
 * @author Radu Pop
 */
class MVO: public Isa100Object {
        LOG_DEF("Isa100.AL.SMAP.MVO");

        LogDeviceDetail loggingDetailLevel;


    public:

        MVO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine);

        virtual ~MVO();

        Isa100::AL::ObjectID::ObjectIDEnum getObjectID() const;

        void execute(  Uint32 currentTime );

        void cleanupOnError();

        virtual bool expectIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        virtual bool expectConfirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);

        //static void createRequest();

        bool isJobFinished(){
            return false;
        }


        static void setFlagSignalCommands() {
            flagCommands = true;
        }

        static void setFlagSignalConfigs() {
            flagConfigs = true;
        }

    protected:

        void indicateExecute(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        void confirmExecute(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);

        void confirmWrite(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);

        void confirmRead(Isa100::ASL::Services::PrimitiveConfirmationPointer confirmation);

    private:

        /**
         * Represents the requests that have been sent and need to wait for a response.
         * The responses will match the request id.
         */
        std::vector<SentRequest> lastRequestsSent;

        /**
         * if SIGUSR2 message recieved . ...flagCommands is marked as true
         */
        static bool flagCommands;


        /**
         * if SIGUSR2 message recieved . ...flagConfigs is marked as true
         */
        static bool flagConfigs;


        // std::map<Uint16, Uint16> lastNrOfDevices; // map<subnetID, last...>
        std::map<Uint16, Uint16> lastNrOfConfirmedDevices; // map<subnetID, last...>
        Uint32 lastUnstableCheck;

        void unstableCheck(Uint32 currentTime);



        /**
         * Processes a command from the commands file.
         */
        void processCommand(Uint16 commandId, const Address64& ip64, std::vector<std::string>& cmdParameters);

        /**
         * Initially false and set to true after a request has been sent to a device.
         */
        bool expectResponse;

        Address64 deviceAddressToReadStatistics;

        int getSentRequestIndexForRequestId(int requestId);

        /**
         *
         */
        void readCommandFile();

        void generateReadAttributeCommand(const Address64& ip64, std::vector<std::string>& cmdParameters);

        void generateExecuteCommand(const Address64& ip64, std::vector<std::string>& cmdParameters);

        void removeUnprovisionedDevices();

};

}

}

}

#endif /* MVO_H_ */
