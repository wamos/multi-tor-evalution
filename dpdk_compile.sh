#
# Sets RTE_TARGET and does a "make install".
#

HUGEPGSZ=`cat /proc/meminfo  | grep Hugepagesize | cut -d : -f 2 | tr -d ' '`
cd $RTE_SDK
echo $RTE_SDK
setup_target()
{
	export RTE_TARGET=x86_64-native-linuxapp-gcc
	# set CONFIG_RTE_EAL_IGB_UIO=y, so we can have igb_uio kernel module
	echo "CONFIG_RTE_EAL_IGB_UIO=y" | tee -a $RTE_SDK/config/defconfig_$RTE_TARGET

	make config install -j8 T=${RTE_TARGET} DESTDIR=$RTE_SDK MAKE_PAUSE=n

	echo "------------------------------------------------------------------------------"
	echo " RTE_TARGET exported as $RTE_TARGET"
	echo "------------------------------------------------------------------------------"
	cp ~/efs/igb_uio.ko $RTE_SDK/$RTE_TARGET/kmod/ 
}

setup_target

