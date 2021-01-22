export RTE_SDK=~/efs/multi-tor-evalution/dpdk_deps/dpdk-20.08
export RTE_TARGET=x86_64-native-linuxapp-gcc

line_num=$1 # we use line num in replica addr list to set up default dest 
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

PATH=$PATH:$HOME/.local/bin:$HOME/bin:/usr/sbin
export PATH

LD_LIBRARY_PATH=/usr/local/lib
export LD_LIBRARY_PATH

#which lspci
#cd efs
echo "config dpdk-client-" ${line_num}
cd efs/multi-tor-evalution/
sudo python3 ${RTE_SDK}/usertools/dpdk-devbind.py --bind=ena 0000:00:06.0
sh dpdk_client_config.sh ${line_num}
sh dpdk_setup_aws.sh
cd onearm_lb/test-pmd-clean-state/
if [ -f dpdk_${line_num}.log ]; then
    rm dpdk_${line_num}.log
fi