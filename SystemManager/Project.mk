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



_LIB_FLAGS=$(if $(DEBUG),st-sd,st-s)

PROJECT_NAME=SystemManager
SRC_DIR=src

ISA100LIB=../AccessNode/IsaGw/ISA100
AccessNode_DIR=../AccessNode
#var link MUST NOT change after this point
ifeq "$(link_stack)" "dynamic"
	LIBISA_EXT :=.so	
else
	#default
	LIBISA_EXT :=.a	
endif
#$(warning LIBISA_EXT=$(LIBISA_EXT))

NET_ENGINE_DIR=../NetworkEngine
NET_ENGINE_LIBDIR=$(NET_ENGINE_DIR)/out/$(if $(DEBUG),debug,release)/$(TARGET_OSTYPE)
NET_ENGINE_LIB=NetworkEngine-$(CC_VERSION)-$(_LIB_FLAGS)-$(DIST_TAR_VERSION)

OUT_DIR=out
DIST_DIR=out-dist
TEST_DIR=tests
BOOST_TEST_DIR=btests

ifeq ($(TOOLCHAIN),gcc-cygwin)    # gccc-cygwin tool chain
  export CC_VERSION=gcc34
  ISA100LIB_DIR=../AccessNode/out/cyg/IsaGw/ISA100
endif
ifeq ($(TOOLCHAIN),gcc-linux-pc)    # linux-pc tool chain
#  export CC_VERSION=gcc44
  ISA100LIB_DIR=../AccessNode/out/i386/IsaGw/ISA100
endif
ifeq ($(TOOLCHAIN),gcc-linux-arm)   # linux-arm tool chain
  export CC_VERSION=gcc34
  ISA100LIB_DIR=../AccessNode/out/arm/IsaGw/ISA100
endif
ifeq ($(TOOLCHAIN),m68k-unknown-linux-uclibc)
  export CC_VERSION=gcc44
  ISA100LIB_DIR=../AccessNode/out/m68k-unknown-linux-uclibc/IsaGw/ISA100
endif
ifeq ($(TOOLCHAIN),gcc-linux-trinity2)  # trinity2 tool chain
  export CC_VERSION=gcc40
  ISA100LIB_DIR=../AccessNode/out/arm-926ejs-linux-gnu/IsaGw/ISA100
endif
ifeq ($(TOOLCHAIN),gcc-linux-m68k)  # linux-m68k ColdFire tool chain
  export CC_VERSION=gcc43
  ISA100LIB_DIR=../AccessNode/out/m68k-unknown-linux-uclibc/IsaGw/ISA100
endif

DIST_TAR_VERSION=$(shell grep '\#define[ ]*SYSTEM_MANAGER_VERSION' ./src/Version.h | cut -f 2 -d \")
CVSROOT = ':pserver:radu.pop@cljsrv01:/ISA100'

OUT_DIR := $(OUT_DIR)/$(if $(DEBUG),debug,release)/$(TARGET_OSTYPE)
MAIN_EXE = $(OUT_DIR)/$(PROJECT_NAME)
DIST_TAR_FILE = $(DIST_DIR)/$(PROJECT_NAME)-$(DIST_TAR_VERSION)-$(TARGET_OSTYPE).tgz
TEST_MAIN_EXE = $(OUT_DIR)/Test_$(PROJECT_NAME).$(TARGET_OSTYPE_EXE_EXT)
BTEST_MAIN_EXE = $(OUT_DIR)/BTest_$(PROJECT_NAME).$(TARGET_OSTYPE_EXE_EXT)

_DIST_FLAGS=$(if $(DEBUG),mt-sd,mt-s)
MAIN_AR = $(OUT_DIR)/lib$(PROJECT_NAME)-$(CC_VERSION)-$(_DIST_FLAGS)-$(DIST_TAR_VERSION).a

COMPONENT_DIST_NAME=S04
UPGRADE_DIST_TAR_FILE=$(DIST_DIR)/Upgrade-$(COMPONENT_DIST_NAME)-$(PROJECT_NAME)-$(DIST_TAR_VERSION)-$(TARGET_OSTYPE).tgz
UPGRADE_DIST_TAR_FILE_FAKE=$(DIST_DIR)/Upgrade-$(COMPONENT_DIST_NAME)-$(PROJECT_NAME)-$(DIST_TAR_VERSION).fake-$(TARGET_OSTYPE).tgz

CPP_INCLUDES = -I$(OUT_DIR) -Isrc -I$(NET_ENGINE_DIR)/src
CPP_INCLUDES += -I$(CPPLIB_PATH)/trunk/log4cplus_1_0_2 
CPP_INCLUDES += -I$(CPPLIB_PATH)/trunk/boost_1_36_0
CPP_INCLUDES += -I$(CPPLIB_PATH)/trunk/nlib
CPP_INCLUDES += -I$(ISA100LIB)
CPP_INCLUDES += -I$(AccessNode_DIR)

CPP_FLAGS = $(if $(DEBUG), -g3 -O0 -D_DEBUG, -Os)
CPP_FLAGS += -Wall -fmessage-length=0 -pipe 
ifeq ($(TOOLCHAIN),m68k-unknown-linux-uclibc)
  #CPP_FLAGS += -DBOOST_SP_USE_PTHREADS -DBOOST_AC_USE_PTHREADS 
	CPP_FLAGS += -DBOOST_SP_DISABLE_THREADS -DBOOST_DISABLE_THREADS 
endif
ifeq ($(TOOLCHAIN),gcc-linux-m68k)  # linux-m68k ColdFire tool chain
  #CPP_FLAGS += -DBOOST_SP_USE_PTHREADS -DBOOST_AC_USE_PTHREADS
	CPP_FLAGS += -DBOOST_SP_DISABLE_THREADS -DBOOST_DISABLE_THREADS 
endif
ifeq ($(TOOLCHAIN),gcc-linux-arm)   # linux-arm tool chain
	CPP_FLAGS += -DBOOST_SP_DISABLE_THREADS -DBOOST_DISABLE_THREADS 
endif
ifeq ($(TOOLCHAIN),gcc-linux-trinity2)  # trinity2 tool chain
	CPP_FLAGS += -DHW_TRINITY2
	#CPP_FLAGS += -DBOOST_SP_DISABLE_THREADS -DBOOST_DISABLE_THREADS 
endif

CPP_FLAGS += -DCPPLIB_LOG_LOG4CPLUS_ENABLED #enable log4cplus

#add linking strip flag (-s) to release
#LFLAGS = $(if $(DEBUG),,-s)

LFLAGS += -L$(CPPLIB_PATH)/trunk/log4cplus_1_0_2/lib/$(TARGET_OSTYPE)
LFLAGS += -L$(CPPLIB_PATH)/trunk/boost_1_36_0/lib/$(TARGET_OSTYPE)
LFLAGS += -L$(NET_ENGINE_LIBDIR)
#LFLAGS += -L$(ISA100LIB_DIR)

LIBS  = -llog4cplus-$(CC_VERSION)-$(_LIB_FLAGS)-1_0_2 
#LIBS  = -llog4cplus-$(CC_VERSION)-$(_LIB_FLAGS)-1_0_2 -lpthread

LIBISA = $(ISA100LIB_DIR)/libisa100$(LIBISA_EXT)
LIBS += $(LIBISA)

LIBS += -lNetworkEngine-$(CC_VERSION)-$(_LIB_FLAGS)-$(DIST_TAR_VERSION)


LIBNE = $(NET_ENGINE_LIBDIR)/lib$(NET_ENGINE_LIB).a

ifeq ($(TOOLCHAIN),gcc-cygwin)
  # cygwin tool chain
  #CPP_FLAGS += -D__USE_W32_SOCKETS
  #CPP_FLAGS += -D__INSIDE_CYGWIN_NET__   
  CPP_FLAGS += -D_WIN32_WINNT=0x0501
  CPP_FLAGS += -DSI_SUPPORT_IOSTREAMS 
  CPP_FLAGS += -DSI_CONVERT_GENERIC 
  CPP_FLAGS += -DSI_SUPPORT_IOSTREAMS
  CPP_FLAGS += -DCPPLIB_LOG_LOG4CPLUS_ENABLED #enable log4cplus
  ifdef DEBUG 
  CPP_FLAGS += -DBOOST_ENABLE_ASSERT_HANDLER 
  endif
  LIBS += -lws2_32

else ifeq ($(TOOLCHAIN),gcc-linux-pc)
  # linux-pc tool chain
  CPP_FLAGS += -DCPPLIB_LOG_LOG4CPLUS_ENABLED #enable log4cplus
  #CPP_FLAGS += -include all.h #inject include  to all sources
  ifdef DEBUG 
  CPP_FLAGS += -DBOOST_ENABLE_ASSERT_HANDLER 
  endif
  
  #LIBS += -ldl -lpthread
  LIBS += -lboost-test-$(CC_VERSION)-mt-s-1_36_0
else ifeq ($(TOOLCHAIN),gcc-linux-arm)
  # linux-arm tool chain
  #LIBISA = $(ISA100LIB_DIR)/libisa100.so
  CPP_FLAGS += -DCPPLIB_LOG_LOG4CPLUS_ENABLED #enable log4cplus
  CPP_FLAGS += -D_LINUX_UCLIBC_
  #CPP_FLAGS += -include all.h #inject include  to all sources
  
#  LIBS += -ldl -lpthread
else ifeq ($(TOOLCHAIN),m68k-unknown-linux-uclibc)
  # VR900 tool chain
  #LIBISA = $(ISA100LIB_DIR)/libisa100.so
#  CPP_FLAGS += -DCPPLIB_LOG_LOG4CPLUS_ENABLED #enable log4cplus
#  CPP_FLAGS += -D_LINUX_UCLIBC_
#  LIBS += -ldl -lpthread
else
  abort "Undefined TOOLCHAIN=$(TOOLCHAIN) !"
endif

-include ./User.mk

ifdef TARGET_TEST
	TARGET_TEST = $(TARGET_TEST)
else 
	TARGET_TEST = ALL_TESTS
endif

MAIN_EXE_DEPS = $(LIBISA) $(LIBNE) 

#//TODO [ovidiu.rauca] find a make compliente solution !! CPP_SRCS:= $(foreach dir, $(MY_SRC_DIR), $(wildcard $(dir)/*.cpp))
CPP_SRCS = $(shell find $(SRC_DIR) -iname "*.cpp") 
CPP_OBJS = $(patsubst %.cpp, %.o, $(addprefix $(OUT_DIR)/, $(CPP_SRCS))) 
CPP_DEPS = $(CPP_OBJS:.o=.d)

TEST_SRCS = $(shell find $(TEST_DIR) -iname '*.cpp') 
TEST_OBJS = $(patsubst %.cpp, %.o, $(addprefix $(OUT_DIR)/, $(TEST_SRCS))) 
TEST_DEPS = $(TEST_OBJS:.o=.d)

ifdef USER.tests.SRC
BTEST_SRCS = $(USER.tests.SRC) $(BOOST_TEST_DIR)/BTestsSetup.cpp  $(BOOST_TEST_DIR)/Utils/TestSmSettings.cpp 
else
BTEST_SRCS = $(shell find $(BOOST_TEST_DIR) -iname '*.cpp')
endif 

ifdef USER.tests.run_params
TES_RUN_PARAMS = $(USER.tests.run_params)
else
TES_RUN_PARAMS = --log_level=test_suite --catch_system_errors=yes --report_level=detailed --build_info=yes --run_test=Isa100
endif

BTEST_OBJS = $(patsubst %.cpp, %.o, $(addprefix $(OUT_DIR)/, $(BTEST_SRCS))) 
BTEST_DEPS = $(BTEST_OBJS:.o=.d)


