#!/bin/bash

#set -x

# default names for files/directories included in the package
SYSTEM_MANAGER_BINARY="SystemManager"
SYSTEM_MANAGER_INI="system_manager.ini"
SYSTEM_MANAGER_CONFIG_INI="smConfigFiles/pc/config.ini"
SYSTEM_MANAGER_VERSIONS="ver_history.txt"
LOG4CPP_PROPERTIES="smConfigFiles/pc/log4cpp.properties"
#RESTART_SM="restart_sm.sh"
SCRIPTS_DIR="etc"
PC_RELEASE_DIR="out/release/linux-pc"
PUBLISH_DIR="/var/www/sm_builds"

PROFILE_DIR="bin"
FIRMWARE_DIR="bin"
LIBS_DIR="lib"
#KIT_SCRIPTS_DIR="/etc"

# the list of libraries
libraries="libboost_signals-gcc42-mt-1_34_1.so.1.34.1 \
libboost_thread-gcc42-mt-1_34_1.so.1.34.1 \
../local/lib/liblog4cpp.so.4.0.6"

# try to build the SystemManager
if [ -f ../SystemManagerAutomated/release_all.sh ]; 
then
	echo Run ../SystemManagerAutomated/release_all.sh to build the System Manager application
	../SystemManagerAutomated/release_all.sh
fi

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
    echo "Run the build_pc_package script inside the system manager project directory."
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

[ $UID == 0 ] || exit_error "You must have root privileges to run the script."

[ -d ${WORK_DIR}/src ] || exit_error "The working folder must contain a valid src: $WORK_DIR"
# read version
SM_VERSION=`grep SYSTEM_MANAGER_VERSION $WORK_DIR/src/RunLib/Version.h | grep -v APPLICATION_VERSION | cut -f 2 -d \"`

echo "Building version $SM_VERSION"


TMP_DIR="$WORK_DIR/tmp"
PACKAGE_DIR="$TMP_DIR/$$"
PC_RELEASE_PATH="$WORK_DIR/$PC_RELEASE_DIR"

# check if the PC libraries directory was valid
if [ "$1" != "" ]; then
    PC_LIBS_DIR="$1"
else
	PC_LIBS_DIR="/usr/lib"
    #PC_LIBS_DIR=`dirname "$WORK_DIR"`"/usr/lib"
fi
ls "$PC_LIBS_DIR" 2> /dev/null | grep "libboost_" &> /dev/null \
        || exit_error "Invalid PC libraries directory $PC_LIBS_DIR."

cleanup "$TMP_DIR"

# create the firmware directory
mkdir -p "$PACKAGE_DIR/$FIRMWARE_DIR" \
        || exit_error "Unable to create the firmware directory $FIRMWARE_DIR" "$TMP_DIR"
mkdir -p "$PACKAGE_DIR/$LIBS_DIR" \
        || exit_error "Unable to create the libs directory $LIBS_DIR" "$TMP_DIR"
mkdir -p "$PACKAGE_DIR/$PROFILE_DIR" \
        || exit_error "Unable to create the profile directory $PROFILE_DIR" "$TMP_DIR"
mkdir -p "$PACKAGE_DIR/SCRIPTS_DIR" \
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
cp -r "$WORK_DIR/$SCRIPTS_DIR" "$PACKAGE_DIR/" \
        || exit_error "Unable to copy $SCRIPTS_DIR." "$TMP_DIR"
rm -r $PACKAGE_DIR/$SCRIPTS_DIR/setup

echo $SM_VERSION > "$PACKAGE_DIR/version"

# copy libraries
for library in $libraries; do
    cp "$PC_LIBS_DIR/$library" "$PACKAGE_DIR/$LIBS_DIR" \
            || exit_error "Unable to copy library $library" "$TMP_DIR"
done

# create package file
pushd "$PACKAGE_DIR" &> /dev/null
archive_name="$SYSTEM_MANAGER_BINARY-$SM_VERSION-INSTALL.tar.gz"
tar czf "$archive_name" *
popd &> /dev/null
mv "$PACKAGE_DIR/$archive_name" "$WORK_DIR" \
        || exit_error "Unable to copy the package $SYSTEM_MANAGER_BINARY to the directory $WORK_DIR" "$TMP_DIR"
cp "$WORK_DIR/$archive_name" "$PUBLISH_DIR" \
        || exit_error "Unable to copy the package $SYSTEM_MANAGER_BINARY to dir $PUBLISH_DIR" "$TMP_DIR"
cp "$WORK_DIR/$SCRIPTS_DIR/setup/install.sh" "$PUBLISH_DIR/update.sh" \
        || exit_error "Unable to copy the package install.sh to dir $PUBLISH_DIR" "$TMP_DIR"
cleanup "$TMP_DIR"
