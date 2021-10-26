# simple_nosql
A simple key-value store system implemented with hashmap and memory pool

## Quick start

Build kv, execute the following command:

```
make
```

**Use -help for parameter usage**

```
./bin/server -h
```

or

```
./bin/client -h
```

The parameters are as follows:

```
Options:
 -s <server>                        bind to server address(default 10.10.88.201)
 -p <port>                          bind to server port(default 8099)
 -t <threads>                       handle connection with multi-threads(default 1, max 100)
 -m <max-mempool-size(GB/MB/KB)>    maximum memory pool size(default 0)
 -e <each-chunk-size(GB/MB/KB)>     each chunk size(default 0)
 -h  
```

Run simply as follows:

**server side**

```
./bin/server -s ip -p port
```

**client side**

```
./bin/client -s ip -p port
```

