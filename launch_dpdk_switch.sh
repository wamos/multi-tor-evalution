LAUNCH_OR_KILL=$1
FWD_MODE="replica-select" #"5tswap" #$2
LOGFILE=$2 
LAMBDA=$3
#GOSSIP="100"
#LOAD_DELTA="1"
GOSSIP=$4
LOAD_DELTA=$5
REDIRECT_BOUND=$6
RELAUNCH=$7

RATE_in_k=$((LAMBDA / 1000))
echo "rate:"$RATE_in_k"kRPS"

export RTE_SDK=~/efs/multi-tor-evalution/dpdk_deps/dpdk-20.08
export RTE_TARGET=x86_64-native-linuxapp-gcc

switch_ip_list=(172.31.33.204  #dpdk-switch-0 eth0
                172.31.36.241  #dpdk-switch-1 eth0
                172.31.45.234) #dpdk-switch-2 eth0

n=0 #switch_index
for ip in "${switch_ip_list[@]}"
do
	echo ${LAUNCH_OR_KILL} " dpdk-switch-" ${n} " on " ${ip}
	if [[ "$LAUNCH_OR_KILL" == "launch" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${ip} 'sh -s' < run_dpdk_switch_launch.sh ${FWD_MODE} ${ip} ${n} ${LOGFILE} ${GOSSIP} ${LOAD_DELTA} ${RATE_in_k}"k" ${REDIRECT_BOUND} 2>&1 &
		#echo "launch!"
	elif [[ "$LAUNCH_OR_KILL" == "check" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${ip} 'sh -s' < run_dpdk_switch_check.sh  ${FWD_MODE} ${ip} ${n} ${LOGFILE} ${GOSSIP} ${LOAD_DELTA} ${RATE_in_k}"k" ${REDIRECT_BOUND} ${RELAUNCH} 2>&1 &
		sleep 2
	elif [[ "$LAUNCH_OR_KILL" == "config" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${ip} 'sh -s' < run_dpdk_switch_config.sh ${FWD_MODE} ${ip} ${n} 2>&1 &
	elif [[ "$LAUNCH_OR_KILL" == "stop" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${ip} 'sh -s' < run_dpdk_kill.sh SIGTERM 2>&1 &
	elif [[ "$LAUNCH_OR_KILL" == "kill" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${ip} 'sh -s' < run_dpdk_kill.sh SIGKILL 2>&1 &
	else
		echo "invalid command, try again!"
		echo "sh launch_dpdk_switch.sh {launch/check/stop} {log_file_name} {relaunch(optional)}"
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

