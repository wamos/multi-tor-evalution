LAUNCH_OR_KILL=$1
EXP_NAME=$2
RELAUNCH=$3

RAND_SELECT="0"     # default: 1= random shredding, other values: 0="none"
REPLICA_SELECT="1"  # default: 1= "select", other values: 0="none"

client_ip_list=(172.31.39.135) #udp-client0
               #172.31.47.229) #udp-client1

n=0 #server_index
for i in "${client_ip_list[@]}"
do
    echo ${LAUNCH_OR_KILL} "at" ${i}
    if [[ "$LAUNCH_OR_KILL" == "launch" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${i} 'sh -s' < run_udp_client_launch.sh ${n} ${RAND_SELECT} ${EXP_NAME} 2>&1 &
	elif [[ "$LAUNCH_OR_KILL" == "check" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${i} 'sh -s' < run_udp_client_check.sh ${n} 2>&1 &
		sleep 1
	else
    	ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${i} 'sh -s' < run_udp_client_kill.sh 2>&1 &
	fi
done

echo "ssh commands:"
for i in "${client_ip_list[@]}"
do
     	echo "ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@"${i} 
done
echo ""