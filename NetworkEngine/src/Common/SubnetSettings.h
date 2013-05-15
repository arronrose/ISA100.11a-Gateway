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

#ifndef SUBNETSETTINGS_H_
#define SUBNETSETTINGS_H_

#include "Common/NEAddress.h"
#include "Common/NETypes.h"
#include "Common/simpleini/SimpleIni.h"
#include "Model/SecurityKey.h"
#include "Model/modelDefault.h"
#include <vector>

namespace NE {
namespace Common {

//250 miliseconds as 2^(-20) units
#define QUARTERSEC_IN_MICROSECONDS_2POWER 262144

class SubnetSettings {
    static const int SLOTS_NEIGHBOR_DISCOVERY_LENGTH = 4;
    private:
        Uint16 timeslotLength;
        Uint16 slotsPer250;
        Uint16 slotsPerSec;
        Uint16 slotsPer30Sec;

    public:
        std::vector<Uint8> channel_list; //[CHANNEL_LIST_LENGTH];
        std::vector<Uint8> reduced_hopping; //7,10,8,11,9,7,10,8,11,9,7,10,8,11,9,7

        Uint16 join_reserved_set;
        Uint16 join_adv_period;
        Uint16 join_bbr_adv_period;
        Uint16 join_rxtx_period;
        Uint16 mng_link_r_out;
        Uint16 mng_link_r_in;
        Uint16 mng_link_s_out;
        Uint16 mng_link_s_in;
        Uint16 mng_link_s_in_Star;
        Uint16 mng_r_in_band;
        Uint16 mng_r_out_band;
        Uint16 mng_s_in_band;
        Uint16 mng_s_in_band_Star;
        Uint16 mng_s_out_band;
//        Uint16 mng_inc_band;
        Uint16 mng_alloc_band;
        Uint16 mng_disc_bbr;
        Uint16 mng_disc_r;
        Uint16 mng_disc_s;
        Uint16 retry_proc;
        Uint8 slotsNeighborDiscovery[SLOTS_NEIGHBOR_DISCOVERY_LENGTH];
        std::vector<Uint8> neigh_disc_channel_list; // [NEIG_DIS_CHANNEL_LIST_LENGTH];
        Uint16 nrRoutersPerBBR;
        Uint16 nrNonRoutersPerBBR;
        bool enableStarTopology;
        bool enableStarTopologyRouters;
        Uint16 nrRoutersPerBBRinStar;
        Uint16 nrNonRoutersPerBBRinStar;
        Uint16 nrRoutersPerBBRinStarRouters;
        Uint16 nrNonRoutersPerBBRinStarRouters;
        Uint16 nrRoutersPerRouter;
        Uint16 nrNonRoutersPerRouter;

        /**
         * Max number of layers.
         */
        Uint16 maxNumberOfLayers;

        bool enableAlertsForGateway;
        Uint16 alertReceivingObjectID;
        Uint16 alertReceivingPort;
        Uint16 alertTimeout;
        bool enableMultiPath;
        Uint16 accelerationInterval;

        Uint32 delayActivateMultiPathForDevice; //delay the activation of multipath with X seconds after its join time
        Uint16 superframeNeighborDiscoveryLength;
        /**
         * Length of superframe when running infast mode.
         */
        Uint16 superframeNeighborDiscoveryFastLength;

        /**
         * Time to maintain fast discovery(in seconds).
         */
        Uint16 fastDiscoveryTimespan;

        /**
         * Publish to SM will be configured on a device after this period from join (in seconds).
         */
        Uint16 delayConfigurePublish;

        Uint16 delayConfigureAlerts;

        /**
         * Configure the neighbor discovery after this interval (in seconds).
         */
        Uint16 delayConfigureNeighborDiscovery;

        /**
         * Activate router role (add adv links) after this delay (in seconds).
         */
        Uint16 delayActivateRouterRole;
        Uint16 delayPostJoinTasks;
        /**
         * The metadata required on router on join device through router.
         */
        Uint16 contractsMetadataThreshold;
        Uint16 neighborsMetadataThreshold;
        Uint16 superframesMetadataThreshold;
        Uint16 graphsMetadataThreshold;
        Uint16 linksMetadataThreshold;
        Uint16 routesMetadataThreshold;
        Uint16 diagsMetadataThreshold;

        bool randomizeChannelsOnStartup;

        /**
         * a device with low sent_successful/sent_failed rate will be ignored for a period of time(in minutes)
         */
        Uint32 timeToIgnoreBadRateDevice;

        /**
         * the threshold for setting a device as "bad rate" for success/failed (percent 0-100)
         */
        Uint16 badTransferRateThreshlod;

        /**
         * the threshold for setting a backup device as "bad rate" for success/failed (percent 0-100)
         */
        Uint16 badTransferRateThreshlodOnBackup;

        /**
         * the threshold for setting a device as "bad rate" for success/failed on  short period (percent 0-100)
         */
        Uint16 badTransferRateThreshlodShort;

        /**
         * the threshold for setting a backup device as "bad rate" for success/failed on short periodc(percent 0-100)
         */
        Uint16 badTransferRateThreshlodOnBackupShort;

        Uint16 numberOfSentPackagesForFailRate;

        Uint16 numberOfSentPackagesForFailRateOnBackup;

        Uint16 numberOfSentPackagesForFailRateShort;

        Uint16 numberOfSentPackagesForFailRateOnBackupShort;

        Uint16 k1factorOnEdgeCost;

        Uint16 superframeBirth;

        Uint16 advChannelsMask;

        Uint16 noOfPublishForFailRate;

        Uint16 noOfPublishForFailRateOnBackup;

        Uint16 noOfPublishForFailRateShortPeriod;

        Uint16 noOfPublishForFailRateOnBackupShortPeriod;

        Uint16 deviceCommandsSilenceInterval;

        Uint16 appSlotsStartOffset;

        bool freezeLevelOneRouters;

        /**
         * ping interval is used as interval for a device to ping its backup
         */
        Uint16 pingInterval;

        Uint16 nrOfOutboundGraphsToBeEvaluated;

        /**
         * number of used frequencies
         */
        Uint16 numberOfFrequencies;

        /**
         * max number of Udo contracts that can run at 1 second in paralel
         */
        Uint16 maxNoUdoContractsAtOneSecond;

        /**
         * vector of prioritiesQueue Uint16 = (Uint8)priorityNumber << 8 + (Uint8)QMax
         */
        std::vector<Uint16>  queuePriorities;


public:
        SubnetSettings() :
            join_reserved_set(23),
            join_adv_period(7),//set of possible values: 1,7,9,11,13,14,17,19,21,22,23,26,29 on change verify the value of superframeNeighborDiscoveryFastLength should be (adv*100+100)
            join_bbr_adv_period(1),
            join_rxtx_period(1),
            mng_link_r_out(4),
            mng_link_r_in(4),
            mng_link_s_out(8),
            mng_link_s_in(8),
            mng_link_s_in_Star(4),
            mng_r_in_band(8),
            mng_r_out_band(8),
            mng_s_in_band(8),
            mng_s_in_band_Star(4),
            mng_s_out_band(8),
//            mng_inc_band(4),
            mng_alloc_band(4),
            mng_disc_bbr(1),
            mng_disc_r(17),
            mng_disc_s(17),
            retry_proc(0),
            nrRoutersPerBBR(10),
            nrNonRoutersPerBBR(28),
            enableStarTopology(false),
            enableStarTopologyRouters(false),
            nrRoutersPerBBRinStar(1),
            nrNonRoutersPerBBRinStar(51),
            nrRoutersPerBBRinStarRouters(51),
            nrNonRoutersPerBBRinStarRouters(1),
            nrRoutersPerRouter(6),
            nrNonRoutersPerRouter(6),
            maxNumberOfLayers(2),
            enableAlertsForGateway(false),
            alertReceivingObjectID(8),
            alertReceivingPort(0xF0B2),
            alertTimeout(120),
            enableMultiPath(true),
            accelerationInterval(120),
            delayActivateMultiPathForDevice(360),
            superframeNeighborDiscoveryLength(5700),
            superframeNeighborDiscoveryFastLength(0),//wil be calculate in constr
            fastDiscoveryTimespan(370),//~~6 minutes
            delayConfigurePublish(60),
            delayConfigureAlerts(360),
            delayConfigureNeighborDiscovery(90),
            delayActivateRouterRole(180),
            delayPostJoinTasks(30),
//            dlmoMaxLifetime(50){
            contractsMetadataThreshold(0),
            neighborsMetadataThreshold(1),
            superframesMetadataThreshold(0),
            graphsMetadataThreshold(1),
            linksMetadataThreshold(3),
            routesMetadataThreshold(0),
            diagsMetadataThreshold(0),
            randomizeChannelsOnStartup(true),
            timeToIgnoreBadRateDevice(60), //60 minutes
            badTransferRateThreshlod(15),
            badTransferRateThreshlodOnBackup(15),
            badTransferRateThreshlodShort(50),
            badTransferRateThreshlodOnBackupShort(60),
            numberOfSentPackagesForFailRate(20),
            numberOfSentPackagesForFailRateOnBackup(10),
            numberOfSentPackagesForFailRateShort(9),
            numberOfSentPackagesForFailRateOnBackupShort(3),
            k1factorOnEdgeCost(20),
            superframeBirth(MAX_SUPERFRAME_BIRTH + 2), //must be greater than 127 to force randomize the birth
            advChannelsMask(0x0928),
            noOfPublishForFailRate(10),
            noOfPublishForFailRateOnBackup(10),
            noOfPublishForFailRateShortPeriod(3),
            noOfPublishForFailRateOnBackupShortPeriod(3),
            deviceCommandsSilenceInterval(60),
            appSlotsStartOffset(100),
            freezeLevelOneRouters(false),
            pingInterval(60),
            nrOfOutboundGraphsToBeEvaluated(10),
            numberOfFrequencies(14),
            maxNoUdoContractsAtOneSecond(5)
            {

            channel_list.push_back(8);
            channel_list.push_back(1);
            channel_list.push_back(9);
            channel_list.push_back(13);
            channel_list.push_back(5);
            channel_list.push_back(12);
            channel_list.push_back(7);
            channel_list.push_back(3);
            channel_list.push_back(10);
            channel_list.push_back(0);
            channel_list.push_back(4);
            channel_list.push_back(11);
            channel_list.push_back(6);
            channel_list.push_back(2);


            slotsNeighborDiscovery[0] = 1;
            slotsNeighborDiscovery[1] = 3;
            slotsNeighborDiscovery[2] = 13;
            slotsNeighborDiscovery[3] = 15;

            neigh_disc_channel_list.push_back(11);
            neigh_disc_channel_list.push_back(8);
            neigh_disc_channel_list.push_back(5);
            neigh_disc_channel_list.push_back(3);
            neigh_disc_channel_list.push_back(11);
            neigh_disc_channel_list.push_back(8);
            neigh_disc_channel_list.push_back(5);

            setTimeslotLength(0x28E0);//value should be in units of 2^-20 (aprox microsec)

            superframeNeighborDiscoveryFastLength = join_adv_period * getSlotsPerSec() + getSlotsPerSec();
            Uint16 priorityNo = 11;
            Uint16 queMax = 	12;
            queuePriorities.push_back(((priorityNo << 8) + queMax));
        }

        virtual ~SubnetSettings() {
        }

        void randomizeChannelList();

        string getChannelListAsString();

        string getNeighborDiscoveryListAsString();

        string getReducedChannelHoppingListAsString();

        string getQueuePrioritiesListAsString();

        void createChannelListWithReducedHopping();

        void detectNumberOfUsedFrequencies();

        void setTimeslotLength(Uint16 timeslotLength_){
            timeslotLength = timeslotLength_;
            slotsPer250 = QUARTERSEC_IN_MICROSECONDS_2POWER / timeslotLength;
            slotsPerSec = slotsPer250 * 4;
            slotsPer30Sec = slotsPerSec * 30;
        }

        Uint16 getTimeslotLength() const{
            return timeslotLength;
        }

        Uint16 getSlotsPer250ms() const{
            return slotsPer250;
        }
        Uint16 getSlotsPerSec() const{
            return slotsPerSec;
        }
        Uint16 getSlotsPer30Sec() const{
            return slotsPer30Sec;
        }
        Uint16 getSuperframeLengthAPP() const{
            return slotsPer30Sec;
        }
        Uint16 getSuperframeLengthDefault() const{
            return slotsPer30Sec;
        }

        friend std::ostream& operator<<(std::ostream& stream, const SubnetSettings& entity);
};


}
}
#endif /*SETTINGSLOGIC_H_*/
