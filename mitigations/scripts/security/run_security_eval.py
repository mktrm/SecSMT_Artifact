
# %%

from pathlib import Path
from itertools import combinations
from itertools import combinations_with_replacement
import logging
from time import sleep
import sys,os,re

# %%
sys.path.append(str(Path(__file__).parent.parent.resolve()))
from spec17 import config
# %%
for scheme in ["all_shared", "all_partitioned", "all_adaptive", "all_asymmetric__"]:
  for i in range(1,2):
        for j in range(1,14):
          gem5_base_cmd = f"""{config.GEM5_BIN} \
                --outdir={config.ROOT}/results/sec_results/m5out/out_{i}_{j}_{scheme}  \
                --stats-file=stats.txt  \
                {config.GEM5_CONFIG}  \
                  -I 100000000 --smt\
                  --caches --l2cache  --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8\
                  --cpu-type=Skylake_3 --maxinsts_threadID=0 \
                  --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 """
          gem5_cmd = gem5_base_cmd + f"-c '{config.ROOT}/workloads/security/receiver_{i};{config.ROOT}/workloads/security/sender_{j}' "
          gem5_cmd += "-o 'thread0;thread1' "
          partitioning_args = (row["gem5_args"] for row in config.jobs if row["name"]==scheme)
          gem5_cmd += next(partitioning_args)
          gem5_cmd = gem5_cmd.replace("Interval=100000", "Interval=100000000")

          print("gem5_cmnd:", gem5_cmd)
          os.makedirs(f"{config.ROOT}/results/sec_results/", exist_ok=True)
          print (sys.argv)
          if len(sys.argv) > 1 and  sys.argv[1]=="slurm":
            res = config.nonblocking_out(f"srun --exclude=lynx[08-12] {gem5_cmd}  &> {config.ROOT}/results/sec_results/results_{i}_{j}_{scheme}.log")
          else:
            res = config.nonblocking_out(f"{gem5_cmd}  &> {config.ROOT}/results/sec_results/results_{i}_{j}_{scheme}.log")

# %%
