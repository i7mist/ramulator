from subprocess import call
import sys

single_threaded = ["429.mcf", "433.milc", "434.zeusmp", "437.leslie3d", "450.soplex", "459.GemsFDTD", "462.libquantum", "470.lbm", "471.omnetpp", "482.sphinx3", "483.xalancbmk", "stream-add", "stream-copy", "stream-scale", "stream-triad", "tpcc64", "tpch17", "tpch2", "ubench_r080_w025_rl000_wl000"]
workloads = [["437.leslie3d", "462.libquantum", "482.sphinx3", "437.leslie3d", "437.leslie3d", "462.libquantum", "433.milc", "stream-scale"],
          ["stream-add", "437.leslie3d", "462.libquantum", "stream-add", "tpcc64", "stream-add", "stream-copy", "tpcc64"],
          ["tpch2", "stream-scale", "stream-copy", "482.sphinx3", "stream-copy", "462.libquantum", "tpcc64", "stream-copy"],
          ["450.soplex", "433.milc", "tpch17", "stream-copy", "437.leslie3d", "462.libquantum", "433.milc", "470.lbm"],
          ["433.milc", "stream-scale", "stream-triad", "tpch2", "470.lbm", "tpch17", "stream-triad", "stream-copy"],
          ["stream-scale", "450.soplex", "482.sphinx3", "tpcc64", "437.leslie3d", "stream-triad", "stream-scale", "482.sphinx3"],
          ["462.libquantum", "482.sphinx3", "470.lbm", "stream-scale", "stream-triad", "stream-add", "470.lbm", "437.leslie3d"],
          ["470.lbm", "tpch17", "stream-triad", "stream-triad", "tpcc64", "stream-add", "437.leslie3d", "462.libquantum"],
          ["tpch17", "482.sphinx3", "482.sphinx3", "470.lbm", "tpch17", "stream-scale", "stream-triad", "462.libquantum"],
          ["stream-scale", "433.milc", "482.sphinx3", "470.lbm", "462.libquantum", "stream-scale", "462.libquantum", "437.leslie3d"],
          ["stream-scale", "tpcc64", "stream-add", "tpcc64", "stream-add", "tpch2", "433.milc", "470.lbm"],
          ["462.libquantum", "450.soplex", "482.sphinx3", "stream-add", "462.libquantum", "470.lbm", "462.libquantum", "462.libquantum"],
          ["462.libquantum", "tpch2", "tpcc64", "450.soplex", "437.leslie3d", "stream-copy", "470.lbm", "tpch2"],
          ["stream-scale", "stream-add", "stream-triad", "stream-copy", "stream-scale", "437.leslie3d", "482.sphinx3", "stream-copy"],
          ["437.leslie3d", "450.soplex", "462.libquantum", "stream-copy", "437.leslie3d", "tpch17", "tpch17", "462.libquantum"],
          ["437.leslie3d", "450.soplex", "tpch2", "450.soplex", "tpch2", "stream-triad", "tpch2", "482.sphinx3"],
          ["433.milc", "470.lbm", "tpcc64", "482.sphinx3", "stream-triad", "tpch17", "462.libquantum", "433.milc"],
          ["tpch2", "450.soplex", "470.lbm", "tpch2", "stream-copy", "tpcc64", "tpch2", "470.lbm"],
          ["482.sphinx3", "stream-triad", "462.libquantum", "433.milc", "tpcc64", "tpcc64", "437.leslie3d", "462.libquantum"],
          ["437.leslie3d", "tpch2", "462.libquantum", "stream-add", "stream-scale", "stream-triad", "stream-add", "stream-copy"],
          ["462.libquantum", "434.zeusmp", "437.leslie3d", "ubench_r080_w025_rl000_wl000", "tpch2", "429.mcf", "434.zeusmp", "tpch17"],
          ["482.sphinx3", "tpch2", "459.GemsFDTD", "ubench_r080_w025_rl000_wl000", "tpcc64", "462.libquantum", "ubench_r080_w025_rl000_wl000", "462.libquantum"],
          ["471.omnetpp", "433.milc", "tpcc64", "483.xalancbmk", "429.mcf", "ubench_r080_w025_rl000_wl000", "483.xalancbmk", "470.lbm"],
          ["459.GemsFDTD", "ubench_r080_w025_rl000_wl000", "stream-copy", "tpch17", "ubench_r080_w025_rl000_wl000", "433.milc", "450.soplex", "470.lbm"],
          ["stream-scale", "433.milc", "470.lbm", "tpcc64", "tpcc64", "483.xalancbmk", "483.xalancbmk", "471.omnetpp"],
          ["stream-add", "462.libquantum", "tpch17", "stream-add", "429.mcf", "ubench_r080_w025_rl000_wl000", "450.soplex", "459.GemsFDTD"],
          ["stream-triad", "stream-add", "471.omnetpp", "450.soplex", "471.omnetpp", "433.milc", "tpch2", "437.leslie3d"],
          ["429.mcf", "tpch2", "462.libquantum", "stream-scale", "ubench_r080_w025_rl000_wl000", "482.sphinx3", "433.milc", "483.xalancbmk"],
          ["470.lbm", "ubench_r080_w025_rl000_wl000", "stream-add", "471.omnetpp", "tpcc64", "434.zeusmp", "482.sphinx3", "tpcc64"],
          ["462.libquantum", "450.soplex", "stream-add", "429.mcf", "482.sphinx3", "stream-add", "stream-copy", "ubench_r080_w025_rl000_wl000"],
          ["471.omnetpp", "450.soplex", "450.soplex", "tpch17", "482.sphinx3", "tpcc64", "tpch17", "459.GemsFDTD"],
          ["483.xalancbmk", "stream-copy", "tpcc64", "471.omnetpp", "tpch2", "462.libquantum", "tpch17", "459.GemsFDTD"],
          ["437.leslie3d", "429.mcf", "470.lbm", "471.omnetpp", "482.sphinx3", "470.lbm", "434.zeusmp", "437.leslie3d"],
          ["437.leslie3d", "462.libquantum", "459.GemsFDTD", "483.xalancbmk", "434.zeusmp", "stream-triad", "tpch2", "429.mcf"],
          ["tpcc64", "482.sphinx3", "459.GemsFDTD", "482.sphinx3", "tpcc64", "483.xalancbmk", "462.libquantum", "stream-triad"],
          ["stream-triad", "450.soplex", "433.milc", "tpch17", "tpch17", "tpcc64", "470.lbm", "stream-copy"],
          ["tpch17", "437.leslie3d", "stream-triad", "471.omnetpp", "482.sphinx3", "429.mcf", "ubench_r080_w025_rl000_wl000", "437.leslie3d"],
          ["462.libquantum", "429.mcf", "stream-triad", "437.leslie3d", "483.xalancbmk", "471.omnetpp", "stream-scale", "tpcc64"],
          ["459.GemsFDTD", "471.omnetpp", "437.leslie3d", "470.lbm", "tpcc64", "tpch2", "433.milc", "450.soplex"],
          ["482.sphinx3", "tpch17", "482.sphinx3", "429.mcf", "470.lbm", "482.sphinx3", "462.libquantum", "459.GemsFDTD"]]

ramulator_bin = "/Users/tianshi/backup-lol/tianshi-Workspace/ramulator/ramulator"

if not len(sys.argv) == 7:
  print "python run_spec.py trace_dir output_dir config_dir workloadid DRAM [multicore|single-threaded]"
  sys.exit(0)

option = sys.argv[6]
DRAM = sys.argv[5]
workload_id = int(sys.argv[4])
config_dir = sys.argv[3]
output_dir = sys.argv[2]
trace_dir = sys.argv[1]

if option == "multicore":
  output_dir += "/workload" + str(workload_id)
  print output_dir
else:
  output_dir += "/" + single_threaded[workload_id]
  print output_dir

call(["mkdir", "-p", output_dir])

output = output_dir + "/" + DRAM + ".stats"
config = config_dir + "/" + DRAM + "-config.cfg"

if option == "multicore":
  traces = [trace_dir + "/" + t for t in workloads[workload_id]]

  print " ".join([ramulator_bin, "--config", config, "--mode", "cpu", "--stats", output, "--trace"] + traces)
  call([ramulator_bin, "--config", config, "--mode", "cpu", "--stats", output, "--trace"] + traces)
elif option == "single-threaded":
  trace = trace_dir + "/" + single_threaded[workload_id]
  print " ".join([ramulator_bin, "--config", config, "--mode", "cpu", "--stats", output, "--trace", trace])
  call([ramulator_bin, "--config", config, "--mode", "cpu", "--stats", output, "--trace", trace])
