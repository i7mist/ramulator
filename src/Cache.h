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

  // wait_list contains miss requests with their latencies in
  // cache. When this latency is met, the send_memory function
  // will be called to send the request to the memory system.
  std::list<std::pair<long, Request> > wait_list;

  // hit_list contains hit requests with their latencies in cache.
  // callback function will be called when this latency is met and
  // set the instruction status to ready in processor's window.
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
    long addr;
    long tag;
    bool lock; // When the lock is on, the value is not valid yet.
    bool dirty;
    Line(long addr, long tag):
        addr(addr), tag(tag), lock(true), dirty(false) {}
    Line(long addr, long tag, bool lock, bool dirty):
        addr(addr), tag(tag), lock(lock), dirty(dirty) {}
  };

  Cache(int size, int assoc, int block_size, int mshr_entry_num,
      Level level, std::shared_ptr<CacheSystem> cachesys);

  // L1, L2, L3 accumulated latencies
  int latency[int(Level::MAX)] = {4, 4 + 12, 4 + 12 + 31};
  int latency_each[int(Level::MAX)] = {4, 12, 31};

  std::shared_ptr<CacheSystem> cachesys;
  Cache* higher_cache;
  Cache* lower_cache;

  bool send(Request req);

  void concatlower(Cache* lower);

  void callback(Request& req);

protected:

  int size;
  int assoc;
  int block_num;
  int index_mask;
  int block_size;
  int index_offset;
  int tag_offset;
  int mshr_entry_num;
  std::vector<std::pair<long, std::list<Line>::iterator>> mshr_entries;

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

  // Align the address to cache line size
  long align(long addr) {
    return (addr & ~(block_size-1l));
  }

  // Evict the cache line from higher level to this level.
  // Pass the dirty bit and update LRU queue.
  void evictline(long addr, bool dirty);

  // Invalidate the line from this level to higher levels
  // The return value is a pair. The first element is invalidation
  // latency, and the second is wether the value has new version
  // in higher level and this level.
  std::pair<long, bool> invalidate(long addr);

  // Evict the victim from current set of lines.
  // First do invalidation, then call evictline(L1 or L2) or send
  // a write request to memory(L3) when dirty bit is on.
  void evict(std::list<Line>* lines,
      std::list<Line>::iterator victim);

  // First test whether need eviction, if so, do eviction by
  // calling evict function. Then allocate a new line and return
  // the iterator points to it.
  std::list<Line>::iterator allocate_line(
      std::list<Line>& lines, long addr);

  // Check whether the set to hold addr has space or eviction is
  // needed.
  bool need_eviction(const std::list<Line>& lines, long addr);

  // Check whether this addr is hit and fill in the pos_ptr with
  // the iterator to the hit line or lines.end()
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

  std::vector<std::pair<long, std::list<Line>::iterator>>::iterator
  hit_mshr(long addr) {
    auto mshr_it =
        find_if(mshr_entries.begin(), mshr_entries.end(),
            [addr, this](std::pair<long, std::list<Line>::iterator>
                   mshr_entry) {
              return (align(mshr_entry.first) == align(addr));
            });
    return mshr_it;
  }

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
