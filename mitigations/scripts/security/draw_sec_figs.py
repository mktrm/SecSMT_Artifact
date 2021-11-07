
# %%

from pathlib import Path
from itertools import combinations
from itertools import combinations_with_replacement
import logging
from time import sleep
import sys,os,re
import seaborn as sns
import pandas as pd
import matplotlib.pyplot as plt

sys.path.append(str(Path(__file__).parent.parent.resolve()))
from spec17 import config
# %%
def set_style():
    #plt.style.use(['seaborn-white', 'seaborn-paper'])
    plt.rcParams["font.family"] = "serif"
    plt.rcParams['pdf.fonttype'] = 42
    plt.rcParams['ps.fonttype'] = 42
    sns.set(rc={'figure.figsize':(11.7,8.27),"font.size":20, "font.family": "serif", 
                "axes.titlesize":20,"axes.labelsize":20, "ytick.labelsize":20, 
                 "xtick.labelsize":16 , 'legend.fontsize':18, 'legend.title_fontsize': 18}, style="white")

set_style()
# %%
instructions = ["movb", 
                "add", 
                "xor", 
                "bt",
                "bts",
                "movq",
                "mov ymm",
                "lea",
                "nop8",
                "load",
                "cpuid",
                "lfence",
                "nop"]
for i in range(1,2):
  time = []
  schm = []
  indx = []
  index = 0 
  if i == 7:
    continue
  for schemek,scheme in enumerate(["all_shared", "all_partitioned", "all_adaptive", "all_asymmetric__"]):
    index = 0
    for j in range(1,14):
      results_file_path = f"{config.ROOT}/results/sec_results/results_{i}_{j}_{scheme}.log"
      first_measurement_skiped = 0
      with open (results_file_path, 'r') as f:
        for line in f:
          
          m = re.match(".*thread0\,\s*(\S*)\,", line)
          if m:
            if first_measurement_skiped > 4:
              indx += [index]
              time += [int(m.group(1))]
              if schemek == 0:
                correct_scheme = "Shared"
              elif schemek == 1:
                correct_scheme = "Partitioned"
              elif schemek == 2:
                correct_scheme = "Adaptive"
              elif schemek == 3:
                correct_scheme = "Asymmetric"
              schm += [correct_scheme]
              index += 1
            else:
              first_measurement_skiped += 1
  df = pd.DataFrame(list(zip(indx, time, schm)),
              columns =["Index", 'Cycles', 'Method'])
  print(df)
  fig, ax = plt.subplots(figsize=(15,3))
  ax = sns.lineplot(data = df,
                x= "Index",
                y = "Cycles",
                hue = "Method",

              )
  ax.text( -55 , df["Cycles"].max()*1.05, "T1:" , fontsize=18)
  for bar in range(1,13):
    text = instructions[bar-1] if bar < 7 else instructions[bar]
    ax.text( (bar-1)*95+10, df["Cycles"].max()*1.05, text , fontsize=18)
    ax.axvspan(95*(bar-1), 95*bar, 
              color="grey", alpha=0.05*(bar%2)+0.07)
  #ax.margins(x=0)
  plt.title("T0: "+instructions[i-1], loc="left")
  ax.legend(loc=("upper center"), bbox_to_anchor=(0.55, 1.28), ncol=4)
  #ax.get_xaxis().set_ticks([])
  plt.xlabel('Measurement Index', fontsize = 22)
  plt.ylabel('Cycles', fontsize = 20)
  plt.ylim(top=1.15*df["Cycles"].max())
  plt.xticks(fontsize = 16)
  plt.yticks(fontsize = 18)
  plt.savefig(f"fig-sec-{i}.pdf", bbox_inches='tight')
      
# %%
