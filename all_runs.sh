EXP_NAME=$1

#https://serverfault.com/questions/477503/check-if-array-is-empty-in-bash/477506
#gossip="25"
gossip_period_list=(25 50 100 200 400 800)
#load_delta_list=(1 2 3 4 5 6 7 8 9 10)
#TODO redireciont bound list
delta="1"
redirection="1"

#method_list=("fixed-dest" "random-only" "power-of-2" "random-select")
# method_list=("random-only" 
#             "random-select")
method_list=("random-select")

#coarse_rate_list=(10000 15000)
#coarse_rate_list=(20000 25000 30000 35000)
#coarse_rate_list=(30000 31000 32000 33000 34000)
coarse_rate_list=(33000 34000)

zoomin_rate_list=(20000 21000 22000 23000 24000
        25000 26000 27000 28000 29000 30000
        31000 32000 33000 34000 35000 36000)
#zoomin_rate_list=(27000)

for method in "${method_list[@]}"
do
    for rate in "${coarse_rate_list[@]}"
    do
        if [[ "$method" == "random-only" ]]; then
            echo ${method} ${rate}" RPS for "${EXP_NAME}
            for gossip in "${gossip_period_list[@]}"
            #for delta in "${load_delta_list[@]}"            
            do
                EXP_NAME_SUFFIX="randonly" #_g_"${gossip}"_d_"${delta}"_r_"${redirection}
                #EXP_NAME_SUFFIX="randonly_g_"${gossip} #_d_"${delta}"_r_"${redirection}

                sh launch_dpdk_switch.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} ${gossip} ${delta} ${redirection}
                sh launch_udp_servers.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate}
                sh launch_dpdk_client.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} "random" "none"
                sleep 40 # we run smaller experiments for 30 secs
                sh launch_dpdk_switch.sh stop
                sh launch_udp_servers.sh stop
                #sh launch_dpdk_client.sh stop
            done
        elif [[ "$method" == "random-select" ]]; then
            echo "random-select" ${rate}" RPS for "${EXP_NAME}
            for gossip in "${gossip_period_list[@]}" #for load_delta in "${load_delta_list[@]}"            
            do
                #EXP_NAME_SUFFIX="randselect" #_g_"${gossip}"_d_"${delta}"_r_"${redirection}
                EXP_NAME_SUFFIX="randselect_g_"$gossip

                sh launch_dpdk_switch.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} $gossip ${delta} ${redirection}
                sh launch_udp_servers.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate}
                sh launch_dpdk_client.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} "random" "select"
                sleep 40 # we run smaller experiments for 30 secs
                sh launch_dpdk_switch.sh stop
                sh launch_udp_servers.sh stop
                ## for diff gossip values
                sleep 20
                #sh launch_dpdk_client.sh stop
            done
        elif [[ "$method" == "fixed-dest" ]]; then
            echo "fixed-dest" ${rate}" RPS for "${EXP_NAME}
            # for gossip in "${gossip_period_list[@]}"
            # #for load_delta in "${load_delta_list[@]}"            
            # do
            #     EXP_NAME_SUFFIX="fixed" #_g_"${gossip}"_d_"${delta}"_r_"${redirection}
            #     sh launch_dpdk_switch.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} ${gossip} ${delta} ${redirection}
            #     sh launch_udp_servers.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate}
            #     sh launch_dpdk_client.sh launch ${EXP_NAME}"_"${EXP_NAME_SUFFIX} ${rate} "none" "none"
            #     sleep 40 # we run smaller experiments for 30 secs
            #     sh launch_dpdk_switch.sh stop
            #     sh launch_udp_servers.sh stop
            # done
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
# cd ../plotting
# python3 plot_pctl.py ${LOGDIR_NAME} log log ${EXP_NAME}_pctl
