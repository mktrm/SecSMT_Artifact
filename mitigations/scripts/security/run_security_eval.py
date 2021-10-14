
# %%
import config
from pathlib import Path
from itertools import combinations
from itertools import combinations_with_replacement
import logging
from time import sleep
import sys,os,re

# %%


# %%
for scheme in ["all_shared", "all_partitioned"]:
  for i in range(1,14):
        for j in range(1,14):
          gem5_base_cmd = f"""/u/mtaram/gem5-smt/build/X86/gem5.opt \
                --outdir=m5out/sec_results/out_{i}_{j}_{scheme}  \
                --stats-file=stats.txt  \
                /u/mtaram/gem5-smt/configs/example/se.py  \
                  -I 100000000 --smt\
                  --caches --l2cache  --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8\
                  --cpu-type=Skylake_3 --maxinsts_threadID=1 \
                  --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 """
          gem5_cmd = gem5_base_cmd + f"-c '/u/mtaram/SecSMT/agner/TestScripts/port/receiver_{i};/u/mtaram/SecSMT/agner/TestScripts/port/sender_{j}' "
          gem5_cmd += "-o 'thread0;thread1' "
          partitioning_args = (row["gem5_args"] for row in config.jobs if row["name"]==scheme)
          gem5_cmd += next(partitioning_args)
          print(gem5_cmd)
          res = config.nonblocking_out(f"srun {gem5_cmd}  &> /u/mtaram/gem5-smt/sec_results/results_{i}_{j}_{scheme}.log")
# %%
