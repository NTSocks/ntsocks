#ifndef YCSB_C_KVS_DB_H_
#define YCSB_C_KVS_DB_H_

#include "core/db.h"
#include "core/timer.h"
#include <iostream>
#include <string>
#include "core/properties.h"
#include "simple_kvs/kvs_client.h"

using std::cout;
using std::endl;

namespace ycsbc {

class KVSDB : public DB {
  
private:
    KVSClient kvs_client_;
public:

  KVSDB(const char *host, int port):kvs_client_(host, port) {
    std::cout << "KVS_DB begins working" << endl;
  }

  int Read(const std::string &table, const std::string &key,
           const std::vector<std::string> *fields,
           std::vector<KVPair> &result);  

  int Scan(const std::string &table, const std::string &key,
           int len, const std::vector<std::string> *fields,
           std::vector<std::vector<KVPair>> &result);

  int Update(const std::string &table, const std::string &key,
             std::vector<KVPair> &values);

  int Insert(const std::string &table, const std::string &key,
             std::vector<KVPair> &values);

  int Delete(const std::string &table, const std::string &key);
};

}


#endif // YCSB_C_KVS_DB_H_