#!/bin/bash

#set -x

# default names for files/directories included in the package
SYSTEM_MANAGER_BINARY="SystemManager"
SYSTEM_MANAGER_ARCHIVE="System_Manager"
SYSTEM_MANAGER_INI="system_manager.ini"
SYSTEM_MANAGER_CONFIG_INI="smConfigFiles/pc/config.ini"
SYSTEM_MANAGER_VERSIONS="sm_ver_history.txt"
LOG4CPP_PROPERTIES="smConfigFiles/pc/log4cpp.properties"
#RESTART_SM="restart_sm.sh"
SCRIPTS_DIR="etc"
PC_RELEASE_DIR="out/release/linux-pc"
PUBLISH_DIR="/var/www/sm_builds"

DIST_DIR=s03
PROFILE_DIR="bin"
FIRMWARE_DIR="bin"
KIT_SCRIPTS_DIR="etc"

# try to build the SystemManager
#if [ -f ../SystemManagerAutomated/release_all.sh ]; 
#then
#	echo Run ../SystemManagerAutomated/release_all.sh to build the System Manager application
#	../SystemManagerAutomated/release_all.sh
#fi

cleanup() {
    tmp_dir="$1"
    base_dir=`dirname "$tmp_dir"`
    [ "$base_dir" != "" ] && [ "$base_dir" != "/" ] && rm -fr "$TMP_DIR"
}

exit_error()
{
    echo ""
    echo -e "ERROR: $1"
    echo "usage: ./build_pc_package"
    echo "Run the build_pc_package script inside the system manager project directory. "
    #echo "You can specify the local PC libraries project directory"
    #echo "(default /usr/lib/) as an argument to the script."
    [ "$2" != "" ] && cleanup "$2"
    exit 1
}


# set directories
WORK_DIR=`pwd`
if [ ! -d "$WORK_DIR/$PC_RELEASE_DIR" ]; then
    WORK_DIR=`dirname "$0"`
    [ ! -d "$WORK_DIR/$PC_RELEASE_DIR" ] \
            && exit_error "Unable to find the PC release directory.\nThe build_pc_package script must be located in the SystemManager project directory."
fi

#[ $UID == 0 ] || exit_error "You must have root privileges to run the script."

[ -d ${WORK_DIR}/src ] || exit_error "The working folder must contain a valid src: $WORK_DIR"

# read version
SM_VERSION=`grep SYSTEM_MANAGER_VERSION $WORK_DIR/src/Version.h | grep -v APPLICATION_VERSION | cut -f 2 -d \"`
SM_VERSION_INSTALL_SH=`grep -e 'version=' $WORK_DIR/$SCRIPTS_DIR/setup/$SM_VERSION/install.sh | cut -f 2 -d '='`
echo $SM_VERSION
echo $SM_VERSION_INSTALL_SH
if [ "$SM_VERSION" != "$SM_VERSION_INSTALL_SH" ]; then
	exit_error "The version from the intall.sh is not the same with the application version!"
fi 

echo "Building version $SM_VERSION"


TMP_DIR="$WORK_DIR/tmp"
PACKAGE_DIR="$TMP_DIR/$$"
PC_RELEASE_PATH="$WORK_DIR/$PC_RELEASE_DIR"

# check if the PC libraries directory was valid
#if [ "$1" != "" ]; then
#    PC_LIBS_DIR="$1"
#else
#	PC_LIBS_DIR="/usr/lib"
    #PC_LIBS_DIR=`dirname "$WORK_DIR"`"/usr/lib"
#fi
#ls "$PC_LIBS_DIR" 2> /dev/null | grep "libboost_" &> /dev/null \
#        || exit_error "Invalid PC libraries directory $PC_LIBS_DIR."

cleanup "$TMP_DIR"

# create the firmware directory
mkdir -p "$PACKAGE_DIR/$FIRMWARE_DIR" \
        || exit_error "Unable to create the firmware directory $FIRMWARE_DIR" "$TMP_DIR"
#mkdir -p "$PACKAGE_DIR/$LIBS_DIR" \
#        || exit_error "Unable to create the libs directory $LIBS_DIR" "$TMP_DIR"
mkdir -p "$PACKAGE_DIR/$PROFILE_DIR" \
        || exit_error "Unable to create the profile directory $PROFILE_DIR" "$TMP_DIR"
mkdir -p "$PACKAGE_DIR/$KIT_SCRIPTS_DIR" \
        || exit_error "Unable to create the profile directory $SCRIPTS_DIR" "$TMP_DIR"

# copy the SystemManager binary and ini files
cp "$PC_RELEASE_PATH/$SYSTEM_MANAGER_BINARY" "$PACKAGE_DIR/$FIRMWARE_DIR" \
        || exit_error "Unable to copy $SYSTEM_MANAGER_BINARY." "$TMP_DIR"
cp "$WORK_DIR/$SYSTEM_MANAGER_INI" "$PACKAGE_DIR/$PROFILE_DIR" \
        || exit_error "Unable to copy $SYSTEM_MANAGER_INI." "$TMP_DIR"
cp "$WORK_DIR/$SYSTEM_MANAGER_CONFIG_INI" "$PACKAGE_DIR/$PROFILE_DIR" \
        || exit_error "Unable to copy $SYSTEM_MANAGER_CONFIG_INI." "$TMP_DIR"
cp "$WORK_DIR/$SYSTEM_MANAGER_VERSIONS" "$PACKAGE_DIR/$PROFILE_DIR" \
        || exit_error "Unable to copy $SYSTEM_MANAGER_VERSIONS." "$TMP_DIR"
cp "$WORK_DIR/$LOG4CPP_PROPERTIES" "$PACKAGE_DIR/$PROFILE_DIR" \
        || exit_error "Unable to copy $LOG4CPP_PROPERTIES." "$TMP_DIR"
cp $WORK_DIR/$SCRIPTS_DIR/*.sh "$PACKAGE_DIR/$KIT_SCRIPTS_DIR" \
        || exit_error "Unable to copy *.sh." "$TMP_DIR"
chmod +x $PACKAGE_DIR/$KIT_SCRIPTS_DIR/*.sh

echo $SM_VERSION > "$PACKAGE_DIR/version"

# copy libraries
#for library in $libraries; do
#    cp "$PC_LIBS_DIR/$library" "$PACKAGE_DIR/$LIBS_DIR" \
#            || exit_error "Unable to copy library $library" "$TMP_DIR"
#done
#cp $WORK_DIR/../../../../NetworkEngine/trunk/NetworkEngine/Release/* "$PACKAGE_DIR/$LIBS_DIR"\
#           || exit_error "Unable to copy library $library" "$TMP_DIR"  

# create package file
pushd "$PACKAGE_DIR" &> /dev/null
pwd
archive_name="$SYSTEM_MANAGER_ARCHIVE-$SM_VERSION-INSTALL.tar.gz"
tar czf "$archive_name" *
rm -r ./bin
rm -r ./etc
#rm -r ./lib
rm version
mkdir S03
mv "$archive_name" S03
cp $WORK_DIR/$SCRIPTS_DIR/setup/$SM_VERSION/*.sh S03
distribution_archive_name="$SYSTEM_MANAGER_ARCHIVE-$SM_VERSION-UPDATE.tar.gz"
tar czf "$distribution_archive_name" *
#ls -l ./S03/
#ls -l
popd &> /dev/null
pwd
mv "$PACKAGE_DIR/S03/$archive_name" "$WORK_DIR" \
        || exit_error "Unable to copy the package $archive_name to the directory $WORK_DIR" "$TMP_DIR"
mv "$PACKAGE_DIR/$distribution_archive_name" "$WORK_DIR" \
        || exit_error "Unable to copy the package $distribution_archive_name to the directory $WORK_DIR" "$TMP_DIR"


#Check the publish location
if [ ! -d "$PUBLISH_DIR/$SM_VERSION" ]; then
sudo mkdir "$PUBLISH_DIR/$SM_VERSION"
fi
#copy the release o publish location
sudo cp "$WORK_DIR/$archive_name" "$PUBLISH_DIR/$SM_VERSION" \
        || exit_error "Unable to copy the package $archive_name to dir $PUBLISH_DIR" "$TMP_DIR"
sudo cp "$WORK_DIR/$distribution_archive_name" "$PUBLISH_DIR/$SM_VERSION" \
        || exit_error "Unable to copy the package $SYSTEM_MANAGER_BINARY to dir $PUBLISH_DIR/$SM_VERSION" "$TMP_DIR"
sudo cp "$WORK_DIR/$SCRIPTS_DIR/setup/$SM_VERSION/install.sh" "$PUBLISH_DIR/$SM_VERSION" \
        || exit_error "Unable to copy install.sh to dir $PUBLISH_DIR/$SM_VERSION"
sudo cp "$WORK_DIR/$SYSTEM_MANAGER_VERSIONS" "$PUBLISH_DIR/$SM_VERSION" \
        || exit_error "Unable to copy the ver_hiostory.txt to dir $PUBLISH_DIR/$SM_VERSION" "$TMP_DIR"

echo "Build completed. Archives copied to $PUBLISH_DIR/$SM_VERSION"
sudo ls -l "$PUBLISH_DIR/$SM_VERSION"
cleanup "$TMP_DIR"
