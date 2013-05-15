#!/bin/bash
     
# default names for files/directories included in the package
SYSTEM_MANAGER_BINARY="SystemManager"
SYSTEM_MANAGER_INI="system_manager.ini"
SYSTEM_MANAGER_CONFIG_INI="config.ini"
SYSTEM_MANAGER_VERSIONS="ver_history.txt"
LOG4CPP_PROPERTIES="log4cpp.properties"
RESTART_SM="restart_sm.sh"
PC_RELEASE_DIR="Release_Linux"
PUBLISH_DIR="/var/www/sm_builds/"

PROFILE_DIR="/local/bin"
FIRMWARE_DIR="/local/bin"
LIBS_DIR="/local/lib"

# the list of libraries
libraries="libboost_signals-gcc42-mt-1_34_1.so.1.34.1 \
libboost_thread-gcc42-mt-1_34_1.so.1.34.1 \
../local/lib/liblog4cpp.so.4.0.6"


cleanup() {
    tmp_dir="$1"
    base_dir=`dirname "$tmp_dir"`
    [ "$base_dir" != "" ] && [ "$base_dir" != "/" ] && rm -fr "$TMP_DIR"
}

exit_error()
{
    echo ""
    echo -e "ERROR: $1"
    echo "usage: ./build_pc_package [pc_libs_project_dir]"
    echo "Run the build_pc_package script inside the system manager project"
    echo "directory. You can specify the local PC libraries project directory"
    echo "(default /usr/lib/) as an argument to the script."
    [ "$2" != "" ] && cleanup "$2"
    exit 1
}


# read version
SM_VERSION=`grep SYSTEM_MANAGER_VERSION ./src/RunLib/Version.h | grep -v APPLICATION_VERSION | cut -f 2 -d \"`


# set directories
WORK_DIR=`pwd`
if [ ! -d "$WORK_DIR/$PC_RELEASE_DIR" ]; then
    WORK_DIR=`dirname "$0"`
    [ ! -d "$WORK_DIR/$PC_RELEASE_DIR" ] \
            && exit_error "Unable to find the PC release directory.\nThe build_pc_package script must be located in the SystemManager project directory."
fi

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
cp "$WORK_DIR/$RESTART_SM" "$PACKAGE_DIR/$PROFILE_DIR" \
        || exit_error "Unable to copy $RESTART_SM." "$TMP_DIR"

# copy libraries
for library in $libraries; do
    cp "$PC_LIBS_DIR/$library" "$PACKAGE_DIR/$LIBS_DIR" \
            || exit_error "Unable to copy library $library" "$TMP_DIR"
done

# create package file
pushd "$PACKAGE_DIR" &> /dev/null
archive_name="$SYSTEM_MANAGER_BINARY-$SM_VERSION-PC.tar.gz"
tar czf "$archive_name" *
popd &> /dev/null
mv "$PACKAGE_DIR/$archive_name" "$WORK_DIR" \
        || exit_error "Unable to copy the package $SYSTEM_MANAGER_BINARY to the directory $WORK_DIR" "$TMP_DIR"
cp "$WORK_DIR/$archive_name" "$PUBLISH_DIR" \
        || exit_error "Unable to copy the package $SYSTEM_MANAGER_BINARY to dir $PUBLISH_DIR" "$TMP_DIR"
cleanup "$TMP_DIR"
