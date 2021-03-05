#!/bin/bash

LD_PRELOAD=~/bob/mtu_nts/NTSock/libnts/build/src/lib/.libs/libnts.so ./bin/client 10.176.22.210 8091
