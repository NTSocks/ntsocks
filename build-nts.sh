#!/bin/bash

if [ $# != 1 ]; then
echo "Usage: $0 [NTSocks Base Path]"
echo " e.g.: $0 ~/NTSock"
exit 1;
fi

NTS_ROOT_DIR=$1
NTS_UTILS_DIR=./libnts-utils
LIBNTS_DIR=./libnts
NTM_DIR=./monitor
NTP_DIR=./ntb-proxy

cd $NTS_ROOT_DIR

# first build libnts-utils
cd $NTS_UTILS_DIR
make clean
make
sudo make install

# build ntb-proxy
cd $NTS_ROOT_DIR
cd $NTP_DIR/src
make clean
make


# build monitor
cd $NTS_ROOT_DIR
cd $NTM_DIR
make clean
make

# build libnts
cd $NTS_ROOT_DIR
cd $LIBNTS_DIR
./build.sh