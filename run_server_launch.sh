#cd ~/efs
INDEX=$1
LOGFILE=$2
CLIENT_RATE=$3
MULTI_PROCESS=$4
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
echo "server" ${INDEX} " on " ${IP_ADDR}
#taskset 0x00000004 ./redirection_udp_server ${IP_ADDR} 7000 30 ${LOGFILE}_${INDEX} ${CLIENT_RATE}
if [[ "$MULTI_PROCESS" == "two" ]]; then
    taskset 0x00000002 ./redirection_udp_server ${IP_ADDR} 7000 1 ${LOGFILE}_${INDEX} ${CLIENT_RATE}
    taskset 0x00000004 ./redirection_udp_server ${IP_ADDR} 7001 1 ${LOGFILE}_${INDEX} ${CLIENT_RATE}
    # taskset 0x00000008 ./redirection_udp_server ${IP_ADDR} 7002 1 ${LOGFILE}_${INDEX} ${CLIENT_RATE}
    # taskset 0x00000010 ./redirection_udp_server ${IP_ADDR} 7003 1 ${LOGFILE}_${INDEX} ${CLIENT_RATE}
    # taskset 0x00000020 ./redirection_udp_server ${IP_ADDR} 7004 1 ${LOGFILE}_${INDEX} ${CLIENT_RATE}
elif [[ "$MULTI_PROCESS" == "eight" ]]; then
    taskset 0x00000001 ./redirection_udp_server ${IP_ADDR} 7000 1 ${LOGFILE}_${INDEX} ${CLIENT_RATE}
    taskset 0x00000002 ./redirection_udp_server ${IP_ADDR} 7001 1 ${LOGFILE}_${INDEX} ${CLIENT_RATE}
    taskset 0x00000004 ./redirection_udp_server ${IP_ADDR} 7002 1 ${LOGFILE}_${INDEX} ${CLIENT_RATE}
    taskset 0x00000008 ./redirection_udp_server ${IP_ADDR} 7003 1 ${LOGFILE}_${INDEX} ${CLIENT_RATE}
    taskset 0x00000010 ./redirection_udp_server ${IP_ADDR} 7004 1 ${LOGFILE}_${INDEX} ${CLIENT_RATE}
    taskset 0x00000020 ./redirection_udp_server ${IP_ADDR} 7005 1 ${LOGFILE}_${INDEX} ${CLIENT_RATE}
    taskset 0x00000040 ./redirection_udp_server ${IP_ADDR} 7006 1 ${LOGFILE}_${INDEX} ${CLIENT_RATE}
    taskset 0x00000080 ./redirection_udp_server ${IP_ADDR} 7007 1 ${LOGFILE}_${INDEX} ${CLIENT_RATE}
else
    taskset 0x00000004 ./redirection_udp_server ${IP_ADDR} 7000 2 ${LOGFILE}_${INDEX} ${CLIENT_RATE}
fi

