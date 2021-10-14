#!/bin/bash

home_root="/u/mtaram"

spec_root=$home_root/spec2017/
spec_run_root="${spec_root}"

gem5_root=$home_root/gem5-smt/
gem5_bin=$gem5_root/build/X86/gem5.opt
gem5_config=$gem5_root/configs/example/se.py



declare -a spec_num_chkpoints=([1]=7
                                [2]=7
                                [3]=7
                                [4]=7
                                [5]=6
                                [6]=5
                                [7]=3
                                [8]=8
                                [9]=6
                                [10]=5
                                [11]=4
                                [12]=6
                                [13]=6
                                [14]=5
                              )

declare -a spec_bench_names=([1]="perl"
                             [2]="gcc"       
                             [3]="mcf"       
                             [4]="namd"   
                             [5]="xalan" 
                             [6]="exchange"      
                             [7]="deepsjeng"  
                             [8]="leela"      
                             [9]="xz"         
                             [10]="lbm"       
                             [11]="nab"
                             [12]="wrf"
                             [13]="povray"
                             [14]="fotonik"     
                            )

declare -a spec_heaviest_chkpoints=([1]=6
                                [2]=2
                                [3]=6
                                [4]=4
                                [5]=4
                                [6]=4
                                [7]=2
                                [8]=2
                                [9]=3
                                [10]=2
                                [11]=0
                                [12]=0
                                [13]=4
                                [14]=5
                              )

declare -a spec_bench_nums=( [1]=600
                             [2]=602
                             [3]=605
                             [4]=508
                             [5]=623
                             [6]=648
                             [7]=631
                             [8]=641
                             [9]=657
                             [10]=619
                             [11]=644
                             [12]=621
                             [13]=511
                             [14]=649
                            )

declare -a spec_bench_dir=$spec_bench_names

declare -a spec_bench_commands=([1]="perlbench_s_base.mytest-m64"
                                [2]="sgcc_base.mytest-m64"
                                [3]="mcf_s_base.mytest-m64"
                                [4]="namd_r_peak.mytest-m64"
                                [5]="xalancbmk_s_base.mytest-m64"
                                [6]="exchange2_s_peak.mytest-m64"
                                [7]="deepsjeng_s_base.mytest-m64"
                                [8]="leela_s_peak.mytest-m6"
                                [9]="xz_s_base.mytest-m64"
                                [10]="lbm_s_base.mytest-m64"
                                [11]="nab_s_base.mytest-m64"
                                [12]="wrf_s_base.mytest-m64"
                                [13]="povray_r_peak.mytest-m64"
                                [14]="fotonik3d_s_peak.mytest-m64"
                            )


declare -a nodelist=([1]="lynx10"
             [2]="lynx11"
            [3]="hermes1"
            [4]="lynx12"
            [5]="granger3"
            [6]="granger4"
            [7]="hermes4"
            [8]="granger6"
            [9]="hermes2"
            [10]="granger8"
            [11]="hermes3"
            )

declare -a nodelist_pool=([1]="lynx08"
             [2]="lynx09"
            [3]="lynx10"
            [4]="lynx11"
            [5]="lynx12"
            [6]="hermes1"
            [7]="hermes2"
            [8]="hermes3"
            [9]="hermes4"
            [10]="granger3"
            [11]="granger4"
            [12]="granger6"
            [13]="granger8"
            [14]="trillian1"
            [15]="trillian2"
            [16]="trillian3"
            )


#povray needs to be fixed!
get_bench_input() {
    local  spec_bench_inputs=([1]="-I./lib checkspam.pl 2500 5 25 11 150 1 1 1 1"
                              [2]="gcc-pp.c -O5 -finline-limit=24000 -fgcse -fgcse-las -fgcse-lm -fgcse-sm -o gcc-pp.opts-O5_-finline-limit_24000_-fgcse_-fgcse-las_-fgcse-lm_-fgcse-sm.s"
                              [3]="inp.in"
                              [4]="--input apoa1.input --output apoa1.ref.output --iterations 65"
                              [5]="-v t5.xml xalanc.xsl"
                              [6]="6"
                              [7]="ref.txt"
                              [8]="ref.sgf"
                              [9]="cpu2006docs.tar.xz 6643 055ce243071129412e9dd0b3b69a21654033a9b723d874b2015c774fac1553d9713be561ca86f74e4f16f22e664fc17a79f30caa5ad2c04fbc447549c2810fae 1036078272 1111795472 4"
                              [10]="2000 reference.dat 0 0 200_200_260_ldc.of"
                              [11]="3j1n 20140317 220"
                              [12]=""
                              [13]="${spec_run_root}/${spec_bench_dir[$1]}/SPEC-benchmark-ref.ini"
                              [14]=""
                            )

    echo "${spec_bench_inputs[$1]}"
}



create_gem5_run_chk_slurm_batch_file (){

    local bench_input="$(get_bench_input $1)"
    local gem5_slurm_file="${spec_run_root}/${spec_bench_dir[$1]}/\
        gem5_run_${spec_bench_names[$1]}_${spec_bench_names[$2]}.slurm"
    if [ -e ${gem5_slurm_file} ]; then
        rm ${gem5_slurm_file}
    fi

    let node_number=$2/2

#------------------------------------------------------------------------------
cat <<EOF > ${gem5_slurm_file}
#!/bin/bash
# Submission script for MKT
#SBATCH --job-name=mkt_gem5
#SBATCH -N 1
#SBATCH -n 1
#SBATCH -c 1
#
#SBATCH --partition=main
#SBATCH --nodelist=cortado01
#
#SBATCH --mail-user=mtaram@ucsd.edu
#SBATCH --mail-type=ALL
#
#SBATCH --comment=MKT

source /etc/profile.d/modules.sh
module load gcc-6.3.0


#make the joint checkpoint on upgraded checkpoints

srun -N 1 -n 1 
srun -N 1 -n 1 ${home_root}/${gem5}  \
    --outdir=m5out_sim_$2_super \
    ${home_root}/${gem5_config} \
    -r $2 -I 100000000 \
    --checkpoint-dir ${spec_bench_names[$1]}.sim.64G \
    -c ./${spec_bench_commands[$1]} -o '${bench_input}' \
    --caches --l2cache --cpu-type=O3_X86_skylake_1   \
    --mem-type=DDR4_2400_16x4 --mem-size=64GB --mem-channels=2  \

wait

EOF
#------------------------------------------------------------------------------



}



spec_bnech_names_length=${#spec_bench_names[@]}

for (( i=13; i<=${spec_bnech_names_length}; i++ ));
do
#    for (( j=$(i+1); j<=${spec_bnech_names_length}; j++ ));
#    do

#   create_gem5_run_chk_slurm_batch_file $i $j
echo $gem5_root/util/cpt_upgrader.py   ${spec_bench_names[$i]}/${spec_bench_names[$i]}.sim.64G/cpt.${spec_heaviest_chkpoints[$i]}/m5.cpt 
     $gem5_root/util/cpt_upgrader.py   ${spec_bench_names[$i]}/${spec_bench_names[$i]}.sim.64G/cpt.${spec_heaviest_chkpoints[$i]}/m5.cpt
#    done

  #( cd "${spec_run_root}/${spec_bench_dir[$i]}/" ; sbatch "gem5_${spec_bench_names[$i]}.slurm" ; cd "${home_root}/${bbv_gen_slurm_script_dir}" )
done



#TODO: change directory to each benchmark directory

