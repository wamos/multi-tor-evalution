FWD_MODE=$1
IP_ADDR=$2
NUM=$3
RELAUNCH=$4
export RTE_SDK=~/efs/multi-tor-evalution/dpdk_deps/dpdk-20.08
export RTE_TARGET=x86_64-native-linuxapp-gcc

if [[ $(mount | grep nfs4) ]]; then
    echo "efs fs-89d4d58c mounted"
else
    sudo mount -t efs fs-89d4d58c:/ ~/efs
    if [ $? -eq "0" ]; then
        echo "efs fs-89d4d58c mounted"
    else
        echo "efs mount failed"
    fi
fi


PATH=$PATH:$HOME/.local/bin:$HOME/bin:/usr/sbin
export PATH

LD_LIBRARY_PATH=/usr/local/lib
export LD_LIBRARY_PATH

#cd efs/multi-tor-evalution
#cd onearm_lb/test-pmd/

USER=$(ps aux | grep ./build/app/testpmd | awk '{print $1}' | head -1)
if [[ "$USER" == "ec2-user" ]]; then
	echo "dpdk switch needs relaunch on" ${IP_ADDR}
	echo "check per-host config:"
	echo "switch_self_ip.txt"
	cat /tmp/switch_self_ip.txt
	echo "local_ip_list.txt"
	cat /tmp/local_ip_list.txt
	if [[ "$RELAUNCH" == "relaunch" ]]; then
		sudo python3 ${RTE_SDK}/usertools/dpdk-devbind.py --bind=ena 0000:00:06.0
		cd efs/multi-tor-evalution
		sh dpdk_switch_config.sh
		sh dpdk_setup_aws.sh
		cd onearm_lb/test-pmd/	
		if [[ "$FWD_MODE" == "5tswap" ]]; then
			sudo ./build/app/testpmd -l 0-7 -n 4 -- -a --portmask=0x1 --nb-cores=6 --forward-mode=5tswap > sw_${NUM}.log
		else
        	#sudo ./build/app/testpmd -l 0-7 -n 4 -- -a --portmask=0x1 --nb-cores=6 --forward-mode=replica-select
			sudo ./build/app/testpmd -l 0-5 -n 4 -- -a --portmask=0x1 --nb-cores=5 --forward-mode=replica-select --enable-info-exchange > sw_${NUM}.log
		fi
	fi
else
	echo "dpdk switch has launched on" ${IP_ADDR} #"with" ${FWD_MODE}
	ps aux | grep ./build/app/testpmd
	echo "display per-host config:"
	echo "switch_self_ip.txt"
	cat /tmp/switch_self_ip.txt
	echo "local_ip_list.txt"
	cat /tmp/local_ip_list.txt
	echo "------------------------"
fi
