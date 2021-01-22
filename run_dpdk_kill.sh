#rebind the ena non-pmd driver
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

export RTE_SDK=~/efs/multi-tor-evalution/dpdk_deps/dpdk-20.08
sudo python3 ${RTE_SDK}/usertools/dpdk-devbind.py --bind=ena 0000:00:06.0
sudo ifconfig eth1 up

ps aux | grep ./build/app/testpmd 
PROCESS_ID=$(ps aux | grep ./build/app/testpmd | awk '{print $2}' | head -1)
#sudo kill ${PROCESS_ID} 
sudo kill -9 ${PROCESS_ID}
