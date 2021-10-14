#%%
from pathlib import Path
from itertools import combinations
from itertools import combinations_with_replacement
import itertools
import logging
import sys,os,re
import os.path
from os import path
import matplotlib
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import seaborn as sns
import config
import altair as alt
from scipy import stats 

print("Python version:\n{}\n".format(sys.version))
print("matplotlib version: {}".format(matplotlib.__version__))
print("pandas version: {}".format(pd.__version__))
print("numpy version: {}".format(np.__version__))
print("seaborn version: {}".format(sns.__version__))


#%%
def get_joint_name (bench_a, bench_b):
    return  f"{bench_a['name']}_{bench_b['name']}"

def set_style():
    #plt.style.use(['seaborn-white', 'seaborn-paper'])
    plt.rcParams["font.family"] = "serif"
    plt.rcParams['pdf.fonttype'] = 42
    plt.rcParams['ps.fonttype'] = 42
    sns.set(rc={'figure.figsize':(11.7,8.27),"font.size":20, "font.family": "serif", 
                "axes.titlesize":20,"axes.labelsize":20, "ytick.labelsize":20, 
                 "xtick.labelsize":16 , 'legend.fontsize':18, 'legend.title_fontsize': 18}, style="white")

def geo_mean(iterable):
    a = np.array(iterable)
    return a.prod()**(1.0/len(a))

def parse_gem5_stats (stat_file, element):
    if not path.exists(stat_file):
        return 0
    m = False
    with open (stat_file, 'r') as f: 
        for line in f: 
            m = re.match(element + "\s*(\S*)", line)
            if m: 
                return m.group(1)
#%%
    
def get_job_folder2 (bench_a, bench_b, _resource, _interval, _limit, first_tid, mult=""):
    joint_name = get_joint_name(bench_a, bench_b)
    _out_folder = f"{config.RUN_DIR}/adaptive/{joint_name}_{first_tid}/{_resource}_{_interval}_{_limit}_{mult}"
    return _out_folder
#%%
def read_adaptive_sweep():
    df = pd.DataFrame(columns = ["Benchmark", "Interval", "Limit", "Resource", "Mult", "Execution Time Improvement"])
    for i,j in combinations(config.spec17, 2):
        if 'type' in i and 'type' in j:
            first_tid = 0
            base_line_fold = get_job_folder2(i, j, 0, 0, 0, first_tid)
            base_line_fold  = base_line_fold + "/stats.txt"
            for res in config.adaptive_resources:
                resource = res['name']

                if res['type'] == "statefull":
                    for limit in config.adaptive_limits_statefull:
                        interval = 100000
                        out_folder = get_job_folder2(i, j, resource, interval, limit, first_tid)
                        err_file_path = out_folder + "/stats.txt"
                        print (base_line_fold)
                        try:
                            numCycles = parse_gem5_stats(err_file_path, "system.switch_cpus.cpi_total")
                            base = parse_gem5_stats(base_line_fold, "system.switch_cpus.cpi_total")
                            if numCycles is not None and base is not None:
                                print (float(base)/float(numCycles)) 
                                if first_tid == 1:
                                    df.loc[len(df)] = [get_joint_name(i,j), interval, limit, resource, 0, float(base)/float(numCycles)]
                                else:
                                    df.loc[len(df)] = [get_joint_name(j,i), interval, limit, resource, 0, float(base)/float(numCycles)]
                        except: 
                            print (err_file_path)


                if res['type'] == "stateless":
                    for limit in config.adaptive_limits_stateless:
                        for mult in config.adaptive_mults_stateless:
                            interval = 100000
                            out_folder = get_job_folder2(i, j, resource, interval, limit, first_tid, mult)
                            err_file_path = out_folder + "/stats.txt"
                            print (base_line_fold)
                            try:
                                numCycles = parse_gem5_stats(err_file_path, "system.switch_cpus.cpi_total")
                                base = parse_gem5_stats(base_line_fold, "system.switch_cpus.cpi_total")
                                if numCycles is not None and base is not None:
                                    print (float(base)/float(numCycles)) 
                                    if first_tid == 1:
                                        df.loc[len(df)] = [get_joint_name(i,j), interval, limit, resource, mult, float(base)/float(numCycles)]
                                    else:
                                        df.loc[len(df)] = [get_joint_name(j,i), interval, limit, resource, mult, float(base)/float(numCycles)]
                            except: 
                                print (err_file_path)

                            
    return df
# %%
df2 = read_adaptive_sweep()
print (df2)
# %%
def draw_stateful(df2, resource):
    df = df2[df2["Resource"] == resource]

    avgs = df.groupby(['Limit']).mean().reset_index()
    print (avgs)
    avgs["Benchmark"] = "Mean"
    df = pd.concat([df,avgs], axis=0, ignore_index=True, sort=False)

    # fig, ax = plt.subplots(figsize=(15,3))
    set_style()
    ax = sns.catplot(x="Benchmark", y="Execution Time Improvement", hue="Limit", data=df, kind="bar", 
                    palette=("Purples"), height=6, aspect=1.2, legend=False)
    ax.set_xticklabels(rotation=90)
    ax.fig.tight_layout()

    plt.legend(loc=("upper center"),
            bbox_to_anchor=(.50, 1.40),
            ncol=5,
            prop={'size': 15},
            title="Adaptation Limit",
    )
    ax.axes.flatten()[0].set_ylim(df["Execution Time Improvement"].min()-0.02, df["Execution Time Improvement"].max()+0.02)
    ax.axes.flatten()[0].set_ylabel('Speedup \nover Shared', fontsize = 20)
    axes = ax.axes.flatten()
    for ax in axes:
        ax.set_xlabel("")
        for i, bar in enumerate(ax.patches):
            bar.set_edgecolor('black')

    plt.savefig("fig-adaptive-{}.pdf".format(resource), bbox_inches='tight')
# %%
def draw_stateless(df2, resource):
    df = df2[df2["Resource"] == resource]
    avgs = df.groupby(['Limit','Mult']).mean().reset_index()
    print (avgs)
    avgs["Benchmark"] = "Mean"
    df = pd.concat([df,avgs], axis=0, ignore_index=True, sort=False)

    set_style()
    ax = sns.catplot(x="Benchmark", y="Execution Time Improvement", hue="Limit", col="Mult", data=df, kind="bar", 
                    palette=("Purples"),  height=6, aspect=0.9, legend=False)
    ax.set_xticklabels(rotation=90)
    ax.fig.tight_layout()
    plt.legend(loc=("upper center"),
            bbox_to_anchor=(-.5, 1.49),
            ncol=5,
            prop={'size': 20},
            title="Adaptation Limit",
    
    )
    ax.axes.flatten()[0].set_ylim(df["Execution Time Improvement"].min()-0.02, df["Execution Time Improvement"].max()+0.02)
    ax.axes.flatten()[0].set_ylabel('Speedup \nover Shared', fontsize = 20)

    axes = ax.axes.flatten()
    for ax in axes:
        ax.set_xlabel("")
        for i, bar in enumerate(ax.patches):
            bar.set_edgecolor('black')

    plt.savefig("fig-adaptive-{}.pdf".format(resource), bbox_inches='tight')

#%%
draw_stateful(df2, "SQ" )
# %%
draw_stateful(df2, "LQ" )
# %%
draw_stateful(df2, "Cache" )
# %%
draw_stateful(df2, "BTB" )
# %%
draw_stateful(df2, "PhysReg" )
# %%
draw_stateful(df2, "IQ" )
# %%
draw_stateful(df2, "ROB" )

#%%
draw_stateless(df2, "Issue")
# %%
draw_stateless(df2, "Fetch")

#%%
draw_stateless(df2, "Commit")# %%

# %%


# %%

# %%
