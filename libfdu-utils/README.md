# fdu-utils library for common codes/modules in NTSocks

## Build & Install

- `cd libfdu-utils`
- `make all` or `make -j8`  (note: generated **libfdu-utils.so** and **libfdu-utils.a** files are in **libfdu-utils/libs** directory.)
- `sudo make install`   (note: install libfdu-utils.so into **/usr/local/lib**) 

## Clean 

- `make clean`
- `make distclean`

## Deploy

scp libs/* ntsocks@10.10.88.211:/home/ntsocks/ntsocks-deploy/libs
scp libs/* ntsocks@10.10.88.210:/home/ntsocks/ntsocks-deploy/libs
