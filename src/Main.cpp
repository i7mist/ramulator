#include "Processor.h"
#include "Config.h"
#include "Controller.h"
#include "SpeedyController.h"
#include "Memory.h"
#include "DRAM.h"
#include "Statistics.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdlib.h>
#include <functional>
#include <map>
#include <boost/program_options.hpp>

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
namespace po = boost::program_options;

static int gcd(int u, int v) {
  if (v > u) {
    swap(u,v);
  }
  while (v != 0) {
    int r = u % v;
    u = v;
    v = r;
  }
  return u;
}

class OutstandingReqWindow {
 private:
  int inflight_req = 0;
  int inflight_limit = -1; // if inflight_limit == -1, the window is unlimited

 public:
  OutstandingReqWindow(int inflight_limit):inflight_limit(inflight_limit) {}
  bool is_unlimited() const {return (inflight_limit == -1);}
  bool is_full() const { return !is_unlimited() && (inflight_limit == inflight_req); }
  bool is_empty() const { return (inflight_req == 0);}
  void insert() {
    if (!is_unlimited()) {
      assert(!is_full());
      inflight_req++;
    }
  }
  void retire() {
    if (!is_unlimited()) {
      assert(!is_empty());
      inflight_req--;
    }
  }
};

template<typename T>
void run_dramtrace(const Config& configs, Memory<T, Controller>& memory, const string& tracename) {

    /* initialize DRAM trace */
    Trace trace(tracename);

    /* run simulation */
    bool stall = false, end = false;
    int reads = 0, writes = 0, clks = 0;
    long addr = 0;
    Request::Type type = Request::Type::READ;
    map<int, int> latencies;
    int inflight_limit = -1;
    if (configs.contains("inflight_limit")) {
      inflight_limit = configs.get_int_value("inflight_limit");
    }
    OutstandingReqWindow window(inflight_limit);
    auto read_complete =
        [&latencies, &window](Request& r) {
          latencies[r.depart - r.arrive]++;
          window.retire();
        };

    // TODO allow request streams coming from different cores
    Request req(addr, type, read_complete, 0);

    while (!end || memory.pending_requests()){
        if (!end && !stall){
            end = !trace.get_dramtrace_request(addr, type);
        }

        if (!end){
            req.addr = addr;
            req.type = type;
            if (window.is_full()) {
              stall = true;
            } else {
              stall = !memory.send(req);
            }
            if (!stall){
                if (type == Request::Type::READ) {
                  reads++;
                  window.insert(); // increment request counter. Write is posted.
                }
                else if (type == Request::Type::WRITE) writes++;
            }
        }
        memory.tick();
        clks ++;
        Stats::curTick++; // memory clock, global, for Statistics
    }
    // This a workaround for statistics set only initially lost in the end
    memory.finish();
    Stats::statlist.printall();

}

template <typename T>
void run_cputrace(const Config& configs, Memory<T, Controller>& memory, const std::vector<string>& files)
{
    // time unit is ps
    int cpu_tick = configs.get_cpu_tick();
    int mem_tick = memory.clk_ns() * 1000;
    int tick_gcd = gcd(cpu_tick, mem_tick);
    printf("tick_gcd: %d\n", tick_gcd);
    cpu_tick /= tick_gcd;
    printf("cpu_tick: %d\n", cpu_tick);
    mem_tick /= tick_gcd;
    printf("mem_tick: %d\n", mem_tick);
    long next_cpu_tick = cpu_tick - 1;
    long next_mem_tick = mem_tick - 1;

    auto send = bind(&Memory<T, Controller>::send, &memory, placeholders::_1);
    Processor proc(configs, files, send, memory);
    for (long i = 0; ; i++) {
      if (i == next_cpu_tick) {
        next_cpu_tick += cpu_tick;
        proc.tick();
        Stats::curTick++; // processor clock, global, for Statistics
        if (configs.calc_weighted_speedup()) {
          if (proc.has_reached_limit()) {
            break;
          }
        } else {
          if (configs.is_early_exit()) {
            if (proc.finished())
                break;
          } else {
            if (proc.finished() && (memory.pending_requests() == 0))
                break;
          }
        }
//         printf("cpu tick: %ld\n", i/1000);
      }
      if (i == next_mem_tick) {
        next_mem_tick += mem_tick;
        memory.tick();
//         printf("mem tick: %ld\n", i/1000);
      }
    }
    // This a workaround for statistics set only initially lost in the end
    memory.finish();
    Stats::statlist.printall();
}

template<typename T>
void start_run(const Config& configs, T* spec, const vector<string>& files) {
  // initiate controller and memory
  int C = configs.get_channels(), R = configs.get_ranks();
  // Check and Set channel, rank number
  spec->set_channel_number(C);
  spec->set_rank_number(R);
  std::vector<Controller<T>*> ctrls;
  for (int c = 0 ; c < C ; c++) {
    DRAM<T>* channel = new DRAM<T>(spec, T::Level::Channel);
    channel->id = c;
    channel->regStats("");
    Controller<T>* ctrl = new Controller<T>(configs, channel);
    ctrls.push_back(ctrl);
  }
  Memory<T, Controller> memory(configs, ctrls);

  assert(files.size() != 0);
  if (configs["trace_type"] == "CPU") {
    run_cputrace(configs, memory, files);
  } else if (configs["trace_type"] == "DRAM") {
    run_dramtrace(configs, memory, files[0]);
  }
}

int main(int argc, const char *argv[])
{
    po::options_description desc;
    desc.add_options()
      ("help", "print simple manual")
      ("stats", po::value<string>(), "path to output file.")
      ("config", po::value<string>(), "path to config file.")
      ("mode", po::value<string>(), "simulator running mode.")
      ("channel", po::value<string>(), "# of DRAM channel.")
      ("rank", po::value<string>(), "# of DRAM rank.")
      ("cache", po::value<string>(), "cache setting.")
      ("inflight-limit", po::value<string>(), "maximum in flight memory requests number.") // for DRAM alone mode
      ("trace", po::value<std::vector<string>>()->multitoken(), "a single or a list of file name(s) that are the traces to run.")
      ("print-cmd-trace", po::value<string>(), "whether print DRAM command to standard output.")
      ("cpu-frequency", po::value<string>(), "define CPU frequency.")
      ("translation", po::value<string>(), "translation mode, selected from: None, Random")
      ("org", po::value<string>(), "specify DRAM organization")
      ("rcd_thresh_bin0", po::value<string>(), "rcd_thresh_bin0")
      ("rcd_thresh_bin1", po::value<string>(), "rcd_thresh_bin1")
      ("rp_thresh_bin0", po::value<string>(), "rp_thresh_bin0")
      ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (argc < 2 || vm.count("help")) {
      cout << desc << "\n";
      return 1;
    }

    if (!vm.count("config")) {
      cout << "config file is required. (missing --config [configfile])" << endl;
      return 1;
    }
    Config configs(vm["config"].as<string>());
    if (vm.count("channel")) {
      configs.set("channel", vm["channel"].as<string>());
    }
    if (vm.count("rank")) {
      configs.set("rank", vm["rank"].as<string>());
    }
    if (vm.count("cache")) {
      configs.set("cache", vm["cache"].as<string>());
    }
    if (vm.count("print-cmd-trace")) {
      configs.set("print_cmd_trace", vm["print-cmd-trace"].as<string>());
    }
    if (vm.count("inflight-limit")) {
      configs.set("inflight_limit", vm["inflight-limit"].as<string>());
    }
    if (vm.count("cpu-frequency")) {
      configs.set("cpu_frequency", vm["cpu-frequency"].as<string>());
    }
    if (vm.count("translation")) {
      configs.set("translation", vm["translation"].as<string>());
    }
    if (vm.count("org")) {
      configs.set("org", vm["org"].as<string>());
    }
    if (vm.count("rcd_thresh_bin0")) {
      configs.set("rcd_thresh_bin0", vm["rcd_thresh_bin0"].as<string>());
    }
    if (vm.count("rcd_thresh_bin1")) {
      configs.set("rcd_thresh_bin1", vm["rcd_thresh_bin1"].as<string>());
    }
    if (vm.count("rp_thresh_bin0")) {
      configs.set("rp_thresh_bin0", vm["rp_thresh_bin0"].as<string>());
    }

    const std::string& standard = configs["standard"];
    assert(standard != "" || "DRAM standard should be specified.");

    if (vm.count("mode")) {
      const string& mode_str = vm["mode"].as<string>();
      if (mode_str == "cpu") {
        configs.add("trace_type", "CPU");
      } else if (mode_str == "dram") {
        configs.add("trace_type", "DRAM");
      } else {
        cout << "invalid trace type: " << mode_str << endl;
        return 1;
      }
    } else {
      cout << "mode is required. (missing --mode [cpu|dram])" << endl;
    }

    string stats_out;
    if (vm.count("stats")) {
      stats_out = vm["stats"].as<string>();
    } else {
      stats_out = standard + string(".stats");
    }
    Stats::statlist.output(stats_out);

    std::vector<string> files;
    if (vm.count("trace")) {
      files = vm["trace"].as<vector<string>>();
    } else {
      cout << "trace file name(s) is(are) required. (missing --trace [core1-trace core2-trace...])";
    }

    configs.set_core_num(files.size());

    if (standard == "DDR3") {
      if (configs["speed"].find("VLDRAM") != std::string::npos) {
        configs.set("VLDRAM", "true");
      }
      DDR3* ddr3 = new DDR3(configs["org"], configs["speed"]);
      start_run(configs, ddr3, files);
    } else {
      assert(false);
    }

    cout << "Simulation done. Statistics written to " << stats_out << endl ;

    return 0;
}
