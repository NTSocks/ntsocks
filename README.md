# NTSocks: An ultra-low latency and compatible PCIe interconnect

*Hope that you'd be glad to add a star if you think this repo is helpful!*

## Overview

NTSocks develops an ultra-fast and compatible PCIe interconnect targetting for
rack-scale disaggregations, leveraging routable PCIe fabric (e.g., Non Transparent
Bridge -- NTB). Compared to state-of-the-art RDMA and user-space network stack 
(e.g., mTCP), such routable PCIe fabric achieves lower nanoseceond-level latency,
and higher throughput close to PCIe bandwidth limits, due to eliminating the
protocol translation overhead (between PCIe and network protocols) and complex
in-NIC resource management (e.g., limited NIC cache for RDMA connection context 
and memory mapping table). This potentially meets the increasing demand of high-speed 
networking required by rack-scale disaggregation. NTSocks is thereby motivated as 
a software indirection layer to efficiently virtualize the PCIe transport resources
for rack-scale disaggregated applications. At a high level, the NTSocks layer gets 
socket API calls from apps and process them by interacting with a user-level runtime 
library on behalf of the app, as the following architecture figure shows.
NTSocks consists of three components: *NTSocks library*, *Monitor*, and *Proxy*.
The runtime NTSocks library, called *libnts*, is located inside the application process 
and provides user-friendly and performant socket abstractions for applications. It is 
responsible for parsing and fowarding control and data path operations into NTSocks monitor
and proxy, respectively. The control plane component monitor, called *NTM*, performs control
path operations (such as connection setup when `connect()`). The data plane component proxy, called *NTP*,
virtualizes global PCIe fabric transport resource (such as NTB mapped memory) in a lightweight manner,
and then coordinates with *libnts* to perform fast data path operations (e.g., `write()`/`read()`). 
These three components communicate via shared memory. This repo contains implementation of NTSocks system. 
Please refer to each `README` under those subdirectories for more informations.

Note that our NTSocks work has been published at ACM CoNEXT 2022 -- 
[**Best Paper Award**](https://conferences2.sigcomm.org/co-next/2022/#!/home).


## System Components

<div align=center><img width="700" alt="image" src="https://github.com/NTSocks/ntsocks/assets/91358910/1fc121a4-9027-4869-9ec1-4b607c14fda5"></div>

### NTSocks Runtime Library -- libnts

The `libnts` directory contains the implementation of NTSocks runtime library, including 
control (e.g., `nts_connect()`) and data path interfaces (e.g., `nts_send()`). This libnts
library is located inside application process, which is responsible for parsing and forwarding
control and data path requests into NTSocks monitor and proxy, respectively.


### NTSocks Monitor -- NTM

The `monitor` directory contains the implementation of NTSocks control plane component, including 
initilizing PCIe fabric routing information, allocating the allocation between connection and 
point-to-point PCIe interconnect, connection establishment over routable PCIe interconnect,
and destroying PCIe fabric resources when disconnecting with remote sides.

### NTSocks Proxy -- NTP 

The `ntb-proxy` directory contains the implementation of NTSocks data plane component, including 
partitioning PCIe transport resources with multi-cores for scaling up, batching tx/rx messages foarding, 
and hierarchical performance isolation mechanism. 

### NTSocks Applications and Benchmarks

The `./apps` directory contains an NTSocks-enhanced key-value store system with varying YCSB workloads. 
Wherea the `./benchmark` directory contains a set of function tests to evaluate NTSocks 
control and data plane interfaces.

## Building NTSocks System

Please refer to `./build-nts.sh` script to build three NTSocks components. This will produce:

- **libnts:** `libnts.so` and `libnts.a` in `libnts/lib/` directory; `libnts.h`, `nts_config.h`, and
  `socket.h` in `libnts/include/` directory.
- **NTM:** An executable `ntb-monitor` binary in `monitor/bin/` directory.
- **NTP:** An executable `ntb-proxy` binary in `ntb-proxy/src/` directory.

## Experimental Workflows

Before executation, please ensure the PCIe NTB is enabled in BIOS and physical machines, while
correctly setuping DPDK NTB Poll-Mode Driver.

1. Startup NTSocks control plane monitor (refer to `./monitor.sh`).
2. Startup NTSocks data plane proxy (refer to `./proxy.sh`).
3. Assuming to use `LD_PRELOAD` runtime library *libnts*, a socket based server program (refer to
   `benchmark/unified_benchmark/ntb-server`) and a client program (refer to `benchmark/unified_benchmark/ntb-client`)
   are executed without any code changes. They can communciate with each other transparently.

## License

The code is released under the [Apache 2.0 License](https://opensource.org/license/apache-2-0).

This work was done collobratively with Intel DPDK team. Thanks to Intel for the PCIe NTB testbed and technical support!

If you use our NTSocks or related codes in your research, please cite our paper:

```bib
@inproceedings{huang2022ultra,
  title={An ultra-low latency and compatible PCIe interconnect for rack-scale communication},
  author={Huang, Yibo and Huang, Yukai and Yan, Ming and Hu, Jiayu and Liang, Cunming and Xu, Yang and Zou, Wenxiong and Zhang, Yiming and Zhang, Rui and Huang, Chunpu and others},
  booktitle={Proceedings of the 18th International Conference on emerging Networking EXperiments and Technologies},
  pages={232--244},
  year={2022}
}
```
