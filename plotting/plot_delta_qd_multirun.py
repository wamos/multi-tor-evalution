import numpy as np
import pandas as pd
import sys, os
import matplotlib.pyplot as plt
from scipy.stats import sem

working_dir = sys.argv[1]
title_filter1 = sys.argv[2]
title_filter2 = sys.argv[3]
fig_title   = sys.argv[4]
yaxis_limit = float(sys.argv[5])
#base_case = sys.argv[5]
#xaxis_limit = float(sys.argv[5])
extension = "qd"

fig_xlabel  = "Load delta"
fig_ylabel  = "Queue depth difference" #sys.argv[3]

file_list=[]
for root, subdirs, files in os.walk(working_dir):
	for f in files:
		if title_filter1 in f and title_filter2 in f:
			f_path = os.path.join(root, f)
			file_list.append(f_path)

# for f in file_list:
#     print(f)

rate = title_filter1
client_tick = ["s0", "s1" , "s2"]
method_tick  = [title_filter2+"_randselect"]
method_array = ["replica-select with gossip=25us, redirction=1"]
#delta_tick=["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"]
delta_tick=["1", "10", "20", "40", "80"]
run_tick=["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19"]


#method_array = ["random", "replica-select min load delta=1"]
#method_tick  = ["qd_random", "delta2_randselect", "delta3_randselect", "delta4_randselect"]
#method_array = ["random", "replica-select min load delta=2", "replica-select min load delta=3", "replica-select min load delta=4"]

# rate_tick =[ "10k", "15k", "20k"]
# rate_array =[ "10000", "15000", "20000"]

# rate_tick =[ "10k", "15k", "20k", "25k", "30k", "35k"]
# rate_array =[ "10000", "15000", "20000", "25000", "30000", "35000"]
# rate_tick =[ "20k", "25k", "30k"]
# rate_array =[ "20000", "25000", "30000"]

# method_tick  = ["saturation_randonly", "saturation_randselect"]
# method_array = ["random", "replica-select with gossip=25us, delta=1"]
# rate_tick =["35k", "36k", "37k", "38k"]

#suffix = "_g_25_d_1_r_1"
#rate_tick =[ "30k", "31k", "32k", "33k", "34k", "35k"]
#rate_array =[ "30000", "31000" , "32000", "33000", "34000", "35000"]
color_list = ['xkcd:blue', 'xkcd:orange', 'xkcd:orchid', 'xkcd:sienna', 'xkcd:grey']

fig, ax = plt.subplots(figsize=(10, 5))

mean_per_delta_mean_array = np.zeros(len(delta_tick))
mean_per_delta_sem_array = np.zeros(len(delta_tick))

pct95th_per_delta_mean_array = np.zeros(len(delta_tick))
pct95th_per_delta_sem_array = np.zeros(len(delta_tick))

pct99th_per_delta_mean_array = np.zeros(len(delta_tick))
pct99th_per_delta_sem_array = np.zeros(len(delta_tick))

pct99th9_per_delta_mean_array = np.zeros(len(delta_tick))
pct99th9_per_delta_sem_array = np.zeros(len(delta_tick))
#loss_rate = np.zeros(len(delta_tick))

method_index=0
delat_index=0
#for method in method_tick:
for delta in delta_tick:
    mean_array = np.zeros(len(run_tick*3))
    pct66th_array = np.zeros(len(run_tick*3))
    pct95th_array = np.zeros(len(run_tick*3))
    pct99th_array = np.zeros(len(run_tick*3))
    pct99th9_array = np.zeros(len(run_tick*3))
    loss_rate = np.zeros(len(run_tick*3))
    
    array_index=0    
    for run in run_tick:
        #### base cases
        #filename0 = "/Users/wamos/matplot/kv_cdf/"+ working_dir + "/"+ method + "_" + rate + "." + extension
        #data = pd.read_csv(filename0, delimiter = ",", usecols=[1]).values
        parent_dir = "/home/ec2-user/efs/multi-tor-evalution/log/"
        filename_prefix = method_tick[method_index] + "_d_" + delta + "_run" + run

        filename0 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ client_tick[0] + "_" + rate + "." + extension
        filename1 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ client_tick[1] + "_" + rate + "." + extension
        filename2 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ client_tick[2] + "_" + rate + "." + extension

        ## normal cases
        ## column 0, column 1, column 2
        ## number of redirections, end-to-end latency, server processing latency
        local_f0 = pd.read_csv(filename0, delimiter = ",", usecols=[0]).values
        remote0_f0 = pd.read_csv(filename0, delimiter = ",", usecols=[1]).values
        remote1_f0= pd.read_csv(filename0, delimiter = ",", usecols=[2]).values
        diff0_f0 = local_f0 - remote0_f0
        diff1_f0 = local_f0 - remote1_f0
        diff_f0 = np.concatenate([diff0_f0, diff1_f0])

        local_f1 = pd.read_csv(filename1, delimiter = ",", usecols=[0]).values
        remote0_f1 = pd.read_csv(filename1, delimiter = ",", usecols=[1]).values
        remote1_f1= pd.read_csv(filename1, delimiter = ",", usecols=[2]).values
        diff0_f1 = local_f1 - remote0_f1
        diff1_f1 = local_f1 - remote1_f1
        diff_f1 = np.concatenate([diff0_f1, diff1_f1])

        local_f2 = pd.read_csv(filename2, delimiter = ",", usecols=[0]).values
        remote0_f2 = pd.read_csv(filename2, delimiter = ",", usecols=[1]).values
        remote1_f2= pd.read_csv(filename2, delimiter = ",", usecols=[2]).values
        diff0_f2 = local_f2 - remote0_f2
        diff1_f2 = local_f2 - remote1_f2
        diff_f2 = np.concatenate([diff0_f2, diff1_f2])

        #data = np.concatenate([diff_f0, diff_f1, diff_f2])
        #data = np.concatenate([diff0_f0, diff1_f0, diff0_f1, diff1_f2, diff0_f2, diff1_f2])

        mean_array[array_index]    = np.percentile(diff_f0, 50)
        pct66th_array[array_index] = np.percentile(diff_f0, 66)
        pct95th_array[array_index] = np.percentile(diff_f0, 95)
        pct99th_array[array_index] = np.percentile(diff_f0, 99)
        pct99th9_array[array_index] = np.percentile(diff_f0, 99.9)
        array_index = array_index + 1

        mean_array[array_index]    = np.percentile(diff_f1, 50)
        pct66th_array[array_index] = np.percentile(diff_f1, 66)
        pct95th_array[array_index] = np.percentile(diff_f1, 95)
        pct99th_array[array_index] = np.percentile(diff_f1, 99)
        pct99th9_array[array_index] = np.percentile(diff_f1, 99.9)
        array_index = array_index + 1

        mean_array[array_index]    = np.percentile(diff_f2, 50)
        pct66th_array[array_index] = np.percentile(diff_f2, 66)
        pct95th_array[array_index] = np.percentile(diff_f2, 95)
        pct99th_array[array_index] = np.percentile(diff_f2, 99)
        pct99th9_array[array_index] = np.percentile(diff_f2, 99.9)
        array_index = array_index + 1

    #calculate std error of mean 
    print( "method:" + method_tick[method_index] + ",rate:" + rate + ",delta:" + delta)
    mean_sem     = sem(mean_array)
    pct95th_sem  = sem(pct95th_array)
    pct99th_sem  = sem(pct99th_array)
    pct99th9_sem = sem(pct99th9_array)

    mean_mean     = np.percentile(mean_array, 50)
    pct66th_mean     = np.percentile(pct66th_array, 50)
    pct95th_mean  = np.percentile(pct95th_array, 50)
    pct99th_mean  = np.percentile(pct99th_array, 50)
    pct99th9_mean = np.percentile(pct99th9_array, 50)

    print("mean:" + str(mean_mean))
    print("66-percentile:" + str(pct66th_mean))
    print("95-percentile:" + str(pct95th_mean))	
    print("99-percentile:" + str(pct99th_mean))
    print("99.9-percentile:"+ str(pct99th9_mean))

    mean_per_delta_mean_array[delat_index] = mean_mean
    mean_per_delta_sem_array[delat_index]  = mean_sem

    pct95th_per_delta_mean_array[delat_index]  = pct95th_mean
    pct95th_per_delta_sem_array[delat_index]   = pct95th_sem

    pct99th_per_delta_mean_array[delat_index]  = pct99th_mean
    pct99th_per_delta_sem_array[delat_index]   = pct99th_sem

    pct99th9_per_delta_mean_array[delat_index] = pct99th9_mean
    pct99th9_per_delta_sem_array[delat_index]  = pct99th9_sem
    
    delat_index = delat_index + 1 


for i in range(0,len(mean_per_delta_mean_array)):
    print(mean_per_delta_mean_array[i], end =" ")
print("\n-----------------------")

ax.errorbar(delta_tick, mean_per_delta_mean_array, yerr=mean_per_delta_sem_array,color=color_list[method_index], linestyle='solid',
    label=method_array[method_index]+":50th pct", marker='.', lw=1.5)

for i in range(0,len(pct95th_per_delta_mean_array)):
    print(pct95th_per_delta_mean_array[i], end =" ")
print("\n-----------------------")

ax.errorbar(delta_tick, pct95th_per_delta_mean_array, yerr=pct95th_per_delta_sem_array ,color=color_list[method_index], linestyle='dashed',
    label=method_array[method_index]+":95th pct", marker='.', lw=1.5)

for i in range(0,len(pct99th_per_delta_mean_array)):
    print(pct99th_per_delta_mean_array[i], end =" ")
print("\n-----------------------")

ax.errorbar(delta_tick, pct99th_per_delta_mean_array, yerr=pct99th_per_delta_sem_array ,color=color_list[method_index], linestyle='dotted',
    label=method_array[method_index]+":99th pct", marker='.', lw=1.5)

for i in range(0,len(pct99th9_per_delta_mean_array)):
    print(pct99th9_per_delta_mean_array[i], end =" ")
print("\n-----------------------")

ax.errorbar(delta_tick, pct99th9_per_delta_mean_array, yerr=pct99th9_per_delta_sem_array ,color=color_list[method_index], linestyle='dashdot',
    label=method_array[method_index]+":99.9th pct", marker='.', lw=1.5)

ax.grid(True)
ax.legend(loc='upper left', prop={'size': 8})
ax.set_title(fig_title)
ax.set_xlabel(fig_xlabel)
ax.set_ylabel(fig_ylabel)
#ax.set_xlim(left=0, right=1000)
#ax.set_ylim(bottom=0, top=yaxis_limit)
#ax.set_yscale('log')
#ax.ticklabel_format(useOffset=False, style='plain')

method_index = method_index + 1


# setup_file = open("/Users/wamos/matplot/kv_cdf/"+ working_dir +"/setup.txt","r") 
# tor_servers = setup_file.readline()
# plt.gcf().text(0.02, 0.95, tor_servers , fontsize=7)
# clients = setup_file.readline()
# plt.gcf().text(0.02, 0.93, clients, fontsize=7)
# gossip_period = setup_file.readline()
# plt.gcf().text(0.02, 0.91, gossip_period, fontsize=7)
# service_time = setup_file.readline()
# plt.gcf().text(0.02, 0.89, service_time, fontsize=7)

plt.savefig(fig_title+'.png', format='png', dpi=500)
#plt.show()
