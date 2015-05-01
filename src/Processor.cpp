#include "Processor.h"
#include <cassert>

using namespace std;
using namespace ramulator;

Processor::Processor(vector<const char*> trace_list,
    function<bool(Request)> send_memory,
    bool early_exit)
    :ipcs(trace_list.size(), -1), early_exit(early_exit),
     cachesys(new CacheSystem(send_memory)),
     llc(l3_size, l3_assoc, l3_blocksz,
         mshr_per_bank * trace_list.size(),
         Cache::Level::L3, cachesys) {
  assert(cachesys != nullptr);
  int tracenum = trace_list.size();
  assert(tracenum > 0);
  printf("tracenum: %d\n", tracenum);
  for (int i = 0 ; i < tracenum ; ++i) {
    printf("trace_list[%d]: %s\n", i, trace_list[i]);
  }
  if (no_shared_cache) {
    for (int i = 0 ; i < tracenum ; ++i) {
      cores.emplace_back(
          i, trace_list[i], send_memory, nullptr);
    }
  } else {
    for (int i = 0 ; i < tracenum ; ++i) {
      cores.emplace_back(i, trace_list[i],
          std::bind(&Cache::send, &llc, std::placeholders::_1),
          &llc);
    }
  }
  for (int i = 0 ; i < tracenum ; ++i) {
    cores[i].callback = std::bind(&Core::receive, &cores[i],
        placeholders::_1);
  }
}

void Processor::tick() {
  if (!no_shared_cache) {
    cachesys->tick();
  }
  for (auto& core : cores) {
    core.tick();
  }
}

bool Processor::finished() {
  if (early_exit) {
    for (unsigned int i = 0 ; i < cores.size(); ++i) {
      if (cores[i].finished()) {
        for (unsigned int j = 0 ; j < cores.size() ; ++j) {
          ipc += cores[j].calc_ipc();
        }
        return true;
      }
    }
    return false;
  } else {
    for (unsigned int i = 0 ; i < cores.size(); ++i) {
      if (!cores[i].finished()) {
        return false;
      }
      if (ipcs[i] < 0) {
        ipcs[i] = cores[i].calc_ipc();
        ipc += ipcs[i];
      }
    }
    return true;
  }
}

Core::Core(int coreid, const char* trace_fname,
    function<bool(Request)> send_next, Cache* llc)
    : id(coreid), llc(llc), trace(trace_fname)
{
  // Build cache hierarchy
  if (no_core_caches) {
    send = send_next;
  } else {
    // L2 caches[0]
    caches.push_back(Cache(
        l2_size, l2_assoc, l2_blocksz, l2_mshr_num,
        Cache::Level::L2, llc->cachesys));
    // L1 caches[1]
    caches.push_back(Cache(
        l1_size, l1_assoc, l1_blocksz, l1_mshr_num,
        Cache::Level::L1, llc->cachesys));
    send = bind(&Cache::send, &caches[1], placeholders::_1);
    caches[0].concatlower(llc);
    caches[1].concatlower(&caches[0]);
  }
  if (no_core_caches) {
    more_reqs = trace.get_filtered_request(
        bubble_cnt, req_addr, req_type);
  } else {
    more_reqs = trace.get_unfiltered_request(
        bubble_cnt, req_addr, req_type);
  }
}


double Core::calc_ipc()
{
    printf("[%d]retired: %ld, clk, %ld\n", id, retired, clk);
    return (double) retired / clk;
}

void Core::tick()
{
    clk++;

    retired += window.retire();

    if (!more_reqs) return;
    // bubbles (non-memory operations)
    int inserted = 0;
    while (bubble_cnt > 0) {
        if (inserted == window.ipc) return;
        if (window.is_full()) return;

        window.insert(true, -1);
        inserted++;
        bubble_cnt--;
    }

    if (req_type == Request::Type::READ) {
        // read request
        if (inserted == window.ipc) return;
        if (window.is_full()) return;

        Request req(req_addr, req_type, callback);
        if (!send(req)) return;

        window.insert(false, req_addr);
    }
    else {
        // write request
        assert(req_type == Request::Type::WRITE);
        Request req(req_addr, req_type, callback);
        if (!send(req)) return;
    }

    if (no_core_caches) {
      more_reqs = trace.get_filtered_request(
          bubble_cnt, req_addr, req_type);
    } else {
      more_reqs = trace.get_unfiltered_request(
          bubble_cnt, req_addr, req_type);
    }
}

bool Core::finished()
{
    return !more_reqs && window.is_empty();
}
void Core::receive(Request& req)
{
    window.set_ready(req.addr, ~(l1_blocksz - 1l));
    if (llc != nullptr) {
      llc->callback(req);
    }
}


bool Window::is_full()
{
    return load == depth;
}

bool Window::is_empty()
{
    return load == 0;
}


void Window::insert(bool ready, long addr)
{
    assert(load <= depth);

    ready_list.at(head) = ready;
    addr_list.at(head) = addr;

    head = (head + 1) % depth;
    load++;
}


long Window::retire()
{
    assert(load <= depth);

    if (load == 0) return 0;

    int retired = 0;
    while (load > 0 && retired < ipc) {
        if (!ready_list.at(tail))
            break;

        tail = (tail + 1) % depth;
        load--;
        retired++;
    }

    return retired;
}


void Window::set_ready(long addr, int mask)
{
    if (load == 0) return;

    for (int i = 0; i < load; i++) {
        int index = (tail + i) % depth;
        if ((addr_list.at(index) & mask) != (addr & mask))
            continue;
        ready_list.at(index) = true;
    }
}



Trace::Trace(const char* trace_fname) : file(trace_fname)
{
    if (!file.good()) {
        std::cerr << "Bad trace file: " << trace_fname << std::endl;
        exit(1);
    }
}

bool Trace::get_unfiltered_request(long& bubble_cnt, long& req_addr, Request::Type& req_type)
{
    string line;
    getline(file, line);
    if (file.eof()) {
        return false;
    }
    size_t pos, end;
    bubble_cnt = std::stoul(line, &pos, 10);
    pos = line.find_first_not_of(' ', pos+1);
    req_addr = std::stoul(line.substr(pos), &end, 0);

    pos = line.find_first_not_of(' ', pos+end);

    if (pos == string::npos || line.substr(pos)[0] == 'R')
        req_type = Request::Type::READ;
    else if (line.substr(pos)[0] == 'W')
        req_type = Request::Type::WRITE;
    else assert(false);
    return true;
}

bool Trace::get_filtered_request(long& bubble_cnt, long& req_addr, Request::Type& req_type)
{
    static bool has_write = false;
    static long write_addr;
    static int line_num = 0;
    if (has_write){
        bubble_cnt = 0;
        req_addr = write_addr;
        req_type = Request::Type::WRITE;
        has_write = false;
        return true;
    }
    string line;
    getline(file, line);
    line_num ++;
    if (file.eof() || line.size() == 0) {
        file.clear();
        file.seekg(0);
        line_num = 0;
        return false;
    }

    size_t pos, end;
    bubble_cnt = std::stoul(line, &pos, 10);

    pos = line.find_first_not_of(' ', pos+1);
    req_addr = stoul(line.substr(pos), &end, 0);
    req_type = Request::Type::READ;

    pos = line.find_first_not_of(' ', pos+end);
    if (pos != string::npos){
        has_write = true;
        write_addr = stoul(line.substr(pos), NULL, 0);
    }
    return true;
}

bool Trace::get_dramtrace_request(long& req_addr, Request::Type& req_type)
{
    string line;
    getline(file, line);
    if (file.eof()) {
        return false;
    }
    size_t pos;
    req_addr = std::stoul(line, &pos, 16);

    pos = line.find_first_not_of(' ', pos+1);

    if (pos == string::npos || line.substr(pos)[0] == 'R')
        req_type = Request::Type::READ;
    else if (line.substr(pos)[0] == 'W')
        req_type = Request::Type::WRITE;
    else assert(false);
    return true;
}
