#! /bin/sh

#expects to be run from AccessNode parent folder
link=static
hw=pc
b_no=""

#[ ! -d AccessNode -o ! -d NetworkEngine -o ! -d SystemManager ] && echo "Should be run from AccessNode parent folder" && exit 1
[ ! -d AccessNode ] && echo "Should be run from AccessNode parent folder" && exit 1

(cd AccessNode/IsaGw/ISA100 && make -s release=isa link=${link} hw=${hw} clean_module_exe && make release=isa link=${link} hw=${hw} || exit 1 )

#MAKE="make -s TOOLCHAIN=m68k-unknown-linux-uclibc all"
#MAKE="make TOOLCHAIN=gcc-linux-pc TARGET_OSTYPE=linux-pc all"

#( cd cpplib/trunk/boost_1_36_0 && ${MAKE} )
#( cd cpplib/trunk/nlib && ${MAKE} )
#( cd cpplib/trunk/log4cplus_1_0_2 && ${MAKE} )
#( cd NetworkEngine && ${MAKE} || exit 1 )
#( cd SystemManager && ${MAKE} || exit 1 )

#mkdir -p AccessNode/config/FW_mesh_HW_${hw}/release_isa/SysManager/
#cp SystemManager/Release/SystemManager AccessNode/config/FW_mesh_HW_${hw}/release_isa/SysManager/

#( cd AccessNode && make -s cfast=no release=isa link=dynamic hw=${hw} b_no=${b_no} || exit 1 )