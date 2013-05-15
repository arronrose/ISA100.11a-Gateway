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
 * @author catalin.pop, eduard.budulea, sorin.bidian, ioan.pocol, beniamin.tecar
 */
#ifndef SUBNET_H_
#define SUBNET_H_

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <set>
#include "Common/NEAddress.h"
#include "Common/logging.h"
#include "Common/SubnetSettings.h"
#include "Common/SettingsLogic.h"
#include "Model/Routing/Edge.h"
#include "Model/Routing/Graph.h"
#include "Model/Device.h"
#include "Model/IDeletedDeviceListener.h"
#include "Model/Operations/IEngineOperation.h"
#include "Model/Tdma/TdmaTypes.h"
#include "Model/BlacklistChannels.h"


using namespace NE::Model::Routing;
using namespace NE::Common;

namespace NE {
namespace Model {

    class IDeviceRemover;

namespace Operations {
    class OperationsProcessor;

    class OperationsContainer;
}

//maximum 200 devices. Adress 0 not used, Address 1 = SM,  Address 2 = GW
#define MAX_NUMBER_OF_DEVICES 203
#define MAX_NUMBER_OF_BANDWIDTH_CHUNCKS 100
//#define MAX_NUMBER_OF_FREQUENCY 14
#define MAX_NR_OF_ADV_PERIODS 30
#define MAX_NR_OF_ADV_BANDWIDTH_CHUNCKS 3

/**
 * Map with key=graphID and value=GraphPointer.
 */
//typedef boost::unordered_map<Uint16, GraphPointer> GraphsMap;
typedef std::map<Uint16, GraphPointer> GraphsMap;
typedef std::list<NE::Model::IDeletedDeviceListener *> DeletedDeviceListenersList;

typedef std::map<Address64, Address32> AddressMapping64_32;

struct JoinCounter {
    /**
     * Number of join attempts.
     */
    Uint32 joinCount;

    /**
     * Number of joins in which the device becomes JOINED_AND_CONFIGURED.
     */
    Uint32 fullJoinCount;

    JoinCounter() : joinCount(0), fullJoinCount(0) {

    }
};

typedef std::map<Address64, JoinCounter> DeviceJoinCounter;

/**
 * For each subnet there is an instance of this class that keeps the topology, the
 * routing graphs and source routing paths. A source routing path is kept as a graph routing
 * with the type set to source routing (see Route class).
 * A source routing is a like a graph routing with only one path.
 * There are several events that change the status of the topology:
 * <ul>
 * <li>1. A device sends a join request. (joinNode() function)</li>
 * <li>2. A device is removed from the subnet. (removeNode() function)</li>
 * <li>3. A device reports that can view another device ( addVisibleNeighbor() function).</li>
 * <li>4. From time to time a routing graph evalution event is fired. (evaluateRoute() function)</li>
 * </ul>
 *
 * @author Radu Pop, Ioan-Vasile Pocol
 * @version 1.1
 */

class Subnet {

        LOG_DEF("I.M.R.Subnet");

    private:


        Uint16 lastUnmatchDeviceId;

        /**
         * The id of the subnet for which the data is saved.
         */
        Uint16 subnetId;

        NE::Model::Device * backbone; //shortcut to backbone. Exists also in subnetNodes.

        NE::Common::SettingsLogic *settingsLogic;

        /**
         * The list of devices from subnet.
         */
        NE::Model::Device * subnetNodes[MAX_NUMBER_OF_DEVICES];

        /**
         * The list of active devices' address16 from subnet (gateway included).
         */
        NE::Common::Address16Set activeDevices;

        /**
         * Mapping that holds the count of the join attempts for each device.
         * Mappings for each device will remain until a SM restart, even if at some point the device is removed from the network forever.
         */
        DeviceJoinCounter deviceJoinCounter;

        /**
         * Set of all devices that are not yet configured for publication to SM (HRCO configuration).
         * This publication will be started after a period of time from device join.
         */
        NE::Common::Address32Set notPublishingDevices;

        /**
         * Holds the list of devices that do not have alerts configured.
         */
        NE::Common::Address32Set notConfiguredForAlerts;

        /**
         * Matrix containing subnet's edges.
         */

        EdgePointer subnetEdges[MAX_NUMBER_OF_DEVICES][MAX_NUMBER_OF_DEVICES];

        /**
         * The graphs from the current subnet.
         */
        GraphsMap graphs;

        Uint16 lastGraphId;

        /**
         * Contains all the callback that must be called when a device is deleted.
         */
        DeletedDeviceListenersList lstDeleteDeviceCallbacks;

        /**
         * The current consistency check index.
         */
        Uint16 consistencyCheckIdx;

        bool updateAdvPeriod;

        /**
         * When a G1 graph evaluation begins, this flag is set to false in order to not allow a new G1 evaluation until operations are executed with success.
         */
        bool defaultGraphCanBeReevaluated;

        /**
         * Last time when defaultGraphCanBeReevaluated was initialized with false.
         */
        Uint32 lastLockInboundGraphCanBeReevaluated;

        /**
         * Whean an outbound graph is on evaluation , this flag is set as true. If this flag is true, dirtyEdges will not be deleted
         */
        bool outBoundGraphCanBeEvaluated;

        /**
         * Last time when outBoundGraphCanBeEvaluated was initialized with false.
         */
        Uint32 lastLockOutboundGraphCanBeReevaluated;


    public:

        Uint16 lastLoggedDeviceId;

        /**
         * The mapping between address64 and address32.
         */
        AddressMapping64_32 addressMapping;

        NE::Common::Address32Set devicesToActivateRole;
        EntityIndexList routesToBeEvaluated;

        GraphsList graphsToBeEvaluated;
        GraphsSet graphsToBeRemoved;
        GraphsSet garbageCollectionGraphs;

        /**
         * This list holds all the contracts marked as dirty after a device that served this contracts was removed.
         */
        EntityIndexList contractsToBeEvaluated;

        /**
         * List with dirty links that should be removed. This are app links and are added when a device is removed.
         * Will contain the list with all links used by the contracts of removed device.
         */
        EntityIndexList dirtyLinks;

        /**
         * Holds all the udo contracts marked as dirty after a device that served this contracts was removed or changed the parent.
         */
        EntityIndexList udoContractsToBeEvaluated;

        /**
         * Devices that have no yet activated the neighbor discovery.
         */
        NE::Common::Address32Set noNeighborDiscoveryActivatedDevices;

        NE::Common::Address32Set postJoinTaskDevices;

        MngChunk ** mngChunksTable; //[MAX_NUMBER_OF_BANDWIDTH_CHUNCKS][MAX_NUMBER_OF_FREQUENCY]
        Address16 advertiseChuncksTable[MAX_NR_OF_ADV_BANDWIDTH_CHUNCKS][MAX_NR_OF_ADV_PERIODS];
        NE::Model::Tdma::AppSlots appSlots;

        typedef boost::shared_ptr<Subnet> PTR;



        BlacklistChannels channelsBlacklist;

        /**
         * List with devices that have some attributes changed. Used by the MVO execute method.
         */
        Address16List changedDevices;

        /**
         * List of changed devices on outbound graphs...used to reevaluate client-server contracts
         */
        Address16List changedDevicesOnOutboundGraphs;

        Address16List listDeviceToReadJoinReason;

        Address16List listOfAdvertisingRouters;

        Address16List listDiscoveryToBeAccelerated;
        Address16List listOfFastDiscovery;

        /**
         * list with dirty edges from the network edge32 = (source16 << 16 + destination16)
         */
        Uint32List dirtyEdges;

    public:

        Subnet(Uint16 subnetId, NE::Model::Device * manager, SettingsLogic *settingsLogic);

        virtual ~Subnet();

        void copySubnetShort(Subnet::PTR& subnet);
        Address32 getNextAddress32( const Address64 & device64, Address32 oldAddress32 );

        Address32 getAddress32(const Address64& address64);

        Address32 getAddress32(const Address128& address128);

        bool existsDevice(const Address64& address64) {
            return getDevice(getAddress32(address64)) != NULL;
        }

        Uint16 getSubnetId() const {
            return subnetId;
        }

        Address32 getManagerAddress32() {
            return ADDRESS16_MANAGER;
        }

        NE::Model::Device* getManager() {
            return subnetNodes[ADDRESS16_MANAGER];
        }
        NE::Model::Device* getGateway() {
            return subnetNodes[ADDRESS16_GATEWAY];
        }

        Device * getBackbone() {
            return backbone;
        }

        void addDevice(Device * device);

        NE::Model::Device* getDevice(Address32 address) {
            Uint16 address16 = Address::getAddress16(address);
            if (address16 >= MAX_NUMBER_OF_DEVICES) {
                LOG_ERROR("Invalid address: " << std::hex << address);
                return NULL;
            }
            return subnetNodes[address16];
        }

        NE::Model::Device* getDevice(Address16 address) {
        	if (address >= MAX_NUMBER_OF_DEVICES) {
				LOG_ERROR("Invalid address: " << std::hex << address);
				return NULL;
			}
            return subnetNodes[address];
        }

        void destroyDevice(const Address128 & deletedDevAddr128, const Address64 & address64, const Address32 address32);

        static Uint16 getAddress16(const Address32 address32){
            return Address::getAddress16(address32);
        }

        void enableInboundGraphEvaluation(){
            LOG_INFO("INBOUND GRAPH EVAL enable");
            defaultGraphCanBeReevaluated = true;
        }

        void enableOutBoundGraphEvaluation(){
            LOG_INFO("OUTBOUND GRAPH EVAL enable");
            outBoundGraphCanBeEvaluated = true;
        }


        void disableInboundGraphEvaluation(){
            LOG_INFO("DEFAULT GRAPH EVAL disable");
            defaultGraphCanBeReevaluated = false;
            lastLockInboundGraphCanBeReevaluated = time(NULL);
        }

        void disableOutboundGraphEvaluation(){
            LOG_INFO("OUTBOUND GRAPH EVAL disable");
            outBoundGraphCanBeEvaluated = false;
            lastLockOutboundGraphCanBeReevaluated = time(NULL);
        }

        bool isEnabledInboundGraphEvaluation(){return defaultGraphCanBeReevaluated;}

        bool isEnabledOutboundGraphEvaluation(){return outBoundGraphCanBeEvaluated;}

        Uint32 getLastLockInboundGraphCanBeReevaluated() {
            return lastLockInboundGraphCanBeReevaluated;
        }

        Uint32 getLastLockOutboundGraphCanBeReevaluated() {
            return lastLockOutboundGraphCanBeReevaluated;
        }

        bool isRouter(Address16 device);

        bool isFieldDevice(Address16 device);

        bool isBackboneDevice(Address16 device);

        bool isManager(Address16 device);

        bool isGateway(Address16 device);

        /**
         * Generates the next available graph id
         */
        Uint16 getNextGraphId();

        /**
         * Return the level of the node.
         *
         * @return -1 if device not found
         */
        Int8 getDeviceLevel(Address32 address32);

        /**
         * Deletes this node from the subnet.
         * First initializes all the nodes from the subnet as active nodes.
         * Sets the node as marked for removal. Than starts a DPS (Depth First Search)
         * from the SM to determined the nodes that can be reached.
         */
        void removeNode(Address32 nodeRemovedAddress);

        /**
         * Mark for remove the route.
         */
        void markForRemoveRoute(Uint16 routeId);

        /**
         *
         */
        void removeGraph(Address32 source, Uint16 graphId);

        /**
         * Registers a new callback for delete device event.
         */
        void registerDeleteDeviceCallback(NE::Model::IDeletedDeviceListener * deleteCallback);

        /**
         * Creates a new graph with the id = graphID.
         */
        void createGraph(Uint16 graphID, Address16 destination);

        GraphPointer getGraph(Uint16 graphID);

        GraphsMap& getGraphsMap(){
            return graphs;
        }

        void addGraphToBeEvaluated(Uint16 graphId) ;
        void addGraphToBeRemoved(Uint16 graphId) {
            graphsToBeRemoved.insert(graphId);
            GraphsList::iterator existsGraph = std::find(graphsToBeEvaluated.begin(), graphsToBeEvaluated.end(), graphId);
            if(existsGraph != graphsToBeEvaluated.end()) {
                graphsToBeEvaluated.erase(existsGraph);
            }
        }

        float  evalEdgeCost(Address32 source, Address32 destination, Uint16 evalGraphTraffic, bool isFieldDevice);

        float getEdgeCost(Address16 source, Address16 destination);

        /**
         * get outBound edges for a given device
         */
        void getOutBoundEdges(Address16 deviceAddress16, EdgesList &edges);

        /**
         * get Destination nodes from the outBoundEdges for a given device
         */
        void getOutBoundEdgesTargets(Uint16 deviceAddress16, NE::Common::Address16Set &targets);

        /**
         * get Source nodes from the InBoundEdges for a given device
         */
        void getInboundEdgesSources(Uint16 deviceAddress16, NE::Common::Address16Set &targets);

        Uint8 getDeviceNrOfNeighbors(Address16 device);

        /**
         * get inBound edges for a given device
         */
        void getInboundEdges(Address16 deviceAddress16, std::list<EdgePointer> &edges);

        /**
         * Get all inBound edges that enters in a cycle..in a set of vertexes.
         */
        void getInboundEdgesToCycle(NE::Common::Address16Set devicesAddress16, NE::Common::Address16Set &cycle, std::list<EdgePointer> &edges);

        void getChildrenList(Address16 device, NE::Common::Address16Set &childrenSet);

        void getCountDirectChilds(Device * device, Uint8 &devices);

        void getCountDirectChildsAndRouters(Device * device, Uint8 &routers, Uint8 &nonRouters);

        static Edge32 generateEdge32(Address32 source32, Address32 destination32){
			return (Address::getAddress16(source32) << 16) + Address::getAddress16(destination32);
        }
        /**
         * Add edge to graph.
         */

        void addEdgeToGraph(Address32 source32, Address32 destination32, Uint16 graphID) {
                addEdgeToGraph(Address::getAddress16(source32), Address::getAddress16(destination32), graphID);
        }

        void addEdgeToGraph(Address16 source, Address16 destination, Uint16 graphID);

        /**
         * Delete edge from a graph
         */
        void deleteEdgeFromGraph(Address16 source, Address16 destination, Uint16 graphID);

        /**
         * Deletes a graph form edge's list of graphs
         * @param source
         * @param destination
         * @param graphID
         */
        void deleteGraphFromEdge(Address16 source, Address16 destination, Uint16 graphID);

        /**
         * Adds a graphs to edge's list of graphs
         * @param source
         * @param destination
         * @param graphID
         */

        void addGraphToEdge(Address16 source, Address16 destination, Uint16 graphID);


        /**
         * Update subnetEdges when device is removed.
         */

        void UpdateModelOnRemoveDevice(Address16 deviceToRemove);


        /**
         * Get number of active devices from subnet.
         */
        Uint16 getNumbetOfDevicesFromSubnet(void) const { return activeDevices.size();}

        /**
         *
         * @return
         */
        const NE::Common::Address16Set& getActiveDevices() const{ return activeDevices; }

        const DeviceJoinCounter& getDeviceJoinCounter() const {
            return deviceJoinCounter;
        }

        Uint32 getDeviceJoinCount(Address64& deviceAddress) {
            return deviceJoinCounter[deviceAddress].joinCount;
        }

        Uint32 getDeviceFullJoinCount(Address64& deviceAddress) {
            return deviceJoinCounter[deviceAddress].fullJoinCount;
        }

        void incrementFullJoinCount(Address64& deviceAddress) {
            ++deviceJoinCounter[deviceAddress].fullJoinCount;
        }

        bool isMngChunckReserved(Uint8 setNumber, Uint8 freq) const { return (mngChunksTable[setNumber][freq].owner != 0); }
        void unreserveMngChunck(Uint8 setNumber, Uint8 freq) { mngChunksTable[setNumber][freq].owner = 0; }
        void reserveMngChunck(Uint8 setNumber, Uint8 freq, Address16 owner) { mngChunksTable[setNumber][freq].owner = owner; }

        bool wasAddressAllocatedToOther(const Address64 & address64, const Address32 address32);

        bool getAvailableMngChunckInbound(NE::Model::Operations::OperationsContainer & operationsContainter,
                    Device* device,
                    Device* parent,
                    Uint8   &setNo,
                    Uint8   &freqOffset,
                    Uint8   joinReservedSet);

        bool getAvailableMngChunckOutbound(Operations::OperationsContainer & operationsContainer, Device* parent,
                                                Device* device,
                                                Uint8   &setNo,
                                                Uint8   &freqOffset,
                                                Uint8   joinReservedSet);

        EdgePointer getEdge(Address16 source, Address16 destination) {
        	return subnetEdges[source][destination];
        }

        EdgePointer getEdge(Address32 source, Address32 destination) {
            return getEdge(Address::getAddress16(source), Address::getAddress16(destination));
        }

        void changeEdgeStatus(Address16 source, Address16 destination, Status::StatusEnum edgeStatus) {
            RETURN_ON_NULL( subnetEdges[source][destination]);
            subnetEdges[source][destination]->setEdgeStatus(edgeStatus);
        }

        void addEdge(EdgePointer edge);

        bool existsEdge(Address16 source, Address16 destination);

        bool existsEdge(Address32 source, Address32 destination) {
        	return existsEdge(getAddress16(source), getAddress16(destination));
        }

        bool existsGraph(Uint16 graphId) {
            return (graphs.find(graphId) != graphs.end());
        }

        void addDevieToGraph(Address16 destinationDevice, Uint16 graphID);

        void removeDeviceFromGraph(Address16 deviceToRemove, Uint16 graphID);

        Address16 getAdvertiseChuncksTableElement(Uint8 advertiseSetId, Uint8 advPeriodId);

        void setAdvertiseChuncksTableElement(Uint8 advertiseSetId, Uint8 advPeriodId, Address16 address);

        Uint32 getInboundChildrenNo(Device * device);

        void createGraphFromParent(Uint16 graphId, Uint16 parentGraph);

        void getNeighborsOnGraph(Uint16 graph, Uint16 device, NE::Common::Address16Set &neighbors);

        bool existsConfirmedDevice(Address16 device);

        /**
         * Check one device at a time for physical settings consistency. In case of a problem a LOG_ERROR will be performed.
         * The checks performed are:
         * - a contract with Sm must exist
         * - for every contract a route must exist (route with selector=contactID OR a default route)
         * - for every route that is based on graph, the specified graph must exists
         * - for every route(source or hybrid) the first(source)/second(hybrid) node in routes vector must exist in NeighborsTable
         * - in bbr exist a route with selector(destination) = current device(the outbound route)
         * - for every graph the nodes from neighbors vector must exists also in Neighbors Table
         * - for every transmit link :
         *                              -the destination device must exist in NeighborsTable
         *                              -the destination device must exist (JOIN_CONFIRMED)
         *                              -Optional: on the destination device must exist a Receive link on the current link slot
         * - check if exists pending entities and no pending operations(no operations in OperationProcessor).
         * - foreach edge(from device) with no graphs - delete Linksl, neighbors;
         */
        void periodicPhyConsistencyCheck(NE::Model::Operations::OperationsProcessor & perationsProcessor);

        /**
         *
         * @param device
         * @param consistencyCheck - only for logging
         */
        void consistencyCheckForPendings(Device* device, std::string& consistencyCheck, bool& inconsistencyfound);

        void consistencyCheckForClockSourceLoops(Device* device, std::string& consistencyCheck, bool& inconsistencyfound);

        void checkClockSourceCycle(Address16 preferedClockSrc, Address16 cycleSrc, Address16 &cycleDst, bool& isCycle ) ;

        void checkPreferedClockSourceCycle(Address16 src, Address16 dst, bool& isCycle );

        void consistencyGraph(Device * device, std::string& consistencyCheck, bool& inconsistencyfound,Operations::OperationsContainer & operationsContainer);

        bool isDeviceVisited(Address16 device) {
            if(subnetNodes[device] ) {
                return subnetNodes[device]->isAlreadyVisited();
            }

            return false;
        }

        void setDeviceVisited(Address16 device) {
            if(subnetNodes[device] ) {
                subnetNodes[device]->setVisited();
            }
        }

        void getDevicesVisited(Address16List &targetsList);

        bool deviceHasNeighbor(Address16 device, Address16 neighbor) {
            if(subnetNodes[device] ) {
               return  subnetNodes[device]->hasNeighbor(neighbor);
            }

            return false;
        }

        bool deviceHasCandidate(Address16 device, Address16 candidate) {
            if(subnetNodes[device]) {
               return  subnetNodes[device]->hasCandidate(candidate);
            }

            return false;
        }


        void unVisitedDevice(Address16 device) {
            if(subnetNodes[device] ) {
                subnetNodes[device]->unsetVisited();
            }
        }

//        void clearUsedApplicationSlotsFromContainer(NE::Model::Operations::OperationsContainer& container);
        void clearUsedApplicationSlot(NE::Model::Operations::IEngineOperationPointer& operation);

        void unreserveSlotsOperation(Device * device, const NE::Model::Operations::IEngineOperationPointer& operation );

        void unreserveLinkForDevice(Address16 device, Address16 neighbor, NE::Model::Operations::OperationsContainer* operationsContainer,
                    NE::Model::Operations::OperationsList & operationsDependencies);

        void unreserveLink(Device * device, PhyLink * link, NE::Model::Operations::OperationsContainer * container = NULL);

//        void printGraphs();

        void markAllDevicesUnVisited() {
            for(Address16Set::iterator it = activeDevices.begin(); it != activeDevices.end(); ++it) {
                if(subnetNodes[*it] ) {
                    subnetNodes[*it]->unsetVisited();
                    subnetNodes[*it]->theoAttributes.isSelectedAsBackup = false;
                }
            }
        }

        bool hasChildSpaceForDevice(Device * parent, Device * child);

        bool hasOUTChunksSpaceForChild(Device * parent, Device * child);

        float getChildsOccupationPercent(Device* device);

        bool candidateIsEligibleAsNewParent(Address16 deviceAddress, Address16 candidate);

        bool candidateIsEligibleAsBackup(Address16 deviceAddress, Address16 candidate, bool isFinalPhase=false);

        //when choosing a backup from device's candidates, check if new backup is on the preferred path of the device

        bool candidateNotValidOrIsOnPreferedPath( Device * device, Device * candidateDevice);

        float getInboundAppTraffic( Device * device, const DoubleExitEdges & edges);
        float getOutboundAppTraffic( Device * device );

        float getInboundAppTraffic_reentrant(Device * device, const DoubleExitEdges & edges);

        float getOutboundAppTraffic_reentrant(Device * device, const DoubleExitEdges & edges);

        NE::Common::SubnetSettings& getSubnetSettings() {
            return settingsLogic->getSubnetSettings(subnetId);
        }

        void addDeviceCandidate(Address16 device, Address16 candidate);

        void printManagementChuncks();

        void deleteGraph(Uint16 graphId) {
            if(graphs.find(graphId) != graphs.end()) {
                graphs.erase(graphId);
            }
        }

       void  setEdgeHasLinks(Address16 src16, Address16 dst16 ) {
            if(subnetEdges[src16][dst16]) {
                subnetEdges[src16][dst16]->setHasMngLinks();
            }

        }

        bool edgeHasMngLinks(Address16 src16, Address16 dst16 ) {

            if(subnetEdges[src16][dst16]) {
                return subnetEdges[src16][dst16]->HasMngLinks();
            }

            return false;
        }

        void periodicStartPublish(Uint32 currentTime, NE::Model::Operations::OperationsProcessor & operationsProcessor);
        void periodicConfigureAlerts(Uint32 currentTime, NE::Model::Operations::OperationsProcessor & operationsProcessor);

        void evaluateIgnoredDevices(Uint32 currentTime, NE::Model::Operations::OperationsProcessor & operationsProcessor);

        bool configurePublish(Device * device,NE::Model::Operations::OperationsProcessor & operationsProcessor);
        bool configureAlerts(Device * device, NE::Model::Operations::OperationsProcessor & operationsProcessor);

        Uint8 getDiagLevel(Address32 deviceAddress, Address32 neighborAddress);


        BlacklistChannels& ChannelBlacklist() {
            return channelsBlacklist;
        }

        void addNotPublishingDevice(Address32 address){
            notPublishingDevices.insert(address);
        }

        void addNotConfiguredForAlertsDevice(Address32 address){
            notConfiguredForAlerts.insert(address);
        }

        void addNoNeighborDiscoveryConfiguratedDevice(Address32 address){
            noNeighborDiscoveryActivatedDevices.insert(address);
        }

        void addDeviceToPostJoinTask(Address32 address){
            postJoinTaskDevices.insert(address);
        }

        bool existsDirectPathBetween(Address16 source16, Address16 destination16, PhyRoute *route) ;

        void setUpdateAdvPeriod(bool value);
        bool getUpdateAdvPeriod();

        bool neighborsAreValid(Address16 src,  DoubleExitDestinations &dst);

        bool nodeHasChangedDestinations(Address16 src,  DoubleExitDestinations &dst, Uint16 graphId);

        /**
         * Mark all contracts that used the usedDevice on their direct path, as dirty.
         * This method is used when a device(usedDevice) is about to be removed.
         * The dirty contracts will be reevaluated later.
         * @param usedDevice
         */
        void markServedContractsDirty(Device * usedDevice);

        /**
         * Moves all the links from contractLinks list to dirtyLinks list. After this the contractLinks list will be empty.
         * @param contractLinks
         */
        void moveToDirtyLinks(LinksList& contractLinks);

        /**
         *  Remove from dirty  links all links from the usedDevice because they were
         *  already deleted on removeDevicOnError or on redirectDevice
         * @param usedDevice
         */
        void removeFromDirtyLinks(Device * usedDevice);

        /**
         * Add a contract if not already exist to the list of contracts to be evaluated.
         * @param contractEntityIndex
         */
        void addContractToBeEvaluated(EntityIndex contractEntityIndex);

        /**
         * Returns the number of app slots per second.
         * Used in generation of NetworkResourceReport.
         */
        //OBSOLETE
        float getNumberOfSlotsAPPperSecond();

        /**
         * Used in generation of NetworkResourceReport.
         */
        //OBSOLETE
        float  getNumberOfUsedMngAndAdvBC();

        /**
         * Computes the number of allocated management + advertise slots per second and the number of allocated application slots per second.
         * Used in generation of NetworkResourceReport.
         */
        void getNumberOfSlotsPerSecond(float& allocatedSlotsAdvMngPerSecond, float& allocatedSlotsAppPerSecond);

        void getNumberOfSlotsPerSecondOnBBR(float& allocatedSlotsAdvMngPerSecond, float& allocatedSlotsAppPerSecond);

        /**
         * Called every so often to check if any modifications need to be made.
         */
        void checkExpiringBlacklists(Uint32 passedTimeSeconds, NE::Model::Operations::OperationsProcessor & operationsProcessor);

        /**
         * Process channel diagnostics to decide if blacklisting is in order.
         */
        void processChannelDiag(const Model::Tdma::ChannelDiagnostics& channelDiags, const Address32& source,
                    NE::Model::Operations::OperationsProcessor & operationsProcessors);

        void generateChannelBlackListOperations(NE::Model::Operations::OperationsProcessor & operationsProcessor);

        void generateBackupPingOperation( NE::Model::Operations::OperationsContainer *  operationsContainer , Address16 device);


        // bool deviceHasReachedMaxChildsNo(Address16 device, Address16 joinningDevice);
        bool checkMaxNrOfChildren(const Address16 parentDeviceAddress64, const Address64 & childDeviceAddress64);

        bool addTheoAttributesChild(const Address16 parentDeviceAddress16, const Address16 deviceAddress16);

        bool deleteTheoAttributesChild(const Address16 parentDeviceAddress16, const Address16 & deviceAddress16);
        bool deleteTheoAttributesChild(const Address16 parentDeviceAddress16, const Address64 & deviceAddress64);

//        bool updateTheoAttributesChildren(const Address16 parentDeviceAddress16, Uint32 currentTime, NE::Model::IDeviceRemover * devicesRemover,
//                    NE::Model::Operations::OperationsProcessor & operationsProcessor);

        // bool deviceReachedTheMaxChildsRouterNo(Address16 device, Address16 joinningDevice, bool isRouter);
        bool checkMaxNrOfChildren(Address16 parentDeviceAddress16 /*Address16 childDeviceAddress16,*/, bool childIsRouting, bool printWarnMessage = true);

        /**
         * check if there is still links space on edge for a number of new neededLinks
         * @param src
         * @param dst
         * @param neededLinks
         * @param pendingContainer
         * @return
         */
        ResponseStatus::ResponseStatusEnum  checkMaxNumberOfLinksOnEdge(Address16 src, Address16 dst, Uint8 neededLinks = 0,
                    NE::Model::Operations::OperationsContainer *  pendingContainer = NULL);


        Uint16 getNumberOfChildren(Address16 parentDeviceAddress16);

        bool deviceReachedTheMaxLinksNo(Address16 device, Address64 newDeviceAddress64, bool printWarnMessage = true);

        bool isBackupForDevice(Address16 device, Address16 backup);

        void removeDeviceFromAdvertiseChuncksTable(Device * device);


        void sortCandidates(Address16 device);

        void clearChangedDevicesList() {
            changedDevices.clear();
        }

        void addDirtyEdge(Address16 src, Address16 dst) {
            Uint32 edge = ((src << 16) + dst);
            dirtyEdges.push_back(edge);
        }

        void addToFastDiscovery(Device * device, Uint32 currentTime){
            listOfFastDiscovery.push_back(Address::getAddress16(device->address32));
            device->fastDiscoveryTime = currentTime;
        }

        /**
         * check PER betwwen device and its neighbor (neighbor is even its parent or its backup)
         * @param device
         * @param neighbor
         * @return
         */
        bool deviceHasHighFailRateWithNeighbor( Device * device, Device * neighbor);

        /**
         * check id there is a high fail rate on the direct path from device to backbone (inbound prefered path)
         * @param candidate
         * @return
         */
        bool checkFailRateOnNewPath( Device * device);

        void addBadRateCandidate( Device * device, Device * neighbor);

};


typedef std::map<Uint16, Subnet::PTR> SubnetsMap;

}
}

#endif /* SUBNET_H_ */
