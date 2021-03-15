EXP_NAME=$1

# #https://serverfault.com/questions/477503/check-if-array-is-empty-in-bash/477506
gossip="25"
#gossip_period_list=(25 50 100 200 400 800)
#load_delta_list=(0 12)
#TODO redireciont bound list
delta="0"
redirection="1"
gossip_thresh="4"

#method_list=("fixed-dest" "random-only" "power-of-2" "random-select")
method_list=("random-select")
#method_list=("random-only")
#method_list=("random-select" "random-only")
#run_list=(0 1 2 3 4 5 6 7 8 9)
#coarse_rate_list=(450000 430000 400000 300000 200000 100000)
#coarse_rate_list=("200000" "180000" "150000" "130000" "100000" "80000" "50000" "30000")
#zoomin_rate_list=("150000" "155000" "160000" "165000" "170000" "175000" "180000" "185000" "190000" "195000" "200000")

#coarse_rate_list=("200000" "190000" "180000" "170000" "160000" "150000" "140000" "130000" "120000" "110000" "100000" "90000" "80000" "70000" "60000" "50000")
#coarse_rate_list=("190000")
#coarse_rate_list=("2500" "2600" "2700" "2800" "2900" "3000" "3100" "3200" "3300" "3400" "3500" "3600" "3700" "3800" "3900" "4000")
run_list=(0 1 2 3 4)
#coarse_rate_list=(48000 47000 46000 45000 44000 43000 42000 41000 40000 39000 38000 37000 36000) 
#coarse_rate_list=(35000 34000 33000 32000 31000 30000 29000 28000 27000 26000 25000 24000 23000 22000 21000 20000)
coarse_rate_list=(35000)
# EXP_NAME_SUFFIX="randselect_test"${run}
# sh launch_dpdk_switch.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} ${gossip} ${delta} ${redirection} ${gossip_thresh}
# sh launch_udp_servers.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate}
# sh launch_dpdk_client.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} "random" "select"
# sleep 35
# sh launch_dpdk_switch.sh stop
# sh launch_udp_servers.sh stop

for run in "${run_list[@]}"
do
    for rate in "${coarse_rate_list[@]}"
    do
        for method in "${method_list[@]}"
        do
            if [[ "$method" == "random-select" ]]; then
                echo "random-select" ${rate}" RPS for "${EXP_NAME}
                EXP_NAME_SUFFIX="randselect_run"${run}

                sh launch_dpdk_switch.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} ${gossip} ${delta} ${redirection} ${gossip_thresh}
                sh launch_udp_servers.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate}
                sh launch_dpdk_client.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} "random" "select"
                #sleep 25 # we run smaller experiments for 30 secs for 2us and 200us
                sleep 40 
                sh launch_dpdk_client.sh stop
                sh launch_dpdk_switch.sh stop
                sh launch_udp_servers.sh stop
                sleep 5            
            elif [[ "$method" == "random-only" ]]; then
                echo ${method} ${rate}" RPS for "${EXP_NAME}
                EXP_NAME_SUFFIX="randonly_run"${run}

                sh launch_dpdk_switch.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} ${gossip} ${delta} ${redirection} ${gossip_thresh}
                sh launch_udp_servers.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate}
                sh launch_dpdk_client.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} "random" "none"
                #sleep 25 # we run smaller experiments for 30 secs for 2us and 200us
                sleep 40
                sh launch_dpdk_client.sh stop 
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
mv ${EXP_NAME}_* ${LOGDIR_NAME}
cd ${LOGDIR_NAME}
touch setup.txt
#echo "bimodal prob 0.9 13us, prob 0.1 130us, mean service time = 24.7us" | tee -a setup.txt 
#echo "exp mean service time= 25us" | tee -a setup.txt 
echo "constant mean service time= 20us" | tee -a setup.txt 
echo "piggyback with gossip thresholds"${gossip_thresh} | tee -a setup.txt
echo "fixed load_delta:"${delta} | tee -a setup.txt    
echo "fixed redirection:"${redirection} | tee -a setup.txt
