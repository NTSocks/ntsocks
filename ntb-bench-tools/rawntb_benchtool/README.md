# 运行方式：

放置在dpdk/example/ntb 目录下

make后运行

首先分别在两台服务器上输入

`./build/ntb_fwd -l6 --proc-type=primary -- -s 1024 -n 100000 -m 1`

在cpu序号为6的核上运行守护进程

其中，-s参数为payload size，-n参数为请求的总数量，-m为benchmark类型。

详细信息可通过执行如下-h命令查看
`./build/ntb_fwd -l6 --proc-type=primary -- -h`