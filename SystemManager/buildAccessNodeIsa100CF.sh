#!/bin/bash
# 
# This make command is initiated from SysMgr directory.
# Execution moves to AccessNode/IsaGw/ISA100 folder.
#

echo
cd ../AccessNode
echo Current folder : `pwd`

# Detect host parameter or default
if [ $1 = ""]; then
	echo No host specified. Detect default host ...	
	if [ "`uname`" = "Linux" ]; then
		HOST=i386
	else
		HOST=cyg	
	fi;
else 
	HOST=$1
fi;

echo Using  host : $HOST 
echo

# clean the build folder  
echo clean IsaGw/Isa100 ...
make clean
echo


# build library
cd IsaGw/ISA100
echo Current folder : `pwd`
echo start building using gcc-linux-m68k tools for VR900 HW  ...
#make dist=mesh hw=vr900 release=isa link=static TOOLCHAIN=gcc-linux-m68k
#make hw=vr900 link=static -s
#make ${silent} cfast=no release=isa link=${link} hw=${hw} b_no=${b_no}
make -s cfast=no release=isa link=static hw=vr900 && echo end building
[ $? -ne 0 ] && exit 1 
