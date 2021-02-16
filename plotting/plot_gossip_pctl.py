import numpy as np
import pandas as pd
import sys, os
import matplotlib.pyplot as plt

working_dir = sys.argv[1]
title_filter1 = sys.argv[2]
title_filter2 = sys.argv[3]
fig_title   = sys.argv[4]
yaxis_limit = float(sys.argv[5])
#base_case = sys.argv[5]
#xaxis_limit = float(sys.argv[5])
extension = "log"

fig_xlabel  = "Gossip Period (usec)"
fig_ylabel  = "RTT Latency (usec)" #sys.argv[3]

file_list=[]
for root, subdirs, files in os.walk(working_dir):
	for f in files:
		if title_filter in f and extension in f:
			f_path = os.path.join(root, f)
			file_list.append(f_path)

# for f in file_list:
#     print(f)

###TODO:
### auto_parse rates and method name
### use set to get unique rate and method name
### then covert the set to array to sort it! 


rate = title_filter1
client_tick = ["c0", "c1" , "c2"]
method_tick  = [title_filter2+"_randselect"]
method_array = ["replica-select with delta=1, redirction=1"]
gossip_tick=["25", "50", "100", "200", "400", "800"]


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

method_index=0
for method in method_tick:
    mean_array = np.zeros(len(gossip_tick))
    pct95th_array = np.zeros(len(gossip_tick))
    pct99th_array = np.zeros(len(gossip_tick))
    pct99th9_array = np.zeros(len(gossip_tick))
    loss_rate = np.zeros(len(gossip_tick))
    #rate = rate_tick[0]
    
    array_index=0    
    for gossip in gossip_tick:
        #### base cases
        #filename0 = "/Users/wamos/matplot/kv_cdf/"+ working_dir + "/"+ method + "_" + rate + "." + extension
        #data = pd.read_csv(filename0, delimiter = ",", usecols=[1]).values
        parent_dir = "/home/ec2-user/efs/multi-tor-evalution/log/"

        filename0 = parent_dir + working_dir + "/"+ method + "_g_" + gossip + "_"+ client_tick[0] + "_" + rate + "." + extension
        filename1 = parent_dir + working_dir + "/"+ method + "_g_" + gossip + "_"+ client_tick[1] + "_" + rate + "." + extension
        filename2 = parent_dir + working_dir + "/"+ method + "_g_" + gossip + "_"+ client_tick[2] + "_" + rate + "." + extension        

        #np.loadtxt() is too slow!!
        #data0 = np.loadtxt(filename0, delimiter=",", usecols=(1))
        #data1 = np.loadtxt(filename1, delimiter=",", usecols=(1))
        #data2 = np.loadtxt(filename2, delimiter=",", usecols=(1))  

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
    
        print( "method:" + method + ",rate:" + rate)
        print("loss_count:"+ str(loss_count))
        print("loss_rate:"+str(loss_rate[array_index]))
        print("min:"+str(np.min(data)/1000))
        print("mean:" + str(mean_array[array_index]))
        print("95-percentile::" + str(pct95th_array[array_index]))	
        print("99-percentile:" + str(pct99th_array[array_index]))
        print("99.9-percentile:"+ str(pct99th9_array[array_index]))
        #pct99th9data = data[data > pct99th9_array[array_index]*1000 ]
        #print(pct99th9data)
        array_index = array_index + 1

    ax.plot(gossip_tick, mean_array, color=color_list[method_index], linestyle='solid',
        label=method_array[method_index]+":mean", marker='.', lw=1.5)
    ax.plot(gossip_tick, pct95th_array, color=color_list[method_index], linestyle='dashed',
        label=method_array[method_index]+":95th pct", marker='.', lw=1.5)
    ax.plot(gossip_tick, pct99th_array, color=color_list[method_index], linestyle='dotted',
        label=method_array[method_index]+":99th pct", marker='.', lw=1.5)
    ax.plot(gossip_tick, pct99th9_array, color=color_list[method_index], linestyle='dashdot',
        label=method_array[method_index]+":99.9th pct", marker='.', lw=1.5)

        
    
    ax.grid(True)
    ax.legend(loc='upper left', prop={'size': 8})
    ax.set_title(fig_title)
    ax.set_xlabel(fig_xlabel)
    ax.set_ylabel(fig_ylabel)
    #ax.set_xlim(left=0, right=1000)
    ax.set_ylim(bottom=0, top=yaxis_limit)
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
