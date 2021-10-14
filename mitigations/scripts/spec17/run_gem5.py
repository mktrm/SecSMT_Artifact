import config
from pathlib import Path
from itertools import combinations
from itertools import combinations_with_replacement
import logging
from time import sleep
import sys,os,re

def get_joint_name (bench_a, bench_b):
    return  f"{bench_a['name']}_{bench_b['name']}"

def get_job_folder (bench_a, bench_b, job, first_tid):
    joint_name = get_joint_name(bench_a, bench_b)
    out_folder = f"{config.RUN_DIR}/{joint_name}/{job['name']}{first_tid}"
    return out_folder
def make_gem5_command (bench_a, bench_b, job, first_tid):

    out_folder = get_job_folder(bench_a, bench_b, job, first_tid)
    joint_name = get_joint_name(bench_a, bench_b)

    bin_b = f"{config.SPEC_ROOT}/{bench_a['name']}/{bench_a['bin_name']}"
    bin_a = f"{config.SPEC_ROOT}/{bench_b['name']}/{bench_b['bin_name']}"
    ckp_folder = f"{config.JOINT_CKP_FOLDER}/ckp_{joint_name}/"

    cmd  = f"{config.GEM5_BIN} "
    cmd += f"--outdir={out_folder} "
    cmd += f"{config.GEM5_CONFIG} "
    cmd += "-r 1 -I 100000000  "
    cmd += f"--checkpoint-dir {ckp_folder} "
    cmd += f"-c '{bin_a};{bin_b}' "
    cmd += "--caches --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8  --l2cache --cpu-type=Skylake_3 "
    cmd += "--mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2 "
    cmd += "--smt "
    cmd += f"--maxinsts_threadID={first_tid} "
    cmd += job['gem5_args']
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
for i,j in combinations(config.spec17, 2):
    # for i,j in combinations_with_replacement(config.spec17, 2):
    if i['name'] == 'namd' and j['name'] == 'deepsjeng':
        continue
    for job in config.jobs:
        for first_tid in range(0,2):
            gem5_cmd = make_gem5_command(i, j, job, first_tid)
            out_folder = get_job_folder(i, j, job, first_tid)
            job_name = get_joint_name(i,j)+"_"+job['name']+str(first_tid)

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
