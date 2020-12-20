## Quick Start

1. make clean

2. make all

##### Use -help for parameter usage

```sh
./bin/server -h
```

##### or

```sh
./bin/client -h
```

### execute code sample

#### if you checkout througput 

##### server side

```sh
./bin/server -a {ipaddr} -p {port}  -w -e -c {conn_num} -s {payload_size}
You can add -w with ack (payloadsize)
```

##### client side

```sh
./bin/client -a {ipaddr} -p {port}  -w -e -c {conn_num} -s {payload_size}
You can add -w with ack (payloadsize)
```

#### if you checkout latency 

##### server side

```sh
./bin/server -a {ipaddr} -p {port}  -w -l -c {conn_num} -s {payload_size}
You can add -w with ack (payloadsize)
```

##### client side

```sh
./bin/client -a {ipaddr} -p {port}  -w -l -c {conn_num} -s {payload_size}
You can add -w with ack (payloadsize)
```

