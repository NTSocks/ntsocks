//
//  ycsbc.cc
//  YCSB-C
//
//

#include <cstring>
#include <string>
#include <iostream>
#include <vector>
#include <future>
#include "core/utils.h"
#include "core/timer.h"
#include "core/client.h"
#include "core/core_workload.h"
#include "db/db_factory.h"
#include <algorithm>

using namespace std;

double use_time[100001]; 
int error_num = 0;

void UsageMessage(const char *command);
bool StrStartWith(const char *str, const char *pre);
string ParseCommandLine(int argc, const char *argv[], utils::Properties &props);

int DelegateClient(ycsbc::Client client, const int num_ops, bool is_loading) {

  int oks = 0;
  for (int i = 0; i < num_ops; ++i) {
    if (is_loading) {
      oks += client.DoInsert();
    } else {
      oks += client.DoTransaction();
    }
  }

  return oks;
}

// each thread pins to one core
void pin_1thread_to_1core(){
    cpu_set_t cpuset;
    pthread_t thread;
    thread = pthread_self();
    CPU_ZERO(&cpuset);
    int s;
    CPU_SET(1, &cpuset);
    s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0)
      printf("pthread_setaffinity_np error\n");

}

int main(const int argc, const char *argv[]) {
  utils::Properties props;
  string file_name = ParseCommandLine(argc, argv, props);

  ycsbc::DB *db = ycsbc::DBFactory::CreateDB(props);

  if (!db) {
    cout << "Unknown database name " << props["dbname"] << endl;
    exit(0);
  }
  
  pin_1thread_to_1core();
  ycsbc::CoreWorkload wl;
  wl.Init(props);

  int sum = 0;
  db->Init();
  ycsbc::Client client(*db, wl);
  int total_ops = stoi(props[ycsbc::CoreWorkload::RECORD_COUNT_PROPERTY]);
  sum = DelegateClient(client , total_ops , true);  
  printf("# Loading records:\t%d\n", sum);          
  db->Close();

  // Peforms transactions
  total_ops = stoi(props[ycsbc::CoreWorkload::OPERATION_COUNT_PROPERTY]);
  db->Init();
  ycsbc::Client Client(*db, wl);
  sum = DelegateClient(Client , total_ops , false);  
  db->Close();
  printf("# OPERATION record:\t%d\n",sum);

  //for print all data
  // for(int i = 0; i < sum; i++){
  //   printf("%lf\n", use_time[i]*1000000);
  // }

  std::sort(use_time, use_time+sum);
  double all_time = 0;
  for(int i = 0; i < sum; i++){
    all_time += use_time[i];
  }

  //cerr << all_time << endl;

  double throughput = sum / all_time /1000;
  printf("ALL TIME = %lf\n", all_time);
  printf("THROUGHPUT = %lfKTPS\n",throughput);
  printf("AVERAGE = %lfus\n", all_time / sum * 1000000);
  printf("MEDIAN = %lfus\n", use_time[ ((int)(sum * 0.5) - 1) ] * 1000000);
  printf("99 = %lfus\n", use_time[ ((int)(sum * 0.99) - 1) ] * 1000000);
  printf("99.9 = %lfus\n", use_time[ ((int)(sum * 0.999) - 1) ] * 1000000);
  printf("99.99 = %lfus\n", use_time[ ((int)(sum * 0.9999) - 1) ] * 1000000);
  printf("error num %d", error_num);

}

string ParseCommandLine(int argc, const char *argv[], utils::Properties &props) {
  int argindex = 1;
  string filename;
  while (argindex < argc && StrStartWith(argv[argindex], "-")) {
    if (strcmp(argv[argindex], "-threads") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("threadcount", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-db") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("dbname", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-host") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("host", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-port") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("port", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-slaves") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("slaves", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-P") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      filename.assign(argv[argindex]);
      ifstream input(argv[argindex]);
      try {
        props.Load(input);
      } catch (const string &message) {
        cout << message << endl;
        exit(0);
      }
      input.close();
      argindex++;
    } else {
      cout << "Unknown option '" << argv[argindex] << "'" << endl;
      exit(0);
    }
  }

  if (argindex == 1 || argindex != argc) {
    UsageMessage(argv[0]);
    exit(0);
  }

  return filename;
}

void UsageMessage(const char *command) {
  cout << "Usage: " << command << " [options]" << endl;
  cout << "Options:" << endl;
  cout << "  -threads n: execute using n threads (default: 1)" << endl;
  cout << "  -db dbname: specify the name of the DB to use (default: basic)" << endl;
  cout << "  -P propertyfile: load properties from the given file. Multiple files can" << endl;
  cout << "                   be specified, and will be processed in the order specified" << endl;
}

inline bool StrStartWith(const char *str, const char *pre) {
  return strncmp(str, pre, strlen(pre)) == 0;
}

