#run_dpdk_switch_launch.sh ${FWD_MODE} ${ip} ${n} ${LOGFILE} ${GOSSIP} ${LOAD_DELTA} 2>&1 &
FWD_MODE=$1
IP_ADDR=$2
INDEX=$3
LOGFILE=$4
GOSSIP=$5
LOAD_DELTA=$6
LAMBDA=$7
REDIRECT_BOUND=$8
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
#sh dpdk_switch_config.sh
#sh dpdk_setup_aws.sh
cd onearm_lb/test-pmd/
if [ -f sw_${INDEX}.log ]; then
    rm sw_${INDEX}.log
fi

echo "run dpdk-switch-" ${INDEX} on ${IP_ADDR}
sudo ./build/app/testpmd -l 0-5 -n 4 -- -a --portmask=0x1 --nb-cores=5 --forward-mode=${FWD_MODE} --enable-info-exchange \
    --switch-logfile=${LOGFILE}"_s"${INDEX}_${LAMBDA} --load-delta=${LOAD_DELTA} --gossip-period=${GOSSIP} --redirect_bound=${REDIRECT_BOUND}

# if [[ "$FWD_MODE" == "5tswap" ]]; then
# 	sudo ./build/app/testpmd -l 0-7 -n 4 -- -a --portmask=0x1 --nb-cores=6 --forward-mode=5tswap > sw_${NUM}.log
# else
	#sudo ./build/app/testpmd -l 0-7 -n 4 -- -a --portmask=0x1 --nb-cores=6 --forward-mode=replica-select
	#sudo ./build/app/testpmd -l 0-5 -n 4 -- -a --portmask=0x1 --nb-cores=5 --forward-mode=replica-select --enable-info-exchange > sw_${NUM}.log 
#fi
