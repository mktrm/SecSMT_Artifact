
from subprocess import PIPE, run, Popen
import pathlib

def out(command):
    result = run(command, stdout=PIPE, stderr=PIPE, universal_newlines=True, shell=True)
    return result.stdout


def nonblocking_out(command):
    result = Popen(command, stdout=PIPE, stderr=PIPE, universal_newlines=True, shell=True)
    return result.stdout





ROOT = str(pathlib.Path(__file__).parent.parent.parent.resolve())
SPEC_ROOT = ROOT + "/workloads/spec2017" 
GEM5_ROOT = ROOT + "/gem5"
GEM5_BIN =  GEM5_ROOT + "/build/X86/gem5.fast"
GEM5_CONFIG = GEM5_ROOT + "/configs/example/se.py"

JOINT_CKP_FOLDER = SPEC_ROOT + "/joint_chkpoints_spec2017/checkpoints"
RUN_DIR = ROOT + "/results"



spec17 = [
dict(
name = 'perl',
num_ckp = 7, 
heaviest_ckp = 7,
bin_name = 'perlbench_s_base.mytest-m64',
args = '-I./lib checkspam.pl 2500 5 25 11 150 1 1 1 1',
type ='int'
),
 
dict(
name = 'gcc',
num_ckp = 7, 
heaviest_ckp = 3,
bin_name = 'sgcc_base.mytest-m64',
args = 'gcc-pp.c -O5 -finline-limit=24000 -fgcse -fgcse-las -fgcse-lm -fgcse-sm -o gcc-pp.opts-O5_-finline-limit_24000_-fgcse_-fgcse-las_-fgcse-lm_-fgcse-sm.s',
),
 
dict(
name = 'mcf',
type = 'mem',
num_ckp = 7, 
heaviest_ckp = 7,
bin_name = 'mcf_s_base.mytest-m64',
args = 'inp.in',
),
 
dict(
name = 'namd',
num_ckp = 7, 
heaviest_ckp = 5,
bin_name = 'namd_r_peak.mytest-m64',
args = '--input apoa1.input --output apoa1.ref.output --iterations 65',
),
 
dict(
name = 'xalan',
num_ckp = 6, 
heaviest_ckp = 5,
bin_name = 'xalancbmk_s_base.mytest-m64',
args = '-v t5.xml xalanc.xsl',
),
 
dict(
name = 'exchange',
num_ckp = 5, 
heaviest_ckp = 5,
bin_name = 'exchange2_s_peak.mytest-m64',
args = '6',
),
 
dict(
name = 'deepsjeng',
num_ckp = 3, 
type = 'int',
heaviest_ckp = 3,
bin_name = 'deepsjeng_s_base.mytest-m64',
args = 'ref.txt',
),
 
dict(
name = 'leela',
num_ckp = 8, 
heaviest_ckp = 3,
bin_name = 'leela_s_peak.mytest-m64',
args = 'ref.sgf',
),
 
dict(
name = 'xz',
num_ckp = 6, 
heaviest_ckp = 4,
bin_name = 'xz_s_base.mytest-m64',
args = 'cpu2006docs.tar.xz 6643 055ce243071129412e9dd0b3b69a21654033a9b723d874b2015c774fac1553d9713be561ca86f74e4f16f22e664fc17a79f30caa5ad2c04fbc447549c2810fae 1036078272 1111795472 4',
),
 
dict(
name = 'lbm',
num_ckp = 5, 
type = 'fp',
heaviest_ckp = 3,
bin_name = 'lbm_s_base.mytest-m64',
args = '2000 reference.dat 0 0 200_200_260_ldc.of',
),
 
dict(
name = 'nab',
num_ckp = 4, 
heaviest_ckp = 1,
bin_name = 'nab_s_base.mytest-m64',
args = '3j1n 20140317 220',
),
 
dict(
name = 'wrf',
num_ckp = 6, 
heaviest_ckp = 1,
bin_name = 'wrf_s_base.mytest-m64',
args = '',
),
 
dict(
name = 'povray',
num_ckp = 6, 
heaviest_ckp = 5,
bin_name = 'povray_r_peak.mytest-m64',
args = SPEC_ROOT +'/povray/SPEC-benchmark-ref.ini',
),
 
dict(
name = 'fotonik',
num_ckp = 5, 
heaviest_ckp = 5,
bin_name = 'fotonik3d_s_peak.mytest-m64',
args = '',
),
 
]

adaptive_intervals=[
    100, 1000, 10000, 100000
]

adaptive_limits_statefull=[
    1.0, 0.9, 0.8, 0.7, 0.6
]
adaptive_limits_stateless=[
    4, 8, 10, 16 #32 is also available
]
adaptive_mults_stateless=[
    1, 1.2, 2 #1.5, 4 are also available but are commented for figure
]

adaptive_resources=[
    dict( name = 'IQ',  type ='statefull'),
    dict( name = 'LQ',  type ='statefull'),
    dict( name = 'SQ',  type ='statefull'),
    dict( name = 'ROB', type = 'statefull'),
    dict( name = 'TLB', type = 'statefull'),
    dict( name= 'PhysReg', type = 'statefull'),
    dict( name= 'Cache', type = 'statefull'),
    dict( name= 'BTB',  type = 'statefull'),

    #stateless:
    dict (name  = 'Fetch', type = 'stateless'),
    dict (name = 'Commit', type = 'stateless'),
    dict (name = 'Issue', type = 'stateless'),
]




jobs=[
        #this is the baseline and should remain in index 0
        dict(
            name = 'all_shared',
            gem5_args = '',
            structure = 'all_shared'
        ),
        dict(
            name = 'all_partitioned',
            gem5_args = '--smtROBPolicy=PartitionedSMTPolicy --smtIQPolicy=PartitionedSMTPolicy '
                        '--smtTLBPolicy=PartitionedSMTPolicy --smtPhysRegPolicy=PartitionedSMTPolicy '
                        ' --smtSQPolicy=PartitionedSMTPolicy --smtLQPolicy=PartitionedSMTPolicy '
                        '--smtIssuePolicy=MultiplexingFUs '
                        '--smtFetchPolicy=StrictRoundRobin ' 
                        '--smtBTBPolicy=PartitionedSMTPolicy '
                        '--smtCachePolicy=PartitionedSMTPolicy '
                        '--smtCommitPolicy=StrictRoundRobin ',
            structure = 'all_partitioned'
        ),
        dict(
            name = 'all_adaptive',
            gem5_args = '--smtROBPolicy=AdaptiveSMTPolicy --smtIQPolicy=AdaptiveSMTPolicy '
                        '--smtTLBPolicy=AdaptiveSMTPolicy --smtPhysRegPolicy=AdaptiveSMTPolicy '
                        ' --smtSQPolicy=AdaptiveSMTPolicy --smtLQPolicy=AdaptiveSMTPolicy '
                        '--smtIssuePolicy=Adaptive --smtAdaptiveInterval=100000 '
                        '--smtFetchPolicy=Adaptive ' 
                        '--smtBTBPolicy=AdaptiveSMTPolicy '
                        '--smtCachePolicy=AdaptiveSMTPolicy '
                        '--smtCommitPolicy=Adaptive ',
            structure = 'all_adaptive'
        ),
        dict(
            name = 'all_asymmetric__',
            gem5_args = '--smtROBPolicy=AdaptiveSMTPolicy --smtIQPolicy=AdaptiveSMTPolicy '
                        '--smtTLBPolicy=AdaptiveSMTPolicy --smtPhysRegPolicy=AdaptiveSMTPolicy '
                        '--smtSQPolicy=AdaptiveSMTPolicy --smtLQPolicy=AdaptiveSMTPolicy '
                        '--smtIssuePolicy=Asymmetric --smtAdaptiveInterval=100000 '
                        '--smtFetchPolicy=Asymmetric ' 
                        '--smtBTBPolicy=AsymmetricSMTPolicy '
                        '--smtCachePolicy=AsymmetricSMTPolicy '
                        '--smtCommitPolicy=Asymmetric ',
            structure = 'all_asymmetric'
        ),
        # dict(
        #      name = 'Fetch_asymmetric',
        #     gem5_args = '--smtAdaptiveInterval=100000 '
        #                 '--smtFetchPolicy=Asymmetric ',
        #     structure = 'Fetch_asymmetric'
        # ),
        # dict(
        #     name = 'Fetch_adaptive',
        #     gem5_args = '--smtAdaptiveInterval=100000 '
        #                 '--smtFetchPolicy=Adaptive ',
        #     structure = 'Fetch_adaptive'
        # ),
        # dict(
        #     name = 'Fetch_partitioned',
        #     gem5_args = '--smtFetchPolicy=StrictRoundRobin ' ,
        #     structure = 'Fetch_partitioned'
        # ),
        # dict(
        #      name = 'Issue_asymmetric',
        #     gem5_args = '--smtAdaptiveInterval=100000 '
        #                 '--smtIssuePolicy=Asymmetric ',
        #     structure = 'Issue_asymmetric'
        # ),
        # dict(
        #     name = 'Issue_adaptive',
        #     gem5_args = '--smtAdaptiveInterval=100000 '
        #                 '--smtIssuePolicy=Adaptive ',
        #     structure = 'Issue_adaptive'
        # ),
        # dict(
        #     name = 'Issue_partitioned',
        #     gem5_args = '--smtIssuePolicy=MultiplexingFUs ' ,
        #     structure = 'Issue_partitioned'
        # ),
        #  dict(
        #      name = 'Issue_mux',
        #     gem5_args = '--smtIssuePolicy=MultiplexingThreads ',
        #     structure = 'Issue_mux'
        # ),
        # dict(
        #      name = 'Cache_asymmetric',
        #     gem5_args = '--smtAdaptiveInterval=100000 '
        #                 '--smtCachePolicy=AsymmetricSMTPolicy ',
        #     structure = 'Cache_asymmetric'
        # ),
        # dict(
        #     name = 'Cache_adaptive',
        #     gem5_args = '--smtAdaptiveInterval=100000 '
        #                 '--smtCachePolicy=AdaptiveSMTPolicy ',
        #     structure = 'Cache_adaptive'
        # ),
        # dict(
        #     name = 'Cache_partitioned',
        #     gem5_args = '--smtCachePolicy=PartitionedSMTPolicy ' ,
        #     structure = 'Cache_partitioned'
        # ),
]