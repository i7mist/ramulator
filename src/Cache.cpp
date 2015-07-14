#include "Cache.h"

#ifndef DEBUG_CACHE
#define debug(...)
#else
#define debug(...) do { \
          printf("\033[36m[DEBUG]"); \
          printf(__VA_ARGS__); \
          printf("\033[0m\n"); \
      } while (0)
#endif

namespace ramulator
{

Cache::Cache(int size, int assoc, int block_size,
    Level level, std::function<bool(Request)> send_memory):
    level(level), size(size), assoc(assoc), block_size(block_size),
    send_memory(send_memory), lower_cache(nullptr),
    higher_cache(nullptr) {

  debug("Cache::Cache: level %d size %d assoc %d block_size %d\n",
      int(level), size, assoc, block_size);
    // check size, block size and assoc are 2^N
    assert((size & (size - 1)) == 0);
    assert((block_size & (block_size - 1)) == 0);
    assert((assoc & (assoc - 1)) == 0);
    assert(size >= block_size);
    block_num = size / (block_size * assoc);
    index_mask = block_num - 1;
    index_offset = calc_log2(block_size);
    tag_offset = calc_log2(block_num) + index_offset;
  debug("Cache::Cache: index_offset %d", index_offset);
  debug("Cache::Cache: index_mask 0x%x", index_mask);
  debug("Cache::Cache: tag_offset %d", tag_offset);
}

bool Cache::send(Request req) {
  debug("Cache::send: level %d req.addr %lx req.type %d",
      int(level), req.addr, int(req.type));
  std::list<Line>::iterator line_it;

  if (is_hit(req.addr, &line_it)) {
    line_it->timestamp = clk;
    hit_list.push_back(make_pair(clk + latency[int(level)], req));

    debug("Cache::send: hit, update timestamp %ld", clk);
    debug("Cache::send: hit finish time %ld",
        clk + latency[int(level)]);

    return true;

  } else {
    debug("Cache::send: miss @level %d", level);
    auto victim = get_victim(Line(req.addr, get_tag(req.addr),
                                  clk));
    if (level == Level::L1) {
      assert(higher_cache == nullptr);
    }
    if (level == Level::L3) {
      assert(lower_cache == nullptr);
    }

    if (level != Level::L3) {
      if (victim.addr != -1) {
        assert(lower_cache != nullptr);
        lower_cache->evict(victim);

        if (higher_cache != nullptr) {
          higher_cache->invalidate(victim);
        }

        debug("Cache::send: level %d miss evict", int(level));

      }
      return lower_cache->send(req);
    } else {
      if (victim.addr != -1) {
        Request write_req(victim.addr, Request::Type::WRITE,
            [](Request& req){});
        wait_list.push_back(make_pair(clk + latency[int(level)],
                                      write_req));

        debug("Cache::send: level %d miss evict", int(level));
        debug("Cache::send: inject one write request req.addr %lx,"
            " finish time %ld",
            req.addr, clk + latency[int(level)]);

        if (higher_cache != nullptr) {
          higher_cache->invalidate(victim);
        }
      }
      wait_list.push_back(make_pair(clk + latency[int(level)], req));
      return true;
    }
  }
}

void Cache::evict(Line line) {
  Line newline(line.addr, get_tag(line.addr), line.timestamp);
  auto it = cache_lines.find(get_index(line.addr));
  assert(it != cache_lines.end()); // check inclusive feature
  auto& lines = it->second;
  auto line_it = find_if(lines.begin(), lines.end(),
      [&line, this](Line l){return (l.tag == get_tag(line.addr));});
  assert(line_it != lines.end()); // check inclusive feature
  line_it->timestamp = line.timestamp;
}

bool Cache::is_hit(long addr, std::list<Line>::iterator* pos) {

  debug("Cache::is_hit: addr %lx, index %x, tag %lx",
      addr, get_index(addr), get_tag(addr));

  auto it = cache_lines.find(get_index(addr));
  if (it == cache_lines.end()) {
    return false;
  } else {
    auto& lines = it->second;
    *pos = find_if(lines.begin(), lines.end(),
        [addr, this](Line l){return (l.tag == get_tag(addr));});
    bool hit = ((*pos) != lines.end());
    return hit;
  }
}

void Cache::invalidate(Line line) {
  auto it = cache_lines.find(get_index(line.addr));
  auto& lines = it->second;
  auto pos = find_if(lines.begin(), lines.end(),
      [&line, this](Line l){return (l.tag == get_tag(line.addr));});
  // If the line is in this level cache, then erase it from
  // the buffer.
  if (pos != lines.end()) {
    lines.erase(pos);
    if (higher_cache != nullptr) {
      higher_cache->invalidate(line);
    }
  }
}

void Cache::concatlower(Cache* lower) {
  lower_cache = lower;
  assert(lower != nullptr);
  lower->higher_cache = this;
};

void Cache::tick() {
//   debug("Cache::tick: clk %ld, level %d", clk, int(level));

  ++clk;
  // Sends ready waiting request to memory
  auto it = wait_list.begin();
  while (it != wait_list.end() && clk >= it->first) {
    if (!send_memory(it->second)) {
      ++it;
    } else {

      debug("Cache::tick: complete req: addr %lx",
          (it->second).addr);

      it = wait_list.erase(it);
    }
  }
  // hit request callback
  it = hit_list.begin();
  while (it != hit_list.end()) {
    if (clk >= it->first) {
      it->second.callback(it->second);

      debug("Cache::tick: finish hit: addr %lx",
          (it->second).addr);

      it = hit_list.erase(it);
    } else {
      ++it;
    }
  }
}

} // namespace ramulator
