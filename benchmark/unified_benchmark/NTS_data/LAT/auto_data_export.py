#!/usr/bin/python

import os
import re
import pandas as pd
from io import open

path = "./"

files = os.listdir(path)

# Latency (LAT)
list_lat = []
# BandWidth (BW)
list_bw = []
# Throughtput (TPUT)
list_tput = []

err_flag_lat = err_flag_bw = err_flag_tput = False

for file in files:
    if file == "auto_data_export.py" or file == "result":
        continue
    else:
        split_file = file.split("_")

        # Split out the params        
        metric = int(split_file[1])
        payload = int(split_file[2])
        request_num = int(split_file[3])
        thread = int(split_file[4])
        partition = int(split_file[5])
        split_packet_size = split_file[6].split(".")
        packet_size = int(split_packet_size[0])

        # Remote Write - Latency
        if metric == 0:
            with open(file, encoding='utf-8') as f:

                source_file = f.read()
                temp = source_file
                # temp = source_file.decode("utf-8")
                
                avg = re.findall(r"^AVERAGE = (.*) us", temp, re.M)
                avg_list = [float(i) for i in avg]
                
                t50 = re.findall(r"^50 TAIL = (.*) us", temp, re.M)
                t50_list =  [float(i) for i in t50]

                t90 = re.findall(r"^90 TAIL = (.*) us", temp, re.M)
                t90_list =  [float(i) for i in t90]

                t99 = re.findall(r"^99 TAIL = (.*) us", temp, re.M)
                t99_list =  [float(i) for i in t99]

                t99_9 = re.findall(r"^99.9 TAIL = (.*) us", temp, re.M)
                t99_9_list =  [float(i) for i in t99_9]

                if not avg: # Find Error File
                    print("ERROR when Metric = %d, Payload size = %d, Thread = %d, Partition = %d" % (metric, payload, thread, partition))
                    err_flag_lat = True
                else:
                    avg_avg = sum(avg_list) / len(avg_list)
                    avg_min = min(avg_list)
                    avg_max = max(avg_list)

                    t50_avg = sum(t50_list) / len(t50_list)
                    t50_min = min(t50_list)
                    t50_max = max(t50_list)

                    t90_avg = sum(t90_list) / len(t90_list)
                    t90_min = min(t90_list)
                    t90_max = max(t90_list)

                    t99_avg = sum(t99_list) / len(t99_list)
                    t99_min = min(t99_list)
                    t99_max = max(t99_list)

                    t99_9_avg = sum(t99_9_list) / len(t99_9_list)
                    t99_9_min = min(t99_9_list)
                    t99_9_max = max(t99_9_list)

                    list_element = [payload, thread, partition, packet_size, request_num, \
                                    avg_avg, avg_min, avg_max, \
                                    t50_avg, t50_min, t50_max, \
                                    t90_avg, t90_min, t90_max, \
                                    t99_avg, t99_min, t99_max, \
                                    t99_9_avg, t99_9_min, t99_9_max]
                    list_lat.append(list_element)

        # BandWidth
        elif metric == 1:
            with open(file, encoding='utf-8') as f:

                source_file = f.read()
                temp = source_file
                # temp = source_file.decode("utf-8")

                avg_bw = re.findall(r"^average bandwidth is (.*) Gbits/sec", temp, re.M)
                med_bw = re.findall(r"^medium bandwidth is (.*) Gbits/sec", temp, re.M)
                max_bw = re.findall(r"^max bandwidth is (.*) Gbits/sec", temp, re.M)

                if not avg_bw: # Find Error File
                    print("ERROR when Metric = %d, Payload size = %d, Thread = %d, Partition = %d, Packet size = %d" % (metric, payload, thread, partition, packet_size))
                    err_flag_bw = True
                else:
                    list_element = [payload, thread, partition, packet_size, request_num, \
                                    avg_bw, med_bw, max_bw]
                    list_bw.append(list_element)

        # Throughput
        elif metric == 2:
            with open(file, encoding='utf-8') as f:

                source_file = f.read()
                temp = source_file
                # temp = source_file.decode("utf-8")

                avg_tput = re.findall(r"^average throughput is (.*) reqs/sec", temp, re.M)
                med_tput = re.findall(r"^medium throughput is (.*) reqs/sec", temp, re.M)
                max_tput = re.findall(r"^max throughput is (.*) reqs/sec", temp, re.M)

                if not avg_tput: # Find Error File
                    print("ERROR when Metric = %d, Payload size = %d, Thread = %d, Partition = %d, Packet size = %d" % (metric, payload, thread, partition, packet_size))
                    err_flag_tput = True
                else:
                    list_element = [payload, thread, partition, packet_size, request_num, \
                                    avg_tput, med_tput, max_tput]
                    list_tput.append(list_element)

list_lat.sort()
list_bw.sort()
list_tput.sort()

# result lat       
column_lat = ['Payload Size', '# of Threads', '# of partition', 'Packet Size', 'request_num', \
            'Average Average (us)', 'Minimum Average (us)', 'Maximum Average (us)', \
            'Average 50 Tail (us)', 'Minimum 50 Tail (us)', 'Maximum 50 Tail (us)', \
            'Average 90 Tail (us)', 'Minimum 90 Tail (us)', 'Maximum 90 Tail (us)', \
            'Average 99 Tail (us)', 'Minimum 99 Tail (us)', 'Maximum 99 Tail (us)', \
            'Average 99.9 Tail (us)', 'Minimum 99.9 Tail (us)', 'Maximum 99.9 Tail (us)']
result_lat = pd.DataFrame(columns = column_lat, data = list_lat)

result_lat.to_csv(path + 'result/result_lat.csv', encoding='utf8')
if err_flag_lat is False:
    print("Latency Benchmark OK")


# result bw
name_bw = ['Payload Size', '# of Threads', '# of partition', 'Packet Size', 'request_num', \
            'Average BW (Gbps)', 'Medium BW (Gbps)', 'Maximum BW (Gbps)']
result_bw = pd.DataFrame(columns = name_bw, data = list_bw)

result_bw.to_csv(path + 'result/result_bw.csv', encoding='utf8')
if err_flag_bw is False:
    print("BandWidth Benchmark OK")


# result tput
name_tput = ['Payload Size', '# of Threads', '# of partition', 'Packet Size', 'request_num', \
            'Average TPUT (req/s)', 'Medium TPUT (req/s)', 'Maximum TPUT (req/s)']
result_tput = pd.DataFrame(columns = name_tput, data = list_tput)

result_tput.to_csv(path + 'result/result_tput.csv', encoding='utf8')
if err_flag_tput is False:
    print("Throughtput Benchmark OK")