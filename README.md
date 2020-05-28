# NTRPC -- A NTB-based RPC Library
## Introduction
NTB based RPC library -- NTRPC: Focus on latency-sensitive small message, and aim to improve message rate (maybe include message throughput).
1. Design and Implementation of NTB Verbs for Transport Layer.
2. Definition and Design of key components of NTB-based RPC.
3. Case Study: Key-Value Store over NTB Network.


## Code lines statistics for NTSocks

C/C++ code lines: **5131(ntm) + 4762(libnts) + 4015(ntp) = 13908 lines**.

### **Updated at 2020.05.28:**

Component Name | File numbers |  Lines of C code (LoC) | Lines of C/C/C++ header file  
-|-|-|-
Libnts      | 61  | 5901 | 1307  | 6398
Monitor     | 49  | 4245 | 1130  | 5375
Proxy       | 44  | 2366 | 960   | 3326
Libfdu-util | 22  | 1459 | 498   | 1957
In tatal    |176  |13971 | 3895  | 18042




## Note

4月3日及之前的commits是正常的。