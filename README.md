## SecSMT: Securing SMT Processors against Contention-Based Covert Channels 
Authors: Mohammadkazem Taram, Xida Ren, Ashish Venkat, Dean Tullsen. 

Appears in *USENIX Security 2022*. 



# Introduction 

This artifact comprises two main relatively separable components: the framework for covert channel measurements and the simulation infrastructure for our mitigations. 




## Covert Channels
### Overview

* System Requirements
* Getting Started (5 minutes)
* Install dependencies (10 minutes)
* Switch to Performance Mode (5 minutes)
* Run the Covert Channels Measurements (2 hours)
* Validate Results (10 minutes)



### 0. Requirements
* Intel(R) Core(TM) i7-6770HQ Processor
* AMD Ryzen Threadripper 3960X Processor


### 1. Getting Started 
* Install git if it is not already installed: `sudo apt-get install git`
 - Use an appropriate package manager if not running Ubuntu/Debian.
* Clone our git repository: `git clone https://github.com/mktrm/secSMT`
* Checkout the right commit hash: `git checkout <commit-hash>`


### 2. Install dependencies 
* Install all the dependencies required for covert channel framework `cd channels/DriverSrcLinux && sudo ./init.sh`
* Install performance counter module `sudo ./install.sh`

### 3. Switch to Performance Mode 
* Force the first physical core into performance mode: `sudo cpupower -c 1 frequency-set -g performance`

### 4. Evaluate the Covert Channels
* Go back to the channel top folder `cd ..`
* Run all the measurements (should take around 1 hour): `make run` 
* This should output a list with the average bandwidth achieved for each covert channel


### 5. Validate the Results
If the measurements take place on the mentioned processors, the bandwidth numbers should be around the results reported in Table 1, but not exactly the same. 
Note that the fluctuation in bandwidth and error rate numbers can be caused by various sources of noise such as voltage/frequency scaling, OS scheduling, etc. 

## Mitigations


### Overview
* System Requirements
* Getting Started (5 minutes)
* Compile gem5 (15 minutes)
* Run Spec Experiments (min of ~800 core hours)
* Validate Results (5 minutes)

### 0. Requirements
* Access to SPEC 2017 CPU benchmarks and simpoints 
* 800 core hours, some SPEC benchmarks require 64 GB of memory
* The run script for SPEC 2017 experiments requires **Slurm workload manager**

### 1. Getting Started 
* Install git if it is not already installed: `sudo apt-get install git`
 - Use an appropriate package manager if not running Ubuntu/Debian.
* Clone our git repository: `git clone https://github.com/mktrm/secSMT && cd secSMT/mitigations`
* Checkout the right commit hash: `git checkout <commit-hash>`


### 2. Install gem5 dependency 
* Install all the dependencies required for gem5 compilation by running `sudo ./scripts/install_requirements.sh`

### 3. Build the simulator
* Run `scripts/build_gem5.sh` to build the simulator.

### 4. Run Spec Experiments
* Create the SPEC folder `workloads/spec2017` with all the available simpoints, alternatively, make sure the script `scripts/spec17/config.py` has a correct pointer to the spec17 root folder. 
* If you already have access to SPEC joint checkpoints skip this step. Use `python scripts/merge_ckp.py` to create joint multithreaded SMT checkpoints from single-threaded Spec Simpoints. 
* Use `python3 scripts/spec17/run_gem5.py` to submit all of the simulation jobs to the Slurm scheduler. 
 - It should take around 2 to 3 hours if enough computing nodes are available.


### 4. Validate Results
* Run `python3 scripts/spec17/draw_figs.py` to parse the results and draw the performance figure.
* Open the pdf file that is stored in the current directory (`fig-mitigation-all.pdf`) and compare the results with figure 7 of the paper.




### Citation
Please cite the following paper if you use this artifact:
```
@inproceedings{secsmt,
 author = {Mohammadkazem Taram and Xida Ren and Ashish Venkat and Dean Tullsen},
 title = {{SecSMT: Securing SMT Processors against Contention-Based Covert Channels}},
 booktitle = {{USENIX} Security Symposium ({USENIX} Security)},
 year = {2022}
}
```
