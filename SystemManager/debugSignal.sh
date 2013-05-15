#!/bin/bash
set -x
echo
echo Send a SIGUSR2 signal to the current instance of the SystemManager to read the commands from the sm_command.ini.
echo If there would be any topology write request then the *.dot files will be generated to the subnetCaptures folder.
echo 

manual= echo "Available commands:
 		10 - Send a Read Attribute command to a field device with address from Parameter 
			Parameter - EUI64 address, TSAP, Object_ID, Attribute_ID .Ex: 10  0000:0000:0A10:0085 1 1 1 
		11 - Write SystemManager internal graph information
			Parameter- EUI64 address .Ex:11 0000:0000:0A10:0085
		12 - Read SystemManager time and TAI
			Parameter- EUI64 address .Ex:12 0000:0000:0A10:0085 
		13 - Prints the graphs contents from the given subnet ID 
			Parameter- EUI64 address, subnet id .Ex: 13 0000:0000:0A10:00A0 35 
		14 - Prints reserved chuncks from given subnet
			Parameter- EUI64 address, subnet id .Ex: 14 0000:0000:0A10:00A0 35
		15 - Prints reserved chuncks for a given device
			Parameter- EUI64 address, subnet id, device short address .Ex: 15 6302:0304:0506:0B01 35 2
		16 - Prints phy attributes of device
			Parameter- EUI64 address.Ex: 16 6302:0304:0506:0B01
		17 - Prints phy attributes of all devices"   .Ex: 17
		
	
if [ $# = 0 ] ; then
	echo type -h for help
	exit 1
fi

if [ $# = 1 ] ; then
	string="-h"
	if [ $1 = $string ] ; then
		echo -e $manual
		exit 1
#	else
#		echo type -h for help
	fi
#	exit 1
fi

if [ $1 = 10 ]; then
	if [ $# -ne 5 ]; then
		echo Not enough parameters
		exit 1
	fi		
fi
	
if [ $1 -eq 11 -o  $1 -eq 12 ]; then
	if [ $# -ne 2 ]; then
		echo Not enough parameters
		exit 1
	fi		
fi

if [ $1 -eq 13 -o $1 -eq 14 ]; then
	if [ $# -ne 3 ]; then
		echo Not enough parameters
		exit 1
	fi		
fi	

if [ $1 -eq 15 ]; then
	if [ $# -ne 4 ]; then
		echo Not enough parameters
		exit 1
	fi		
fi	

arguments=$1
numargs=$#
args=("$@")
for ((i=1 ; i < numargs ; i++)); do
		arguments=$arguments,"${args[$i]}"
done



COMMAND_FILE=./sm_command.ini



address=$(ini_editor -f $COMMAND_FILE -s SM_COMMANDS -v "COMMAND" -r)

if [ $address!=$arguments ] ; then
	address=$arguments
fi

ini_editor -f $COMMAND_FILE -s SM_COMMANDS -v  "COMMAND" -w $address

TMPDATE=`date +%Y.%m.%d_%H.%M.%S.%N`
SUBNET_CAPTURES_FOLDER=subnetCaptures

if [ -d $SUBNET_CAPTURES_FOLDER ] ;  then
	echo;
else
	echo create folder $SUBNET_CAPTURES_FOLDER
	mkdir $SUBNET_CAPTURES_FOLDER 
fi

if [ `ls -1 subnetCaptures/ | grep -c '\.dot'` -gt 0 ] ; then
	echo back up the existing files 
	echo create folder subnetCaptures/$TMPDATE
	mkdir $SUBNET_CAPTURES_FOLDER/$TMPDATE
			
	echo move existing files to subnetCaptures/$TMPDATE
	folder=$SUBNET_CAPTURES_FOLDER/$TMPDATE
	
	mv $SUBNET_CAPTURES_FOLDER/*\.dot* $folder
fi 

# send the signal
echo sends signal SIGUSR2 to SystemManager ...
kill -s SIGUSR2 `pgrep SystemManager`

# try to transform the dot files to image files if there would be any dot file generated
#sleep 1
#echo transforms the *dot files to *png
#for name in `find $SUBNET_CAPTURES_FOLDER -maxdepth 1 -iname '*.dot'`; do   
#	echo '    ' $name ---  $name.png; 
#	dot $name -Tpng > $name.png;
#done;

#echo 
