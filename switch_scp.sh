
#ref: https://www.geeksforgeeks.org/array-basics-shell-scripting-set-1/
switch_ip_list=(172.31.33.204 #dpdk-switch-0 
		172.31.36.241 #dpdk-switch-1
		172.31.45.234) #dpdk-switch-2 
#switch_ip_list=(172.31.46.203)

#TODO: disable host key checking
# ssh -o StrictHostKeyChecking=no yourHardenedHost.com
for i in "${switch_ip_list[@]}"
do
	echo "launch dpdk switch program on" ${i}

	scp -i ~/.ssh/replica-selection-key-pair.pem ~/multi-tor-evalution/onearm_lb/test-pmd/*.c ec2-user@${i}:~/multi-tor-evalution/onearm_lb/test-pmd/
	scp -i ~/.ssh/replica-selection-key-pair.pem ~/multi-tor-evalution/onearm_lb/test-pmd/*.h ec2-user@${i}:~/multi-tor-evalution/onearm_lb/test-pmd/
	scp -i ~/.ssh/replica-selection-key-pair.pem ~/multi-tor-evalution/onearm_lb/test-pmd/*.txt ec2-user@${i}:~/multi-tor-evalution/onearm_lb/test-pmd/
done
