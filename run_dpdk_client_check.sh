line_num=$1
IP_ADDR=$2
RELAUNCH=$3

IP_ADDR=$1
INDEX=$2 # we use line num aka index in replica addr list to set up default dest 
LAMBDA=$3
LOGFILE=$4
RANDOM=$5
SELECT=$6
RELAUNCH=$7

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
#sh dpdk_client_config.sh ${line_num}
#sh dpdk_setup_aws.sh
#cd onearm_lb/test-pmd-clean-state/

USER=$(ps aux | grep ./build/app/testpmd | awk '{print $1}' | head -1)
if [[ "$USER" == "ec2-user" ]]; then
	echo "dpdk client needs relaunch on" ${IP_ADDR}
	echo "chekc per-host config:"
	echo "client_self_ip.txt"
	cat /tmp/client_self_ip.txt 
	echo "replica_addr_list.txt"
	cat /tmp/replica_addr_list.txt
	ifconfig | grep eth1
	echo "hugepages"
	cat  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
	if [[ "$RELAUNCH" == "relaunch" ]]; then
		cd efs/multi-tor-evalution/onearm_lb/test-pmd-clean-state/
		sh run_dpdk_client_launch.sh ${ip} ${n} ${LAMBDA} ${LOGFILE} ${RANDOM} ${SELECT}
		#sudo ./build/app/testpmd -l 0-4 -n 4 -- -a --portmask=0x1 --nb-cores=1 --forward-mode=txonly --lambda_rate=20000 > dpdk_${line_num}.log
	fi
	echo "------------------------"
else
	echo "dpdk client has launched on" ${IP_ADDR}
	echo "display per-host config:"
    echo "client_self_ip.txt"
    cat /tmp/client_self_ip.txt
    echo "replica_addr_list.txt"
    cat /tmp/replica_addr_list.txt
	ps aux | grep testpmd
	echo "------------------------"
fi
