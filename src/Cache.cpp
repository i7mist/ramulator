#include "Cache.h"

#ifndef DEBUG_CACHE
#define debug(...)
#else
#define debug(...) do { \
          printf("\033[36m[DEBUG] %s ", __FUNCTION__); \
          printf(__VA_ARGS__); \
          printf("\033[0m\n"); \
      } while (0)
#endif

namespace ramulator
{

Cache::Cache(int size, int assoc, int block_size,
    int mshr_entry_num, Level level,
    std::shared_ptr<CacheSystem> cachesys):
    level(level), size(size), assoc(assoc), block_size(block_size),
    mshr_entry_num(mshr_entry_num), cachesys(cachesys),
    lower_cache(nullptr), higher_cache(nullptr) {

  debug("level %d size %d assoc %d block_size %d\n",
      int(level), size, assoc, block_size);

  // Check size, block size and assoc are 2^N
  assert((size & (size - 1)) == 0);
  assert((block_size & (block_size - 1)) == 0);
  assert((assoc & (assoc - 1)) == 0);
  assert(size >= block_size);

  // Initialize cache configuration
  block_num = size / (block_size * assoc);
  index_mask = block_num - 1;
  index_offset = calc_log2(block_size);
  tag_offset = calc_log2(block_num) + index_offset;

  debug("index_offset %d", index_offset);
  debug("index_mask 0x%x", index_mask);
  debug("tag_offset %d", tag_offset);
}

bool Cache::send(Request req) {
  debug("level %d req.addr %lx req.type %d, index %d, tag %ld",
      int(level), req.addr, int(req.type), get_index(req.addr),
      get_tag(req.addr));

  // If there isn't a set, create it.
  auto& lines = get_lines(req.addr);
  std::list<Line>::iterator line;

  if (is_hit(lines, req.addr, &line)) {
    lines.push_back(Line(req.addr, get_tag(req.addr), false,
        line->dirty || (req.type == Request::Type::WRITE)));
    lines.erase(line);
    cachesys->hit_list.push_back(
        make_pair(cachesys->clk + latency[int(level)], req));

    debug("hit, update timestamp %ld", cachesys->clk);
    debug("hit finish time %ld",
        cachesys->clk + latency[int(level)]);

    return true;

  } else {
    debug("miss @level %d", level);

    // The dirty bit will be set if this is a write request and @L1
    bool dirty = (req.type == Request::Type::WRITE);

    // Modify the type of the request to lower level
    if (req.type == Request::Type::WRITE) {
      req.type = Request::Type::READ;
    }

    // Look it up in MSHR entries
    assert(req.type == Request::Type::READ);
    auto mshr = hit_mshr(req.addr);
    if (mshr != mshr_entries.end()) {
      debug("hit mshr");
      mshr->second->dirty = dirty || mshr->second->dirty;
      return true;
    }

    // All requests come to this stage will be READ, so they
    // should be recorded in MSHR entries.
    if (mshr_entries.size() == mshr_entry_num) {
      // When no MSHR entries available, the miss request
      // is stalling.
      debug("no mshr entry available");
      return false;
    }

    // Check whether there is a line available
    if (all_sets_locked(lines)) {
      return false;
    }

    auto newline = allocate_line(lines, req.addr);
    newline->dirty = dirty;

    // Add to MSHR entries
    mshr_entries.push_back(make_pair(req.addr, newline));

    // Send the request to next level;
    if (level != Level::L3) {
      lower_cache->send(req);
    } else {
      cachesys->wait_list.push_back(
          make_pair(cachesys->clk + latency[int(level)], req));
    }
    return true;
  }
}

void Cache::evictline(long addr, bool dirty) {

  auto it = cache_lines.find(get_index(addr));
  assert(it != cache_lines.end()); // check inclusive cache
  auto& lines = it->second;
  auto line = find_if(lines.begin(), lines.end(),
      [addr, this](Line l){return (l.tag == get_tag(addr));});

  // Update LRU queue. The dirty bit will be set if the dirty
  // bit inherited from higher level(s) is set.
  lines.push_back(Line(addr, get_tag(addr), false,
      dirty || line->dirty));
  lines.erase(line);
}

std::pair<long, bool> Cache::invalidate(long addr) {
  long delay = latency_each[int(level)];
  bool dirty = false;

  auto it = cache_lines.find(get_index(addr));
  auto& lines = it->second;
  auto line = find_if(lines.begin(), lines.end(),
      [addr, this](Line l){return (l.tag == get_tag(addr));});

  // If the line is in this level cache, then erase it from
  // the buffer.
  if (line != lines.end() && !line->lock) {
    debug("invalidate %lx @ level %d", addr, int(level));
    lines.erase(line);
  } else {
    // If it's not in current level, then no need to go up.
    return make_pair(delay, false);
  }

  if (higher_cache != nullptr) {
    auto result = higher_cache->invalidate(addr);
    if (result.second) {
      delay += result.first * 2;
    } else {
      delay += result.first;
    }
    dirty = line->dirty || result.second;
  } else {
    dirty = line->dirty;
  }
  return make_pair(delay, dirty);
}


void Cache::evict(std::list<Line>* lines,
    std::list<Line>::iterator victim) {
  debug("level %d miss evict victim %lx", int(level), victim->addr);

  long addr = victim->addr;
  long invalidate_time = 0;
  bool dirty = victim->dirty;

  // First invalidate the victim line in higher level.
  if (higher_cache != nullptr) {
    auto result = higher_cache->invalidate(addr);
    invalidate_time = result.first + (result.second ? latency_each[int(level)] : 0);
    dirty = result.second || victim->dirty;
  }

  debug("invalidate delay: %ld, dirty: %s", invalidate_time,
      dirty ? "true" : "false");

  if (level != Level::L3) {
    // L1 or L2 eviction
    assert(lower_cache != nullptr);
    lower_cache->evictline(addr, dirty);
  } else {
    // L3 eviction
    if (dirty) {
      Request write_req(addr, Request::Type::WRITE);
      cachesys->wait_list.push_back(make_pair(
          cachesys->clk + invalidate_time + latency[int(level)],
          write_req));

      debug("inject one write request to memory system "
          "addr %lx, invalidate time %ld, issue time %ld",
          write_req.addr, invalidate_time,
          cachesys->clk + invalidate_time + latency[int(level)]);
    }
  }

  lines->erase(victim);
}

std::list<Cache::Line>::iterator Cache::allocate_line(
    std::list<Line>& lines, long addr) {
  // See if an eviction is needed
  if (need_eviction(lines, addr)) {
    // Get victim.
    // The first one might still be locked due to reorder in MC
    auto victim = find_if(lines.begin(), lines.end(),
        [](Line line) {
          return !line.lock;
        });
    evict(&lines, victim);
  }

  // Allocate newline, with lock bit on and dirty bit off
  lines.push_back(Line(addr, get_tag(addr)));
  auto last_element = lines.end();
  --last_element;
  return last_element;
}

bool Cache::is_hit(std::list<Line>& lines, long addr,
    std::list<Line>::iterator* pos_ptr) {
  auto pos = find_if(lines.begin(), lines.end(),
      [addr, this](Line l){return (l.tag == get_tag(addr));});
  *pos_ptr = pos;
  if (pos == lines.end()) {
    return false;
  }
  return !pos->lock;
}

void Cache::concatlower(Cache* lower) {
  lower_cache = lower;
  assert(lower != nullptr);
  lower->higher_cache = this;
};

bool Cache::need_eviction(const std::list<Line>& lines, long addr) {
  if (find_if(lines.begin(), lines.end(),
      [addr, this](Line l){
        return (get_tag(addr) == l.tag);})
      != lines.end()) {
    // Due to MSHR, the program can't reach here. Just for checking
    assert(false);
  } else {
    if (lines.size() < assoc) {
      return false;
    } else {
      return true;
    }
  }
}

void Cache::callback(Request& req) {
  debug("level %d", int(level));

  auto it = find_if(mshr_entries.begin(), mshr_entries.end(),
      [&req, this](std::pair<long, std::list<Line>::iterator> mshr_entry) {
        return (align(mshr_entry.first) == align(req.addr));
      });

  if (it != mshr_entries.end()) {
    it->second->lock = false;
    mshr_entries.erase(it);
  }

  if (higher_cache != nullptr) {
    higher_cache->callback(req);
  }
}

void CacheSystem::tick() {
  debug("clk %ld", clk);

  ++clk;

  // Sends ready waiting request to memory
  auto it = wait_list.begin();
  while (it != wait_list.end() && clk >= it->first) {
    if (!send_memory(it->second)) {
      ++it;
    } else {

      debug("complete req: addr %lx", (it->second).addr);

      it = wait_list.erase(it);
    }
  }

  // hit request callback
  it = hit_list.begin();
  while (it != hit_list.end()) {
    if (clk >= it->first) {
      it->second.callback(it->second);

      debug("finish hit: addr %lx", (it->second).addr);

      it = hit_list.erase(it);
    } else {
      ++it;
    }
  }
}

} // namespace ramulator
