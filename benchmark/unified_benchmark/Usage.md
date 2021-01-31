# Test delay
Server: `bin/server -a 10.176.22.210 -l 0 -s 4096 -m 2 -w -t 10 -n 1000000`
Client: `bin/client -a 10.176.22.210 -l 0 -s 4096 -m 2 -w -t 10 -n 1000000`

# Test bandwidth
Server: `bin/server -a 10.176.22.210 -l 1 -s 4096 -m 2 -t 10 -d 30`
Client: `bin/client -a 10.176.22.210 -l 1 -s 4096 -m 2 -t 10 -d 30`

# Test throughput in request num limit
Server: `bin/server -a 10.176.22.210 -l 2 -s 4096 -m 2 -t 10 -n 1000000`
Client: `bin/client -a 10.176.22.210 -l 2 -s 4096 -m 2 -t 10 -n 1000000`

# Test throughput in time limit
Server: `bin/server -a 10.176.22.210 -l 3 -s 4096 -m 2 -t 10 -d 15 `
Client: `bin/client -a 10.176.22.210 -l 3 -s 4096 -m 2 -t 10 -d 15`

## Parameter Description:

-a 10.176.22.210: refers to the bound ip address on the server side, and refers to the ip address of the server connected to on the client side.

-w: Whether ack is required. It does only work when testing latency.

-l 1: Test type. 0 means testing latency, 1 means testing bandwidth. 2 means testing throughput in request num limit. 3 means testing throughput in time limit.

-s 4096: The size of the message transmission. The unit is byte.

-m 0: Bind the CPU core. 0 is bound to even-numbered cores, 1 is bound to cores from 16-31, 48-63, and 2 is bound to cores from 0-15, 32-47. The default is 2.

-t 10: Number of threads. Default is 1

-n 100000: The number of messages sent or received in each connection. The default is 100000. It does not work when l is 1 or 3;

-d 30: Specify the test time when testing  bandwidth and testing throughput in time limit. The unit is seconds. It does not work when testing latency and throughput in request num limit.

