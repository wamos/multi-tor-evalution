LAUNCH_OR_KILL=$1
server_ip_list=(172.31.36.20   #replica-select-rack0-udp-server-0
                172.31.42.171  #replica-select-rack0-udp-server-1
                172.31.47.248) #replica-select-rack0-udp-server-2

for i in "${server_ip_list[@]}"
do
	echo ${LAUNCH_OR_KILL} "at" ${i}
	if [[ "$LAUNCH_OR_KILL" == "launch" ]]; then
		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${i} 'sh -s' < run_server_launch.sh
	else
    		ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${i} 'sh -s' < run_server_kill.sh
	fi
done

### run_server_launch.sh
#cd efs/kvstore_testbed/multithread/build/
#IP_ADDR=$(/sbin/ifconfig eth0 | awk '$1 == "inet" {print $2}' | tee -a /tmp/client_self_ip.txt)
#echo ${IP_ADDR}
#taskset 0x00000004 ./redirection_udp_server ${IP_ADDR} 7000 30 test 1
