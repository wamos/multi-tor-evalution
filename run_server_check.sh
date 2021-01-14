#cd ~/efs
INDEX=$1
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

echo "checking server" ${INDEX}
ps aux | grep redirection_udp_server
echo "---------------"
#cd ~/efs/kvstore_testbed/multithread/build
#IP_ADDR=$(/sbin/ifconfig eth0 | awk '$1 == "inet" {print $2}' | tee -a /tmp/client_self_ip.txt)
#echo "using" ${IP_ADDR} "as server addr with direct client-server"
#ohup taskset 0x00000004 ./redirection_udp_server ${IP_ADDR} 7000 30 test 1 &

