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
* Clone our git repository: `git clone https://github.com/mktrm/secSMT_Artifact`
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
* Run Javascript Benchmarks (~40 core hours)
* Validate Results (5 minutes)

### 0. Requirements
* Access to SPEC 2017 CPU benchmarks and simpoints 
* 800 core hours, some SPEC benchmarks require 64 GB of memory
* The run script for SPEC 2017 experiments requires **Slurm workload manager**

### 1. Getting Started 
* Install git if it is not already installed: `sudo apt-get install git`
 - Use an appropriate package manager if not running Ubuntu/Debian.
* Clone our git repository: `git clone https://github.com/mktrm/secSMT_Artifact && cd secSMT_Artifact/mitigations`
* Checkout the right commit hash: `git checkout <commit-hash>`


### 2. Install gem5 dependency 
* Install all the dependencies required for gem5 compilation by running `sudo ./scripts/install_requirements.sh`
  - [ArtifactEvaluators] you can run `./scripts/load_requirements.sh` on our servers to load required Environment Modules

### 3. Build the simulator
* Run `scripts/build_gem5.sh` to build the simulator.

### 4. Generate SPEC Simpoints
* If you already have access to simpoint checkpoints, please skip this step.
* Download the pinplay tool from https://www.intel.com/content/www/us/en/developer/articles/tool/program-recordreplay-toolkit.html. The version that we used: `pinplay-drdebug-3.7-pin-3.7-97619-g0d0c92f4f-gcc-linux.tar.gz`.  
* After downloading the pinplay tool extract and rename it to pin:

```
  gunzip pinplay-drdebug-3.7-pin-3.7-97619-g0d0c92f4f-gcc-linux.tar.gz
  tar -xvf pinplay-drdebug-3.7-pin-3.7-97619-g0d0c92f4f-gcc-linux.tar
  mv pinplay-drdebug-3.7-pin-3.7-97619-g0d0c92f4f-gcc-linux pin
```
* Setup environment for pin tool:
```
  export PIN_KIT=$(pwd)/pin
  cd $PIN_KIT/extras/pinplay/examples
  make
```

* Create a pin configuration file for each benchmark. For example, here is the content of mcf.cfg, the config file that we used for mcf benchmark:

```
[Parameters]
program_name: mcf
input_name: ref
command: SPEC2017/benchspec/CPU/605.mcf_s/run/run_base_refspeed_mytest-m64.0007/mcf_s_base.mytest-m64 inp.in
maxk: 32
mode: st
warmup_length: 100000
slice_size: 100000000
pinplayhome: [replace_with_PIN_path]   
```
 * Run the pinplay tool to create simpoints [it should takes several hours]:

 ```
$PIN_KIT/extras/pinplay/scripts/pinpoints.py --cfg mcf.cfg -l -b  --simpoint  --whole_pgm_dir run/mcf/whole
```

* If the previous command is successful, you should have a folder named `mcf.ref.[num].Data` that contains simpoint files: `t.simpoints` and `t.weights`. 

* Now we use gem5 in fast Atomic mode to create checkpoints of the representative regions [this will take up to several weeks]
```
build/X86/gem5.fast configs/example/se.py --take-simpoint  --checkpoint=t.simpoints,t.weights,100000000,30000000 -c -c ./mcf_s_base.mytest-m64 -o 'inp.in' --cpu-type=AtomicSimpleCPU 
```

* After this, we will have all the checkpoints stored in the `m5out` folder. 
* For more information on Pin, Pinplay, and Simpoint tools, see [Intel Pinpoints](https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-binary-instrumentation-tool-pinpoints.html), [PinPoint HPCA Tutorial](http://snipersim.org/documents/2013-02-23%20PinPoints%20Sniper%20HPCA%20Tutorial.pdf), and [Simpoint](https://cseweb.ucsd.edu/~calder/simpoint/).

### 5. Run Spec Experiments

* Create the SPEC folder `workloads/spec2017` with all the available simpoints, alternatively, make sure the script `scripts/spec17/config.py` has a correct pointer to the spec17 root folder. 
  - [ArtifactEvaluators] You can run ` ln -s  /p/csd/mtaram/spec2017-2/ workloads/spec2017` on our servers to link the folder containing spec2017 simpoints into the correct path.
*  If you already have access to SPEC joint checkpoints skip this step. Use `python scripts/spec17/merge_ckp.py` to create joint multithreaded SMT checkpoints from single-threaded Spec Simpoints. 
    - [ArtifactEvaluators] You should skip this step.
    - Users should manually set up `scripts/spec17/config.py` and set `heaviest_ckp` to the simpoint number with maximum weight from `t.weights` from previous step.
    - The script, then will go through all pairs of the benchmarks specified in `scripts/spec17/config.py` and generates the joint checkpoints.

* Use `python3 scripts/spec17/run_gem5.py` to submit all the simulation jobs to the Slurm scheduler. 
    - It should take around 30 min to submit all the jobs, and they should take around 2 to 3 hours to complete if enough computing nodes are available. You can see the status of the jobs using `squeue`. 


### 6. Validate Results
* Run `python3 scripts/spec17/draw_figs.py` to parse the results and draw the performance figure.
* Open the pdf file that is stored in the current directory (`fig-mitigation-all.pdf`) and compare the results with figure 7 of the paper.

### 7. Run Javascript Experiments
* Use `./scripts/browserlike/runall.sh slurm` to submit Javascript experiments to the Slurm scheduler.
  - Alternatively, you can run this locally using `./scripts/browserlike/runall.sh`
  - This should take about an hour if enough parallelism is available.

### 8. Validate Results
* Run `python3 scripts/browserlike/graph.py` to parse the results and draw the performance figure.
* Open the pdf file that is stored in the current directory (`browserlike.pdf`) and compare the results with figure 8 of the paper.

### 9. Run Security Experiments
* Use `python3 scripts/security/run_security_eval.py slurm` to submit security experiments to the Slurm scheduler.
  - Alternatively, you can run this locally using `python3 scripts/security/run_security_eval.py`
  - This should take about ten minutes if enough parallelism is available.

### 10. Validate Results
* Run `python3 scripts/security/draw_sec_figs.py` to parse the results and draw the performance figure.
* Open the pdf file that is stored in the current directory (`fig-sec-1.pdf`) and compare the results with figure 6 of the paper.

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
