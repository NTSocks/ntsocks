# NTSocks: A Generic and High-Performance Socket System over Mordern NTB Hardware for Edge Cloud

## Introduction

NTB based RPC library -- NTRPC: Focus on latency-sensitive small message, and aim to improve message rate (maybe include message throughput).
1. Design and Implementation of NTB Verbs for Transport Layer.
2. Definition and Design of key components of NTB-based RPC.
3. Case Study: Key-Value Store over NTB Network.


## Code lines statistics for NTSocks

C/C++ code lines: **5131(ntm) + 4762(libnts) + 4015(ntp) = 13908 lines**.

### **Updated at 2021.02.08:**

Component Name | Lines of C code (LoC) | Lines of C/C/C++ header file | Total LoC 
-|-|-|-
libnts      | 3402 | 368  | 3770
monitor     | 3941 | 195  | 787
ntb-proxy   | 2301 | 208   | 2509
libfdu-util | 3137 | 822   | 3959
ntkv        | 2016 | 349   | 2365
nts-benchmark | 1578 | 168 | 1746
ntb-bench-tool| 1650 | 66  | 1716
in tatal    | 18025 | 2176  | 20201


## Note

4月3日及之前的commits是正常的。

