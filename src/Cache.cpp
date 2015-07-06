#include "Cache.h"

#ifndef DEBUG_CACHE
#define debug(...)
#else
#define debug(...) do { \
          debug("\033[36m[DEBUG] %s", __FUNCTION__); \
          debug(__VA_ARGS__); \
          debug("\033[0m\n"); \
      } while (0)
#endif

namespace ramulator
{

Cache::Cache(int size, int assoc, int block_size,
    Level level, std::function<bool(Request)> send_next,
    std::function<void(Line)> evict_next):
    level(level), size(size), assoc(assoc), block_size(block_size),
    send_next(send_next), evict_next(evict_next) {

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
  debug("Cache::Cache: index_offset %d\n", index_offset);
  debug("Cache::Cache: index_mask 0x%x\n", index_mask);
  debug("Cache::Cache: tag_offset %d\n", tag_offset);
}

bool Cache::send(Request req) {
  debug("Cache::send: level %d req.addr %lx req.type %d\n",
      int(level), req.addr, int(req.type));
  // Now it ignores the different latency of different level hit.
  std::vector<Line>::iterator line_it;

  if (is_hit(req.addr, &line_it)) {
    line_it->timestamp = clk;
    hit_list.push_back(make_pair(clk + latency[int(level)], req));

    debug("Cache::send: hit, update timestamp %ld\n", clk);
    debug("Cache::send: hit finish time%ld\n",
        clk + latency[int(level)]);

    if (level != Level::L3) {
      return send_next(req);
    }

  } else {
    auto victim = get_victim(Line(req.addr, get_tag(req.addr), clk));
    if (level != Level::L3) {
      if (victim.addr != -1) {
        evict_next(victim);

        debug("Cache::send: level %d miss evict\n", int(level));

      }
      return send_next(req);
    } else {
      if (victim.addr != -1) {
        Request write_req(victim.addr, Request::Type::WRITE,
            [](Request& req){});
        wait_list.push_back(make_pair(clk + latency[int(level)], write_req));

        debug("Cache::send: level %d miss evict\n", int(level));
        debug("Cache::send: inject one write request req.addr %lx,"
            " finish time %ld\n",
            req.addr, clk + latency[int(level)]);

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
  auto line_it = find_line(lines, get_tag(line.addr));
  assert(line_it != lines.end()); // check inclusive feature
  line_it->timestamp = line.timestamp;
}

bool Cache::is_hit(long addr, std::vector<Line>::iterator* pos) {

  debug("Cache::is_hit: addr %lx, index %x, tag %lx\n",
      addr, get_index(addr), get_tag(addr));

  auto it = cache_lines.find(get_index(addr));
  if (it == cache_lines.end()) {
    return false;
  } else {
    auto& lines = it->second;
    *pos = find_line(lines, get_tag(addr));
    return (*pos) != lines.end();
  }
}

void Cache::tick() {
  assert(level == Level::L3);

  debug("Cache::tick: clk %ld\n", clk);

  ++clk;
  // Sends ready waiting request to memory
  auto it = wait_list.begin();
  while (it != wait_list.end() && clk >= it->first) {
    if (!send_next(it->second)) {
      ++it;
    } else {

      debug("Cache::tick: complete req: addr %lx\n",
          (it->second).addr);

      it = wait_list.erase(it);
    }
  }
  // hit request callback
  it = hit_list.begin();
  while (it != hit_list.end()) {
    if (clk >= it->first) {
      it->second.callback(it->second);

      debug("Cache::tick: finish hit: addr %lx\n",
          (it->second).addr);

      it = hit_list.erase(it);
    } else {
      ++it;
    }
  }
}

} // namespace ramulator
