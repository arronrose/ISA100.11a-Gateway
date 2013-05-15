#! /bin/bash -e

#expects to be run from AccessNode parent folder
link=dynamic
hw="vr900"
c_action=all
b_no=""
silent="-s"
procs="-j4"
c_fast="no"
TOOLCHAIN="m68k-unknown-linux-uclibc"
host=""

DisplayHelp()
{
	echo "Usage: $1 <OPTIONS>"
	echo "OPTIONS:"
	echo "	hw					vr900"
	echo "	b_auto				(if provided): auto-increment build number"
	echo "	checkout			perform a source checkout of all modules"
	echo "	update				perform a source update in all directories"
	echo "	clean				run make clean in SM/NE/AN only (does NOT clean cpplib)"
	echo "	silent	[yes|no]	run make silently"
	echo "	-h|--help|help	print me"
}

Checkout()
{
	scm=$1
	module=$2
	parm=""
	echo "Checkout: $2"
	if [ "$1" = "cvs" ]; then
		echo "Enter your CVS username"
		read user
		parm="-q -d :pserver:$user@manga:/home/cvs"
	fi
	$1 ${parm} checkout $2
}

Update()
{
	dir=$1
	pwd="`pwd`"

	echo "Update: $dir"
	Chdir $dir
	if [ -d CVS ]; then
		cvs -q up -dP
		cd "$pwd"
		return
	fi
	if [ -d .svn ]; then
		svn up
		cd "$pwd"
		return
	fi
	echo "No versioning control found in $dir"
	cd "$pwd"
}

UpdateSources()
{
	echo "Updating all sources"
	Update cpplib/trunk/nlib/
	Update cpplib/trunk/nbuild
	Update cpplib/trunk/boost_1_36_0
	Update cpplib/trunk/log4cplus_1_0_2/
	Update SystemManager/
	Update NetworkEngine/
	Update MCSWebsite/
	Update AccessNode/
	Update AuxLibs/
}

CheckoutSources()
{
	echo "Checkout all sources"
	Checkout svn https://cljsrv01.nivis.com:8443/svn/cpplib
	Checkout svn https://cljsrv01.nivis.com:8443/svn/svnroot/branches/opensource-developer-isa-2.5.4/SystemManager
	Checkout svn https://cljsrv01.nivis.com:8443/svn/svnroot/branches/opensource-developer-isa-2.5.4/NetworkEngine
	Checkout svn https://cljsrv01.nivis.com:8443/svn/svnroot/branches/opensource-developer-isa-2.5.4/MCSWebsite
	Checkout cvs AuxLibs
	Checkout cvs AccessNode
	[ ! -L boost ] && ln -sf cpplib/trunk/boost_1_36_0 boost
}

GetHwType()
{
	case $1 in
		vr900 )
			TOOLCHAIN=m68k-unknown-linux-uclibc
			host=m68k-unknown-linux-uclibc
		;;
		i386 | pc )
			TOOLCHAIN=gcc-linux-pc
			host=i386
		;;
		* )
		echo "Error: hw=$1 not supported"
		exit 1
		;;
	esac
}

Make()
{
	cmd="$*"
	echo
	echo "================================================================"
	echo "-> make $*"
	make $*
}

Chdir()
{
	echo "-> entering $*"
	if [ ! -d $* ]; then
		echo "   error: no such directory:$*"
		exit 1
	fi
	cd $*
}

while test -n "$*"; do
	case "$1" in
		"help"|"-h"|"--help")
			DisplayHelp $0
			exit 0
			;;
		"checkout")
			CheckoutSources
			exit 0
			;;
		"update" )
			UpdateSources
			exit 0
			;;
		"clean" )
			c_action=clean
			shift
			;;
		"all" )
			c_action=all
			shift
			;;
		"procs")
			procs=$2
			shift 2
			;;
		"silent")
			if [ "$2" = "no" ]; then
				silent=""
			fi
			shift 2
			;;
		"b_auto")
			[ ! -f "b_no.txt" ] && touch "b_no.txt"
			b_no="`cat b_no.txt`"
			[ -z "$b_no" ] && b_no=0
			let b_no=b_no+1
			echo ${b_no} > b_no.txt
			b_no="_b"${b_no}
			shift
			;;
		"" )
			shift
			;;
		* )
			GetHwType $1
			hw=$1
			shift
			;;
	esac
done

echo "Making $c_action for $hw"
echo


[ ! -d AccessNode -o ! -d NetworkEngine -o ! -d SystemManager ] && echo "Should be run from AccessNode parent folder" && exit 1

if [ "$c_action" = "clean" ]; then
	echo "Cleaning ONLY AccessNode/SystemManager/NetworkEngine"
	Chdir AccessNode
	Make ${silent} hw=$hw clean
	cd ..
	Chdir SystemManager
	Make ${silent} TOOLCHAIN=$TOOLCHAIN clean
	cd ..
	Chdir NetworkEngine
	Make ${silent} TOOLCHAIN=$TOOLCHAIN clean
	cd ..
	echo "TAKE CARE: nlib/nbuild/boost/log4cplus were NOT cleaned"
	exit
fi

rm -f AccessNode/out/${host}/Shared/objs/app.o
rm -f AccessNode/out/${host}/IsaGw/Monitor_Host/objs/src/ConfigApp.o

if [ "$hw" = "i386" -o "$hw" = "pc" ]; then
	Chdir AccessNode/IsaGw/
else
	Chdir AccessNode/IsaGw/ISA100
fi

Make ${silent} release=isa link=${link} hw=${hw} clean_module_exe
Make ${silent} release=isa link=${link} hw=${hw} ${c_action} cfast=$c_fast
cd -

MAKE="Make ${silent} TOOLCHAIN=$TOOLCHAIN link_stack=$link ${c_action} $procs"
# For Debug version: uncomment below (for addr2line). See SystemManager/Debug/SystemManager
#MAKE="make ${silent} TOOLCHAIN=$TOOLCHAIN link_stack=$link ${c_action} $procs DEBUG=true"

( Chdir cpplib/trunk/boost_1_36_0 && ${MAKE} )
( Chdir cpplib/trunk/nlib && ${MAKE} )
( Chdir cpplib/trunk/log4cplus_1_0_2 && ${MAKE} )
( Chdir NetworkEngine && ${MAKE} )

Chdir SystemManager
rm -f Release/*
find out -name SystemManager -type f -print | xargs /bin/rm -f
${MAKE}
[ $? -ne 0 ] && cd .. && exit 1
cd -

[ "$hw" = "i386" -o "$hw" = "pc" ] && exit 0

cp SystemManager/Release/SystemManager AccessNode/config/FW_mesh_HW_${hw}/release_isa/SysManager/

Chdir AccessNode
Make ${silent} cfast=$c_fast release=isa link=${link} hw=${hw} b_no=${b_no}
[ $? -ne 0 ] && cd .. && exit 1

[ ! -d ../releases.debug ] && mkdir ../releases.debug
[ -f last_build ] && . last_build
DEBUG_SM="../releases.debug/SystemManager_${hw}_${SW_VERSION}"
DEBUG_MH="../releases.debug/MonitorHost_${hw}_${SW_VERSION}"

echo ""
if [ -f ${DEBUG_SM} ]; then
    echo "Debug binary ${DEBUG_SM} already there. Keep OLD"
else
    echo "Save debug binary ${DEBUG_SM}"
    cp ../SystemManager/Debug/SystemManager ${DEBUG_SM}	# save debug version
fi
if [ -f ${DEBUG_MH} ]; then
    echo "Debug binary ${DEBUG_MH} already there. Keep OLD"
else
    echo "Save debug binary ${DEBUG_MH}"
    cp ../AccessNode/out/m68k-unknown-linux-uclibc/IsaGw/Monitor_Host/MonitorHost ${DEBUG_MH}	# save debug version (CF ONLY!!)
fi
cd -
