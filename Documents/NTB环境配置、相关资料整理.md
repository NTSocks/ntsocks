# NTB环境配置、相关资料整理

## 一、NTB环境配置

1. 在Bios中将两台B2B相连的服务器设置为： ntb pcie port 为 NTB to NTB ；Enable NTB bars 为 Enabled ；BAR size 设置为29；

2. 系统后 执行

   ```
   modprobe ntb_transport
   ```

   之后 lsmod | grep ntb 应该有3个mod

   ![image-20191214154725442](C:\Users\JingQi\AppData\Roaming\Typora\typora-user-images\image-20191214154725442.png)

3. 执行

   ```
   ./dpdk/usetools/dpdk_setup.sh
   ```

   对DPDK进行编译，之后Insert IGB UIO模块，并Setup hugepage mappings

   ![image-20191214154952832](C:\Users\JingQi\AppData\Roaming\Typora\typora-user-images\image-20191214154952832.png)

4. 设置1GB大页，由于Intel的机器有两个NUMA节点，因此需要为两个节点各分配一页大页。

   ```
   mount -t hugetlbfs -o pagesize=1GB nodev /mnt/huge_1GB/
   echo 1 > /sys/devices/system/node/node0/hugepages/hugepages-1048576kB/nr_hugepages
   echo 1 > /sys/devices/system/node/node1hugepages/hugepages-1048576kB/nr_hugepages
   ```

   分配后可使用

   ```
    cat /sys/kernel/mm/hugepages/hugepages-1048576kB/free_hugepages
   ```

   来查看1GB大页内存的数量，以及

   ```
   mount | grep huge
   ```

   查看挂载大页内存的目录

5. 将NTB设备绑定到DPDK Driver，首先执行

   ```
   .dpdk/usetools/dpdk-devbind.py -s
   ```

   查看NTB设备的编号

   ![image-20191214160831619](C:\Users\JingQi\AppData\Roaming\Typora\typora-user-images\image-20191214160831619.png)

   之后执行

   ```
   ./dpdk/usetools/dpdk-devbind.py --bind=igb_uio 17:00.0
   ```

   将NTB绑定到DPDK Driver。如果想将NTB从DPDK Driver绑回Kernel Driver，使用

   ```
   ./usertools/dpdk-devbind.py -b ntb_hw_intel 17:00.0
   ```

6. 为了使NTB Remote Write获得最佳性能，首先执行任意NTB可执行程序，获得其打印的NTB所属系统地址空间的地址，之后执行

   ```
   echo "disable=1" >> /proc/mtrr
   echo "base=0x387fe8000000 size=0x8000000 type=write-combining" >> /proc/mtrr
   cat /proc/mtrr
   ```

   第二条指令中的Size为NTB拥有的地址空间的大小

   ![image-20191214162301182](C:\Users\JingQi\AppData\Roaming\Typora\typora-user-images\image-20191214162301182.png)

   

## 二、相关网页链接

1. DPDK Programmer's Guide（包括Ring、Mempool、Mbuf相关介绍）

     https://doc.dpdk.org/guides/prog_guide/  

2. DPDK Multi-process Support（参考其多进程通讯的实现）

    http://doc.dpdk.org/guides/prog_guide/multi_proc_support.html 

3. Intel NTB Startup Guide

   https://github.com/davejiang/linux/wiki/Intel-NTB-Startup-Guide

4. Linux kernel.org NTB Drivers Documentation

   https://www.kernel.org/doc/html/latest/driver-api/ntb.html

5. Arm 公司开源的NTB Drivers

    https://github.com/jonmason/ntb 

6. Dolphin （一家NTB商用公司）

    https://www.dolphinics.com/index.html 

