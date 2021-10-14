import config
from pathlib import Path
from itertools import combinations
from itertools import combinations_with_replacement
import logging
import sys,os,re

logging.basicConfig(stream=sys.stdout, level=logging.ERROR)
log = logging.getLogger('merge_ckp')

def joint_ckp_path (bench_a, bench_b):
    return f"{config.JOINT_CKP_FOLDER}/ckp_{bench_a}_{bench_b}/cpt.1/"

def run_merger (bench_a, bench_b):

    ckp_1 = (f"{config.SPEC_ROOT}/{bench_a['name']}/"
             f"{bench_a['name']}.sim.64G/cpt.{bench_a['heaviest_ckp']}")
    ckp_2 = (f"{config.SPEC_ROOT}/{bench_b['name']}/"
             f"{bench_b['name']}.sim.64G/cpt.{bench_b['heaviest_ckp']}")
    joint_ckp = joint_ckp_path(bench_a['name'], bench_b['name'])
    
    slurm_file_path = (f"{config.JOINT_CKP_FOLDER}/"
                       f"ckp_{bench_a['name']}_{bench_b['name']}/cpt.1/merge.slurm")
    
    Path(os.path.dirname(slurm_file_path)).mkdir(parents=True, exist_ok=True)
    job_name =  f"merge_ckp_{bench_a['name']}_{bench_b['name']}"
    with open(slurm_file_path, "w") as slurm_file:
        slurm_file.write("#!/bin/bash\n")
        slurm_file.write(f"#SBATCH --job-name={job_name}")
        slurm_file.write("#SBATCH --partition=main\n")
        slurm_file.write(f"#SBATCH --output={joint_ckp}/stdout.out\n")
        slurm_file.write(f"#SBATCH --error={joint_ckp}/stderr.out\n")
        slurm_file.write("source /etc/profile.d/modules.sh\n")
        slurm_file.write("module load python3\n")
        
        slurm_file.write(f"python3 {config.GEM5_ROOT}/util/ckp_merge.py {ckp_1} {ckp_2} {joint_ckp}")
    os.system("sbatch %s" %slurm_file_path)


for i,j in combinations_with_replacement(config.spec17, 2):
    log.info (f"merging {i['name']} and {j['name']}")
    log.info (f"checking if the joint ckp  exists")
    joint_ckp = joint_ckp_path(i['name'], j['name'])+"/system.physmem.store0.pmem"


    err_file_path =joint_ckp_path(i['name'], j['name'])+"/stderr.out"
    rerun = False
    if os.path.isfile(err_file_path):
        err_file = open(err_file_path, "r")
        for line in err_file: 
            if re.search("Errno", line):
                rerun = True


    run_merger(i, j)

