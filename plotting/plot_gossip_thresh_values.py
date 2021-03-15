import numpy as np
import pandas as pd
import sys, os
import matplotlib.pyplot as plt
from scipy.stats import sem

working_dir = sys.argv[1]
title_filter1 = sys.argv[2]
rate = title_filter1
title_filter2 = sys.argv[3]
fig_title   = sys.argv[4]
yaxis_limit = float(sys.argv[5])
#base_case = sys.argv[5]
#xaxis_limit = float(sys.argv[5])
extension = "log"
gossip_ext = "pgy"

fig_xlabel  = "Log Thresholds"
fig_ylabel  = "RTT Latency (usec)" #sys.argv[3]

file_list=[]
for root, subdirs, files in os.walk(working_dir):
	for f in files:
		if title_filter1 in f and title_filter2 in f:
			f_path = os.path.join(root, f)
			file_list.append(f_path)

# for f in file_list:
#     print(f)

switch_tick = ["s0", "s1" , "s2"]
client_tick = ["c0", "c1" , "c2"]
method_tick  = [title_filter2+"_randselect", title_filter2+"_randonly"]
method_array = ["replica-select with redirection=1", "random"]
gossip_load_thresh = ["0", "1", "2", "4", "8", "16", "32", "64", "128"]
run_tick=["0", "1", "2", "3", "4"]
#rate="32k"
#suffix = "_g_25_d_1_r_1"
#rate_tick =[ "30k", "31k", "32k", "33k", "34k", "35k"]
#rate_array =[ "30000", "31000" , "32000", "33000", "34000", "35000"]
color_list = ['xkcd:orange', 'xkcd:blue', 'xkcd:orchid', 'xkcd:sienna', 'xkcd:grey']

fig, ax = plt.subplots(figsize=(10, 5))

mean_per_delta_mean_array = np.zeros(len(gossip_load_thresh))
mean_per_delta_sem_array = np.zeros(len(gossip_load_thresh))

pct95th_per_delta_mean_array = np.zeros(len(gossip_load_thresh))
pct95th_per_delta_sem_array = np.zeros(len(gossip_load_thresh))

pct99th_per_delta_mean_array = np.zeros(len(gossip_load_thresh))
pct99th_per_delta_sem_array = np.zeros(len(gossip_load_thresh))

pct99th9_per_delta_mean_array = np.zeros(len(gossip_load_thresh))
pct99th9_per_delta_sem_array = np.zeros(len(gossip_load_thresh))

gossip_ratio_array = np.zeros(len(gossip_load_thresh))
redirection_ratio_array  = np.zeros(len(gossip_load_thresh))

method_index=0
delat_index=0
#for method in method_tick:
for thresh in gossip_load_thresh:
    mean_array = np.zeros(len(run_tick))
    pct95th_array = np.zeros(len(run_tick))
    pct99th_array = np.zeros(len(run_tick))
    pct99th9_array = np.zeros(len(run_tick))
    loss_rate = np.zeros(len(run_tick))
    
    array_index=0
    redirction_count=0
    gossip_count=0
    data_count=0    
    for run in run_tick:
        #### base cases
        #filename0 = "/Users/wamos/matplot/kv_cdf/"+ working_dir + "/"+ method + "_" + rate + "." + extension
        #data = pd.read_csv(filename0, delimiter = ",", usecols=[1]).values
        parent_dir = "/home/ec2-user/efs/multi-tor-evalution/log/"
        filename_prefix = method_tick[method_index] + "_gth" + thresh + "_run" + run
        
        filename0 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ client_tick[0] + "_" + rate + "." + extension
        filename1 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ client_tick[1] + "_" + rate + "." + extension
        filename2 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ client_tick[2] + "_" + rate + "." + extension

        ## normal cases
        ## column 0, column 1, column 2
        ## number of redirections, end-to-end latency, server processing latency
        data0 = pd.read_csv(filename0, delimiter = ",", usecols=[1]).values
        data1 = pd.read_csv(filename1, delimiter = ",", usecols=[1]).values
        data2 = pd.read_csv(filename2, delimiter = ",", usecols=[1]).values
        data0 = data0[10:]
        data1 = data1[10:]
        data2 = data2[10:]
        data = np.concatenate([data0, data1, data2])

        # for server latency
        #data = data[data > 25*1000]
        loss_count = np.count_nonzero(data==0)
        loss_rate[array_index] = (float) (np.count_nonzero(data==0))/data.shape[0]        
        data = data [data > 0]
        #data = data[data < 1000*1000]

        mean_array[array_index]    = np.percentile(data, 50)/1000
        pct95th_array[array_index] = np.percentile(data, 95)/1000
        pct99th_array[array_index] = np.percentile(data, 99)/1000
        pct99th9_array[array_index] = np.percentile(data, 99.9)/1000

        data0 = pd.read_csv(filename0, delimiter = ",", usecols=[0]).values
        data1 = pd.read_csv(filename1, delimiter = ",", usecols=[0]).values
        data2 = pd.read_csv(filename2, delimiter = ",", usecols=[0]).values
        redirection = np.concatenate([data0, data1, data2])
        data_count += redirection.shape[0]
        redirection = redirection[redirection == 1]
        redirction_count += redirection.shape[0]

        filename0 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ switch_tick[0] + "_" + rate + "." + gossip_ext
        filename1 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ switch_tick[1] + "_" + rate + "." + gossip_ext
        filename2 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ switch_tick[2] + "_" + rate + "." + gossip_ext 
        #print(filename0)
        data0 = pd.read_csv(filename0, delimiter = ",", usecols=[0]).values  
        data1 = pd.read_csv(filename1, delimiter = ",", usecols=[0]).values
        data2 = pd.read_csv(filename2, delimiter = ",", usecols=[0]).values
        gossip_count += data0.shape[0] + data1.shape[0] + data2.shape[0]        
    
        print( "method:" + method_tick[method_index] + ",rate:" + rate + ",thresh:" + thresh + "run:" + run) 
        print("loss_count:"+ str(loss_count))
        # print("loss_rate:"+str(loss_rate[array_index]))
        # print("min:"+str(np.min(data)/1000))
        # print("mean:" + str(mean_array[array_index]))
        # print("95-percentile::" + str(pct95th_array[array_index]))	
        # print("99-percentile:" + str(pct99th_array[array_index]))
        # print("99.9-percentile:"+ str(pct99th9_array[array_index]))
        array_index = array_index + 1

    #calculate std error of mean 
    print( "method:" + method_tick[method_index] + ",rate:" + rate + ",thresh:" + thresh)
    redirction_ratio = float(redirction_count)/float(data_count)
    gossip_ratio = float(gossip_count)/float(data_count)
    #print("redirction_count"+str(redirction_count))
    #print("data_count"+str(data_count))
    print("gossip_ratio:"+str(gossip_ratio))
    gossip_ratio_array[delat_index] = gossip_ratio
    print("redirction_ratio:"+str(redirction_ratio))
    redirection_ratio_array[delat_index] = redirction_ratio

    mean_sem     = sem(mean_array)
    pct95th_sem  = sem(pct95th_array)
    pct99th_sem  = sem(pct99th_array)
    pct99th9_sem = sem(pct99th9_array)

    print(method_tick[method_index]+"95th=[", end =" ")
    for i in range(0,len(pct95th_array)):
        print(pct95th_array[i], end =",")
    print("]")
    
    print(method_tick[method_index]+"99th=[", end =" ")
    for i in range(0,len(pct99th_array)):
        print(pct99th_array[i], end =",")
    print("]")
    
    print(method_tick[method_index]+"99th9=[", end =" ")
    for i in range(0,len(pct99th9_array)):
        print(pct99th9_array[i], end =",")
    print("]")

    mean_mean     = np.percentile(mean_array, 50)
    pct95th_mean  = np.percentile(pct95th_array, 50)
    pct99th_mean  = np.percentile(pct99th_array, 50)
    pct99th9_mean = np.percentile(pct99th9_array, 50)

    mean_per_delta_mean_array[delat_index] = mean_mean
    mean_per_delta_sem_array[delat_index]  = mean_sem

    pct95th_per_delta_mean_array[delat_index]  = pct95th_mean
    pct95th_per_delta_sem_array[delat_index]   = pct95th_sem

    pct99th_per_delta_mean_array[delat_index]  = pct99th_mean
    pct99th_per_delta_sem_array[delat_index]   = pct99th_sem

    pct99th9_per_delta_mean_array[delat_index] = pct99th9_mean
    pct99th9_per_delta_sem_array[delat_index]  = pct99th9_sem
    
    delat_index = delat_index + 1 

print("select_50th=[", end =" ")
for i in range(0,len(mean_per_delta_mean_array)):
    print(mean_per_delta_mean_array[i], end =",")
print("]")

print("select_50th_sem=[", end =" ")
for i in range(0,len(mean_per_delta_sem_array)):
    print(mean_per_delta_sem_array[i], end =",")
print("]")

ax.errorbar(gossip_load_thresh, mean_per_delta_mean_array, yerr=mean_per_delta_sem_array,color=color_list[method_index], linestyle='solid',
    label=method_array[method_index]+":50th pct", marker='.', lw=1.5)

print("select_95th=[", end =" ")
for i in range(0,len(pct95th_per_delta_mean_array)):
    print(pct95th_per_delta_mean_array[i], end =",")
print("]")

print("select_95th_sem=[", end =" ")
for i in range(0,len(pct95th_per_delta_sem_array)):
    print(pct95th_per_delta_sem_array[i], end =",")
print("]")

ax.errorbar(gossip_load_thresh, pct95th_per_delta_mean_array, yerr=pct95th_per_delta_sem_array ,color=color_list[method_index], linestyle='dashed',
    label=method_array[method_index]+":95th pct", marker='.', lw=1.5)

print("select_99th=[", end =" ")
for i in range(0,len(pct99th_per_delta_mean_array)):
    print(pct99th_per_delta_mean_array[i], end =",")
print("]")

print("select_99th_sem=[", end =" ")
for i in range(0,len(pct99th_per_delta_sem_array)):
    print(pct99th_per_delta_sem_array[i], end =",")
print("]")

ax.errorbar(gossip_load_thresh, pct99th_per_delta_mean_array, yerr=pct99th_per_delta_sem_array ,color=color_list[method_index], linestyle='dotted',
    label=method_array[method_index]+":99th pct", marker='.', lw=1.5)

print("select_99th9=[", end =" ")
for i in range(0,len(pct99th9_per_delta_mean_array)):
    print(pct99th9_per_delta_mean_array[i], end =",")
print("]")

print("select_99th9_sem=[", end =" ")
for i in range(0,len(pct99th9_per_delta_sem_array)):
    print(pct99th9_per_delta_sem_array[i], end =",")
print("]")

ax.errorbar(gossip_load_thresh, pct99th9_per_delta_mean_array, yerr=pct99th9_per_delta_sem_array ,color=color_list[method_index], linestyle='dashdot',
    label=method_array[method_index]+":99.9th pct", marker='.', lw=1.5)

print("redirection_ratio=[", end =" ")
for i in range(0,len(redirection_ratio_array)):
    print(redirection_ratio_array[i], end =",")
print("]\n")

print("gossip_load_thresh=[", end =" ")
for i in range(0,len(gossip_load_thresh)):
    print(gossip_load_thresh[i], end =",")
print("]\n")

print("gossip_ratio=[", end =" ")
for i in range(0,len(gossip_ratio_array)):
    print(gossip_ratio_array[i], end =",")
print("]\n")

method_index = method_index + 1

#for random baseline
mean_array = np.zeros(len(run_tick))
pct95th_array = np.zeros(len(run_tick))
pct99th_array = np.zeros(len(run_tick))
pct99th9_array = np.zeros(len(run_tick))
loss_rate = np.zeros(len(run_tick))
array_index=0
for run in run_tick:
    #### base cases
    #filename0 = "/Users/wamos/matplot/kv_cdf/"+ working_dir + "/"+ method + "_" + rate + "." + extension
    #data = pd.read_csv(filename0, delimiter = ",", usecols=[1]).values
    parent_dir = "/home/ec2-user/efs/multi-tor-evalution/log/"
    filename_prefix = method_tick[method_index] + "_run" + run

    filename0 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ client_tick[0] + "_" + rate + "." + extension
    filename1 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ client_tick[1] + "_" + rate + "." + extension
    filename2 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ client_tick[2] + "_" + rate + "." + extension

    ## normal cases
    ## column 0, column 1, column 2
    ## number of redirections, end-to-end latency, server processing latency
    data0 = pd.read_csv(filename0, delimiter = ",", usecols=[1]).values
    data1 = pd.read_csv(filename1, delimiter = ",", usecols=[1]).values
    data2 = pd.read_csv(filename2, delimiter = ",", usecols=[1]).values
    data0 = data0[10:]
    data1 = data1[10:]
    data2 = data2[10:]
    data = np.concatenate([data0, data1, data2])

    # for server latency
    #data = data[data > 25*1000]
    loss_count = np.count_nonzero(data==0)
    loss_rate[array_index] = (float) (np.count_nonzero(data==0))/data.shape[0]        
    data = data [data > 0]
    #data = data[data < 1000*1000]

    mean_array[array_index]    = np.percentile(data, 50)/1000
    pct95th_array[array_index] = np.percentile(data, 95)/1000
    pct99th_array[array_index] = np.percentile(data, 99)/1000
    pct99th9_array[array_index] = np.percentile(data, 99.9)/1000

    data0 = pd.read_csv(filename0, delimiter = ",", usecols=[0]).values
    data1 = pd.read_csv(filename1, delimiter = ",", usecols=[0]).values
    data2 = pd.read_csv(filename2, delimiter = ",", usecols=[0]).values
    redirection = np.concatenate([data0, data1, data2])
    data_count += redirection.shape[0]
    redirection = redirection[redirection == 1]
    redirction_count += redirection.shape[0]        

    array_index = array_index + 1

mean_sem     = sem(mean_array)
pct95th_sem  = sem(pct95th_array)
pct99th_sem  = sem(pct99th_array)
pct99th9_sem = sem(pct99th9_array)

mean_mean     = np.percentile(mean_array, 50)
pct95th_mean  = np.percentile(pct95th_array, 50)
pct99th_mean  = np.percentile(pct99th_array, 50)
pct99th9_mean = np.percentile(pct99th9_array, 50)    

mean_per_delta_mean_array = np.zeros(len(gossip_load_thresh))
np.put(mean_per_delta_mean_array, np.arange(len(gossip_load_thresh)), mean_mean)
mean_per_delta_sem_array = np.zeros(len(gossip_load_thresh))
np.put(mean_per_delta_sem_array, np.arange(len(gossip_load_thresh)), mean_sem)

pct95th_per_delta_mean_array = np.zeros(len(gossip_load_thresh))
np.put(pct95th_per_delta_mean_array, np.arange(len(gossip_load_thresh)), pct95th_mean)
pct95th_per_delta_sem_array = np.zeros(len(gossip_load_thresh))
np.put(pct95th_per_delta_sem_array, np.arange(len(gossip_load_thresh)), pct95th_sem)

pct99th_per_delta_mean_array = np.zeros(len(gossip_load_thresh))
np.put(pct99th_per_delta_mean_array, np.arange(len(gossip_load_thresh)), pct99th_mean)
pct99th_per_delta_sem_array = np.zeros(len(gossip_load_thresh))
np.put(pct99th_per_delta_sem_array, np.arange(len(gossip_load_thresh)), pct99th_sem)

pct99th9_per_delta_mean_array = np.zeros(len(gossip_load_thresh))
np.put(pct99th9_per_delta_mean_array, np.arange(len(gossip_load_thresh)), pct99th9_mean)
pct99th9_per_delta_sem_array = np.zeros(len(gossip_load_thresh))
np.put(pct99th9_per_delta_sem_array, np.arange(len(gossip_load_thresh)), pct99th9_sem)

print("rand_50th=[", end =" ")
for i in range(0,len(mean_per_delta_mean_array)):
    print(mean_per_delta_mean_array[i], end =",")
print("]")

print("rand_50th_sem=[", end =" ")
for i in range(0,len(mean_per_delta_sem_array)):
    print(mean_per_delta_sem_array[i], end =",")
print("]")

ax.errorbar(gossip_load_thresh, mean_per_delta_mean_array, yerr=mean_per_delta_sem_array,color=color_list[method_index], linestyle='solid',
    label=method_array[method_index]+":50th pct", marker='.', lw=1.5)

print("rand_95th=[", end =" ")
for i in range(0,len(pct95th_per_delta_mean_array)):
    print(pct95th_per_delta_mean_array[i], end =",")
print("]")

print("rand_95th_sem=[", end =" ")
for i in range(0,len(pct95th_per_delta_sem_array)):
    print(pct95th_per_delta_sem_array[i], end =",")
print("]")

ax.errorbar(gossip_load_thresh, pct95th_per_delta_mean_array, yerr=pct95th_per_delta_sem_array ,color=color_list[method_index], linestyle='dashed',
    label=method_array[method_index]+":95th pct", marker='.', lw=1.5)

print("rand_99th=[", end =" ")
for i in range(0,len(pct99th_per_delta_mean_array)):
    print(pct99th_per_delta_mean_array[i], end =",")
print("]")

print("rand_99th_sem=[", end =" ")
for i in range(0,len(pct99th_per_delta_sem_array)):
    print(pct99th_per_delta_sem_array[i], end =",")
print("]")

ax.errorbar(gossip_load_thresh, pct99th_per_delta_mean_array, yerr=pct99th_per_delta_sem_array ,color=color_list[method_index], linestyle='dotted',
    label=method_array[method_index]+":99th pct", marker='.', lw=1.5)

print("rand_99th9=[", end =" ")
for i in range(0,len(pct99th9_per_delta_mean_array)):
    print(pct99th9_per_delta_mean_array[i], end =",")
print("]")

print("rand_99th9_sem=[", end =" ")
for i in range(0,len(pct99th9_per_delta_sem_array)):
    print(pct99th9_per_delta_sem_array[i], end =",")
print("]")

ax.errorbar(gossip_load_thresh, pct99th9_per_delta_mean_array, yerr=pct99th9_per_delta_sem_array ,color=color_list[method_index], linestyle='dashdot',
    label=method_array[method_index]+":99.9th pct", marker='.', lw=1.5)


ax.grid(True)
#ax.legend(loc='upper left', prop={'size': 8})
ax.set_title(fig_title)
ax.set_xlabel(fig_xlabel)
ax.set_ylabel(fig_ylabel)
#ax.set_xlim(left=0, right=1000)
ax.set_ylim(bottom=0, top=yaxis_limit)
#ax.set_yscale('log')
#ax.ticklabel_format(useOffset=False, style='plain')




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
