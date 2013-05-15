#
# Copyright (C) 2013 Nivis LLC.
# Email:   opensource@nivis.com
# Website: http://www.nivis.com
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.
#
# Redistribution and use in source and binary forms must retain this
# copyright notice.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#



################################
#	File to hold the user custom build section for Tests
################################

#USER.tests.SRC = $(shell find $(TEST_DIR)/BadFlows/ -iname '*.cpp')
#USER.tests.SRC = $(shell find $(TEST_DIR)/Candidates/ -iname '*.cpp')
#USER.tests.SRC = $(shell find $(TEST_DIR)/ -iname '*.cpp')
#USER.tests.SRC = $(shell find $(TEST_DIR)/Model/ -iname '*.cpp')
#USER.tests.SRC = $(shell find $(TEST_DIR)/Model/OperationsProcessor -iname '*.cpp')
#USER.tests.SRC = $(shell find $(TEST_DIR)/NetworkEngine/ -iname '*.cpp')
#USER.tests.SRC = $(TEST_DIR)/BadFlows/TestJoin_BR_timout_BR_rejoin.cpp
#USER.tests.SRC = $(TEST_DIR)/BadFlows/TestSM_BR_D1_jointimeout.cpp
#USER.tests.SRC = $(TEST_DIR)/BadFlows/TestSM_BR_D2_D1_D1rejoin_through_BBR.cpp
#USER.tests.SRC = $(TEST_DIR)/Candidates/TestAddCandidate_1.cpp
#USER.tests.SRC = $(TEST_DIR)/Candidates/TestMultipath.cpp
#USER.tests.SRC = $(TEST_DIR)/Contracts/MultipleDevicesPublishContractRequest.cpp
#USER.tests.SRC = $(TEST_DIR)/Contracts/MultipleDevicesPublishContractRequest.cpp
#USER.tests.SRC = $(TEST_DIR)/Contracts/MultipleDevicesPublishContractRequestWithRejoin.cpp
#USER.tests.SRC = $(TEST_DIR)/Contracts/testContract_GW_client_server.cpp
#USER.tests.SRC = $(TEST_DIR)/Contracts/testContract_GW_client_server.cpp
#USER.tests.SRC = $(TEST_DIR)/Contracts/testContract_publish_level1.cpp
#USER.tests.SRC = $(TEST_DIR)/Contracts/testContract_publish_level1.cpp
#USER.tests.SRC = $(TEST_DIR)/Contracts/testContract_publish_level2.cpp
#USER.tests.SRC = $(TEST_DIR)/Contracts/TestContractsAfterRejoin.cpp
#USER.tests.SRC = $(TEST_DIR)/Contracts/testLoopbackContract.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/OperationsProcessor/testAddOperation.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/OperationsProcessor/testAddOperation.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/OperationsProcessor/testAddOperation_MultipleOperations.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/OperationsProcessor/testAddOperationsContainer.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/OperationsProcessor/testAddOperation_SplitAddGraphEdge.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/OperationsProcessor/testAddOperationWithWrongOperation.cpp    
#USER.tests.SRC = $(TEST_DIR)/Model/Reports/testReportsEngine_deviceList.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Reports/testReportsEngine_topology.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Routing/Algorithms/InboundAlgMultiLvl_2Exit_MultiSink_deleteNode.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Routing/Algorithms/InboundAlgorithmMultipleLevels_2Exit_MultiSink.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Routing/Algorithms/InboundAlgorithmMultipleLevels_DoubleExit.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Routing/Algorithms/InboundAlgorithmMultipleLevels_DoubleExit.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Routing/Algorithms/InboundFirstLayer.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Routing/Algorithms/OutBoundAlgorithmLevel1.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Routing/Algorithms/OutBoundAlgorithmMultipleLevels.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Routing/Algorithms/OutBoundAlgorithmMultipleLevelsDeleteNode.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Routing/Algorithms/RoutingLimits.cpp
USER.tests.SRC = $(TEST_DIR)/Model/Routing/Algorithms/TestRedirectDeviceToBackup.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Tdma/AccelerationRemove.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Tdma/AlgorithmManagementTests.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Tdma/AlgorithmTests.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Tdma/LinkEnginePublishTest.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Tdma/LinkEngineTerminateContract.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Tdma/LinkEngineTests.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Tdma/LinkEngineUpdateAllocation.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Tdma/LinkIdGeneratorTests.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Tdma/ManagementLinks.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Tdma/MngChunksDistribution.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Tdma/ModelContainerTests.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/Tdma/TdmaModelTests.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/TestAddCandidate.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/TestCheckConsistency.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/TestContracts.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/TestContracts.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/TestJoinDevice_BadFlow.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/TestMultipleSubnets.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/TestNetworkEngine.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/TestNetworkEngineResolveOper.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/TestSubnetContainer.cpp
#USER.tests.SRC = $(TEST_DIR)/Model/TestTopology.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/JoinDevicesImbricated.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/MultipleBackbones/testJoin_SM_BR1_BR2.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/MultipleBackbones/testJoin_SM_BR1_D1_BR2_D2.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/MultipleBackbones/testJoin_SM_BR1_D1_BR2_D2_wrong_subnet.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/networkEngine_joinDevice.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/networkEngine_joinDevice.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/networkEngine_joinLevel2.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/networkEngine_join_sameAddressEnding.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/networkEngine_joinSteps.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/networkEngine_joinSteps.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/networkEngine_rejoinBR.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/networkEngine_rejoinBR_error.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/networkEngine_rejoinDevice2.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/networkEngine_rejoinDevice3.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/networkEngine_removeBackbone.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/networkEngine_removeDevice.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/networkEngine_securityJoin.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/networkEngine_setSettings.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/TestContractRequest.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/testJoinBBR_GW_4D_CGWD1_RejoinGWFail.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/testJoin_SM_BR_20routing_devices.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/testJoin_SM_BR_BR.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/testJoin_SM_BR.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/testJoin_SM_BR_D1_BR.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/testJoin_SM_BR_GW.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/testJoin_SM_GW_BR.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/testJoin_SM_GW.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/testJoin_SM_GW_GW.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/TestMaxNumberOfRoutersPerDevice.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/testRejoinGWBRD1.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/testRemoveAllDevices.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/TestRemoveDevice.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/testSM_BR_D_GW_CD_GW_RejoinGW.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/testSM_BR_RejoinRouter.cpp
#USER.tests.SRC = $(TEST_DIR)/NetworkEngine/testSM_BR_RejoinRouter.cpp

USER.tests.run_params = --log_level=test_suite --catch_system_errors=yes --report_level=detailed --build_info=yes --run_test=Isa100 | tee TestOut.log 2> TestReport.log; cat TestOut.log; cat TestReport.log; grep -e ERROR TestOut.log

test-print-var:
	@echo $(TEST_SRCS)

	
#CUSTOM_CLASS = /src/Model/Device
#CUSTOM_CLASS = /src/Model/NetworkEngine
#CUSTOM_CLASS = /src/Model/OperationsContainer
CUSTOM_CLASS = /src/Model/Operations/OperationsProcessor
#CUSTOM_CLASS = /src/Model/Operations/OperationsContainer

custom-compile: clean-custom $(OUT_DIR)$(CUSTOM_CLASS).o

clean-custom:
	rm -f $(OUT_DIR)$(CUSTOM_CLASS).o
	
precompiler: $(CUSTOM_CLASS).cpp
	@$(MKDIR) $(@D)
	@$(ECHO) "precompiling '$<' ... "
	$(PREC) $(CPP_FLAGS) $(CPP_INCLUDES) -MMD -MP -o $(CUSTOM_CLASS).prec.cpp "$<"
