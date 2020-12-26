# 运行方式：

放置在dpdk/example/ntb 目录下

make后运行

首先分别在两台服务器上输入

`./build/ntb_fwd -l6 --proc-type=primary`

在cpu序号为6的核上运行守护进程

会在屏幕上输出时延信息



之后分别在两台服务器上输入

`./build/ntb_fwd -l7 --proc-type=secondary`

运行用户进程，进行10秒数据传输，并输出不同包长下的吞吐量

---

> 详细信息可通过执行如下-h命令查看: `./build/ntb_fwd -l6 --proc-type=primary -- -h`