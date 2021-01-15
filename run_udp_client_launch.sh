INDEX=$1
RAND_SELECT=$2
REPLICA_SELECT=$3
EXP_NAME=$4

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

cd ~/efs/kvstore_testbed/multithread/build
IP_ADDR=$(/sbin/ifconfig eth0 | awk '$1 == "inet" {print $2}' | tee -a /tmp/client_self_ip.txt)
echo "closed-loop client" ${INDEX} " on " ${IP_ADDR}

# char* recvIP = argv[1];
# in_port_t recvPort = (argc > 2) ? atoi(argv[2]) : 7000;
# int is_replica_selection =  (argc > 3) ? atoi(argv[3]) : 1;
# int is_random_selection = (argc > 4) ? atoi(argv[4]) : 1;
# char* expname = (argc > 5) ? argv[5] : "dpdk_tor_test";
# const char filename_prefix[] = "/home/ec2-user/efs/multi-tor-evalution/onearm_lb/";
# const char log[] = ".log";

# mostly, we don't have dircet client-server in our experiments
# default: ${REPLICA_SELECT}=1, ${RAND_SELECT}=0
./udp_closedloop_client ${IP_ADDR} 7000 ${REPLICA_SELECT} ${RAND_SELECT} ${EXP_NAME} > closedloop_${INDEX}.log