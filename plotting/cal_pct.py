import numpy as np
import pandas as pd
import sys, os
import matplotlib.pyplot as plt
from scipy.stats import sem

working_dir = sys.argv[1]
title_filter2 = sys.argv[2]
#base_case = sys.argv[5]
#xaxis_limit = float(sys.argv[5])
extension = "log"

fig_xlabel  = "Requesst Index"
fig_ylabel  = "Load counter (# of requests)" #sys.argv[3]

file_list=[]
for root, subdirs, files in os.walk(working_dir):
	for f in files:
		if title_filter1 in f and title_filter2 in f:
			f_path = os.path.join(root, f)
			file_list.append(f_path)

#rate = title_filter1
switch_tick = ["s0", "s1" , "s2"]
client_tick = ["c0", "c1" , "c2"]
method_tick  = [title_filter2+"_randselect"]
#method_tick  = [title_filter2+"_randonly"]
#method_tick  = [title_filter2]
#method_array = ["replica-select with gossip=25us, redirction=1"]
#run_tick=["0", "2", "3", "4", "5", "6", "7", "9"]
run_tick=["0"]
rate_tick =["35k"]
color_list = ['xkcd:blue', 'xkcd:orange', 'xkcd:orchid', 'xkcd:sienna', 'xkcd:grey']

#fig, ax = plt.subplots(figsize=(8, 5))

#gossip="8"
qd_type = "qd"
gossip_type = "pgy"
log_type="log"
parent_dir = "/home/ec2-user/efs/multi-tor-evalution/log/"
filename_prefix = method_tick[0] +"_run" + run_tick[0]
working_dir=""
qd_filename     = parent_dir + working_dir + "/"+ filename_prefix + "_"+ switch_tick[2] + "_" + rate_tick[0] + "." + qd_type
print(qd_filename)
gossip_filename = parent_dir + working_dir + "/"+ filename_prefix + "_"+ switch_tick[2] + "_" + rate_tick[0] + "." + gossip_type
print(gossip_filename)
log_filename = parent_dir + working_dir + "/"+ filename_prefix + "_"+ client_tick[2] + "_" + rate_tick[0] + "." + log_type
print(log_filename)

gossip_index = pd.read_csv(gossip_filename, delimiter = ",", usecols=[0,2]).values
gossip_size = gossip_index.shape[0]

local_load = pd.read_csv(qd_filename, delimiter = ",", usecols=[0]).values
mean_value = np.mean(local_load)    
print("mean_value:" + str(mean_value))
#ax.axhline(mean_value, color='tab:green', lw=2)

pct50th_value = np.percentile(local_load, 50)
print("load pct50th_value:" + str(pct50th_value))
#print("mean_value:" + str(mean_value))
#ax.axhline(mean_value, color='xkcd:sienna', lw=2)

pct99th_value = np.percentile(local_load, 99)
print("load pct99th_value:" + str(pct99th_value))
#ax.axhline(pct99th_value, color='xkcd:grey', lw=2)

latency_log = pd.read_csv(log_filename, delimiter = ",", usecols=[1]).values
latency_size = latency_log.shape[0]
ratio=float(gossip_size)/float(latency_size)
print("gossip ratio:"+str(ratio))

pct50th_value = np.percentile(latency_log, 50)/1000
pct95th_value = np.percentile(latency_log, 95)/1000
pct99th_value = np.percentile(latency_log, 99)/1000
pct99th9_value = np.percentile(latency_log, 99.9)/1000
print("latency pct50th:" + str(pct50th_value))
print("latency pct95th:" + str(pct95th_value))
print("latency pct99th:" + str(pct99th_value))
print("latency pct99.9th:" + str(pct99th9_value))

#pct99th = np.percentile(local_load, 99)
#ax.axhline(pct99th, color='red', lw=2)


# ax.grid(True)
# ax.legend(loc='upper left', prop={'size': 8})
# ax.set_title(fig_title)
# ax.set_xlabel(fig_xlabel)
# ax.set_ylabel(fig_ylabel)
# ax.set_xlim(left=xaxis_left, right=xaxis_left+show_rows)
# ax.set_ylim(bottom=0, top=35)
# #ax.set_yscale('log')
# #ax.ticklabel_format(useOffset=False, style='plain')
# plt.savefig(fig_title+'.png', format='png', dpi=500)
#plt.show()
