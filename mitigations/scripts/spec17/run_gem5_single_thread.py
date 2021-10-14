import config
from pathlib import Path
from itertools import combinations
from itertools import combinations_with_replacement
import logging
from time import sleep
import sys,os,re

def get_joint_name (bench_a):
    return  f"{bench_a['name']}"

def get_job_folder (bench_a):
    joint_name = get_joint_name(bench_a)
    out_folder = f"{config.RUN_DIR}/{joint_name}/"
    return out_folder
def make_gem5_command (bench_a):

    out_folder = get_job_folder(bench_a)
    joint_name = get_joint_name(bench_a)

    bin_a = f"{config.SPEC_ROOT}/{bench_a['name']}/{bench_a['bin_name']}"
    args_a = bench_a['args']
    ckp_folder = f"{config.SPEC_ROOT}/{joint_name}/{joint_name}.sim.64G/"

    cmd  = f"{config.GEM5_BIN} "
    cmd += f"--outdir={out_folder} "
    cmd += f"{config.GEM5_CONFIG} "
    cmd += f"-r {bench_a['heaviest_ckp']} -I 100000000 --maxinsts_threadID=1 "
    cmd += f"--checkpoint-dir {ckp_folder} "
    cmd += f"-c '{bin_a}' "
    cmd += f"-o '{args_a}' "
    cmd += "--caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8  --l2cache --cpu-type=Skylake_3 "
    cmd += "--mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 "
    cmd += "\n"

    return cmd

def run_slurm(cmd, job_name, folder, rerun=3):

    slurm_file_path = f"{folder}/run.slurm"
    Path(os.path.dirname(slurm_file_path)).mkdir(parents=True, exist_ok=True)
    with open(slurm_file_path, "w") as slurm_file:
        slurm_file.write("#!/bin/bash\n")
        slurm_file.write(f"#SBATCH --job-name={job_name}\n")
        slurm_file.write(f"#SBATCH --output={folder}/stdout.out\n")
        slurm_file.write(f"#SBATCH --error={folder}/stderr.err\n")
        slurm_file.write(f"#SBATCH --mem=63GB \n")
        slurm_file.write(f"#SBATCH --exclude=hermes4 \n")
        slurm_file.write(f"#SBATCH --requeue \n")
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
for i in config.spec17 :
    #if i['name'] == 'namd' and j['name'] == 'deepsjeng':
    #    continue
    gem5_cmd = make_gem5_command(i)
    out_folder = get_job_folder(i)
    job_name = get_joint_name(i)+"_single"

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
    while int(num_jobs) > 500:
        sleep(60)
        num_jobs = config.out("squeue -u mtaram -o '\%.18i %.9P %.50j \%.8u %.2t %.10M %.6D %R' | wc -l")
    if rerun:
        run_slurm(gem5_cmd, job_name, out_folder)
        num = num+1
        sleep(2)
        print (job_name)
print(num)
