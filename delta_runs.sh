EXP_NAME=$1

#https://serverfault.com/questions/477503/check-if-array-is-empty-in-bash/477506
#gossip_period_list=(25 50 100 200 400 800)
#load_delta_list=(0 1 2 3 4 5 6 7 8 9 10)
load_delta_list=(1 10 20 40 80)
#delta="1"
gossip="25"
redirection="1"

#20 runs
run_list=(0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19)

#method_list=("fixed-dest" "random-only" "power-of-2" "random-select")
# method_list=("random-only" 
#             "random-select")
#method="random-select"
method="random-only" 

#coarse_rate_list=(10000 15000)
#coarse_rate_list=(20000 25000 30000 35000)
#coarse_rate_list=(30000 31000 32000 33000 34000 35000)
coarse_rate_list=(35000)
# 5*20+ 20 = 220

# zoomin_rate_list=(20000 21000 22000 23000 24000
#         25000 26000 27000 28000 29000 30000
#         31000 32000 33000 34000 35000 36000)
#zoomin_rate_list=(27000)

for run in "${run_list[@]}"
do    
    for rate in "${coarse_rate_list[@]}"
    do
        if [[ "$method" == "random-select" ]]; then
            echo "random-select" ${rate}" RPS for "${EXP_NAME}
            for load_delta in "${load_delta_list[@]}"
            do
                EXP_NAME_SUFFIX="randselect_d_"${load_delta}"_run"${run}

                sh launch_dpdk_switch.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} ${gossip} ${load_delta} ${redirection}
                sh launch_udp_servers.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate}
                sh launch_dpdk_client.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} "random" "select"
                sleep 40 # we run smaller experiments for 30 secs
                sh launch_dpdk_switch.sh stop
                sh launch_udp_servers.sh stop
                sleep 20
            done
        elif [[ "$method" == "random-only" ]]; then
            echo ${method} ${rate}" RPS for "${EXP_NAME}
            EXP_NAME_SUFFIX="randonly_run"${run} #_g_"${gossip}"_d_"${delta}"_r_"${redirection}
            #EXP_NAME_SUFFIX="randonly_g_"${gossip} #_d_"${delta}"_r_"${redirection}
            delta="1"

            sh launch_dpdk_switch.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} ${gossip} ${delta} ${redirection}
            sh launch_udp_servers.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate}
            sh launch_dpdk_client.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} "random" "none"
            sleep 40 # we run smaller experiments for 30 secs
            sh launch_dpdk_switch.sh stop
            sh launch_udp_servers.sh stop
            sleep 20
            #sh launch_dpdk_client.sh stop
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
# touch setup.txt
# echo "gossip:"${gossip} | tee -a setup.txt
# echo "load_delta:"${delta} | tee -a setup.txt
# echo "redirection:"${redirection} | tee -a setup.txt
mv ${EXP_NAME}_* ${LOGDIR_NAME}
cd ${LOGDIR_NAME}
touch setup.txt
for load_delta in "${load_delta_list[@]}"
do
    echo "load_delta:"${load_delta} | tee -a setup.txt    
done
echo "fixed gossip values:"${gossip} | tee -a setup.txt
echo "fixed redirection:"${redirection} | tee -a setup.txt
# cd ../plotting
# python3 plot_pctl.py ${LOGDIR_NAME} log log ${EXP_NAME}_pctl
