# utils library for common codes/modules in NTSocks

## Runtime Requirements


## Quick Start
 
### Build & Install

- `cd libnts-utils`
- `make all` or `make -j8`  (note: generated **libnts-utils.so** and **libnts-utils.a** files are in **libnts-utils/libs** directory.)
- `sudo make install`   (note: install libnts-utils.so into **/usr/local/lib**) 

### Clean

- `make clean`
- `make distclean`

### Deploy

scp libs/* ntsocks@10.10.88.211:/home/ntsocks/ntsocks-deploy/libs
scp libs/* ntsocks@10.10.88.210:/home/ntsocks/ntsocks-deploy/libs
cd ntsocks-deploy/libs
sudo cp libnts-utils.* /usr/local/lib/
sudo ldconfig


## Wiki Pages

For more information on configuration, and performance tuning, please visit the NTSocks Project Wiki.


## Contributions

Any PR submissions are welcome!!!
