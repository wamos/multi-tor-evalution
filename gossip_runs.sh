EXP_NAME=$1

#https://serverfault.com/questions/477503/check-if-array-is-empty-in-bash/477506
#gossip="25"
#gossip_period_list=(25 50 100 200 400 800)
gossip_period_list=(200)
#gossip_period_list=(25 50 100 200 400 800)
#load_delta_list=(1 2 3 4 5 6 7 8 9 10)
#TODO redireciont bound list
delta="1"
redirection="1"

#method_list=("fixed-dest" "random-only" "power-of-2" "random-select")
# method_list=("random-only" 
#             "random-select")
method_list=("random-select")
method="random-select"

#coarse_rate_list=(10000 15000)
#coarse_rate_list=(20000 25000 30000 35000)
#coarse_rate_list=(30000 31000 32000 33000 34000)
#coarse_rate_list=(30000 31000 32000 33000 34000 35000)
run_list=(0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24)
#run_list=(0 1 2 3 4 5 6 7 8 9)
#coarse_rate_list=(20000 23000 25000 28000 30000 33000 35000)
#upper half first 
coarse_rate_list=(35000)

zoomin_rate_list=(20000 21000 22000 23000 24000
        25000 26000 27000 28000 29000 30000
        31000 32000 33000 34000 35000 36000)
#zoomin_rate_list=(27000)


for run in "${run_list[@]}"
do
    for rate in "${coarse_rate_list[@]}"
    do
        if [[ "$method" == "random-select" ]]; then
            echo "random-select" ${rate}" RPS for "${EXP_NAME}
            for gossip in "${gossip_period_list[@]}" #for load_delta in "${load_delta_list[@]}"            
            do
                #EXP_NAME_SUFFIX="randselect" #_g_"${gossip}"_d_"${delta}"_r_"${redirection}
                EXP_NAME_SUFFIX="randselect_g_"$gossip"_run"${run}

                sh launch_dpdk_switch.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} $gossip ${delta} ${redirection}
                sh launch_udp_servers.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate}
                sh launch_dpdk_client.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} "random" "select"
                sleep 40 # we run smaller experiments for 30 secs
                sh launch_dpdk_switch.sh stop
                sh launch_udp_servers.sh stop
                sleep 19
            done
        else
            echo "an unknown method to run!"
        fi
        sleep 1
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
for gossip in "${gossip_period_list[@]}"
do
    echo "gossip values:"${gossip} | tee -a setup.txt
done
echo "fixed load_delta:"${delta} | tee -a setup.txt
echo "fixed redirection:"${redirection} | tee -a setup.txt
# cd ../plotting
# python3 plot_pctl.py ${LOGDIR_NAME} log log ${EXP_NAME}_pctl
