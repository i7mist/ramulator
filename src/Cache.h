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
    long timestamp;
    Line(long addr, long tag, long timestamp):
        addr(addr), tag(tag), timestamp(timestamp){}
  };

  Cache(int size, int assoc, int block_size,
      Level level, std::function<bool(Request)> send_next);

  // L1, L2, L3 accumulated latencies
  int latency[int(Level::MAX)] = {4, 4 + 12, 4 + 12 + 31};

  Cache* higher_cache;
  Cache* lower_cache;

  std::function<bool(Request)> send_memory;
  bool send(Request req);
  void evict(Line line);
  void invalidate(Line line);

  void concatlower(Cache* lower);

  void tick();

private:
  int size;
  int assoc;
  int block_num;
  int index_mask;
  int block_size;
  int index_offset;
  int tag_offset;
  long clk = 0;

  // tag, clk
  std::map<int, std::list<Line> > cache_lines;

  int calc_log2(int val) {
      int n = 0;
      while ((val >>= 1))
          n ++;
      return n;
  }

  std::list<std::pair<long, Request> > wait_list;
  std::list<std::pair<long, Request> > hit_list;

  int get_index(long addr) {
    return (addr >> index_offset) & index_mask;
  };

  long get_tag(long addr) {
    return (addr >> tag_offset);
  }

  // line is the new cache line that needs to be inserted to the
  // cache, return Line(-1, -1, -1) when no eviction happens or
  // the corresponding victim line on the other hand.
  Line get_victim(Line line) {
    // get victim and insert new element
    auto it = cache_lines.find(get_index(line.addr));
    if (it == cache_lines.end()) {
      std::list<Line> init_line(1, line);
      cache_lines.insert(make_pair(get_index(line.addr), init_line));
      return Line(-1, -1, -1);
    } else {
      auto& lines = it->second;
      if (find_if(lines.begin(), lines.end(),
          [&line, this](Line l){return (get_tag(line.addr) == l.tag);}) != lines.end()) {
        return Line(-1, -1, -1);
      } else {
        if (lines.size() < assoc) {
          lines.push_back(line);
          return Line(-1, -1, -1);
        } else {
          int lru_timestamp = line.timestamp;
          auto victim_it = lines.begin();
          for (auto it = lines.begin() ;
               it != lines.end() ; ++it) {
            if (it->timestamp < lru_timestamp) {
              lru_timestamp = it->timestamp;
              victim_it = it;
            }
          }
          Line victim = *victim_it;
          lines.erase(victim_it);
          lines.push_back(line);
          return victim;
        }
      }
    }
  }

  bool is_hit(long addr, std::list<Line>::iterator* line_it);

};
} // namespace ramulator

#endif /* __CACHE_H */
