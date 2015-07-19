#ifndef __CACHE_H
#define __CACHE_H

#include "Request.h"
#include <cstdio>
#include <cassert>
#include <functional>
#include <list>
#include <map>
#include <queue>
#include <list>

namespace ramulator
{
class CacheSystem {
public:
  CacheSystem(std::function<bool(Request)> send_memory):
    send_memory(send_memory) {}
  std::list<std::pair<long, Request> > wait_list;
  std::list<std::pair<long, Request> > hit_list;

  std::function<bool(Request)> send_memory;

  long clk = 0;
  void tick();
};

class Cache {
public:
  enum class Level {
    L1,
    L2,
    L3,
    MAX
  } level;

  struct Line {
    bool lock; // When the lock is on, the value is not valid yet.
    long addr;
    long tag;
    Line(long addr, long tag):
        addr(addr), tag(tag), lock(true) {}
  };

  Cache(int size, int assoc, int block_size, int mshr_entry_num,
      Level level, std::shared_ptr<CacheSystem> cachesys);

  // L1, L2, L3 accumulated latencies
  int latency[int(Level::MAX)] = {4, 4 + 12, 4 + 12 + 31};

  std::shared_ptr<CacheSystem> cachesys;
  Cache* higher_cache;
  Cache* lower_cache;

  bool send(Request req);
  void evict(long addr);
  void invalidate(long addr);

  void concatlower(Cache* lower);

  void callback(Request& req);

private:
  int size;
  int assoc;
  int block_num;
  int index_mask;
  int block_size;
  int index_offset;
  int tag_offset;
  int mshr_entry_num;
  std::vector<std::pair<long, std::list<Line>::iterator>> mshr_entries;

  // tag, clk
  std::map<int, std::list<Line> > cache_lines;

  int calc_log2(int val) {
      int n = 0;
      while ((val >>= 1))
          n ++;
      return n;
  }

  int get_index(long addr) {
    return (addr >> index_offset) & index_mask;
  };

  long get_tag(long addr) {
    return (addr >> tag_offset);
  }

  long align(long addr) {
    return (addr & ~(block_size-1l));
  }

  // `line` is the new cache line that needs to be inserted to the
  // cache
  // Returns nullptr when no eviction happens or
  // the corresponding victim line on the other hand.
  bool need_eviction(const std::list<Line>& lines, long addr);

  bool is_hit(std::list<Line>& lines, long addr,
              std::list<Line>::iterator* pos_ptr);

  bool all_sets_locked(const std::list<Line>& lines) {
    if (lines.size() < assoc) {
      return false;
    }
    for (const auto& line : lines) {
      if (!line.lock) {
        return false;
      }
    }
    return true;
  }

  bool hit_mshr(long addr) {
    auto mshr_it =
        find_if(mshr_entries.begin(), mshr_entries.end(),
            [addr, this](std::pair<long, std::list<Line>::iterator>
                   mshr_entry) {
              return (align(mshr_entry.first) == align(addr));
            });
    if (mshr_it != mshr_entries.end()) {
      return true;
    }
    return false;
  }

  std::list<Line>::iterator allocate_line(std::list<Line>& lines,
      long addr);

  std::list<Line>& get_lines(long addr) {
    if (cache_lines.find(get_index(addr))
        == cache_lines.end()) {
      cache_lines.insert(make_pair(get_index(addr),
          std::list<Line>()));
    }
    return cache_lines[get_index(addr)];
  }
};

} // namespace ramulator

#endif /* __CACHE_H */
