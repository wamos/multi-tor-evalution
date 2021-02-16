# sh run_dpdk_client_launch.sh ${ip} ${n} ${LAMBDA} ${LOGFILE} ${RANDOM} ${SELECT}
export RTE_SDK=~/efs/multi-tor-evalution/dpdk_deps/dpdk-20.08
export RTE_TARGET=x86_64-native-linuxapp-gcc

IP_ADDR=$1
INDEX=$2 # we use line num aka index in replica addr list to set up default dest 
LAMBDA=$3
LOGFILE=$4
RAND=$5
SELECT=$6

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

echo "launch dpdk-client-" ${INDEX} #on ${IP_ADDR}
#which lspci
#cd efs
cd efs/multi-tor-evalution/
# sh dpdk_client_config.sh ${line_num}
# sh dpdk_setup_aws.sh
cd onearm_lb/test-pmd-clean-state/
# if [ -f dpdk_${line_num}.log ]; then
#     rm dpdk_${line_num}.log
# fi
#sudo ./build/app/testpmd -l 0-4 -n 4 -- -a --portmask=0x1 --nb-cores=1 --forward-mode=txonly --lambda_rate=20000 > dpdk_${line_num}.log
# echo $RANDOM
# echo $SELECT
if [[ "$RAND" == "random" && "$SELECT" == "select" ]]; then
echo "random-select mode is on!"
sudo ./build/app/testpmd -l 0-4 -n 4 -- -a --portmask=0x1 --nb-cores=1 --forward-mode=txonly \
--enable-random-dest --enable-replica-select --lambda_rate=${LAMBDA} --latency-logfile=${LOGFILE}"_c"${INDEX}

elif [[ "$RAND" == "none" && "$SELECT" == "select" ]]; then
echo "select-only mode is on!"
sudo ./build/app/testpmd -l 0-4 -n 4 -- -a --portmask=0x1 --nb-cores=1 --forward-mode=txonly \
--enable-replica-select --lambda_rate=${LAMBDA} --latency-logfile=${LOGFILE}"_c"${INDEX}

elif [[ "$RAND" == "random" && "$SELECT" == "none" ]]; then
echo "random-only mode is on!"
sudo ./build/app/testpmd -l 0-4 -n 4 -- -a --portmask=0x1 --nb-cores=1 --forward-mode=txonly \
--enable-random-dest --lambda_rate=${LAMBDA} --latency-logfile=${LOGFILE}"_c"${INDEX}

else
    echo "pass-through mode is on!"
    sudo ./build/app/testpmd -l 0-4 -n 4 -- -a --portmask=0x1 --nb-cores=1 --forward-mode=txonly \
    --lambda_rate=${LAMBDA} --latency-logfile=${LOGFILE}"_c"${INDEX}
fi


