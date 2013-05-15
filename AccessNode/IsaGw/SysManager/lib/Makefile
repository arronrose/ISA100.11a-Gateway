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



TOP := ../../../

LIBS := libboost_signals-gcc34-mt-1_34_1.so.1.34.1 \
	libboost_thread-gcc34-mt-1_34_1.so.1.34.1

all:
	if ! test -d $(TOP)/an/lib ; then mkdir -p $(TOP)/an/lib; fi
	cp -f $(LIBS) $(TOP)/an/lib
	cd $(TOP)/an/lib ;\
		ln -sf libboost_signals-gcc34-mt-1_34_1.so.1.34.1 libboost_signals-gcc34-mt-1_34_1.so  ;\
		ln -sf libboost_signals-gcc34-mt-1_34_1.so.1.34.1 libboost_signals-gcc34-mt.so ;\
		ln -sf libboost_thread-gcc34-mt-1_34_1.so.1.34.1  libboost_thread-gcc34-mt-1_34_1.so ;\
		ln -sf libboost_thread-gcc34-mt-1_34_1.so.1.34.1  libboost_thread-gcc34-mt.so

clean:
