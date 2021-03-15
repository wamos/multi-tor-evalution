import numpy as np
import pandas as pd
import sys, os
import matplotlib.pyplot as plt
from scipy.stats import sem
import gc

working_dir = sys.argv[1]
title_filter1 = sys.argv[2]
title_filter2 = sys.argv[3]
fig_title   = sys.argv[4]
yaxis_limit = float(sys.argv[5])
#base_case = sys.argv[5]
#xaxis_limit = float(sys.argv[5])
extension = "log"
gossip_ext = "pgy"
queue_depth_ext = "qd"

fig_xlabel  = "Request Rate (RPS)"
fig_ylabel  = "RTT Latency (usec)" #sys.argv[3]

file_list=[]
for root, subdirs, files in os.walk(working_dir):
	for f in files:
		if title_filter1 in f and title_filter2 in f:
			f_path = os.path.join(root, f)
			file_list.append(f_path)

# for f in file_list:
#     print(f)
debug=True
#rate = title_filter1
switch_tick = ["s0", "s1" , "s2"]
client_tick = ["c0", "c1" , "c2"]
method_tick  = [title_filter2+"_randonly", title_filter2+"_randselect"]
method_array = ["random", "replica-select"]
method_prefix = ["rand", "select"]

#method_tick  = [title_filter2+"_randselect"]
#method_array = ["select"]
#method_array = ["replica-select with gossip=25us, redirction=1"]

run_tick=["0", "1", "2", "3", "4"]
#run_tick=["0", "1", "2", "3"]
#run_tick=["0", "1", "2"]
#run_tick=["0"]

#method_array = ["random", "replica-select min load delta=1"]
#method_tick  = ["qd_random", "delta2_randselect", "delta3_randselect", "delta4_randselect"]
#method_array = ["random", "replica-select min load delta=2", "replica-select min load delta=3", "replica-select min load delta=4"]

# rate_tick =[ "10k", "15k", "20k"]
# rate_array =[ "10000", "15000", "20000"]

# rate_tick =[ "10k", "15k", "20k", "25k", "30k", "35k"]
# rate_array =[ "10000", "15000", "20000", "25000", "30000", "35000"]
#rate_tick =[ "20k", "23k", "25k", "28k", "30k", "33k"]
#rate_tick =[ "20k", "23k", "25k", "28k", "30k", "33k", "35k"]
#rate_tick =[ "20k", "21k", "22k", "23k", "24k", "25k", "26k", "27k", "28k", "29k", "30k", "31k", "32k", "33k", "34k", "35k"]
#rate_tick =[ "30k", "35k", "40k", "45k", "50k", "55k"]
#rate_tick =[ "30k", "33k"]
#coarse_rate_list=(40000 35000 30000 25000 20000)
#rate_tick =[ "30k", "33k", "35k"]
#rate_tick =[ "20k", "25k", "30k", "35k", "40k", "45k", "46k", "47k", "48k"]
### for 20us constant service time
rate_tick =[ "20k", "21k", "22k", "23k", "24k", "25k", "26k", "27k", "28k", "29k", 
"30k", "31k", "32k", "33k", "34k", "35k", "36k","37k", "38k", "39k", "40k", "41k", "42k", "43k", "44k", "45k"]
rate_tick =["35k"]

# rate_tick =["30k", "31k", "32k", "33k", "34k", "35k","36k", 
# "37k", "38k", "39k", "40k", "41k", "42k", "43k", "44k", "45k", "46k", "47k", "48k"]
#rate_tick =[ "20k", "21k", "22k", "23k", "24k", "25k", "26k", "27k", "28k", "29k", "30k", "31k", "32k", "33k", "34k", "35k","36k"]
#rate_tick=["37k", "38k", "39k", "40k", "41k", "42k", "43k", "44k", "45k", "46k", "47k", "48k"]

#rate_tick =[ "50k", "60k", "70k", "80k", "90k", "100k", "110k", "120k", "130k", "140k", "150k", "160k", "170k", "180k", "190k", "200k"]
#rate_tick =["2500", "2600", "2700", "2800", "2900", "3000", "3100", "3200", "3300", "3400", "3500", "3600", "3700", "3800", "3900", "4000"]

# rate_array =[ "20000", "25000", "30000"]

# method_tick  = ["saturation_randonly", "saturation_randselect"]
# method_array = ["random", "replica-select with gossip=25us, delta=1"]
# rate_tick =["35k", "36k", "37k", "38k"]

#suffix = "_g_25_d_1_r_1"
#rate_tick =[ "30k", "31k", "32k", "33k", "34k", "35k"]
#rate_array =[ "30000", "31000" , "32000", "33000", "34000", "35000"]
color_list = ['xkcd:blue', 'xkcd:orange', 'xkcd:orchid', 'xkcd:sienna', 'xkcd:grey']

fig, ax = plt.subplots(figsize=(10, 5))

#loss_rate = np.zeros(len(delta_tick))

method_index=0
#delat_index=0
for method in method_tick:
    mean_per_delta_mean_array = np.zeros(len(rate_tick))
    mean_per_delta_sem_array = np.zeros(len(rate_tick))

    pct95th_per_delta_mean_array = np.zeros(len(rate_tick))
    pct95th_per_delta_sem_array = np.zeros(len(rate_tick))

    pct99th_per_delta_mean_array = np.zeros(len(rate_tick))
    pct99th_per_delta_sem_array = np.zeros(len(rate_tick))

    pct99th9_per_delta_mean_array = np.zeros(len(rate_tick))
    pct99th9_per_delta_sem_array = np.zeros(len(rate_tick))

    gossip_ratio_array         = np.zeros(len(rate_tick))
    redirection_ratio_array    = np.zeros(len(rate_tick))

    pct50th_queue_depth_mean_array  = np.zeros(len(rate_tick))
    pct95th_queue_depth_mean_array  = np.zeros(len(rate_tick))
    pct99th_queue_depth_mean_array  = np.zeros(len(rate_tick))
    pct99th9_queue_depth_mean_array = np.zeros(len(rate_tick))

    print(method_array[method_index]+ ":")

    rate_index=0    
    for rate in rate_tick:
        mean_array = np.zeros(len(run_tick))
        pct95th_array = np.zeros(len(run_tick))
        pct99th_array = np.zeros(len(run_tick))
        pct99th9_array = np.zeros(len(run_tick))
        loss_rate = np.zeros(len(run_tick))

        pct50th_queue_depth_array  = np.zeros(len(run_tick))
        pct95th_queue_depth_array  = np.zeros(len(run_tick))
        pct99th_queue_depth_array  = np.zeros(len(run_tick))
        pct99th9_queue_depth_array = np.zeros(len(run_tick))

        if rate_index >= 0  and "_randonly" in method:
            continue 
        
        array_index=0
        redirction_count=0
        gossip_count=0
        data_count=0    
        for run in run_tick:
            #### base cases
            #filename0 = "/Users/wamos/matplot/kv_cdf/"+ working_dir + "/"+ method + "_" + rate + "." + extension
            #data = pd.read_csv(filename0, delimiter = ",", usecols=[1]).values
            parent_dir = "/home/ec2-user/efs/multi-tor-evalution/log/"
            filename_prefix = method_tick[method_index] + "_run" + run            

            # if rate == "35k" and run == "9":
            #     filename0 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ client_tick[0] + "_" + rate + "." + extension
            #     filename1 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ client_tick[1] + "_" + rate + "." + extension
            #     filename2 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ client_tick[2] + "_" + rate + "." + extension
            #     data0 = pd.read_csv(filename0, delimiter = ",", usecols=[1]).values
            #     data2 = pd.read_csv(filename2, delimiter = ",", usecols=[1]).values
            #     data = np.concatenate([data0, data1, data2])
            #else:

            filename0 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ client_tick[0] + "_" + rate + "." + extension
            filename1 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ client_tick[1] + "_" + rate + "." + extension
            filename2 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ client_tick[2] + "_" + rate + "." + extension

            ## normal cases
            ## column 0, column 1, column 2
            ## number of redirections, end-to-end latency, server processing latency
            data0 = pd.read_csv(filename0, delimiter = ",", usecols=[1]).values
            data1 = pd.read_csv(filename1, delimiter = ",", usecols=[1]).values
            data2 = pd.read_csv(filename2, delimiter = ",", usecols=[1]).values
            data0 = data0[150:]
            data1 = data1[150:]
            data2 = data2[150:]
            data = np.concatenate([data0, data1, data2])

            # for server latency
            #data = data[data > 25*1000]
            loss_count = np.count_nonzero(data==0)
            #loss_rate[array_index] = (float) (np.count_nonzero(data==0))/data.shape[0]   
            if debug == True:
                print(filename_prefix)
                print("loss_count:"+str(loss_count))
            #print("loss_rate:"+str(loss_rate[array_index]))
            data = data [data > 0]
            data = data/1000

            mean_array[array_index]    = np.percentile(data, 50)
            pct95th_array[array_index] = np.percentile(data, 95)
            pct99th_array[array_index] = np.percentile(data, 99)
            pct99th9_array[array_index] = np.percentile(data, 99.9)

            if method_index ==1:
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

                filename0 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ switch_tick[0] + "_" + rate + "." + queue_depth_ext
                filename1 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ switch_tick[1] + "_" + rate + "." + queue_depth_ext
                filename2 = parent_dir + working_dir + "/"+ filename_prefix + "_"+ switch_tick[2] + "_" + rate + "." + queue_depth_ext 
                #print(filename0)
                data0 = pd.read_csv(filename0, delimiter = ",", usecols=[0]).values  
                data1 = pd.read_csv(filename1, delimiter = ",", usecols=[0]).values
                data2 = pd.read_csv(filename2, delimiter = ",", usecols=[0]).values
                redirection = np.concatenate([data0, data1, data2])

                pct50th_queue_depth_array[array_index]  = np.percentile(data, 50)
                pct95th_queue_depth_array[array_index]  = np.percentile(data, 95)
                pct99th_queue_depth_array[array_index]  = np.percentile(data, 99)
                pct99th9_queue_depth_array[array_index] = np.percentile(data, 99.9)

            array_index = array_index + 1
            gc.collect()

        #calculate std error of mean 
        print( "method:" + method_tick[method_index] + ",rate:" + rate)
        if method_index == 1:
            redirction_ratio = float(redirction_count)/float(data_count)
            gossip_ratio = float(gossip_count)/float(data_count)
            #print("redirction_count"+str(redirction_count))
            #print("data_count"+str(data_count))
            print("gossip_ratio:"+str(gossip_ratio))
            print("redirction_ratio:"+str(redirction_ratio))

            gossip_ratio_array[rate_index]       = gossip_ratio
            redirection_ratio_array[rate_index]  = redirction_ratio

            pct50th_queue_depth_mean_array[rate_index]  = np.percentile(pct50th_queue_depth_array, 50)
            pct95th_queue_depth_mean_array[rate_index]  = np.percentile(pct95th_queue_depth_array, 50)
            pct99th_queue_depth_mean_array[rate_index]  = np.percentile(pct99th_queue_depth_array, 50)
            pct99th9_queue_depth_mean_array[rate_index] = np.percentile(pct99th9_queue_depth_array, 50)

        mean_sem     = sem(mean_array)
        pct95th_sem  = sem(pct95th_array)
        pct99th_sem  = sem(pct99th_array)
        pct99th9_sem = sem(pct99th9_array)

        
        print(method_prefix[method_index]+"95th_loop=[", end =" ")
        for i in range(0,len(pct95th_array)):
            print(pct95th_array[i], end =",")
        print("]")
        
        print(method_prefix[method_index]+"99th_loop=[", end =" ")
        for i in range(0,len(pct99th_array)):
            print(pct99th_array[i], end =",")
        print("]")
        
        print(method_prefix[method_index]+"99th9_loop=[", end =" ")
        for i in range(0,len(pct99th9_array)):
            print(pct99th9_array[i], end =",")
        print("]")

        mean_mean     = np.percentile(mean_array, 0)
        pct95th_mean  = np.percentile(pct95th_array, 0)
        pct99th_mean  = np.percentile(pct99th_array, 0)
        pct99th9_mean = np.percentile(pct99th9_array, 0)

        mean_per_delta_mean_array[rate_index] = mean_mean
        mean_per_delta_sem_array[rate_index]  = mean_sem

        pct95th_per_delta_mean_array[rate_index]  = pct95th_mean
        pct95th_per_delta_sem_array[rate_index]   = pct95th_sem

        pct99th_per_delta_mean_array[rate_index]  = pct99th_mean
        pct99th_per_delta_sem_array[rate_index]   = pct99th_sem

        pct99th9_per_delta_mean_array[rate_index] = pct99th9_mean
        pct99th9_per_delta_sem_array[rate_index]  = pct99th9_sem
        
        rate_index = rate_index + 1 

    print(method_prefix[method_index]+"50th=[", end =" ")
    for i in range(0,len(mean_per_delta_mean_array)):
        print(mean_per_delta_mean_array[i], end=",")
    print("]")

    print(method_prefix[method_index]+"50th_sem=[", end =" ")
    for i in range(0,len(mean_per_delta_sem_array)):
        print(mean_per_delta_sem_array[i], end =",")
    print("]\n")

    ax.errorbar(rate_tick, mean_per_delta_mean_array, yerr=mean_per_delta_sem_array,color=color_list[method_index], linestyle='solid',
        label=method_array[method_index]+":50th pct", marker='.', lw=1.5)

    print(method_prefix[method_index]+"95th=[", end =" ")
    for i in range(0,len(pct95th_per_delta_mean_array)):
        print(pct95th_per_delta_mean_array[i], end =",")
    print("]")

    print(method_prefix[method_index]+"95th_sem=[", end =" ")
    for i in range(0,len(pct95th_per_delta_sem_array)):
        print(pct95th_per_delta_sem_array[i], end =",")
    print("]\n")

    ax.errorbar(rate_tick, pct95th_per_delta_mean_array, yerr=pct95th_per_delta_sem_array ,color=color_list[method_index], linestyle='dashed',
        label=method_array[method_index]+":95th pct", marker='.', lw=1.5)

    print(method_prefix[method_index]+"99th=[", end =" ")
    for i in range(0,len(pct99th_per_delta_mean_array)):
        print(pct99th_per_delta_mean_array[i], end =",")
    print("]")

    print(method_prefix[method_index]+"99th_sem=[", end =" ")
    for i in range(0,len(pct99th_per_delta_sem_array)):
        print(pct99th_per_delta_sem_array[i], end =",")
    print("]\n")

    ax.errorbar(rate_tick, pct99th_per_delta_mean_array, yerr=pct99th_per_delta_sem_array ,color=color_list[method_index], linestyle='dotted',
        label=method_array[method_index]+":99th pct", marker='.', lw=1.5)

    print(method_prefix[method_index]+"99th9=[", end =" ")
    for i in range(0,len(pct99th9_per_delta_mean_array)):
        print(pct99th9_per_delta_mean_array[i], end =",")
    print("]")

    print(method_prefix[method_index]+"99th9_sem=[", end =" ")
    for i in range(0,len(pct99th9_per_delta_sem_array)):
        print(pct99th9_per_delta_sem_array[i], end =",")
    print("]\n")

    ax.errorbar(rate_tick, pct99th9_per_delta_mean_array, yerr=pct99th9_per_delta_sem_array ,color=color_list[method_index], linestyle='dashdot',
        label=method_array[method_index]+":99.9th pct", marker='.', lw=1.5)

    if method_index == 1:
        print(method_prefix[method_index]+"redirection_ratio=[", end =" ")
        for i in range(0,len(redirection_ratio_array)):
            print(redirection_ratio_array[i], end =",")
        print("]\n")

        print(method_prefix[method_index]+"gossip_ratio=[", end =" ")
        for i in range(0,len(gossip_ratio_array)):
            print(gossip_ratio_array[i], end =",")
        print("]\n")

        print(method_prefix[method_index]+"pct50th_queue_depth=[", end =" ")
        for i in range(0,len(pct50th_queue_depth_mean_array)):
            print(pct50th_queue_depth_mean_array[i], end =",")
        print("]")

        print(method_prefix[method_index]+"pct95th_queue_depth=[", end =" ")
        for i in range(0,len(pct95th_queue_depth_mean_array)):
            print(pct95th_queue_depth_mean_array[i], end =",")
        print("]")

        print(method_prefix[method_index]+"pct99th_queue_depth=[", end =" ")
        for i in range(0,len(pct99th_queue_depth_mean_array)):
            print(pct99th_queue_depth_mean_array[i], end =",")
        print("]")

        print(method_prefix[method_index]+"pct99th9_queue_depth=[", end =" ")
        for i in range(0,len(pct99th9_queue_depth_mean_array)):
            print(pct99th9_queue_depth_mean_array[i], end =",")
        print("]")


    method_index = method_index + 1

ax.grid(True)
ax.legend(loc='upper left', prop={'size': 8})
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
