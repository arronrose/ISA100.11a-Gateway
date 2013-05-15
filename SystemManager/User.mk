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


#USER.tests.SRC = $(shell find $(BOOST_TEST_DIR) -iname '*.cpp' | grep  -e "NeighborTest" -e "GraphTest1" -e "RouteTest1")
#USER.tests.SRC = $(shell find $(BOOST_TEST_DIR) -iname '*.cpp' | grep  -e "NeighborTest" -e "GraphTest1" -e "RouteTest1")
#USER.tests.SRC = $(shell find $(BOOST_TEST_DIR) -iname '*.cpp' | grep -e "Join.cpp" -e "JoinCallbacks.cpp" -e "SmSettingsLogic.cpp" -e "JoinUtils.cpp" )

#USER.tests.SRC = $(BOOST_TEST_DIR)/AL/ProcessTest.cpp
#USER.tests.SRC = $(BOOST_TEST_DIR)/Model/Isa100OperationsList.cpp
#USER.tests.SRC = $(BOOST_TEST_DIR)/Model/Operations/Tdma/SuperframeAddedOperTest.cpp
#USER.tests.SRC = $(BOOST_TEST_DIR)/Model/Dll/SuperframeTest.cpp

USER.tests.run_params = --log_level=test_suite --catch_system_errors=yes --report_level=detailed --build_info=yes --run_test=Isa100 1> TestOut.log 2> TestReport.log; cat TestOut.log; cat TestReport.log

test-print-var:
	@echo $(USER.tests.SRC)
	@echo $(BTEST_SRCS)

	
#CUSTOM_CLASS = /src/Model/Dll/Superframe
#CUSTOM_CLASS = /src/Model/Operations/OperationsUtils
#CUSTOM_CLASS = /src/Model/Isa100EngineOperationsVisitor

custom-compile: clean-custom $(OUT_DIR)$(CUSTOM_CLASS).o

clean-custom:
	rm -f $(OUT_DIR)$(CUSTOM_CLASS).o
	
precompiler: $(CUSTOM_CLASS).cpp
	@$(MKDIR) $(@D)
	@$(ECHO) "precompiling '$<' ... "
	$(PREC) $(CPP_FLAGS) $(CPP_INCLUDES) -MMD -MP -o $(CUSTOM_CLASS).prec.cpp "$<"P_FLAGS) $(CPP_INCLUDES) -MMD -MP -o $(CUSTOM_CLASS).prec.cpp "$<"