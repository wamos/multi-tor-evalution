line_num=$1
IP_ADDR=$2
RELAUNCH=$3
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
	if [[ "$RELAUNCH" == "relaunch" ]]; then
		sudo python3 ${RTE_SDK}/usertools/dpdk-devbind.py --bind=ena 0000:00:06.0
		cd efs/multi-tor-evalution
		sh dpdk_client_config.sh ${line_num}
		sh dpdk_setup_aws.sh
		cd onearm_lb/test-pmd-clean-state/
		sudo ./build/app/testpmd -l 0-4 -n 4 -- -a --portmask=0x1 --nb-cores=1 --forward-mode=txonly --lambda_rate=50000 > 50k_${line_num}.log
	fi
	echo "------------------------"
else
	echo "dpdk client has launched on" ${IP_ADDR}
	echo "display per-host config:"
    echo "client_self_ip.txt"
    cat /tmp/client_self_ip.txt
    echo "replica_addr_list.txt"
    cat /tmp/replica_addr_list.txt
	echo "------------------------"
fi
