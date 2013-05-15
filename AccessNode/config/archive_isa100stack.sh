set -x
cp AccessNode/config/build_isa100_stack.sh .
tar czvf AccessNode.tgz build_isa100_stack.sh AccessNode/Makefile AccessNode/sys_inc.mk AccessNode/system.mk AccessNode/IsaGw/ISA100/  AccessNode/Shared/MicroSec.* AccessNode/Shared/UdpSocket.*  AccessNode/Shared/Socket.*  AccessNode/Shared/UtilsSolo.*  AccessNode/Shared/log_callback.*  AccessNode/Shared/Common.h AccessNode/Shared/AnPaths.h 
