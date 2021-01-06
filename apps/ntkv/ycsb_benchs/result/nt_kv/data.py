#!/usr/bin/python2.7

import os
import re
import pandas as pd
from io import open

path = "./"

files = os.listdir(path)

# Latency (LAT)
list_result = [] # [payload, thread, average_tput, min_tput, max_tput, total_tput, average_bw, min_bw, max_bw, total_bw]
# Throughtput (TPUT)
list_tput = [] # [payload, thread, avg_avg, min_avg, max_avg, avg_50t, min_50t, max_50t, avg_99t, min_99t, max_99t, avg_99_9t, min_99_9t, max_99_9t, avg_99_99t, min_99_99t, max_99_99t]

err_flag_lat = err_flag_tput = False

for file in files:
    if file == "data.py" or file == "result":
        continue
    else:
        split_file = file.split("-")

        # print file
        # a = len(split_file)
        # print a

        # Split out the params
        db = split_file[0]   
        
        if len(split_file) == 4:
            run_type = split_file[1] + split_file[2]
            p = split_file[3].split(".")
            pa = p[0].split("d")
            payload = int(pa[1])
        else:
            run_type = split_file[1]
            p = split_file[2].split(".")
            pa = p[0].split("d")
            payload = int(pa[1])

        with open(file, encoding='utf-8') as f:
                source_file = f.read()
                temp = source_file
                throughput = re.findall(r"^THROUGHPUT = (.*)KTPS", temp, re.M)
                average = re.findall(r"^AVERAGE = (.*)us", temp, re.M)
                median = re.findall(r"^MEDIAN = (.*)us", temp, re.M)
                t99 = re.findall(r"^99 = (.*)us", temp, re.M)
                t99_9 = re.findall(r"^99.9 = (.*)us", temp, re.M)
                t99_99 = re.findall(r"^99.99 = (.*)us", temp, re.M)

                list_element = [db, run_type, payload, \
                                    throughput[0], \
                                    average[0], \
                                    median[0], \
                                    t99[0], \
                                    t99_9[0], \
                                    t99_99[0]]
                list_result.append(list_element)

column_result = ['db type', '# run type', '# payload', \
            'Average TPUT (kTPS/s)', \
            'Average Latency (us)', \
            '50 Tail Latency (us)', \
            '99 Tail Latency (us)', \
            '99.9 Tail Latency (us)', \
            '99.99 Tail Latency (us)']

result = pd.DataFrame(columns = column_result, data = list_result)

# print result_lat
result.to_csv(path + 'result/result.csv', encoding='utf8')
if err_flag_lat is False:
    print("Latency Benchmark OK")
