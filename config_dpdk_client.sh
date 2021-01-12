export RTE_SDK=~/efs/multi-tor-evalution/dpdk_deps/dpdk-20.08
export RTE_TARGET=x86_64-native-linuxapp-gcc

client_ip_list=(172.31.46.203  #rackclient-dpdk-client-0-eth0
		172.31.33.152  #rackclient-dpdk-client-1-eth0
		172.31.40.245) #rackclient-dpdk-client-2-eth0

for i in "${client_ip_list[@]}"
do
        echo "setup dpdk env onto" ${i}
        ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@${i} 'sh -s' < run_dpdk_client_config.sh
done

echo "ssh commands:"
echo ""

for i in "${client_ip_list[@]}"
do
        echo "ssh -i ~/efs/replica-selection-key-pair.pem ec2-user@"${i}
done

#cd ~/efs/multi-tor-evalution
#sh run_client_config.sh
#sh dpdk_setup_aws.sh
#cd onearm_lb/test-pmd-clean-state/
#sudo ./build/app/testpmd -l 0-4 -n 4 -- -a --portmask=0x1 --nb-cores=1 --forward-mode=txonly > 5us_pass.log &

