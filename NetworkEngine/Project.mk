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




PROJECT_NAME=NetworkEngine
SRC_DIR=src
OUT_DIR=out
TEST_DIR=tests
CAPTURES_DIR=subnetCaptures
CAPTURES_OUT_DIR=png
CVSROOT = ':pserver:radu.pop@cljsrv01:/ISA100'

OUT_DIR := $(OUT_DIR)/$(if $(DEBUG),debug,release)/$(TARGET_OSTYPE)
DIST_DIR=$(OUT_DIR)/dist
DIST_VERSION=$(shell grep '\#define[ ]*SYSTEM_MANAGER_VERSION' ../SystemManager/src/Version.h | cut -f 2 -d \")
	
_DIST_FLAGS=$(if $(DEBUG),st-sd,st-s)
_LIB_FLAGS=$(if $(DEBUG),st-sd,st-s)
MAIN_AR = $(OUT_DIR)/lib$(PROJECT_NAME)-$(CC_VERSION)-$(_DIST_FLAGS)-$(DIST_VERSION).a
MAIN_SHARED = $(OUT_DIR)/lib$(PROJECT_NAME)-$(CC_VERSION)-$(if $(DEBUG),st-d,st)-$(DIST_VERSION).$(TARGET_OSTYPE_DLL_EXT)

TEST_MAIN_EXE = $(OUT_DIR)/Test_$(PROJECT_NAME).$(TARGET_OSTYPE_EXE_EXT)

CAPTURES_EXE = CAPTURES

CPP_INCLUDES = -Isrc 
CPP_INCLUDES += -I$(OUT_DIR) # pentru headere precompilate
CPP_INCLUDES += -I$(CPPLIB_PATH)/trunk/nlib
CPP_INCLUDES += -I$(CPPLIB_PATH)/trunk/log4cplus_1_0_2
CPP_INCLUDES += -I${CPPLIB_PATH}/trunk/boost_1_36_0

#CPP_FLAGS = $(if $(DEBUG), -g3 -O0 -D_DEBUG, -Os -fno-inline)
CPP_FLAGS = $(if $(DEBUG), -g3 -O0 -D_DEBUG, -Os)
CPP_FLAGS += -Wall -fmessage-length=0 
CPP_FLAGS += -DHAVE_SSTREAM -DHAVE_GETTIMEOFDAY
CPP_FLAGS += -DCPPLIB_LOG_LOG4CPLUS_ENABLED
CPP_FLAGS += -DGRAPHS_TESTS
ifeq ($(TOOLCHAIN),m68k-unknown-linux-uclibc)
	CPP_FLAGS = $(if $(DEBUG), -g3 -Os -D_DEBUG, -Os)
	#  CPP_FLAGS += -DBOOST_SP_USE_PTHREADS -DBOOST_AC_USE_PTHREADS
  	CPP_FLAGS += -DBOOST_SP_DISABLE_THREADS -DBOOST_DISABLE_THREADS 
endif
ifeq ($(TOOLCHAIN),gcc-linux-m68k)
	CPP_FLAGS = $(if $(DEBUG), -g3 -Os -D_DEBUG, -Os)
	#  CPP_FLAGS += -DBOOST_SP_USE_PTHREADS -DBOOST_AC_USE_PTHREADS
  	CPP_FLAGS += -DBOOST_SP_DISABLE_THREADS -DBOOST_DISABLE_THREADS 
endif
ifeq ($(TOOLCHAIN),gcc-linux-arm)   # linux-arm tool chain
	CPP_FLAGS = $(if $(DEBUG), -g3 -Os -D_DEBUG, -Os)
	CPP_FLAGS += -DBOOST_SP_DISABLE_THREADS -DBOOST_DISABLE_THREADS 
endif
CPP_FLAGS += -Wall -fmessage-length=0 
CPP_FLAGS += -DHAVE_SSTREAM -DHAVE_GETTIMEOFDAY
CPP_FLAGS += -DCPPLIB_LOG_LOG4CPLUS_ENABLED
CPP_FLAGS += -DGRAPHS_TESTS


LFLAGS = -L$(CPPLIB_PATH)/trunk/nlib/lib/$(TARGET_OSTYPE)
LFLAGS += -L$(CPPLIB_PATH)/trunk/log4cplus_1_0_2/lib/$(TARGET_OSTYPE)
LFLAGS += -L$(CPPLIB_PATH)/trunk/boost_1_36_0/lib/$(TARGET_OSTYPE)
LFLAGS += -L$(NET_ENGINE_DIR)/out/$(if $(DEBUG),debug,release)/$(TARGET_OSTYPE)


#LIBS  = -llog4cplus-$(CC_VERSION)-$(_LIB_FLAGS)-1_0_2 
LIBS  = -llog4cplus-gcc44-$(_LIB_FLAGS)-1_0_2
ifdef USER.tests.SRC 
LIBS += -lboost-test-$(CC_VERSION)-mt-s-1_36_0
endif


ifeq ($(TOOLCHAIN),gcc-cygwin)
  # cygwin tool chain
  CPP_FLAGS += -D_WIN32_WINNT=0x0501
  CPP_FLAGS += -DSI_SUPPORT_IOSTREAMS 
  CPP_FLAGS += -DSI_CONVERT_GENERIC 
  CPP_FLAGS += -DSI_SUPPORT_IOSTREAMS
  CPP_FLAGS += -DCPPLIB_LOG_LOG4CPLUS_ENABLED #enable log4cplus
  LIBS += -lws2_32

else ifeq ($(TOOLCHAIN),gcc-linux-pc)
  # linux-pc tool chain
  CPP_FLAGS += -DCPPLIB_LOG_LOG4CPLUS_ENABLED #enable log4cplus
  #export CC_VERSION=gcc44
  #CPP_FLAGS += -include all.h #inject include  to all sources
  
#  LIBS += -ldl -lpthread

else ifeq ($(TOOLCHAIN),gcc-linux-arm)
  # linux-arm tool chain
  CPP_FLAGS += -DCPPLIB_LOG_LOG4CPLUS_ENABLED #enable log4cplus
  CPP_FLAGS += -D_LINUX_UCLIBC_
  export CC_VERSION=gcc34
  #CPP_FLAGS += -include all.h #inject include  to all sources
  
  LIBS += -ldl -lpthread
else
  abort "Undefined TOOLCHAIN=$(TOOLCHAIN) !"
endif

-include ./User.mk

#//TODO [ovidiu.rauca] find a make compliente solution !! CPP_SRCS:= $(foreach dir, $(MY_SRC_DIR), $(wildcard $(dir)/*.cpp))
CPP_SRCS = $(shell find $(SRC_DIR) -iname '*.cpp') 
CPP_OBJS:=$(patsubst %.cpp, %.o, $(addprefix $(OUT_DIR)/, $(CPP_SRCS))) 
CPP_DEPS=$(CPP_OBJS:.o=.d)

ifdef USER.tests.SRC
TEST_SRCS = $(USER.tests.SRC) $(shell find $(TEST_DIR)/Utils -iname '*.cpp') $(TEST_DIR)/Test.cpp
else
TEST_SRCS = $(shell find $(TEST_DIR) -iname '*.cpp')
endif 

TEST_OBJS = $(patsubst %.cpp, %.o, $(addprefix $(OUT_DIR)/, $(TEST_SRCS))) 
TEST_DEPS = $(TEST_OBJS:.o=.d)

ifdef USER.tests.run_params
TEST_RUN_PARAMS = $(USER.tests.run_params)
else
TEST_RUN_PARAMS = --log_level=error --catch_system_errors=yes --report_level=detailed --build_info=yes
endif


CAPTURES_SRCS = $(shell find $(CAPTURES_DIR) -maxdepth 1 -iname '*.dot')
CAPTURES_OBJS = $(patsubst %.dot, %.png, $(addprefix $(CAPTURES_OUT_DIR)/, $(CAPTURES_SRCS)))
