#include "kvs_db.h"

#include "core/db.h"
#include "core/timer.h"
#include <iostream>
#include <string>
#include "core/properties.h"
#include "simple_kvs/kvs_client.h"


namespace ycsbc {

int KVSDB::Read(const std::string &table, const std::string &key,
           const std::vector<std::string> *fields,
           std::vector<KVPair> &result) {

    message_t msg;
    msg.cmd = CMD_GET;
    msg.key_len = key.size() + 1;
    msg.value_len = 0;
    char *obuf = (char*) malloc(MESSAGE_SIZE + msg.key_len);

    kvs_client_.context()->obuf_len = MESSAGE_SIZE + msg.key_len;
    memcpy(obuf, &msg, MESSAGE_SIZE);
    memcpy(obuf + MESSAGE_SIZE, key.data(), msg.key_len);
    kvs_client_.context()->obuf = obuf;

   // printf("Read key_len is %d , value_len is %d\n", msg.key_len, msg.value_len);
    return kvs_client_.handle_msg(CMD_GET);
  }

int KVSDB::Scan(const std::string &table, const std::string &key,
        int len, const std::vector<std::string> *fields,
        std::vector<std::vector<KVPair>> &result) {
    throw "Scan: function not implemented!";
}

int KVSDB::Update(const std::string &table, const std::string &key,
            std::vector<KVPair> &values) {
    message_t msg;
    msg.cmd = CMD_PUT;
    msg.key_len = key.size() + 1;
    msg.value_len = values[0].second.size() + 1;

    char *out = (char*)malloc(MESSAGE_SIZE + msg.key_len + msg.value_len);
    // memset(out, 0, MESSAGE_SIZE + msg.key_len + msg.value_len);
    kvs_client_.context()->obuf_len = MESSAGE_SIZE + msg.key_len + msg.value_len;
    memcpy(out, &msg, MESSAGE_SIZE);
    memcpy(out + MESSAGE_SIZE, key.data(), msg.key_len);
    memcpy(out + MESSAGE_SIZE + msg.key_len, values[0].second.data(), msg.value_len);

    kvs_client_.context()->obuf = out;

    // printf("Update key_len is %d , value_len is %d\n", msg.key_len, msg.value_len);
    return kvs_client_.handle_msg(CMD_PUT);
}

int KVSDB::Insert(const std::string &table, const std::string &key,
        std::vector<KVPair> &values) {
    return KVSDB::Update(table, key, values);
}

int KVSDB::Delete(const std::string &table, const std::string &key) {
    // cout << "delete start" << endl;
    message_t msg;
    msg.cmd = CMD_REMOVE;
    msg.key_len = key.size() + 1;
    msg.value_len = 0;
    // void *out = malloc(MESSAGE_SIZE + key->len);
    char *out = (char*)malloc(MESSAGE_SIZE + msg.key_len);
    // memset(out, 0, MESSAGE_SIZE + key.size());
    kvs_client_.context()->obuf_len = MESSAGE_SIZE + msg.key_len;
    memcpy(out, &msg, MESSAGE_SIZE);
    memcpy(out + MESSAGE_SIZE, key.data(), msg.key_len);
    kvs_client_.context()->obuf = out;
    return kvs_client_.handle_msg(CMD_REMOVE);
}

} //ycsbc