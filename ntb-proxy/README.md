## Quick_Start:
```
$cd ./src
$make
$./build/ntb_proxy
```

### 210/211 server quick start

```
ssh ntb-server1@10.10.88.210    # passme
source /etc/profile
su
cd /home/ntb-server1/NTSock/ntb-proxy/src
./build/ntb-proxy


ssh ntb-server2@10.10.88.211    # passme
source /etc/profile
su
cd /home/ntb-server1/NTSock/ntb-proxy/src
./build/ntb-proxy

```

## Code lines statistics for libnts

C/C++ code lines: **3123 + 892 = 4015 lines**.


```
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                               17            685            418           3123
C/C++ Header                    20            345            412            892
make                             2             32             10             81
-------------------------------------------------------------------------------
SUM:                            39           1062            840           4096
-------------------------------------------------------------------------------
```
