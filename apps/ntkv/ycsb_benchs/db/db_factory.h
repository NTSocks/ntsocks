#ifndef YCSB_C_DB_FACTORY_H_
#define YCSB_C_DB_FACTORY_H_

#include "core/db.h"
#include "core/properties.h"

namespace ycsbc {

class DBFactory {
 public:
  static DB* CreateDB(utils::Properties &props);
};

} // ycsbc

#endif // YCSB_C_DB_FACTORY_H_

