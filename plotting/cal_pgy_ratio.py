import numpy as np
import pandas as pd
print("request rate: 30kRPS")
print("switch-0")


#thresh_sweep_randselect_gth64_run0_s0_32k.pgy
gossip_load_thresh = ["0", "1", "2", "4", "8", "16", "32", "64", "128"]
run="0"
rate="32k"
for gossip in gossip_load_thresh:
    working_dir= "mar1_thresh_sweep"
    parent_dir = "/home/ec2-user/efs/multi-tor-evalution/log/"
    filename_prefix = "thresh_sweep_randselect" + "_gth" + gossip + "_run" + run
    pgy_filename = parent_dir + working_dir + "/"+ filename_prefix + "_s0_" + rate + "." + "pgy"
    log_filename = parent_dir + working_dir + "/"+ filename_prefix + "_c0_" + rate + "." + "log"
        
    log_data = pd.read_csv(log_filename, delimiter = ",", usecols=[0]).values
    pgy_data = pd.read_csv(pgy_filename, delimiter = ",", usecols=[0]).values

    ratio = float(pgy_data.shape[0])/float(log_data.shape[0])
    ratio = ratio*100
    print("{:.5f}".format(round(ratio, 5)))

# data = pd.read_csv("../log/feb22_piggy_exp_mean25us/piggy_exp_mean25us_randonly_run0_s0_30k.pgy", delimiter = ",", usecols=[0]).values
# ratios = np.zeros(5)
# for i in range(0,5):
#     ratios[i] = (float) (np.count_nonzero(data==i+1))/data.shape[0]
#     print("req per update:"+str(i)+",ratio:"+str(ratios[i]))

# print("switch-1")
# data = pd.read_csv("../log/feb22_piggy_exp_mean25us/piggy_exp_mean25us_randonly_run0_s1_30k.pgy", delimiter = ",", usecols=[0]).values
# ratios = np.zeros(5)
# for i in range(0,5):
#     ratios[i] = (float) (np.count_nonzero(data==i+1))/data.shape[0]
#     print("req per update:"+str(i)+",ratio:"+str(ratios[i]))

# print("switch-2")
# data = pd.read_csv("../log/feb22_piggy_exp_mean25us/piggy_exp_mean25us_randonly_run0_s2_30k.pgy", delimiter = ",", usecols=[0]).values
# ratios = np.zeros(5)
# for i in range(0,5):
#     ratios[i] = (float) (np.count_nonzero(data==i+1))/data.shape[0]
#     print("req per update:"+str(i)+",ratio:"+str(ratios[i]))

# print("request rate: 35kRPS")
# data = pd.read_csv("../log/test_piggy35k_s0.pgy", delimiter = ",", usecols=[0]).values
# ratios = np.zeros(5)
# for i in range(0,5):
#     ratios[i] = (float) (np.count_nonzero(data==i+1))/data.shape[0]
#     print("req per update:"+str(i)+",ratio:"+str(ratios[i]))

# print("request rate: 30kRPS with exp dist")
# data = pd.read_csv("../log/test_piggy_exp_30k_s0.pgy", delimiter = ",", usecols=[0]).values
# ratios = np.zeros(5)
# for i in range(0,5):
#     ratios[i] = (float) (np.count_nonzero(data==i+1))/data.shape[0]
#     print("req per update:"+str(i)+",ratio:"+str(ratios[i]))
