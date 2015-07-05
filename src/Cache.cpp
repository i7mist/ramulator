#include "Cache.h"

namespace ramulator
{

Cache::Cache(int size, int assoc, int block_size,
    Level level, std::function<int(Request)> send_next,
    std::function<void(Line)> evict_next):
    level(level), size(size), assoc(assoc), block_size(block_size),
    send_next(send_next), evict_next(evict_next) {

    // check size, block size and assoc are 2^N
    assert((size & (size - 1)) == 0);
    assert((block_size & (block_size - 1)) == 0);
    assert((assoc & (assoc - 1)) == 0);
    assert(size >= block_size);
    block_num = size / (block_size * assoc);
    index_mask = block_num - 1;
    index_offset = calc_log2(block_size);
    tag_offset = calc_log2(block_num) + index_offset;
}

int Cache::send(Request req) {
  // Now it ignores the different latency of different level hit.
  std::vector<Line>::iterator line_it;
  if (is_hit(req.addr, &line_it)) {
    line_it->timestamp = clk;
    return int(level);
  } else {
    auto victim = get_victim(Line(req.addr, get_tag(req.addr), clk));
    if (level != Level::L3) {
      if (victim.addr != -1) {
        evict_next(victim);
      }
      return send_next(req);
    } else {
      if (victim.addr != -1) {
        Request write_req(victim.addr, Request::Type::WRITE, NULL);
        wait_list.push_back(make_pair(clk + latency, write_req));
      }
      wait_list.push_back(make_pair(clk + latency, req));
      return 0;
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
  ++clk;
  auto it = wait_list.begin();
  while (it != wait_list.end() && it->first >= clk) {
    if (!send_next(it->second)) {
      ++it;
      continue;
    } else {
      it = wait_list.erase(it);
    }
  }
}

} // namespace ramulator
