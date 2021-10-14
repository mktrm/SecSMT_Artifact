import config
from pathlib import Path
from time import sleep
from itertools import combinations
from itertools import combinations_with_replacement
import logging
import sys,os,re

def get_joint_name (bench_a, bench_b):
    return  f"{bench_a['name']}_{bench_b['name']}"

def get_job_folder (bench_a, bench_b, _resource, _interval, _limit, first_tid, _mult = ""):
    joint_name = get_joint_name(bench_a, bench_b)
    _out_folder = f"{config.RUN_DIR}/adaptive/{joint_name}_{first_tid}/{_resource}_{_interval}_{_limit}_{_mult}"
    return _out_folder
def make_gem5_command (bench_a, bench_b, _resource, _interval, _limit, first_tid, _mult = 0):
    
    if _mult:
        _out_folder = get_job_folder(bench_a, bench_b, _resource, _interval, _limit, first_tid, _mult)
    else:
        _out_folder = get_job_folder(bench_a, bench_b, _resource, _interval, _limit, first_tid) 
    joint_name = get_joint_name(bench_a, bench_b)

    bin_b = f"{config.SPEC_ROOT}/{bench_a['name']}/{bench_a['bin_name']}"
    bin_a = f"{config.SPEC_ROOT}/{bench_b['name']}/{bench_b['bin_name']}"
    ckp_folder = f"{config.JOINT_CKP_FOLDER}/ckp_{joint_name}/"

    cmd  = f"{config.GEM5_BIN} "
    cmd += f"--outdir={_out_folder} "
    cmd += f"{config.GEM5_CONFIG} "
    cmd += "-r 1 -I 100000000 "
    cmd += f"--checkpoint-dir {ckp_folder} "
    cmd += f"-c '{bin_a};{bin_b}' "
    cmd += "--caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8  --l2cache --cpu-type=Skylake_3 "
    cmd += "--mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 "
    cmd += "--smt "
    if _interval != 0: # ==0 is the shared baseline
        if _resource in ['Fetch', 'Commit', 'Issue']:
            cmd += f"--smt{_resource}Policy=Adaptive "   
            cmd += f"--smt{_resource}Policy-mult={_mult} " 
        else:
            cmd += f"--smt{_resource}Policy=AdaptiveSMTPolicy "
        cmd += f"--smt{_resource}Policy-limit={_limit} "
        cmd += f"--smtAdaptiveInterval={_interval} "
    cmd += f"--maxinsts_threadID={first_tid} "
    cmd += "\n"

    return cmd

def run_slurm(cmd, job_name, folder, rerun=6):

    slurm_file_path = f"{folder}/run.slurm"
    Path(os.path.dirname(slurm_file_path)).mkdir(parents=True, exist_ok=True)
    with open(slurm_file_path, "w") as slurm_file:
        slurm_file.write("#!/bin/bash\n")
        slurm_file.write(f"#SBATCH --job-name={job_name}\n")
        slurm_file.write(f"#SBATCH --output={folder}/stdout.out\n")
        slurm_file.write(f"#SBATCH --error={folder}/stderr.err\n")
        slurm_file.write(f"#SBATCH --mem=60GB \n")
        slurm_file.write(f"#SBATCH --requeue \n")
        slurm_file.write(f"#SBATCH --exclude=hermes4 \n")
        slurm_file.write("source /etc/profile.d/modules.sh\n")
        slurm_file.write("module load python3\n")
        slurm_file.write("module load gcc-6.3.0\n")
        

        #main command
        slurm_file.write(cmd)

        #make sure cmd returns 1 if unsuccessful
        slurm_file.write(f"if grep -q 'Could not mmap' {folder}/stderr.err; then\n")
        #resubmit the job for 3 times if it fails
        slurm_file.write("[[ $1 -gt 0 ]] && sbatch "
                         "--dependency=afternotok:${SLURM_JOBID} "
                         "%s $(($1-1))\n" % slurm_file_path)
        slurm_file.write("sleep 500\n")
        slurm_file.write("exit 1 \n fi\n")
        slurm_file.write(f"if grep -q 'Killed' {folder}/stderr.err; then\n")
        slurm_file.write("[[ $1 -gt 0 ]] && sbatch "
                         "--dependency=afternotok:${SLURM_JOBID} "
                         "%s $(($1-1))\n" % slurm_file_path)
        slurm_file.write("sleep 500\n")
        slurm_file.write("exit 1 \n fi\n")
        
        #make sure cmd returns 0 if successful
        slurm_file.write("exit 0 \n")


    os.system(f"sbatch {slurm_file_path}  {rerun}")

num=0
for i,j in combinations(config.spec17, 2):
    if 'type' in i and 'type' in j:
        first_tid = 0
    
        #### Shared Baseline
        gem5_cmd = make_gem5_command(i, j, 0, 0, 0, first_tid)
        out_folder = get_job_folder(i, j, 0, 0, 0, first_tid)
        job_name = f"{get_joint_name(i,j)}_0_0_0_{first_tid}"
    
        err_file_path = out_folder + "/stats.txt"
        
        rerun = True
        if os.path.isfile(err_file_path):
            err_file = open(err_file_path, "r")
            for line in err_file: 
                if re.search("system.switch_cpus.numCycles", line):
                    rerun = False
            
        jobs = config.out("squeue -u mtaram -o '\%.18i %.9P %.50j \%.8u %.2t %.10M %.6D %R'")
        if re.search(job_name, jobs):
            rerun = False
        num_jobs = config.out("squeue -u mtaram -o '\%.18i %.9P %.50j \%.8u %.2t %.10M %.6D %R' | wc -l")
        while int(num_jobs) > 700:
            sleep(60)
            num_jobs = config.out("squeue -u mtaram -o '\%.18i %.9P %.50j \%.8u %.2t %.10M %.6D %R' | wc -l")
        if rerun:
            run_slurm(gem5_cmd, job_name, out_folder)
            num = num+1
            sleep(2)
            print (job_name)    

        ########END Baseline 

        for res in config.adaptive_resources:
            resource = res['name']
            if res['type'] == "statefull":
                #first figure out best limit for each resource
                for limit in config.adaptive_limits_statefull:  
                    # set the interval to a constant
                    interval = 100000
                    gem5_cmd = make_gem5_command(i, j, resource, interval, limit, first_tid)
                    out_folder = get_job_folder(i, j, resource, interval, limit, first_tid)
                    job_name = f"{get_joint_name(i,j)}_{resource}_{interval}_{limit}_{first_tid}"
                
                    err_file_path = out_folder + "/stats.txt"
                    
                    print(err_file_path)
                    # exit()
                    rerun = True
                    if os.path.isfile(err_file_path):
                        err_file = open(err_file_path, "r")
                        for line in err_file: 
                            if re.search("system.switch_cpus.numCycles", line):
                                rerun = False
                        
                    jobs = config.out("squeue -u mtaram -o '\%.18i %.9P %.50j \%.8u %.2t %.10M %.6D %R'")
                    if re.search(job_name, jobs):
                        rerun = False
                    num_jobs = config.out("squeue -u mtaram -o '\%.18i %.9P %.50j \%.8u %.2t %.10M %.6D %R' | wc -l")
                    while int(num_jobs) > 700:
                        sleep(60)
                        num_jobs = config.out("squeue -u mtaram -o '\%.18i %.9P %.50j \%.8u %.2t %.10M %.6D %R' | wc -l")
                    if rerun:
                        run_slurm(gem5_cmd, job_name, out_folder)
                        num = num+1
                        sleep(2)
                        print (job_name)      
                
            elif res['type'] == "stateless":
                for limit in config.adaptive_limits_stateless:  
                    for mult in config.adaptive_mults_stateless:  
                        # set the interval to a constant for now
                        interval = 100000
                        gem5_cmd = make_gem5_command(i, j, resource, interval, limit, first_tid, mult)
                        out_folder = get_job_folder(i, j, resource, interval, limit, first_tid, mult)
                        job_name = f"{get_joint_name(i,j)}_{resource}_{interval}_{limit}_{first_tid}_{mult}"
                    
                        err_file_path = out_folder + "/stats.txt"
                        
                        print (err_file_path)
                        
                        rerun = True
                        if os.path.isfile(err_file_path):
                            err_file = open(err_file_path, "r")
                            for line in err_file: 
                                if re.search("system.switch_cpus.numCycles", line):
                                    rerun = False
                            
                        jobs = config.out("squeue -u mtaram -o '\%.18i %.9P %.50j \%.8u %.2t %.10M %.6D %R'")
                        if re.search(job_name, jobs):
                            rerun = False
                        num_jobs = config.out("squeue -u mtaram -o '\%.18i %.9P %.50j \%.8u %.2t %.10M %.6D %R' | wc -l")
                        while int(num_jobs) > 700:
                            sleep(60)
                            num_jobs = config.out("squeue -u mtaram -o '\%.18i %.9P %.50j \%.8u %.2t %.10M %.6D %R' | wc -l")
                        if rerun:
                            # exit()
                            print (job_name)       
                            sleep(2)
                            run_slurm(gem5_cmd, job_name, out_folder)
                            num = num+1
            else : 
                print ("ERROR res type")
print(num)
