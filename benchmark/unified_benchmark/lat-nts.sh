LD_PRELOAD=~/bob/mtu_nts/NTSock/libnts/build/src/lib/.libs/libnts.so ./bin/client -p 9999 -t 1 -r 1 -c 1024 -m 2 -w -l 0 -s 65536 -n $@
