#!/usr/bin/env python3

# %%
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import csv
import seaborn as sns
import logging
import os
import sys
from pathlib import Path


# %%
def clean_df(df):
    df = df.fillna(-1)
    df = df.replace(r'[^0-9]+',-1)
    df = df.replace('',-1)
    df = df.replace(' ',-1)
    df = df.replace('\n',-1)
    return df 


# %%
logging.basicConfig(level=os.environ.get("LOGLEVEL", "INFO"))
logging.getLogger('matplotlib.font_manager').disabled = True
log = logging.getLogger("Logger")


# %%
def generate_lfsr():
    lfsr = 0xACE1>>1
    lfsr = lfsr ^ 0x0B400
    string = [1]
    while (lfsr != 0xACE1):
        lsb  = lfsr & 1
        lfsr = lfsr>>1  
        if lsb == 0: 
            string = string + [0]
        else:  
            lfsr = lfsr ^ 0x0B400
            string = string + [1]
    return string
sent_seq = generate_lfsr()


# %%
def levenshteinDistance(s1, s2):
    if len(s1) > len(s2):
        s1, s2 = s2, s1

    distances = range(len(s1) + 1)
    for i2, c2 in enumerate(s2):
        distances_ = [i2+1]
        for i1, c1 in enumerate(s1):
            if c1 == c2:
                distances_.append(distances[i1])
            else:
                distances_.append(1 + min((distances[i1], distances[i1 + 1], distances_[-1])))
        distances = distances_
    return distances[-1]

# %%
# ## Read Data
def read_res(name):
    df = pd.read_csv(name, skipinitialspace=True)
    df = clean_df(df)
    df2 = df.loc[df['Measure'] == 'Clock']
    df2 = df2["T2/T1"]
    df2.reset_index(drop=True, inplace=True)

    return df2


# %%
covertdf = read_res(sys.argv[1])
# all_zero_df = read_res(sys.argv[2])
# all_one_df = read_res(sys.argv[3])  


# %%
threshold = (np.sort(covertdf[10:1000])[10] +  np.sort(covertdf[10:1000])[-10]) / 2.0
log.debug("calculated threshold: %d  avg=%d max=%d", threshold, np.average(covertdf[10:1000]),  np.sort(covertdf[10:1000])[-10]) 
thresholds = [threshold, 0.8*threshold, 0.9*threshold]


# %%
# ## Visualize Data
if log.level <= logging.DEBUG:
    plt.figure(figsize=(22,8))
    plt.plot((covertdf[1:500]))
    # plt.ylim(0, 10000)
    plt.savefig(str(Path(sys.argv[1]).parent.absolute())+"/plot.pdf")

# %%
# ## Decode Data
def decode(threshold, one_cons, zero_cons, size):
    received_seq = []

    consecutive_1 = 0
    consecutive_0 = 0
    state = 0
    processed = len(covertdf)
    for i in range(1, len(covertdf)):
        # print (i, covertdf[i], state, consecutive_0, consecutive_1)
        if len(received_seq) > size: 
            processed = i
            break
        if covertdf[i] < threshold :  #Change this value based on the high/low threashold 
            
            if state == 1:
                if consecutive_1 > one_cons: #Change this value based on the length of high signal 
                    received_seq = received_seq + [1]
                    # print ("t01 received\t", 1)
                consecutive_1 = 0 
            state = 0 
            consecutive_0 = consecutive_0 + 1
            if consecutive_0 > zero_cons:    #Change this value based on the length of low signal 
                received_seq = received_seq + [0]
                # print ("t00 received\t", 0)
                consecutive_0 = 0
            
        else: #HIGH
            if state == 0:  #transition from LOW
                if consecutive_0 > zero_cons: 
                    received_seq = received_seq + [0]
                    # print ("t10 received\t", 0)
                consecutive_0 = 0 
            state = 1
            consecutive_1 = consecutive_1 + 1 
            if len(sys.argv) > 4 and sys.argv[4] == "high":
                if consecutive_1 > one_cons:    #Change this value based on the length of low signal 
                    received_seq = received_seq + [1]
                    # print ("t11 received\t", 1)
                    consecutive_1 = 0

                


    return (received_seq, processed)
            


# %%
size = 100
best_rate = 1
best_low = -1
best_high = -1
best_received_seq = []
offset = 0
# thresholds = [700]
for thr in thresholds:
    log.debug("trying threshold=%d", thr)
    for offset in range (0,10):
        for high in range (-1, 10):
            for low in range (-1, 20):
                (received_seq, proccessed) = decode (thr, high, low, size)
                distance = levenshteinDistance(sent_seq[offset:size],received_seq[0:size-offset])
                error_rate = distance/size
                if (error_rate < best_rate):
                    best_processed = proccessed
                    best_rate = error_rate
                    best_received_seq = received_seq
                    best_low = low
                    best_high = high
                    log.debug("%d %d %d %f", high, low, offset, error_rate)




# %%
# print(sent_seq[0:100])
# print(best_received_seq[0:97])
log.debug("sent_seq: {}".format(' '.join(map(str, sent_seq[0:100]))))
log.debug("recv_seq: {}".format(' '.join(map(str, best_received_seq[0:100]))))



# %%
# ## calculate Bandwidth
bw_kb = int(sys.argv[3])/(np.sum(covertdf[1:best_processed])/len(best_received_seq)) #argv[4] is frequency of the machine
print(bw_kb, ",", best_rate)
log.debug("bw=%d error-rate=%f", bw_kb, best_rate)
