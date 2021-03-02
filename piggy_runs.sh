EXP_NAME=$1

#https://serverfault.com/questions/477503/check-if-array-is-empty-in-bash/477506
gossip="25"
#gossip_period_list=(25 50 100 200 400 800)
#load_delta_list=(1 2 3 4 5 6 7 8 9 10)
#TODO redireciont bound list
delta="4"
redirection="2"

#method_list=("fixed-dest" "random-only" "power-of-2" "random-select")
#method_list=("random-select")
method_list=("random-select")
#method_list=("random-only")

#coarse_rate_list=(10000 15000)
#coarse_rate_list=(20000 25000 30000 35000)
run_list=(0 1 2 3 4)
#coarse_rate_list=(30000 31000 32000 33000 34000 35000)
#coarse_rate_list=(35000 33000 30000 28000 25000 23000 20000)
#coarse_rate_list=(35000 33000 30000 28000 25000 23000 20000)
#coarse_rate_list=(33000)
#coarse_rate_list=(34000 32000 31000 29000 27000 26000 24000 22000 21000)
rate="32000"
gossip_thresh_list=(4) #16 32 64 128


for run in "${run_list[@]}"
do
    for gossip_thresh in "${gossip_thresh_list[@]}"
    do
        for method in "${method_list[@]}"
        do
            if [[ "$method" == "random-select" ]]; then
                echo "random-select" ${rate}" RPS for "${EXP_NAME}
                EXP_NAME_SUFFIX="randselect_gth${gossip_thresh}_run"${run}

                sh launch_dpdk_switch.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} $gossip ${delta} ${redirection} ${gossip_thresh}
                sh launch_udp_servers.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate}
                sh launch_dpdk_client.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} "random" "select"
                sleep 35 # we run smaller experiments for 30 secs
                sh launch_dpdk_switch.sh stop
                sh launch_udp_servers.sh stop
                sleep 5
            elif [[ "$method" == "random-only" ]]; then
                echo ${method} ${rate}" RPS for "${EXP_NAME}
                EXP_NAME_SUFFIX="randonly_gth${gossip_thresh}_run"${run}

                sh launch_dpdk_switch.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} ${gossip} ${delta} ${redirection} ${gossip_thresh}
                sh launch_udp_servers.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate}
                sh launch_dpdk_client.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} "random" "none"
                sleep 35 # we run smaller experiments for 30 secs
                sh launch_dpdk_switch.sh stop
                sh launch_udp_servers.sh stop
                sleep 5
            else
                echo "an unknown method to run!"
            fi
        done
    done
done


cd log
LOGDIR_NAME=$(TZ='America/Los_Angeles' date | awk '{print tolower($2) $3}')"_"$EXP_NAME
if [ ! -d ${LOGDIR_NAME} ]; then
    mkdir ${LOGDIR_NAME}
fi
# touch setup.txt
# echo "gossip:"${gossip} | tee -a setup.txt
# echo "load_delta:"${delta} | tee -a setup.txt
# echo "redirection:"${redirection} | tee -a setup.txt
mv ${EXP_NAME}_* ${LOGDIR_NAME}
cd ${LOGDIR_NAME}
touch setup.txt
echo "piggyback with gossip thresholds" | tee -a setup.txt
for gth in "${gossip_thresh_list[@]}"
do
    echo "gossip thresholds:"${gth} | tee -a setup.txt    
done
#echo "bimodal prob 0.8 15us, prob 0.2 60us, mean service time = 24us" | tee -a setup.txt 
echo "exp mean service time= 25us" | tee -a setup.txt 
#echo "piggyback with gossip thresholds" | tee -a setup.txt
echo "fixed load_delta:"${load_delta} | tee -a setup.txt    
echo "fixed redirection:"${redirection} | tee -a setup.txt
# cd ../plotting
# python3 plot_pctl.py ${LOGDIR_NAME} log log ${EXP_NAME}_pctl
