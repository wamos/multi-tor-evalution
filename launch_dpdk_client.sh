LAUNCH_OR_KILL=$1

export RTE_SDK=~/efs/multi-tor-evalution/dpdk_deps/dpdk-20.08
export RTE_TARGET=x86_64-native-linuxapp-gcc

client_ip_list=(172.31.46.203  #rackclient-dpdk-client-0-eth0
		172.31.33.152  #rackclient-dpdk-client-1-eth0
		172.31.40.245) #rackclient-dpdk-client-2-eth0

echo "ssh commands:"
echo ""

for i in "${client_ip_list[@]}"
do
        echo "ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@"${i}
done

for i in "${client_ip_list[@]}"
do
        echo ${LAUNCH_OR_KILL} " dpdk client on " ${i}
	if [[ "$LAUNCH_OR_KILL" == "launch" ]]; then
        	ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${i} 'sh -s' < run_dpdk_client_launch.sh 2>&1 &
	else
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${i} 'sh -s' < run_dpdk_kill.sh 2>&1 &
	fi
done

