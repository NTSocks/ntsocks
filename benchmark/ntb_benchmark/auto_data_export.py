#!/usr/bin/python

import os
import re
import pandas as pd
from io import open

path = "./"

files = os.listdir(path)

# Latency (LAT)
list_lat = [] # [payload, thread, average_tput, min_tput, max_tput, total_tput, average_bw, min_bw, max_bw, total_bw]
# Throughtput (TPUT)
list_tput = [] # [payload, thread, avg_avg, min_avg, max_avg, avg_50t, min_50t, max_50t, avg_99t, min_99t, max_99t, avg_99_9t, min_99_9t, max_99_9t, avg_99_99t, min_99_99t, max_99_99t]

err_flag_lat = err_flag_tput = False

for file in files:
    if file == "auto_data_export.py" or file == "result":
        continue
    else:
        split_file = file.split("_")

        # Split out the params        
        metric = int(split_file[2])
        payload = int(split_file[3])
        request_num = int(split_file[4])
        thread = int(split_file[5])

        split_partition = split_file[6].split(".")
        partition = int(split_partition[0])

        # Remote Write - Latency
        if metric == 0:
            with open(file, encoding='utf-8') as f:

                source_file = f.read()
                temp = source_file
                # temp = source_file.decode("utf-8")
                
                avg = re.findall(r"^MEDIAN = (.*) us", temp, re.M)
                avg_list = [float(i) for i in avg]
                
                t50 = re.findall(r"^50 TAIL = (.*) us", temp, re.M)
                t50_list =  [float(i) for i in t50]

                t99 = re.findall(r"^99 TAIL = (.*) us", temp, re.M)
                t99_list =  [float(i) for i in t99]

                t99_9 = re.findall(r"^99.9 TAIL = (.*) us", temp, re.M)
                t99_9_list =  [float(i) for i in t99_9]

                t99_99 = re.findall(r"^99.99 TAIL = (.*) us", temp, re.M)
                t99_99_list =  [float(i) for i in t99_99]

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

                    t99_avg = sum(t99_list) / len(t99_list)
                    t99_min = min(t99_list)
                    t99_max = max(t99_list)

                    t99_9_avg = sum(t99_9_list) / len(t99_9_list)
                    t99_9_min = min(t99_9_list)
                    t99_9_max = max(t99_9_list)

                    t99_99_avg = sum(t99_99_list) / len(t99_99_list)
                    t99_99_min = min(t99_99_list)
                    t99_99_max = max(t99_99_list)

                    list_element = [payload, thread, partition, request_num, \
                                    avg_avg, avg_min, avg_max, \
                                    t50_avg, t50_min, t50_max, \
                                    t99_avg, t99_min, t99_max, \
                                    t99_9_avg, t99_9_min, t99_9_max, \
                                    t99_99_avg, t99_99_min, t99_99_max]
                    list_lat.append(list_element)
        
        # Remote Write - Throughput
        elif metric == 1:
            with open(file, encoding='utf-8') as f:

                source_file = f.read()
                temp = source_file
                # temp = source_file.decode("utf-8")

                tput = re.findall(r"^THROUGHPUT1 = (.*) REQ/s", temp, re.M)
                tput_list = [float(i) for i in tput]

                if not tput: # Find Error File
                    print("ERROR when Metric = %d, Payload size = %d, Thread = %d, Partition = %d" % (metric, payload, thread, partition))
                    err_flag_tput = True
                else:
                    tput_avg = sum(tput_list) / len(tput_list)
                    tput_min = min(tput_list)
                    tput_max = max(tput_list)
                    tput_sum = sum(tput_list)

                    list_element = [payload, thread, partition, request_num, \
                                    tput_avg, tput_min, tput_max, tput_sum]
                    list_tput.append(list_element)



list_lat.sort()
list_tput.sort()

       
            
column_lat = ['Payload Size', '# of Threads', '# of partition', 'request_num', \
            'Average Average (us)', 'Minimum Average (us)', 'Maximum Average (us)', \
            'Average 50 Tail (us)', 'Minimum 50 Tail (us)', 'Maximum 50 Tail (us)', \
            'Average 99 Tail (us)', 'Minimum 99 Tail (us)', 'Maximum 99 Tail (us)', \
            'Average 99.9 Tail (us)', 'Minimum 99.9 Tail (us)', 'Maximum 99.9 Tail (us)', \
            'Average 99.99 Tail (us)', 'Minimum 99.99 Tail (us)', 'Maximum 99.99 Tail (us)']
result_lat = pd.DataFrame(columns = column_lat, data = list_lat)

# print result_lat
result_lat.to_csv(path + 'result/result_lat.csv', encoding='utf8')
if err_flag_lat is False:
    print("Latency Benchmark OK")



name_tput = ['Payload Size', '# of Threads', '# of partition', 'request_num', \
            'Average TPUT (req/s)', 'Minimum TPUT (req/s)', 'Maximum TPUT (req/s)', 'Total TPUT (req/s)']
result_tput = pd.DataFrame(columns = name_tput, data = list_tput)

# print result_tput
result_tput.to_csv(path + 'result/result_tput.csv', encoding='utf8')
if err_flag_tput is False:
    print("Throughput Benchmark OK")
