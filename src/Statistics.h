#ifndef __STATISTICS_H
#define __STATISTICS_H

#include "Request.h"

namespace ramulator {

template <typename T>
class Statistics {
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
      this->rowhit += hit ? 1 : 0;
      this->rowconflict += conflict ? 1 : 0;
    }
  };

  int get_rowhit() {return rowhit;}
  int get_rowconflict() {return rowconflict;}
 private:
  std::map<int, int> latencies;
  int rowhit = 0;
  int rowconflict = 0;
};

}

#endif
