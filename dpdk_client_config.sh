#! /bin/bash

line_num=$1
#config_path=~/multi-tor-evalution/onearm_lb/test-pmd
#config_path=~/efs/multi-tor-evalution/config
config_path=./config

# the switch ip is the ip addr of eth1, the second NIC
gen_host_dep_config()
{
if [ -f /tmp/client_self_ip.txt ];then
	rm /tmp/client_self_ip.txt
else
	touch /tmp/client_self_ip.txt
fi

IP_ADDR=$(/sbin/ifconfig eth1 | awk '$1 == "inet" {print $2}' | tee -a /tmp/client_self_ip.txt)
#echo ${IP_ADDR}
cat /tmp/client_self_ip.txt

if [ -f /tmp/replica_addr_list.txt ];then
	rm /tmp/replica_addr_list.txt
else
	touch /tmp/replica_addr_list.txt
fi

read num_line < ${config_path}/replica_service_ip_aws.txt
## 1. get random number
#rand=0
#rand=$(od -An -N1 -i /dev/random)
#rand=$((rand%num_line))
#line_num=$((rand+2))
## 1. use fixed line num for each client
line_num=$((line_num+2)) # make it start with 1 and skip the first line so we add 2 to it

## 2. print random-th line to replica_addr_list.txt
sed -n ${line_num}p ${config_path}/replica_service_ip_aws.txt  > /tmp/replica_addr_list.txt
echo "/tmp/replica_addr_list.txt:"
cat /tmp/replica_addr_list.txt
}

check_host_dep_config()
{
if [ ! -f /tmp/client_self_ip.txt ];then
        echo "## ERROR: the client doesn't have a file describing its own ip address"
        return
fi

if [ ! -f /tmp/replica_addr_list.txt ];then
        echo "## ERROR: the client doesn't have a file describing its replica addresses"
        return
fi
}

check_routing_config()
{
if [ ! -f ${config_path}/routing_table_aws.txt ];then
        echo "## ERROR: Missed file routing_table_aws.txt"
        return
else
	read num_line < ${config_path}/routing_table_aws.txt
	n=0
        while read line; do # reading each line
        #echo $line
        n=$((n+1))
        done < ${config_path}/routing_table_aws.txt

        n=$((n-1)) #subtract out the first line
        if [ ${n} -ne ${num_line} ]; then
                echo "invlid file of routing_table_aws.txt"
                return
        fi
fi
}

check_ip2mac_config(){
	python3 check_ip2mac.py ${config_path}/	
}

random_test(){
RANDOM=1
num_line=5
for i in `seq 10`
do
	RANDOM=$(od -An -N1 -i /dev/random)
	RANDOM=$((RANDOM%num_line))
	echo $RANDOM
done
}
#random_test

gen_host_dep_config
check_host_dep_config
check_routing_config
check_ip2mac_config
