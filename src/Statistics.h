#ifndef __STATISTICS_H
#define __STATISTICS_H

#include "Request.h"
#include <map>
#include <cassert>

namespace ramulator {

class StatisticsBase {
  public:
    virtual function<void(Request&)> getcallback() = 0;
    virtual int getstat(const std::string& statname) = 0;
};


template <typename T>
class Statistics : public StatisticsBase {
 public: 
  void operator () (const Request& req) {
    callback(req);
  }
  Statistics() {}
  Statistics(function<void(Request&)> callback):
      callback(callback) {}
  
  function<void(Request&)> callback = [this](Request& r){
    this->latencies[r.depart - r.arrive]++;
    if (r.type == Request::Type::READ || r.type == Request::Type::WRITE) {
      this->readreqs += (r.type == Request::Type::READ) ? 1 : 0;
      this->writereqs += (r.type == Request::Type::WRITE) ? 1 : 0;
      bool hit = true;
      bool conflict = false;
      for (int i = 0 ; i < r.cmds.size() ; ++i) {
        if (r.cmds[i].type == int(T::Command::ACT)) {
          hit = false;
        }
        else if (r.cmds[i].type == int(T::Command::PRE)) {
          conflict = true;
        }
      }
      this->read_rowhit += hit && r.type == Request::Type::READ ? 1 : 0;
      this->write_rowhit += hit && r.type == Request::Type::WRITE ? 1 : 0;
      this->rowconflict += conflict ? 1 : 0;
    }
  };

  int getstat (const std::string& statname) {
    if (statname == "readReqs") {
      return readreqs;
    }
    if (statname == "writeReqs") {
      return writereqs;
    }
    if (statname == "rowHits") {
      return read_rowhit + write_rowhit;
    }
    if (statname == "readRowHits") {
      return read_rowhit;
    }
    if (statname == "writeRowHits") {
      return write_rowhit;
    }
    if (statname == "rowConflicts") {
      return rowconflict;
    }
    else {
      printf("invalid statname %s\n", statname.c_str());
      assert(0);
    }
  }

  function<void(Request&)> getcallback() {
    return callback;
  }


 private:
  std::map<int, int> latencies;
  int readreqs = 0;
  int writereqs = 0;
  int read_rowhit = 0;
  int write_rowhit = 0;
  int rowconflict = 0;
};

} // namespace ramulator

#endif
