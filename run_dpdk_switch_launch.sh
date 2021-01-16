FWD_MODE=$1
NUM=$2
export RTE_SDK=~/efs/multi-tor-evalution/dpdk_deps/dpdk-20.08
export RTE_TARGET=x86_64-native-linuxapp-gcc

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

cd efs/multi-tor-evalution
sh dpdk_switch_config.sh
sh dpdk_setup_aws.sh
cd onearm_lb/test-pmd/
if [ -f sw_${NUM}.log ]; then
    rm sw_${NUM}.log
fi

echo "run dpdk-switch-" ${NUM}
if [[ "$FWD_MODE" == "5tswap" ]]; then
	sudo ./build/app/testpmd -l 0-7 -n 4 -- -a --portmask=0x1 --nb-cores=6 --forward-mode=5tswap > sw_${NUM}.log
else
	#sudo ./build/app/testpmd -l 0-7 -n 4 -- -a --portmask=0x1 --nb-cores=6 --forward-mode=replica-select
	sudo ./build/app/testpmd -l 0-5 -n 4 -- -a --portmask=0x1 --nb-cores=5 --forward-mode=replica-select --enable-info-exchange > sw_${NUM}.log 
fi
