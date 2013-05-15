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



TOP:=../..
SRC_DIR=src

#$(shell ln -sf ../AccessNode/sys_inc.mk .)
#$(shell ln -sf ../AccessNode/system.mk .)

include $(TOP)/sys_inc.mk
#override link=static


CC_VERSION?=$(shell $(CC) --version | grep -e '^[0-9a-zA-Z_-]*gcc (.*)' | sed -e 's/[0-9a-zA-Z_-]*gcc (.*) \([0-9]\)\.\([0-9]\).*/gcc\1\2/')


MonitorHost.FILES=$(call GLOB,$(SRC_DIR),'*.cpp' | grep -v -e "thread/win32" | grep -v -e "MySql" | grep -v -e "AsyncTCPServerSocket")

ifeq "$(host)" "i386"
MonitorHost.FILES:=$(call GLOB,$(SRC_DIR),'*.cpp'| grep -v -e "thread/pthread")
endif

MonitorHost.FILES+=$(SHRD_DIR)/TcpSecureSocket

ifeq "$(link)" "static"
	SQLITEXX_LIB_LINK=$(OUT_SHRD_DIR)/SqliteUtil/libsqlitexx.a
else
	SQLITEXX_LIB_LINK=-lsqlitexx  -L$(OUT_SHRD_DIR)/SqliteUtil/
endif

MonitorHost.SUBDIRS = $(SHRD_DIR) $(SHRD_DIR)/SqliteUtil/
MonitorHost.LDFLAGS  =  $(SHRD_LIB_LINK)

MonitorHost.CXXFLAGS = $(BOOST_CXXFLAGS) -I$(CPPLIB_DIR)/nlib -I$(AUX_LIBS_DIR_INC)
MonitorHost.CXXFLAGS+= -I. -I$(CPPLIB_DIR)/mysql-5.0.67 -I$(shell pwd) -I$(TOP)
MonitorHost.CXXFLAGS+= -D_LINUX_UCLIBC_
MonitorHost.CXXFLAGS+= -Wall -fmessage-length=0 -DHAVE_SSTREAM -DHAVE_GETTIMEOFDAY
MonitorHost.CXXFLAGS+= -Wno-shadow
MonitorHost.LDFLAGS +=  $(SQLITEXX_LIB_LINK) -lsqlite3 -L$(TOP)/Shared/lib/$(host) $(AXTLS_LIB_LINK)


#
ifeq "$(hw)" "vr900"
MonitorHost.LDFLAGS +=  -lboost-system-gcc43-mt-s-1_36_0 -L$(CPPLIB_DIR)/boost_1_36_0/lib/linux-m68k
#MonitorHost.LDFLAGS += -Wl,-Map,Mapfile.map
MonitorHost.CXXFLAGS+= -DBOOST_SP_DISABLE_THREADS -DBOOST_AC_DISABLE_THREADS
MonitorHost.CXXFLAGS+= -DUSE_SQLITE_DATABASE
endif

ifeq "$(host)" "i386"
#lmysqlclient
MonitorHost.LDFLAGS += -static-libgcc $(shell g++ -print-file-name=libstdc++.a)

MonitorHost.LDFLAGS += $(CPPLIB_DIR)/mysql-5.0.67/lib/linux-pc/libmysqlclient-gcc42-mt-s-5.0.67.a
MonitorHost.LDFLAGS += $(CPPLIB_DIR)/boost_1_36_0/lib/linux-pc/libboost-system-gcc43-mt-s-1_36_0.a -ldl -lpthread  -L/usr/lib -lz
MonitorHost.CXXFLAGS+=-DUSE_MYSQL_DATABASE
#MonitorHost.CXXFLAGS+= -DUSE_SQLITE_DATABASE
ARCH=pc
endif


TARGET.PROGRAMS = MonitorHost
export EXE_COPY_HOOK:=an_copy

include $(TOP)/system.mk

$(info $(ARCH))

DB3:=$(OUTPUT_DIR)/db/Monitor_Host.db3
makedb:etc/sqlite/*.sql
	$(MKOBJDIR)
	mkdir -p $(OUTPUT_DIR)/db/
	rm -rf $(DB3)
	@echo CreateSchema.sql && sqlite3 $(DB3) < $(CURDIR)/etc/sqlite/CreateSchema.sql
	@echo CreateDefaultData.sql && sqlite3 $(DB3) < $(CURDIR)/etc/sqlite/CreateDefaultData.sql
	chmod 0666 $(DB3)
	@echo ' * GEN $(DB3)'

an_copy: MonitorHost makedb
	[ ! -d $(CONFIG_DIR)/etc ] && mkdir $(CONFIG_DIR)/etc || exit 0
	cp -r etc/*.sh  $(CONFIG_DIR)/etc
	cp $(DB3) $(TOP)/an/Monitor_Host.db3.fixture


DIST_NAME=S04
DIST_VERSION=$(shell grep M_H_VERSION ./src/Version.h | cut -f 2 -d \")
DISTRIBUTION_TGZ = Monitor_Host-$(DIST_VERSION)-linux-$(ARCH).tgz
UPGRADE_DISTRIBUTION_TGZ=Upgrade-$(DIST_NAME)-Monitor_Host-$(DIST_VERSION)-linux-$(ARCH).tgz



.PHONY:$(DISTRIBUTION_TGZ) makedb
dist: exe_copy $(DISTRIBUTION_TGZ)
dist-upgrader: $(UPGRADE_DISTRIBUTION_TGZ)


$(DISTRIBUTION_TGZ):
	rm -rf $(AN_DIR)
	$(MKANDIR)
	make exe_copy
	make makedb
	mkdir -p $(AN_DIR)/etc
	chmod 0600 $(AN_DIR)/etc/*
	cp -r etc/*.sh $(AN_DIR)/etc/
	chmod 0700 $(AN_DIR)/etc/*.sh
	tar -czf $@ -C $(AN_DIR)/ .
	@echo "created '$@' file."


$(UPGRADE_DISTRIBUTION_TGZ): $(DISTRIBUTION_TGZ) etc/setup/*.*
	rm -rf $(DIST_NAME)
	mkdir $(DIST_NAME)
	cp $(DISTRIBUTION_TGZ) $(DIST_NAME)
	cp etc/setup/*.sh  $(DIST_NAME)
	chmod 0770  $(DIST_NAME)/*.sh
	cp -r etc/setup/*.sql $(DIST_NAME)
	cp -r etc/setup/*.awk $(DIST_NAME)
	tar -czf $@ $(DIST_NAME)
	@echo "created '$@' file."


$(UPGRADE_DISTRIBUTION_TGZ_FAKE): $(UPGRADE_DISTRIBUTION_TGZ)
	mkdir $(OUT_DIR)/dist_upgrader_fake_temp/
	rm -r $(OUT_DIR)/dist_upgrader_fake_temp/*
	#untar current upgrader.tgz
	( cd $(OUT_DIR)/dist_upgrader_fake_temp && tar -opxzf $(PWD)/$(UPGRADE_DIST_TAR_FILE) )
	#rename version
	( cd $(OUT_DIR)/dist_upgrader_fake_temp/$(DIST_NAME) && mv `ls $(PROJECT_NAME)*.tgz` $(PROJECT_NAME)-$(DIST_VERSION).fake-$(TARGET_OSTYPE).tgz )
	tar -czf $@ -C $(OUT_DIR)/dist_upgrader_fake_temp .
	echo "created '$@' file."
