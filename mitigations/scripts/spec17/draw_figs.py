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
# import altair as alt
from scipy import stats 

print("Python version:\n{}\n".format(sys.version))
print("matplotlib version: {}".format(matplotlib.__version__))
print("pandas version: {}".format(pd.__version__))
print("numpy version: {}".format(np.__version__))
print("seaborn version: {}".format(sns.__version__))


#%%
def get_joint_name (bench_a, bench_b):
    return  f"{bench_a['name']}_{bench_b['name']}"

def get_job_folder (bench_a, bench_b, job, tid):
    joint_name = get_joint_name(bench_a, bench_b)
    out_folder = f"{config.RUN_DIR}/{joint_name}/{job['name']}{tid}"
    return out_folder

def parse_gem5_stats (stat_file, element):
    if not path.exists(stat_file):
        return 0
    m = False
    with open (stat_file, 'r') as f: 
        for line in f: 
            m = re.match(element + "\s*(\S*)", line)
            if m: 
                return m.group(1)
def set_style():
    plt.rcParams["font.family"] = "serif"
    plt.rcParams['pdf.fonttype'] = 42
    plt.rcParams['ps.fonttype'] = 42
    sns.set(rc={'figure.figsize':(11.7,8.27),"font.size":20, "font.family": "serif", 
                "axes.titlesize":20,"axes.labelsize":20, "ytick.labelsize":20, 
                 "xtick.labelsize":22 , 'legend.fontsize':22, 'legend.title_fontsize': 18}, style="white")


#%%
# go through all the jobs and benches
def read_all_stats(structure=".*", query="system.switch_cpus.numCycles", norm=True):
    time = dict()
    total = dict()
    num_jobs = 0

    for job in config.jobs:
        if not re.search(structure, job['structure']):
            continue 
        num_jobs += 1

    for i in config.spec17:
        time[i['name']] = dict()
        for job in config.jobs:
            if not re.search(structure, job['structure']):
                continue
            time[i['name']][job['name']] = 0
            total[i['name']] = 0
    for i,j in combinations_with_replacement(config.spec17, 2):
        valid_jobs = 0
        for job in config.jobs:
            if not re.search(structure, job['structure']):
                continue 
            for tid in range(0,2):
                out_folder = get_job_folder(i, j, job, tid)
                stats = out_folder + "/stats.txt"
                numCycles = parse_gem5_stats(stats, "system.switch_cpus.cpi_total")
                numCycles = parse_gem5_stats(stats, query)
                if numCycles:
                    valid_jobs += 1

        #if we have the results for all the jobs of this bench
        if valid_jobs == num_jobs*2:
            for tid in range(0,2):
                out_folder = get_job_folder(i, j, config.jobs[0], tid)
                stats = out_folder + "/stats.txt"
                base = parse_gem5_stats(stats, query)
                if i['name'] == j['name']:
                    continue
                for job in config.jobs:
                    if not re.search(structure, job['structure']):
                        continue

                    out_folder = get_job_folder(i, j, job, tid)
                    stats = out_folder + "/stats.txt"
                    numCycles = parse_gem5_stats(stats, query)

                    if tid == 1:
                        if norm:
                            time[i['name']][job['name']] += float(numCycles)/float(base)
                        else:
                            time[i['name']][job['name']] += float(numCycles)

                    else:
                        if norm:
                            time[j['name']][job['name']] += float(numCycles)/float(base)
                        else:
                            time[j['name']][job['name']] += float(numCycles)
                if tid== 1:
                    total[i['name']] += 1
                else:
                    total[j['name']] += 1

    for i in time.keys():
        for j in time[i].keys():
          time[i][j] /= (total[i])
    return time

#%%
time = read_all_stats("all", query="system.switch_cpus.ipc_total",norm=True)
df2 = pd.DataFrame.from_dict(time)
#%%
df = df2.rename(index = {'all_asymmetric__':'Asymmetric'})
df = df.rename(index = {'all_shared':'Shared'})
df = df.rename(index = {'all_partitioned':'Partitioned'})
df = df.rename(index = {'all_adaptive': 'Adaptive'})
df = df.rename(index = {'issue_mux':'Multiplexing Issue BW'})
df = df.rename(columns = {'exchange': 'xchg', 'deepsjeng':'deep', 'povray':'ray', 'fotonik': 'foton'})
df.reset_index(inplace=True)
df = df.rename(columns = {'index':'Method'})
df['GMean'] = stats.gmean(df.iloc[:, 1:], axis=1) 
df['GMean\nno lbm'] = stats.gmean(df.drop(columns = {'lbm'}).iloc[:, 1:], axis=1) 
df = pd.melt(df, id_vars="Method")
patterns = [ "\\" , "/" , "x" , "o" , "\\" , "/", "o", "O", ".", "*" ]
set_style()
fig, ax = plt.subplots(figsize=(18,2.5))
ax = sns.barplot(data = df,
    x="variable",
    y="value",
    hue="Method",
    palette="Purples", 
    linewidth=1,
    saturation=.8
)
num_locations = len(df['variable'].unique())
hatches = itertools.cycle(patterns)
for i, bar in enumerate(ax.patches):
    if i % num_locations == 0:
        hatch = next(hatches)
    bar.set_edgecolor('black')
    text_x = bar.get_x() + 0.008
    text_y = bar.get_height() + 0.08
    if (i % num_locations != num_locations-1) and (i % num_locations != num_locations - 2):
        ax.annotate(str("%.2f"%bar.get_height()), 
                        (text_x, text_y), 
                        rotation=90, fontsize=11)
    else:         
        ax.annotate(str("%.2f"%bar.get_height()), 
                    (text_x, text_y), 
                    rotation=90, weight='bold', fontsize=12) 
    
ax.legend(loc=("upper center"),
           bbox_to_anchor=(.5, 1.29),
           ncol=5,
           prop={'size': 16},
)
ax.set_yticks(np.arange(0,1.51,0.5))
plt.xlabel('', fontsize = 22)
plt.ylabel('Speedup \nover Shared', fontsize = 20)
plt.ylim((0,1.25*df['value'].max()))
plt.xticks(fontsize = 16)
plt.yticks(fontsize = 20)
plt.savefig("fig-mitigation-all.pdf", bbox_inches='tight')


# #%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# time = read_all_stats("Issue", query = 'system.switch_cpus.ipc_total')
# df = pd.DataFrame.from_dict(time)
# df= df.reindex(index=df.index[::-1])
# df = df.rename(index = {'Issue_asymmetric':'Asymmetric'})
# df = df.rename(index = {'Issue_adaptive':'Adaptive'})
# df = df.rename(index = {'Issue_partitioned':'Multiplexing FUs'})
# df = df.rename(index = {'Issue_mux':'Multiplexing Dispatch BW'})
# df.reset_index(inplace=True)
# df = df.rename(columns = {'index':'Method'})
# df['GMean'] = stats.gmean(df.iloc[:, 1:], axis=1) 
# df = pd.melt(df, id_vars="Method")


# patterns = [ "\\" , "/" , "x" , "o" , "\\" , "/", "o", "O", ".", "*" ]
# set_style()
# fig, ax = plt.subplots(figsize=(15,5))
# ax = sns.barplot(data = df,
#     x="variable",
#     y="value",
#     hue="Method",
#     palette="Purples", #bone_r
#     linewidth=1,
#     saturation=.8
# )


# num_locations = len(df['variable'].unique())
# hatches = itertools.cycle(patterns)
# for i, bar in enumerate(ax.patches):
#     if i % num_locations == 0:
#         hatch = next(hatches)
#     bar.set_edgecolor('black')


# ax.legend(loc=("upper center"),
#            bbox_to_anchor=(0.5, 1.15),
#            ncol=4,
#            prop={'size': 17},
# )
# ax.set_yticks(np.arange(0,1.1,0.04))
# plt.xlabel('', fontsize = 22)
# plt.ylabel('Speedup over Shared FUs', fontsize = 20)
# for tick in ax.get_xticklabels():
#     tick.set_rotation(45)
# plt.ylim((0.76,1.01))
# plt.grid(axis='y')
# plt.xticks(fontsize = 18)
# plt.yticks(fontsize = 18)
# plt.savefig("fig-mitigation-fu.pdf", bbox_inches='tight')
# #%%#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

# #%%
# time = read_all_stats("Cache", query = 'system.switch_cpus.ipc_total')
# df = pd.DataFrame.from_dict(time)
# df= df.reindex(index=df.index[::-1])
# df = df.rename(index = {'Cache_asymmetric':'Asymmetric'})
# df = df.rename(index = {'Cache_adaptive':'Adaptive'})
# df = df.rename(index = {'Cache_partitioned':'Partitioned'})
# df.reset_index(inplace=True)
# df = df.rename(columns = {'index':'Method'})
# df['GMean'] = stats.gmean(df.iloc[:, 1:], axis=1) 
# df = pd.melt(df, id_vars="Method")

# patterns = [ "\\" , "/" , "x" , "o" , "\\" , "/", "o", "O", ".", "*" ]
# set_style()
# fig, ax = plt.subplots(figsize=(15,5))
# ax = sns.barplot(data = df,
#     x="variable",
#     y="value",
#     hue="Method",
#     palette="Purples", #bone_r
#     linewidth=1,
#     saturation=.8
# )

# num_locations = len(df['variable'].unique())
# hatches = itertools.cycle(patterns)
# for i, bar in enumerate(ax.patches):
#     if i % num_locations == 0:
#         hatch = next(hatches)
#     bar.set_edgecolor('black')


# ax.legend(loc=("upper center"),
#            bbox_to_anchor=(0.5, 1.17),
#            ncol=4,
#            prop={'size': 18},

# )
# ax.set_yticks(np.arange(0,1.1,0.04))
# plt.xlabel('', fontsize = 22)
# plt.ylabel('Speedup over Shared', fontsize = 20)
# for tick in ax.get_xticklabels():
#     tick.set_rotation(45)
# plt.ylim((0.75,1.03))
# plt.grid(axis='y')
# plt.xticks(fontsize = 18)
# plt.yticks(fontsize = 18)
# plt.savefig("fig-mitigation-cache.pdf", bbox_inches='tight')


# #%%#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

# #%%
# time = read_all_stats("Fetch", query = 'system.switch_cpus.ipc_total')
# df = pd.DataFrame.from_dict(time)
# df= df.reindex(index=df.index[::-1])
# df = df.rename(index = {'Fetch_asymmetric':'Asymmetric'})
# df = df.rename(index = {'Fetch_adaptive':'Adaptive'})
# df = df.rename(index = {'Fetch_partitioned':'Partitioned'})
# df.reset_index(inplace=True)
# df = df.rename(columns = {'index':'Method'})
# df['GMean'] = stats.gmean(df.iloc[:, 1:], axis=1) 
# df = pd.melt(df, id_vars="Method")

# patterns = [ "\\" , "/" , "x" , "o" , "\\" , "/", "o", "O", ".", "*" ]
# set_style()
# fig, ax = plt.subplots(figsize=(15,5))
# ax = sns.barplot(data = df,
#     x="variable",
#     y="value",
#     hue="Method",
#     palette="Purples", #bone_r
#     linewidth=1,
#     saturation=.8
# )

# num_locations = len(df['variable'].unique())
# hatches = itertools.cycle(patterns)
# for i, bar in enumerate(ax.patches):
#     if i % num_locations == 0:
#         hatch = next(hatches)
#     bar.set_edgecolor('black')

# ax.legend(loc=("upper center"),
#            bbox_to_anchor=(0.5, 1.18),
#            ncol=4,
#            prop={'size': 18},

# )
# ax.set_yticks(np.arange(0,1.23,0.04))
# plt.xlabel('', fontsize = 22)
# plt.ylabel('Speedup over Shared', fontsize = 20)
# for tick in ax.get_xticklabels():
#     tick.set_rotation(45)
# plt.ylim((0.76,1.10))
# plt.grid(axis='y')
# plt.xticks(fontsize = 16)
# plt.yticks(fontsize = 18)
# plt.savefig("fig-mitigation-fetch.pdf", bbox_inches='tight')
# #%%#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

# time = read_all_stats("IQ", query = 'system.switch_cpus.ipc_total')
# df = pd.DataFrame.from_dict(time)
# df= df.reindex(index=df.index[::-1])
# df = df.rename(index = {'IQ_asymmetric':'Asymmetric'})
# df = df.rename(index = {'IQ_adaptive':'Adaptive'})
# df = df.rename(index = {'IQ_partitioned':'Partitioned'})
# df.reset_index(inplace=True)
# df = df.rename(columns = {'index':'Method'})
# df['GMean'] = stats.gmean(df.iloc[:, 1:], axis=1) 
# df = pd.melt(df, id_vars="Method")

# patterns = [ "\\" , "/" , "x" , "o" , "\\" , "/", "o", "O", ".", "*" ]
# set_style()
# fig, ax = plt.subplots(figsize=(15,3))
# ax = sns.barplot(data = df,
#     x="variable",
#     y="value",
#     hue="Method",
#     palette="Purples", #bone_r
#     linewidth=1,
#     saturation=.8
# )

# num_locations = len(df['variable'].unique())
# hatches = itertools.cycle(patterns)
# for i, bar in enumerate(ax.patches):
#     if i % num_locations == 0:
#         hatch = next(hatches)
#     bar.set_edgecolor('black')

# ax.legend(loc=("upper center"),
#            bbox_to_anchor=(0.5, 1.28),
#            ncol=4,
#            prop={'size': 15},
# )
# plt.xlabel('', fontsize = 22)
# plt.ylabel('Speedup \nover Shared', fontsize = 20)
# for tick in ax.get_xticklabels():
#     tick.set_rotation(45)
# plt.ylim((0,1.23))
# plt.xticks(fontsize = 16)
# plt.yticks(fontsize = 18)
# plt.savefig("fig-mitigation-IQ2.pdf", bbox_inches='tight')

# #%%#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# time = read_all_stats("PRF", query = 'system.switch_cpus.ipc_total')
# df = pd.DataFrame.from_dict(time)
# df= df.reindex(index=df.index[::-1])
# df = df.rename(index = {'PRF_asymmetric':'Asymmetric'})
# df = df.rename(index = {'PRF_adaptive':'Adaptive'})
# df = df.rename(index = {'PRF_partitioned':'Partitioned'})
# df.reset_index(inplace=True)
# df = df.rename(columns = {'index':'Method'})
# df['GMean'] = stats.gmean(df.iloc[:, 1:], axis=1) 
# df = pd.melt(df, id_vars="Method")

# patterns = [ "\\" , "/" , "x" , "o" , "\\" , "/", "o", "O", ".", "*" ]
# set_style()
# fig, ax = plt.subplots(figsize=(15,3))
# ax = sns.barplot(data = df,
#     x="variable",
#     y="value",
#     hue="Method",
#     palette="Purples", #bone_r
#     linewidth=1,
#     saturation=.8
# )

# num_locations = len(df['variable'].unique())
# hatches = itertools.cycle(patterns)
# for i, bar in enumerate(ax.patches):
#     if i % num_locations == 0:
#         hatch = next(hatches)
#     bar.set_edgecolor('black')
#     ax.annotate(str("%.2f"%bar.get_height()), (bar.get_x() + 0.02, bar.get_height() + 0.04), rotation=90, fontsize=12)

# ax.legend(loc=("upper center"),
#            bbox_to_anchor=(0.5, 1.28),
#            ncol=4,
#            prop={'size': 15},

# )
# plt.xlabel('', fontsize = 22)
# plt.ylabel('Speedup \nover Shared', fontsize = 20)
# for tick in ax.get_xticklabels():
#     tick.set_rotation(45)
# plt.ylim((0,1.23))
# plt.xticks(fontsize = 16)
# plt.yticks(fontsize = 18)
# plt.savefig("fig-mitigation-PRF2.pdf", bbox_inches='tight')

# #%%#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

# time = read_all_stats("LQ", query = 'system.switch_cpus.ipc_total')
# df = pd.DataFrame.from_dict(time)
# df= df.reindex(index=df.index[::-1])
# df = df.rename(index = {'PRF_asymmetric':'Asymmetric'})
# df = df.rename(index = {'PRF_adaptive':'Adaptive'})
# df = df.rename(index = {'PRF_partitioned':'Partitioned'})
# df.reset_index(inplace=True)
# df = df.rename(columns = {'index':'Method'})
# df['GMean'] = stats.gmean(df.iloc[:, 1:], axis=1) 
# df = pd.melt(df, id_vars="Method")

# patterns = [ "\\" , "/" , "x" , "o" , "\\" , "/", "o", "O", ".", "*" ]
# set_style()
# fig, ax = plt.subplots(figsize=(15,3))
# ax = sns.barplot(data = df,
#     x="variable",
#     y="value",
#     hue="Method",
#     palette="Purples", #bone_r
#     linewidth=1,
#     saturation=.8
# )

# num_locations = len(df['variable'].unique())
# hatches = itertools.cycle(patterns)
# for i, bar in enumerate(ax.patches):
#     if i % num_locations == 0:
#         hatch = next(hatches)
#     bar.set_edgecolor('black')
#     ax.annotate(str("%.2f"%bar.get_height()), (bar.get_x() + 0.02, bar.get_height() + 0.04), rotation=90, fontsize=12)

# ax.legend(loc=("upper center"),
#            bbox_to_anchor=(0.5, 1.28),
#            ncol=4,
#            prop={'size': 15},

# )
# plt.xlabel('', fontsize = 22)
# plt.ylabel('Speedup \nover Shared', fontsize = 20)
# for tick in ax.get_xticklabels():
#     tick.set_rotation(45)
# plt.ylim((0,1.23))
# plt.xticks(fontsize = 16)
# plt.yticks(fontsize = 18)
# plt.savefig("fig-mitigation-LQ2.pdf", bbox_inches='tight')

# #%%#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

# time = read_all_stats("SQ", query = 'system.switch_cpus.iew.iewLSQFullEvents', norm=False)
# df = pd.DataFrame.from_dict(time)
# df= df.reindex(index=df.index[::-1])
# df = df.rename(index = {'PRF_asymmetric':'Asymmetric'})
# df = df.rename(index = {'PRF_adaptive':'Adaptive'})
# df = df.rename(index = {'PRF_partitioned':'Partitioned'})
# df.reset_index(inplace=True)
# df = df.rename(columns = {'index':'Method'})
# df['GMean'] = stats.gmean(df.iloc[:, 1:], axis=1) 
# df = pd.melt(df, id_vars="Method")

# patterns = [ "\\" , "/" , "x" , "o" , "\\" , "/", "o", "O", ".", "*" ]
# set_style()
# fig, ax = plt.subplots(figsize=(15,3))
# ax = sns.barplot(data = df,
#     x="variable",
#     y="value",
#     hue="Method",
#     palette="Purples", #bone_r
#     linewidth=1,
#     saturation=.8
# )

# num_locations = len(df['variable'].unique())
# hatches = itertools.cycle(patterns)
# for i, bar in enumerate(ax.patches):
#     if i % num_locations == 0:
#         hatch = next(hatches)
#     bar.set_edgecolor('black')
#     ax.annotate(str("%.2f"%bar.get_height()), (bar.get_x() + 0.02, bar.get_height() + 0.04), rotation=90, fontsize=12)

# ax.legend(loc=("upper center"),
#            bbox_to_anchor=(0.5, 1.28),
#            ncol=4,
#            prop={'size': 15},

# )
# plt.xlabel('', fontsize = 22)
# plt.ylabel('Speedup \nover Shared', fontsize = 20)
# for tick in ax.get_xticklabels():
#     tick.set_rotation(45)
# plt.ylim((0,1.1*df["value"].max()))
# plt.xticks(fontsize = 16)
# plt.yticks(fontsize = 18)
# plt.savefig("fig-mitigation-SQ2.pdf", bbox_inches='tight')

# # %%#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

# def draw_event(event, query):
#     time = read_all_stats("all", query=query, norm=False)
#     df = pd.DataFrame.from_dict(time)
#     df = df.rename(index = {'all_asymmetric__':'Asymmetric'})
#     df = df.rename(index = {'all_shared':'Shared'})
#     df = df.rename(index = {'all_partitioned':'Partitioned'})
#     df = df.rename(index = {'all_adaptive': 'Adaptive'})
#     df = df.rename(columns = {'Normalized Execution Time' : 'Number of {}'.format(event)})
#     df.reset_index(inplace=True)
#     df = df.rename(columns = {'index':'Method'})
#     df['Average'] = df.mean(numeric_only=True, axis=1)
#     df = pd.melt(df, id_vars="Method")
#     patterns = [ "\\" , "/" , "x" , "o" , "\\" , "/", "o", "O", ".", "*" ]
#     set_style()
#     fig, ax = plt.subplots(figsize=(10,4))

#     ax = sns.barplot(data = df,
#         x="variable",
#         y="value",
#         hue="Method",
#         palette=("Purples"),
#         linewidth=1,
#     )
#     num_locations = len(df['variable'].unique())
#     hatches = itertools.cycle(patterns)
#     for i, bar in enumerate(ax.patches):
#         if i % num_locations == 0:
#             hatch = next(hatches)
#         bar.set_edgecolor('black')

#     ax.legend(loc=("upper center"),
#             bbox_to_anchor=(0.5, 1.25),
#             ncol=4,
#             prop={'size': 15},
#     )
#     plt.xlabel('', fontsize = 22)
#     plt.ylabel('Number of \n {}'.format(event), fontsize = 20)
#     for tick in ax.get_xticklabels():
#         tick.set_rotation(45)
#     plt.grid(axis='y')
#     plt.xticks(fontsize = 16)
#     plt.yticks(fontsize = 18)
#     plt.ticklabel_format(axis="y", style="sci", scilimits=(0,0))
#     plt.savefig("fig-mitigation-{}.pdf".format(event).replace(" ", "_"), bbox_inches='tight')

# # %%#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

# draw_event("Full Register Events", "system.switch_cpus.rename.fullRegistersEvents::total")
# #%%
# draw_event("Full ROB Events", "system.switch_cpus.rename.ROBFullEvents::total")
# #%%
# draw_event("Full SQ Events", "system.switch_cpus.rename.SQFullEvents::total")

# # %%
# draw_event("Full LQ Events", "system.switch_cpus.rename.LQFullEvents::total")
# #%%
# draw_event("TLB Misses", "system.switch_cpus.dtb.rdMisses")
# # %%
# draw_event("IQ Full Events", "system.switch_cpus.rename.IQFullEvents::total")

