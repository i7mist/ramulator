from subprocess import call
import sys
import os

single_threaded = ["apache2.1B-rc", "compile-rc", "memcached.1B-rc", "bootup.1B-rc", "filecopy.1B-rc", "mysql.1B-rc", "clonevm_running-rc", "forkset-rc", "shell-rc"]

ramulator_bin = os.environ['RAMULATOR_HOME'] + "/ramulator"

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
  traces = [trace_dir + "/" + "trace." + t for t in workloads[workload_id]]
  cmd_items = [ramulator_bin, "--config", config, "--mode", "cpu", "--stats", output, "--rowclone-trace", "true", "--trace"] + traces
  print " ".join(cmd_items)
  call(cmd_items)
elif option == "single-threaded":
  trace = trace_dir + "/" + "trace." + single_threaded[workload_id]
  cmd_items = [ramulator_bin, "--config", config, "--mode", "cpu", "--stats", output, "--rowclone-trace", "true", "--trace", trace]
  print " ".join(cmd_items)
  call(cmd_items)
