#run_dpdk_switch_check.sh  ${FWD_MODE} ${ip} ${n} ${LOGFILE} ${GOSSIP} ${LOAD_DELTA} ${RELAUNCH} 2>&1 &
FWD_MODE=$1
IP_ADDR=$2
INDEX=$3
LOGFILE=$4
GOSSIP=$5
LOAD_DELTA=$6
LAMBDA=$7
REDIRECT_BOUND=$8
RELAUNCH=$9
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
	echo "dpdk-switch-" ${INDEX} " needs relaunch on" ${IP_ADDR}
	echo "check per-host config:"
	echo "switch_self_ip.txt"
	cat /tmp/switch_self_ip.txt
	echo "local_ip_list.txt"
	cat /tmp/local_ip_list.txt
	ifconfig | grep eth1
	echo "hugepages"
	cat  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
	if [[ "$RELAUNCH" == "relaunch" ]]; then
		sh run_dpdk_switch_launch.sh ${FWD_MODE} ${IP_ADDR} ${INDEX} ${LOGFILE}\
		${GOSSIP} ${LOAD_DELTA} ${LAMBDA} ${REDIRECT_BOUND}
	fi
else
	echo "dpdk-switch-" ${INDEX} "has launched on" ${IP_ADDR} #"with" ${FWD_MODE}
	ps aux | grep ./build/app/testpmd
	echo "display per-host config:"
	echo "switch_self_ip.txt"
	cat /tmp/switch_self_ip.txt
	echo "local_ip_list.txt"
	cat /tmp/local_ip_list.txt
	echo "------------------------"
fi
