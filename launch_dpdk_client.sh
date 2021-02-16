LAUNCH_OR_KILL=$1
LOGFILE=$2
LAMBDA=$3
RAND=$4 #"enable-random-dest"
SELECT=$5 #"enable-replica-select"
RELAUNCH=$6 #"relaunch" or other 
FWD_MODE="txonly"

echo "launch-script:" $RAND
echo "launch-script:" $SELECT

export RTE_SDK=~/efs/multi-tor-evalution/dpdk_deps/dpdk-20.08
export RTE_TARGET=x86_64-native-linuxapp-gcc

client_ip_list=(172.31.46.203  #rackclient-dpdk-client-0-eth0
		172.31.33.97  #rackclient-dpdk-client-1-eth0
		172.31.40.126) #rackclient-dpdk-client-2-eth0

echo "ssh commands:"
for i in "${client_ip_list[@]}"
do
	echo "ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@"${i}
done
echo ""

n=0 #n is our client index 
for ip in "${client_ip_list[@]}"
do
	echo ${LAUNCH_OR_KILL} " dpdk client on " ${ip}
	if [[ "$LAUNCH_OR_KILL" == "launch" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${ip} 'sh -s' < run_dpdk_client_launch.sh ${ip} ${n} ${LAMBDA}\
		${LOGFILE} ${RAND} ${SELECT} 2>&1 &
	elif [[ "$LAUNCH_OR_KILL" == "check" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${ip} 'sh -s' < run_dpdk_client_check.sh ${ip} ${n} ${LAMBDA}\
		${LOGFILE} ${RAND} ${SELECT} ${RELAUNCH} 2>&1 &
		sleep 1
	elif [[ "$LAUNCH_OR_KILL" == "config" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${ip} 'sh -s' < run_dpdk_client_config.sh ${ip} ${n} 2>&1 &
	elif [[ "$LAUNCH_OR_KILL" == "stop" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${ip} 'sh -s' < run_dpdk_kill.sh SIGINT 2>&1 &
	elif [[ "$LAUNCH_OR_KILL" == "kill" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${ip} 'sh -s' < run_dpdk_kill.sh SIGKILL 2>&1 &
	else
		echo "invalid command, try again!"
		echo "sh launch_dpdk_client.sh {launch/check/config/stop/kill} {log_file_name} {lambda_rate} {relaunch(optional)}"
	fi
	n=$((n+1))
done


