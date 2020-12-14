
#ref: https://www.geeksforgeeks.org/array-basics-shell-scripting-set-1/
#switch_ip_list=(172.31.46.203 172.31.36.20)
server_client_ip_list=(172.31.36.20
                172.31.39.135
                172.31.47.248
                172.31.47.229)

for i in "${server_client_ip_list[@]}"
do
        echo "scp .txt files to:"
        echo ${i}
        #scp -i ~/.ssh/replica-selection-key-pair.pem ~/multi-tor-evalution/onearm_lb/test-pmd/*.c ec2-user@${i}:~/multi-tor-evalution/onearm_lb/test-pmd/
        #scp -i ~/.ssh/replica-selection-key-pair.pem ~/multi-tor-evalution/onearm_lb/test-pmd/*.h ec2-user@${i}:~/multi-tor-evalution/onearm_lb/test-pmd/
        scp -i ~/.ssh/replica-selection-key-pair.pem ~/multi-tor-evalution/onearm_lb/test-pmd/*.txt ec2-user@${i}:~/multi-tor-evalution/onearm_lb/test-pmd/
done
