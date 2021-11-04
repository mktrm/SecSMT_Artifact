from matplotlib import pyplot as plt
import seaborn as sns
import numpy as np
import pandas as pd
import pathlib
from glob import glob

ROOT = str(pathlib.Path(__file__).parent.parent.parent.resolve())

columns = ["system.cpu.cpi_total", "system.cpu.cpi::0", "system.cpu.cpi::1"]

def set_style():
    #plt.style.use(['seaborn-white', 'seaborn-paper'])
    plt.rcParams["font.family"] = "serif"
    plt.rcParams['pdf.fonttype'] = 42
    plt.rcParams['ps.fonttype'] = 42
    sns.set(rc={'figure.figsize':(11.7,8.27),"font.size":20, "font.family": "serif", 
                "axes.titlesize":20,"axes.labelsize":20, "ytick.labelsize":20, 
                 "xtick.labelsize":16 , 'legend.fontsize':18, 'legend.title_fontsize': 18}, style="white")
    
def nametable(x):
    if x == "-aes-gcm":
        return "AES GCM"
    if x == "-rsa":
        return "RSA"
    if x in ["shared", "adaptive", "asymmetric", "partitioned"]:
        x = x[0].upper() + x[1:]
        return x
    if x is "asymmetric":
        return "Asymmetric SMT"
    if x is "adaptive":
        return "Adaptive Partitioning"
    if x is "partitioned":
        return "Staticly Partitioned"
    if x is "shared":
        return "Dynammically Shared"
    else:
        return x[:-3] # remove js suffix
records = []

print (ROOT+"/results/*/stat-file.txt")
for fn in glob(ROOT+"/results/*.js/stat-file.txt"):

    # print(fn)
    descriptor, suffix = fn.split("/")[-2:]
    arch, cryptobench, jsbench = descriptor.split("__")
    arch = nametable(arch)
    cryptobench = nametable(cryptobench)
    jsbench = nametable(jsbench)

    # print(f"arch: {arch}, cryptobench: {cryptobench}, jsbench: {jsbench}")


    d = {}
    with open(fn, 'r') as f:
        for line in f:
            if line == "\n":
                # print("skipping empty")
                continue
            if "---------- End Simulation Statistics   ----------" in line:
                # print("skipping last line")
                continue
            if "---------- Begin Simulation Statistics ----------" in line:
                # print("skipping first line")
                continue
            arr = line.split()
            k = arr[0]
            v = arr[1]
            d[k] = v
    dd = {} # filtered dict


    record = {
        "arch": arch,
        "cryptobench": cryptobench,
        "jsbench": jsbench,
    }
    for k in columns:
        record[k] = float(d[k])
    records.append(record)

df = pd.DataFrame.from_records(records)

#%% 
print (df)
for index, row in df.iterrows():
    if row["arch"] == "Shared":
        continue
    dyn_row =  df[ (df["arch"] == "Shared") & (df["jsbench"] == row["jsbench"]) & (df["cryptobench"] == row["cryptobench"]) ] 
    print ("---start---")
    df["system.cpu.cpi_total"][index] = (float(dyn_row["system.cpu.cpi::1"])/float(row["system.cpu.cpi::1"])+float(dyn_row["system.cpu.cpi::0"])/float(row["system.cpu.cpi::0"]))/2
    print ( float(dyn_row["system.cpu.cpi_total"]) )
    print ("---end---")
for index, row in df.iterrows():
    if row["arch"] == "Shared":
        df["system.cpu.cpi_total"][index] = 1
archs = set(df.arch)

cryptobenchs = set(df.cryptobench)

jsbenchs=set(df.jsbench)

#%%

df = df.rename(columns={'system.cpu.cpi_total': 'Speedup over Shared', 'cryptobench': 'Trusted Thread'})

df = df.replace("Partitioned", "Partitioned")
df = df.replace("Adaptive", "Adaptive")
df = df.replace("Shared", "Shared")
df = df.replace("Asymmetric", "Asymmetric")
df = df.replace("controlflow-recursive", "recursive")

#%%

tidy = df
with pd.option_context('display.max_rows', None, 'display.max_columns', None):  # more options can be specified also
    print(df)

#%%

set_style()
g = sns.catplot(
    col='Trusted Thread',
    x='jsbench',
    hue_order = ["Shared", "Partitioned", "Adaptive", "Asymmetric"],
    order = ["string-base64", "math-cordic", "3d-raytrace", "recursive", "regexp-dna"],
    hue='arch',
    y='Speedup over Shared',
    data=tidy, kind="bar",
    palette=("Purples"),linewidth=1, legend=False, aspect=.95)
g.set_xticklabels(rotation=90)

g.fig.tight_layout()
plt.legend(loc=("upper right"),
           bbox_to_anchor=(1.05, 1.40),
           ncol=4,
           prop={'size': 15},
            title="",
)

ges = g.axes.flatten()
for gg in ges:
    gg.set_xlabel("")
    for i, bar in enumerate(gg.patches):
        bar.set_edgecolor('black')
        gg.annotate(str("%.2f"%bar.get_height()), 
                    (bar.get_x() + 0.02, 
                    bar.get_height() - 0.22), 
                    rotation=90, fontsize=10)
plt.savefig("browserlike.pdf", bbox_inches='tight')

