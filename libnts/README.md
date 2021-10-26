
## Quick Build Start

1. ./autogen.sh

2. cd build/ && make distclean

3. ../configure 

4. make -j4

5. make dist

6. tar -xzvf libnts-1.0.0.tar.gz

7. cd libnts-1.0.0

8. ./configure --prefix=/home/spark2/app/libs/libnts

9. make -j4

10. make install

11. cd .. && LD_PRELOAD=/home/spark2/app/libs/libnts/lib/libnts.so examples/hello-ntsock


## Code lines statistics for libnts

C/C++ code lines: **3448 + 1314 = 4762 lines**.

```
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                               30           1014            872           3448
C/C++ Header                    38            642            873           1314
make                             2             17             16             45
-------------------------------------------------------------------------------
SUM:                            70           1673           1761           4807
-------------------------------------------------------------------------------
```
