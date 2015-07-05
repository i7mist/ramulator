#ifndef __PROCESSOR_H
#define __PROCESSOR_H

#include "Cache.h"
#include "Request.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <ctype.h>
#include <functional>

namespace ramulator
{

class Trace {
public:
    Trace(const char* trace_fname);
    // trace file format 1:
    // [# of bubbles(non-mem instructions)] [read address(dec or hex)] <optional: write address(evicted cacheline)>
    bool get_request(long& bubble_cnt, long& req_addr, Request::Type& req_type);
    // trace file format 2:
    // [address(hex)] [R/W]
    bool get_request(long& req_addr, Request::Type& req_type);

private:
    std::ifstream file;
};


class Window {
public:
    int ipc = 4;
    int depth = 128;

    Window() : ready_list(depth), addr_list(depth, -1) {}
    bool is_full();
    bool is_empty();
    void insert(bool ready, long addr);
    long retire();
    void set_ready(long addr);

private:
    int load = 0;
    int head = 0;
    int tail = 0;
    std::vector<bool> ready_list;
    std::vector<long> addr_list;
};


class Core {
public:
    long clk = 0;
    long retired = 0;
    int id = 0;
    function<bool(Request)> send;

    Core(int coreid, const char* trace_fname,
        function<bool(Request)> send_next,
        function<void(Cache::Line)> evict_next);
    void tick();
    void receive(Request& req);
    double calc_ipc();
    bool finished();
    function<void(Request&)> callback;

    bool no_l1l2 = false;
    vector<Cache> caches;
    int l1_size = 1 << 15;
    int l1_assoc = 1 << 3;
    int l1_blocksz = 1 << 6;

    int l2_size = 1 << 18;
    int l2_assoc = 1 << 3;
    int l2_blocksz = 1 << 6;

private:
    Trace trace;
    Window window;

    long bubble_cnt;
    long req_addr;
    Request::Type req_type;
    bool more_reqs;
};

class Processor {
public:
    Processor(vector<const char*> trace_list,
        function<bool(Request)> send, bool early_exit);
    void tick();
    bool finished();

    std::vector<Core> cores;
    std::vector<double> ipcs;
    double ipc = 0;

    // When early_exit is true, the simulation exits when
    // the earliest trace finishes.
    bool early_exit;

    bool no_shared_cache = false;

    int l3_size = 1 << 23;
    int l3_assoc = 1 << 3;
    int l3_blocksz = 1 << 6;

    Cache llc;
};

}
#endif /* __PROCESSOR_H */
