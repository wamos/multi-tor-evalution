export RTE_SDK=~/efs/multi-tor-evalution/dpdk_deps/dpdk-20.08
export RTE_TARGET=x86_64-native-linuxapp-gcc

switch_ip_list=(172.31.33.204  #dpdk-switch-0 eth0
                172.31.36.241  #dpdk-switch-1 eth0
                172.31.45.234) #dpdk-switch-2 eth0

for i in "${switch_ip_list[@]}"
do
	echo "setup dpdk env onto" ${i}	
	ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${i} 'sh -s' < run_dpdk_switch_config.sh
done

echo "ssh commands:"
echo ""

for i in "${switch_ip_list[@]}"
do
     	echo "ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@"${i} 
done


#cd ~/efs/multi-tor-evalution
#sh run_switch_config.sh
#sh dpdk_setup_aws.sh

