#!/bin/bash
#
#  This make commad must be used from  AccessNode/IsaGw/ISA100 folder.
# 

set -x

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
echo start building ...
make -s  cfast=no release=isa link=static hw=$HOST b_no=${b_no}

	
echo end building 
