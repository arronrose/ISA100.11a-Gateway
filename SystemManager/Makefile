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



ifeq ($(shell whoami),root)
  $(error The Root user is disabled to run this script)
endif

#include common defines
export CPPLIB_PATH = ../cpplib
export NET_ENGINE_PATH = ../NetworkEngine
export ROOT_USERID = 0

-include $(CPPLIB_PATH)/trunk/nbuild/Common.mk


all: compile
-include ./Project.mk

.PHONY: clean pixi print-var

clean:
	$(RM) -r $(OUT_DIR)
cleanExe:
	rm $(MAIN_EXE)
	echo "Deleted: $(MAIN_EXE)"

#########################
# COMPILATION targets
#
.PHONY: compile test print-var bCleanTest

compile: compile-init $(MAIN_EXE)


compile-init:
	echo TARGET_OSTYPE=$(TARGET_OSTYPE)
	echo OUT_DIR=$(OUT_DIR)
	echo MAIN_EXE=$(MAIN_EXE)
	$(MKDIR)  $(OUT_DIR)

compile_library: $(MAIN_AR)
	@echo MAIN_AR=$(MAIN_AR)

test: testBuild
	$(TEST_MAIN_EXE)

testBuild: cleanTestCpp $(TEST_MAIN_EXE)

cleanTestCpp:
	$(shell rm -f $(OUT_DIR)/tests/Test.o )

$(MAIN_EXE): $(CPP_OBJS) $(MAIN_EXE_DEPS)
	$(common-gcc-link)
	$(MKDIR) Release
	$(MKDIR) Debug
	$(if $(TOOLCHAIN)=gcc-cygwin, cp, copy) $(MAIN_EXE) Debug
	$(if $(TOOLCHAIN)=gcc-cygwin, cp, copy) $(MAIN_EXE) Release
	$(STRIP) Release/SystemManager

$(OUT_DIR)/src/%.o: src/%.cpp   #$(OUT_DIR)/all.h.gch
	$(common-g++-compile)

$(TEST_MAIN_EXE): $(filter-out %/src/Main.o,$(CPP_OBJS)) $(TEST_OBJS)
	echo "Building test for target $(TARGET_TEST)"
	$(common-gcc-link)


brun-test: $(BTEST_MAIN_EXE)
	echo "B Run tests:"
	echo BTEST_MAIN_EXE= $(BTEST_MAIN_EXE)
	$(BTEST_MAIN_EXE) $(TES_RUN_PARAMS)
#	$(BTEST_MAIN_EXE) --log_level=test_suite --catch_system_errors=yes --report_level=detailed --build_info=yes --run_test=Isa100
# 	$(BTEST_MAIN_EXE) --log_level=test_suite --catch_system_errors=yes --report_level=detailed --build_info=yes --run_test=Isa100 1> TestOut.log 2> TestReport.log; cat TestOut.log; cat TestReport.log

bTestBuild: bCleanTest $(filter-out %/src/Main.o,$(CPP_OBJS)) $(BTEST_MAIN_EXE)

bCleanTest:
	rm -f $(BTEST_MAIN_EXE)

$(BTEST_MAIN_EXE): $(filter-out %/src/Main.o,$(CPP_OBJS)) $(BTEST_OBJS)
	echo "Building test for target $(TARGET_TEST)"
	$(common-gcc-link)

testCompileAll: $(filter-out %/src/Main.o,$(CPP_OBJS)) $(TEST_OBJS)
	echo "Compilation complete."

bTestCompile: $(BTEST_OBJS) $(BTEST_MAIN_EXE)
	echo "BTests Compilation complete."

$(OUT_DIR)/btests/%.o: btests/%.cpp
	$(common-g++-compile)
#	@$(MKDIR) -p $(@D)
#	$(CPP) $(CPP_FLAGS) $(CPP_INCLUDES) -Isrc -Icute -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
#	echo "test-compiled '$<'"


$(OUT_DIR)/tests/%.o: tests/%.cpp   #$(OUT_DIR)/all.h.gch
	@$(MKDIR) -p $(@D)
	$(CPP) -D$(TARGET_TEST)=true $(CPP_FLAGS) $(CPP_INCLUDES) -Isrc -Icute -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	echo "test-compiled '$<'"


$(MAIN_AR): $(CPP_OBJS)
	@echo LIBS=$(LIBS)
	$(common-gcc-ar)
	@echo DONE


#$(OUT_DIR)/all.h.gch: inc/all.h
#	$(MKDIR) -p $(@D)
#	$(CPP_) -x c++-header $(CPP_FLAGS) $(CPP_INCLUDES) -MMD -MP -MF"$(@:%.gch=%.d)" -MT"$(@:%.gch=%.d)" -o"$@" "$<"
#	echo "pch compiled '$<'"
#	cp inc/all_test_pch.h $(OUT_DIR)/all.h



print-var:
	@echo CPP_SRCS=$(CPP_SRCS)
	@echo
	@echo MAIN_AR=$(MAIN_AR)
	@echo
	@echo CPP_OBJS=$(CPP_OBJS)
	@echo
	@echo C_OBJS=$(C_OBJS)
	@echo
	@echo CPP_DEPS=$(CPP_DEPS)
	@echo
	@echo CPP_INCLUDES=$(CPP_INCLUDES)
	@echo
	@echo CPP_FLAGS=$(CPP_FLAGS)
	@echo
	@echo LFLAGS=$(LFLAGS)
	@echo
	@echo CPP_FLAGS=$(CPP_FLAGS)
	@echo
	@echo LIBS=$(LIBS)
	@echo
	@echo DIST_TAR_VERSION=$(DIST_TAR_VERSION)
	@echo $(BTEST_SRCS)
	echo "$(LIBNE)"
	echo "$(LIBISA)"


#########################
# DIST targets
#
.PHONY: dist dist-clean dist-makedb dist-upgrader dist-upgrader-fake

#//FIXME [nicu.dascalu] - the dist target is alwayes executed!! (doesn't use dependency file)
dist: $(DIST_TAR_FILE)

dist-upgrader: $(UPGRADE_DIST_TAR_FILE)


dist-upgrader-fake: $(UPGRADE_DIST_TAR_FILE_FAKE)


dist-clean:
	$(RM) -r $(DIST_DIR)
	$(RM) -r $(OUT_DIR)/dist_*


$(DIST_TAR_FILE): $(MAIN_EXE)  $(OUT_DIR)/db/$(PROJECT_NAME).db3
	$(MKDIR) $(DIST_DIR)
	$(RM) -rf $(OUT_DIR)/dist_temp
	#copy the config files
	$(MKDIR) -p $(OUT_DIR)/dist_temp/etc && $(CP) -r etc/*.conf $(OUT_DIR)/dist_temp/etc/
	$(CHMOD) 0600 $(OUT_DIR)/dist_temp/etc/*.conf
	#copy db file
	$(MKDIR) -p $(OUT_DIR)/dist_temp/db && $(CP) $(OUT_DIR)/db/$(PROJECT_NAME).db3 $(OUT_DIR)/dist_temp/db/
	$(CHMOD) 0777 $(OUT_DIR)/dist_temp/db
	$(CHMOD) 0666 $(OUT_DIR)/dist_temp/db/*
	#copy exe file + script files
	$(CP) $(MAIN_EXE) -p $(OUT_DIR)/dist_temp/
	$(MKDIR) -p $(OUT_DIR)/dist_temp/etc && $(CP) -r etc/*.sh $(OUT_DIR)/dist_temp/etc/
	$(CHMOD) 0700 $(OUT_DIR)/dist_temp/*.$(TARGET_OSTYPE_EXE_EXT) $(OUT_DIR)/dist_temp/etc/*.sh
	#create tar
	tar -czf $@ -C $(OUT_DIR)/dist_temp .
	$(ECHO) "created '$@' file."

$(UPGRADE_DIST_TAR_FILE): $(DIST_TAR_FILE) etc/setup/*.sh
	$(MKDIR) $(DIST_DIR)

	$(MKDIR) $(OUT_DIR)/dist_upgrader_temp/
	$(CHMOD) -R +w $(OUT_DIR)/dist_upgrader_temp && $(RM) -r $(OUT_DIR)/dist_upgrader_temp/*
	$(MKDIR) $(OUT_DIR)/dist_upgrader_temp/$(COMPONENT_DIST_NAME)
	#copy dist tgz
	$(CP) $(DIST_TAR_FILE) $(OUT_DIR)/dist_upgrader_temp/$(COMPONENT_DIST_NAME)
	#copy update scripts
	$(CP) etc/setup/*.sh  $(OUT_DIR)/dist_upgrader_temp/$(COMPONENT_DIST_NAME)
	$(CHMOD) $0770 $(OUT_DIR)/dist_upgrader_temp/$(COMPONENT_DIST_NAME)/*.sh
	#copy sql scripts + sqlite tool
	$(CP) -r etc/setup/*.sql etc/setup/sqlite3.bin  $(OUT_DIR)/dist_upgrader_temp/$(COMPONENT_DIST_NAME)
	$(CHMOD) $0770 $(OUT_DIR)/dist_upgrader_temp/$(COMPONENT_DIST_NAME)/sqlite3.bin
	# create tar
	tar -czf $@ -C $(OUT_DIR)/dist_upgrader_temp .
	$(ECHO) "created '$@' file."

$(UPGRADE_DIST_TAR_FILE_FAKE): $(UPGRADE_DIST_TAR_FILE)
	$(MKDIR) $(OUT_DIR)/dist_upgrader_fake_temp/
	$(RM) -r $(OUT_DIR)/dist_upgrader_fake_temp/*

	#untar current upgrader.tgz
	( cd $(OUT_DIR)/dist_upgrader_fake_temp && tar -opxzf $(PWD)/$(UPGRADE_DIST_TAR_FILE) )

	#rename version
	( cd $(OUT_DIR)/dist_upgrader_fake_temp/$(COMPONENT_DIST_NAME) && mv `ls $(PROJECT_NAME)*.tgz` $(PROJECT_NAME)-$(DIST_TAR_VERSION).fake-$(TARGET_OSTYPE).tgz )
	# create tar
	tar -czf $@ -C $(OUT_DIR)/dist_upgrader_fake_temp .
	$(ECHO) "created '$@' file."


ifeq ($(TOOLCHAIN),gcc-linux-pc)
dist-scp: $(DIST_TAR_FILE)
	scp $(DIST_TAR_FILE) cam117@minipc-cj:/home/cam117/EvalKits/Monitor_Host
endif




#########################
# DEPENDENCIES targets
ifneq ($(MAKECMDGOALS),clean)
-include $(CPP_DEPS)
-include $(C_DEPS)
-include $(TEST_DEPS)
#-include $(OUT_DIR)/all.h.d
endif
