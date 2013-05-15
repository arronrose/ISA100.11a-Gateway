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




PROJECT_NAME=Monitor_Host
SRC_DIR=src hack-boost-lib
TEST_DIR=test
OUT_DIR=out
DIST_DIR=out-dist
DIST_TAR_VERSION=$(shell grep M_H_VERSION ./src/Version.h | cut -f 2 -d \")
#also this should work: DIST_TAR_VERSION=$(shell grep M_H_VERSION ./src/Version.h | awk '{print $3}' | sed s/\"//g)
CVSROOT = ':pserver:nicu.dascalu@cljsrv01:/ISA100'


OUT_DIR := $(OUT_DIR)/$(if $(DEBUG),debug,release)/$(TARGET_OSTYPE)
MAIN_EXE = $(OUT_DIR)/$(PROJECT_NAME).$(TARGET_OSTYPE_EXE_EXT)
TEST_MAIN_EXE = $(OUT_DIR)/Test_$(PROJECT_NAME).$(TARGET_OSTYPE_EXE_EXT)
DIST_TAR_FILE = $(DIST_DIR)/$(PROJECT_NAME)-$(DIST_TAR_VERSION)-$(TARGET_OSTYPE).tgz

COMPONENT_DIST_NAME=S04
UPGRADE_DIST_TAR_FILE=$(DIST_DIR)/Upgrade-$(COMPONENT_DIST_NAME)-$(PROJECT_NAME)-$(DIST_TAR_VERSION)-$(TARGET_OSTYPE).tgz
UPGRADE_DIST_TAR_FILE_FAKE=$(DIST_DIR)/Upgrade-$(COMPONENT_DIST_NAME)-$(PROJECT_NAME)-$(DIST_TAR_VERSION).fake-$(TARGET_OSTYPE).tgz



CPP_INCLUDES = -I$(OUT_DIR) -Iinc

#CPP_INCLUDES += -I${CPPLIB_PATH}/boost_1_35_0 -I${CPPLIB_PATH}/log4cplus_1_0_2 -I${CPPLIB_PATH}/nlib_0_0_3 
CPP_INCLUDES += -I${CPPLIB_PATH}/boost_1_35_0 -I${CPPLIB_PATH}/nlib_0_0_3 


#CPP_FLAGS = $(if $(DEBUG), -g3 -O0 -D_DEBUG, -Os -fno-inline)
#CPP_FLAGS += -Wall -fmessage-length=0 -pipe 

CPP_FLAGS = $(if $(DEBUG), -g3 -O0 -D_DEBUG, -Os)
CPP_FLAGS += -Wall


ifeq ($(TOOLCHAIN),gcc-linux-m68k)
CPP_FLAGS += -DUSE_SQLITE_DATABASE
else
#CPP_FLAGS += -DUSE_SQLITE_DATABASE
CPP_FLAGS += -DUSE_MYSQL_DATABASE
endif


#build with AccessNode
ifeq ($(TOOLCHAIN),gcc-linux-pc)
	#LFLAGS = -L$(CPPLIB_PATH)/nlib_0_0_3/lib/$(TARGET_OSTYPE) -L$(CPPLIB_PATH)/log4cplus_1_0_2/lib/$(TARGET_OSTYPE) -L$(CPPLIB_PATH)/mysql-5.0.67/lib/$(TARGET_OSTYPE) -L../AccessNode/out/i386/Shared/
	LFLAGS = -L$(CPPLIB_PATH)/nlib_0_0_3/lib/$(TARGET_OSTYPE) -L$(CPPLIB_PATH)/mysql-5.0.67/lib/$(TARGET_OSTYPE) -L../AccessNode/out/i386/Shared/

else ifeq ($(TOOLCHAIN),gcc-linux-m68k)
	LFLAGS = -L$(CPPLIB_PATH)/nlib_0_0_3/lib/$(TARGET_OSTYPE) -L../AccessNode/out/m68k-unknown-linux-uclibc/Shared/


else
  abort "Undefined TOOLCHAIN=$(TOOLCHAIN) !"
endif


#LIBS = -lnlib-$(CC_VERSION)-mt-s-0_0_3 -llog4cplus-$(CC_VERSION)-mt-s-1_0_2 -lmysqlclient-$(CC_VERSION)-mt-s-5.0.67 -lshared
ifeq ($(TOOLCHAIN),gcc-linux-m68k)
LIBS = -lnlib-$(CC_VERSION)-mt-s-0_0_3 -lshared
else
LIBS = -lnlib-$(CC_VERSION)-mt-s-0_0_3 -lmysqlclient-$(CC_VERSION)-mt-s-5.0.67 -lshared
endif

ifeq ($(TOOLCHAIN),gcc-linux-pc)
  # linux-pc tool chain
  #CPP_FLAGS += -DCPPLIB_LOG_LOG4CPLUS_ENABLED #enable log4cplus
  #CPP_FLAGS += -include all.h #inject include  to all sources
      
  LIBS += -ldl -lpthread
  
  #HACK:for compiling under linux
  LFLAGS += -L/usr/lib
  LIBS += -lmysqlclient
    
else ifeq ($(TOOLCHAIN),gcc-linux-m68k)
  # linux-m68k tool chain
  CPP_FLAGS += -DRUN_ON_VR900
  CPP_FLAGS += -D_LINUX_UCLIBC_
  #CPP_FLAGS+= -DBOOST_SP_USE_PTHREADS -DBOOST_AC_USE_PTHREADS
  CPP_FLAGS+= -DBOOST_SP_DISABLE_THREADS -DBOOST_AC_DISABLE_THREADS
  CPP_FLAGS += -DBOOST_ASIO_DISABLE_EPOLL
  
  LIBS += -ldl -lpthread

else ifeq ($(TOOLCHAIN),vcl-windows)
	#visual studio compiler
	CPP_INCLUDES = /I"$(OUT_DIR)" /I"inc"
	CPP_INCLUDES += /I"$(CPPLIB_PATH)/boost_1_35_0" /I"$(CPPLIB_PATH)/log4cplus_1_0_2" /I"$(CPPLIB_PATH)/nlib_0_0_3"

	#CPP_FLAGS = /D"CPPLIB_LOG_LOG4CPLUS_ENABLED" #enable log4cplus
	CPP_FLAGS += /D"WIN32" /D"_WINDOWS" /D"_DEBUG" /D"_WIN32_WINNT=0x0501"
	CPP_FLAGS += /W3 /EHsc /RTC1 /MTd 
	#CPP_FLAGS += /ZI /Gm /Fd $(MAIN_EXE:.exe=.pdb)
	
	LFLAGS =
	LIBS =
	
else
  abort "Undefined TOOLCHAIN=$(TOOLCHAIN) !"
endif



#//find a make compliente solution ! CPP_SRCS:= $(foreach dir, $(MY_SRC_DIR), $(wildcard $(dir)/*.cpp))
ifeq ($(TOOLCHAIN),vcl-windows)
	CPP_SRCS = $(shell find $(SRC_DIR) -iname '*.cpp' | grep -v -e "thread/pthread")
	CPP_OBJS = $(patsubst %.cpp, %.obj, $(addprefix $(OUT_DIR)/, $(CPP_SRCS))) 
	CPP_DEPS = $(CPP_OBJS:.obj=.d)

	TEST_SRCS = $(shell find $(TEST_DIR) -iname '*.cxx') 
	TEST_OBJS = $(patsubst %.cxx, %.obj, $(addprefix $(OUT_DIR)/, $(TEST_SRCS))) 
	TEST_DEPS = $(TEST_OBJS:.obj=.d)
	 
else
	CPP_SRCS = $(shell find $(SRC_DIR) -iname '*.cpp' | grep -v -e "thread/win32")
	CPP_OBJS = $(patsubst %.cpp, %.o, $(addprefix $(OUT_DIR)/, $(CPP_SRCS))) 
	CPP_DEPS = $(CPP_OBJS:.o=.d)

	TEST_SRCS = $(shell find $(TEST_DIR) -iname '*.cxx') 
	TEST_OBJS = $(patsubst %.cxx, %.o, $(addprefix $(OUT_DIR)/, $(TEST_SRCS))) 
	TEST_DEPS = $(TEST_OBJS:.o=.d)
endif
