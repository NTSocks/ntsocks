## Quick Start

1. $ ./autogen.sh

2. cd build && make distclean && ../configure

3. make -j4

4. LD_PRELOAD=./libnts.so ./hello-ntsock-server

LD_PRELOAD=./libnts.so ./hello-ntsock
LD_PRELOAD=./libnts.so ./examples/hello-ntsock

LD_PRELOAD=./libnts.so ./examples/client-ntsock