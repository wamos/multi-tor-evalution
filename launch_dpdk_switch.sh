LAUNCH_OR_KILL=$1
FWD_MODE=$2 #"replica-select" #"5tswap" #$2
RELAUNCH=$3

export RTE_SDK=~/efs/multi-tor-evalution/dpdk_deps/dpdk-20.08
export RTE_TARGET=x86_64-native-linuxapp-gcc

switch_ip_list=(172.31.33.204  #dpdk-switch-0 eth0
                172.31.36.241  #dpdk-switch-1 eth0
                172.31.45.234) #dpdk-switch-2 eth0

n=0 #switch_index
for i in "${switch_ip_list[@]}"
do
	echo ${LAUNCH_OR_KILL} " dpdk-switch-" ${n} " on " ${i}
	if [[ "$LAUNCH_OR_KILL" == "launch" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${i} 'sh -s' < run_dpdk_switch_launch.sh ${FWD_MODE} ${n} 2>&1 &
	elif [[ "$LAUNCH_OR_KILL" == "check" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${i} 'sh -s' < run_dpdk_switch_check.sh ${FWD_MODE} ${i} ${n} ${RELAUNCH} 2>&1 &
		sleep 2
	elif [[ "$LAUNCH_OR_KILL" == "config" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${i} 'sh -s' < run_dpdk_switch_config.sh ${n} 2>&1 &
	else
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${i} 'sh -s' < run_dpdk_kill.sh 2>&1 &
	fi
	n=$((n+1))
done

echo "ssh commands:"
for i in "${switch_ip_list[@]}"
do
     	echo "ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@"${i} 
done
echo ""

#cd ~/efs/multi-tor-evalution
#sh run_switch_config.sh
#sh dpdk_setup_aws.sh

