#!/usr/bin/env python3

#%%

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import csv
import seaborn as sns
import logging
import os
import sys
from pathlib import Path

#%% 


df = pd.read_csv("results.csv", skipinitialspace=True, 
                names=["channel", "bw (kbps)", "error"])
df = df[df.error <= 0.1]
# %%
print ("average:")
print (df.groupby("channel").mean())
# %%

# %%
