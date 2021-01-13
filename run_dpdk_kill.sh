PROCESS_ID=$(ps aux | grep ./build/app/testpmd | awk '{print $2}' | head -1)
sudo kill ${PROCESS_ID}
#rebind the ena non-pmd driver
export RTE_SDK=~/efs/multi-tor-evalution/dpdk_deps/dpdk-20.08
sudo python3 ${RTE_SDK}/usertools/dpdk-devbind.py --bind=ena 0000:00:06.0
