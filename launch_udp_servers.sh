LAUNCH_OR_KILL=$1
LOGFILE=$2
CLIENT_RATE=$3
#RELAUNCH=$4
server_ip_list=(172.31.36.20   #replica-select-rack0-udp-server-0
                172.31.42.171  #replica-select-rack0-udp-server-1
                172.31.47.248) #replica-select-rack0-udp-server-2

n=0 #server_index
for i in "${server_ip_list[@]}"
do
	echo ${LAUNCH_OR_KILL} "at" ${i}
	if [[ "$LAUNCH_OR_KILL" == "launch" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${i} 'sh -s' < run_server_launch.sh ${n} ${LOGFILE} ${CLIENT_RATE} 2>&1 &
	elif [[ "$LAUNCH_OR_KILL" == "check" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${i} 'sh -s' < run_server_check.sh ${n} 2>&1 &
		sleep 1
	elif [[ "$LAUNCH_OR_KILL" == "stop" ]]; then
    	ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${i} 'sh -s' < run_server_kill.sh 2>&1 &
	else
		echo "invalid command, try again!"
		echo "sh launch_udp_servers.sh {launch/check/stop} {log_file_name} {clients_lambda_rate}"
	fi
	n=$((n+1))
done

echo "ssh commands:"
for i in "${server_ip_list[@]}"
do
     	echo "ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@"${i} 
done
echo ""

### run_server_launch.sh
#cd efs/kvstore_testbed/multithread/build/
#IP_ADDR=$(/sbin/ifconfig eth0 | awk '$1 == "inet" {print $2}' | tee -a /tmp/client_self_ip.txt)
#echo ${IP_ADDR}
#taskset 0x00000004 ./redirection_udp_server ${IP_ADDR} 7000 30 test 1
