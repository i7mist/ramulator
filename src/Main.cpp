#include "Processor.h"
#include "Controller.h"
#include "SpeedyController.h"
#include "Memory.h"
#include "DRAM.h"
#include "Statistics.h"
#include <cstdio>
#include <cstdlib>
#include <stdlib.h>
#include <functional>
#include <map>

/* Standards */
#include "Gem5Wrapper.h"
#include "DDR3.h"
#include "DDR4.h"
#include "DSARP.h"
#include "GDDR5.h"
#include "LPDDR3.h"
#include "LPDDR4.h"
#include "WideIO.h"
#include "WideIO2.h"
#include "HBM.h"
#include "SALP.h"
#include "ALDRAM.h"
#include "TLDRAM.h"

using namespace std;
using namespace ramulator;


#ifdef RAMULATOR_DRAMTRACE

int main (int argc, char** argv)
{
    int C = 1, R = 1;  // channels, ranks
    if (argc < 2) {
        printf("Usage: %s <dram-trace-file> [<channels> <ranks>]\n Example: %s dram.trace 1 1", argv[0], argv[0]);
        return 0;
    }
    if (argc >= 3) C = strtol(argv[2], NULL, 10);
    if (argc >= 4) R = strtol(argv[3], NULL, 10);

    /* initialize DRAM system */
    DDR3* ddr3 = new DDR3(DDR3::Org::DDR3_2Gb_x8, DDR3::Speed::DDR3_1600K);
    vector<Controller<DDR3>*> ctrls;
    for (int c = 0; c < C; c++) {
        DRAM<DDR3>* channel = new DRAM<DDR3>(ddr3, DDR3::Level::Channel);
        channel->id = c;
        for (int r = 0; r < R; r++)
            channel->insert(new DRAM<DDR3>(ddr3, DDR3::Level::Rank));

        Controller<DDR3>* ctrl = new Controller<DDR3>(channel);
        ctrls.push_back(ctrl);
    }
    Memory<DDR3, Controller> memory(ctrls);

    /* initialize DRAM trace */
    Trace trace(argv[1]);

    /* run simulation */
    bool stall = false, end = false;
    int reads = 0, writes = 0, clks = 0;
    long addr = 0;
    Request::Type type = Request::Type::READ;

    Request req(addr, type, [](Request& r){}, memory.stat->getcallback());

    while (!end || memory.pending_requests()){
        if (!end && !stall){
            end = !trace.get_request(addr, type);
        }

        if (!end){
            req.addr = addr;
            req.type = type;
            stall = !memory.send(req);
            if (!stall){
                if (type == Request::Type::READ) reads++;
                else if (type == Request::Type::WRITE) writes++;
            }
        }
        memory.tick();
        clks ++;
    }

    /* report statistics */

    double tCK = memory.clk_ns(), t = tCK * clks;
    double rbw = 64.0 * reads / 1024 / 1024 / 1024 / (t / 1e9);
    double wbw = 64.0 * writes / 1024 / 1024 / 1024 / (t / 1e9);

    printf("Simulation done %d clocks [%.3lfns], %d reads [%.3lf GB/s], %d writes [%.3lf GB/s], "
        "rowhit %d, rowconflict %d\n",
        clks, t, reads, rbw, writes, wbw, memory.stat->getstat("rowHits"),
        memory.stat->getstat("rowConflicts"));

    /* histogram of read latencies */
    // long total_latency = 0;
    // for (auto& kv : latencies){
    //     printf("[%4d]: %8d\n", kv.first, kv.second);
    //     total_latency += kv.first * kv.second;
    // }
    // printf("Avg latency: %.3lf ns\n", tCK * total_latency / reads);
    
    return 0;
}

#endif



#ifdef RAMULATOR_CPUTRACE

template <typename T>
vector<double> run_simulation(T *spec, std::vector<const char *> files,
    int chan, int rank, int cpu_tick, int mem_tick,
    std::shared_ptr<StatisticsBase>* stat)
{
    vector<Controller<T>*> ctrls;
    for (int c = 0; c < chan; c++){
        DRAM<T> *dram = new DRAM<T>(spec, T::Level::Channel);
        dram->id = c;
        for (int r = 0; r < rank; r++)
            dram->insert(new DRAM<T>(spec, T::Level::Rank));
        // The default scheduler is FRFCFS, default row policy is open-row.
        // Use MAX to keep the default settings.
        Controller<T> *ctrl = new Controller<T>(dram,
            SchedulerType::MAX, RowPolicyType::MAX);
        ctrls.push_back(ctrl);
    }
    Memory<T, Controller> memory(ctrls);
    (*stat) = memory.stat;
    auto send = bind(&Memory<T, Controller>::send, &memory, placeholders::_1);
    Processor proc(files, send, (*stat)->getcallback());
    for (long i = 0; ; i++) {
        // if (i % 100000000 == 0) printf("%ld clocks\n", i);
        proc.tick();
        if (i % cpu_tick == (cpu_tick - 1))
            for (int j = 0; j < mem_tick; j++)
                memory.tick();
        if (proc.finished() && memory.pending_requests() == 0)
            break;
    }
    return proc.ipcs;
}

void print_result(const char* spec_name,
    const vector<double>& ipc, const vector<double>& base_ipc,
    StatisticsBase* stat) {
    printf("%10s: ", spec_name);
    assert(ipc.size() == base_ipc.size());
    for (int i = 0 ; i < ipc.size(); ++i) {
      printf("%.5lf ", ipc[i]/base_ipc[i]);
    }
    printf("rowhit %d rowconflict %d\n",
        stat->getstat("rowHits"), stat->getstat("rowConflicts"));
}

int main(int argc, const char *argv[])
{
    if (argc < 2){
        printf("Usage: %s <cpu-trace-core1> <cpu-trace-core2>...\n"
            "Example: %s cpu.trace cpu.trace", argv[0], argv[0]);
        return 0;
    }
    std::shared_ptr<StatisticsBase> stat;
    std::vector<const char*> files(&argv[1], &argv[argc]);

    vector<double> baseIPC, IPC;
    DDR3* ddr3 = new DDR3(DDR3::Org::DDR3_8Gb_x8, DDR3::Speed::DDR3_1333H);
    baseIPC = run_simulation(ddr3, files, 1, 1, 4, 1, &stat);
    print_result("DDR3", baseIPC, baseIPC, stat.get());

    DDR4* ddr4 = new DDR4(DDR4::Org::DDR4_4Gb_x8, DDR4::Speed::DDR4_2400R);
    IPC = run_simulation(ddr4, files, 1, 1, 8, 3, &stat);
    print_result("DDR4", IPC, baseIPC, stat.get());

    SALP* salp8 = new SALP(SALP::Org::SALP_4Gb_x8, SALP::Speed::SALP_1600K, SALP::Type::MASA, 8);
    IPC = run_simulation(salp8, files, 1, 1, 4, 1, &stat);
    print_result("SALP", IPC, baseIPC, stat.get());

    LPDDR3* lpddr3 = new LPDDR3(LPDDR3::Org::LPDDR3_8Gb_x16, LPDDR3::Speed::LPDDR3_1600);
    IPC = run_simulation(lpddr3, files, 1, 1, 4, 1, &stat);
    print_result("LPDDR3", IPC, baseIPC, stat.get());

    // total cap: 2GB, 1/2 of others
    LPDDR4* lpddr4 = new LPDDR4(LPDDR4::Org::LPDDR4_8Gb_x16, LPDDR4::Speed::LPDDR4_2400);
    IPC = run_simulation(lpddr4, files, 2, 1, 8, 3, &stat);
    print_result("LPDDR4", IPC, baseIPC, stat.get());

    GDDR5* gddr5 = new GDDR5(GDDR5::Org::GDDR5_8Gb_x16, GDDR5::Speed::GDDR5_6000);
    IPC = run_simulation(gddr5, files, 1, 1, 2, 1, &stat); //6400 overclock
    print_result("GDDR5", IPC, baseIPC, stat.get());

    HBM* hbm = new HBM(HBM::Org::HBM_4Gb, HBM::Speed::HBM_1Gbps);
    IPC = run_simulation(hbm, files, 8, 1, 32, 5, &stat);
    print_result("HBM", IPC, baseIPC, stat.get());

    // total cap: 1GB, 1/4 of others
    WideIO* wio = new WideIO(WideIO::Org::WideIO_8Gb, WideIO::Speed::WideIO_266);
    IPC = run_simulation(wio, files, 4, 1, 12, 1, &stat);
    print_result("WideIO", IPC, baseIPC, stat.get());

    // total cap: 2GB, 1/2 of others
    WideIO2* wio2 = new WideIO2(WideIO2::Org::WideIO2_8Gb, WideIO2::Speed::WideIO2_1066);
    wio2->channel_width *= 2;
    IPC = run_simulation(wio2, files, 8, 1, 6, 1, &stat);
    print_result("WideIO2", IPC, baseIPC, stat.get());

    // Various refresh mechanisms
    DSARP* dsddr3_dsarp = new DSARP(DSARP::Org::DSARP_8Gb_x8,
        DSARP::Speed::DSARP_1333, DSARP::Type::DSARP, 8);
    IPC = run_simulation(dsddr3_dsarp, files, 1, 1, 4, 1, &stat);
    print_result("DSARP", IPC, baseIPC, stat.get());

    ALDRAM* aldram = new ALDRAM(ALDRAM::Org::ALDRAM_4Gb_x8, ALDRAM::Speed::ALDRAM_1600K);
    IPC = run_simulation(aldram, files, 1, 1, 4, 1, &stat);
    print_result("ALDRAM", IPC, baseIPC, stat.get());

    TLDRAM* tldram = new TLDRAM(TLDRAM::Org::TLDRAM_4Gb_x8, TLDRAM::Speed::TLDRAM_1600K, 16);
    IPC = run_simulation(tldram, files, 1, 1, 4, 1, &stat);
    print_result("TLDRAM", IPC, baseIPC, stat.get());

    return 0;
}
#endif /*test spec2006*/
