import numpy as np
import pandas as pd
print("request rate: 30kRPS")
print("switch-0")
data = pd.read_csv("../log/feb22_piggy_exp_mean25us/piggy_exp_mean25us_randonly_run0_s0_30k.pgy", delimiter = ",", usecols=[0]).values
ratios = np.zeros(5)
for i in range(0,5):
    ratios[i] = (float) (np.count_nonzero(data==i+1))/data.shape[0]
    print("req per update:"+str(i)+",ratio:"+str(ratios[i]))

print("switch-1")
data = pd.read_csv("../log/feb22_piggy_exp_mean25us/piggy_exp_mean25us_randonly_run0_s1_30k.pgy", delimiter = ",", usecols=[0]).values
ratios = np.zeros(5)
for i in range(0,5):
    ratios[i] = (float) (np.count_nonzero(data==i+1))/data.shape[0]
    print("req per update:"+str(i)+",ratio:"+str(ratios[i]))

print("switch-2")
data = pd.read_csv("../log/feb22_piggy_exp_mean25us/piggy_exp_mean25us_randonly_run0_s2_30k.pgy", delimiter = ",", usecols=[0]).values
ratios = np.zeros(5)
for i in range(0,5):
    ratios[i] = (float) (np.count_nonzero(data==i+1))/data.shape[0]
    print("req per update:"+str(i)+",ratio:"+str(ratios[i]))

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
