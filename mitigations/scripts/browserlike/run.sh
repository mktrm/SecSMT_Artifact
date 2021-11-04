#!/bin/bash
#!/bin/bash                                                                                                                                                                                    
#SBATCH --job-name=perl_perl_all_shared0                                                                                                                                                       
#SBATCH --output=/u/mtaram/spec2017/joint_chkpoints_spec2017/run6/perl_perl/all_shared0/stdout.out                                                                                             
#SBATCH --error=/u/mtaram/spec2017/joint_chkpoints_spec2017/run6/perl_perl/all_shared0/stderr.err
#SBATCH --mem=85GB
#SBATCH --exclude=hermes4
#SBATCH --requeue

module load vscode
module load scons
module load git
module load gcc-6.3.0
module load python3.6.2

BASE=`dirname "$0"`
PROJ="$BASE/../"
WORKLOAD="$PROJ/workloads/browserlike_flat/"
bin1="$WORKLOAD/wolfssl-4.5.0/wolfcrypt/benchmark/benchmark"
args1=$2
bin2="$WORKLOAD/duktape-2.6.0/duk"
args2=$3


case "$1" in
  
  shared)
    simpots=''
    ;;
  partitioned)
    simopts='--smtFetchPolicy=StrictRoundRobin --smtBTBPolicy=PartitionedSMTPolicy --smtCachePolicy=PartitionedSMTPolicy --smtROBPolicy=PartitionedSMTPolicy --smtIQPolicy=PartitionedSMTPolicy --smtTLBPolicy=PartitionedSMTPolicy --smtPhysRegPolicy=PartitionedSMTPolicy --smtSQPolicy=PartitionedSMTPolicy --smtLQPolicy=PartitionedSMTPolicy --smtCommitPolicy=StrictRoundRobin --smtIssuePolicy=MultiplexingFUs'
    ;;
  adaptive)
    simopts='--smtFetchPolicy=Adaptive --smtBTBPolicy=AdaptiveSMTPolicy --smtCachePolicy=AdaptiveSMTPolicy --smtROBPolicy=AdaptiveSMTPolicy --smtIQPolicy=AdaptiveSMTPolicy --smtTLBPolicy=AdaptiveSMTPolicy --smtPhysRegPolicy=AdaptiveSMTPolicy  --smtSQPolicy=AdaptiveSMTPolicy --smtLQPolicy=AdaptiveSMTPolicy --smtIssuePolicy=Adaptive --smtCommitPolicy=Adaptive --smtAdaptiveInterval=100000 --smtAdaptiveLimit=0.8'
    ;;
  asymmetric)
    simopts='--smtFetchPolicy=Asymmetric --smtBTBPolicy=AsymmetricSMTPolicy --smtCachePolicy=AsymmetricSMTPolicy  --smtROBPolicy=AdaptiveSMTPolicy --smtIQPolicy=AdaptiveSMTPolicy --smtTLBPolicy=AdaptiveSMTPolicy --smtPhysRegPolicy=AdaptiveSMTPolicy --smtSQPolicy=AdaptiveSMTPolicy  --smtLQPolicy=AdaptiveSMTPolicy  --smtIssuePolicy=Asymmetric --smtCommitPolicy=Asymmetric  --smtAdaptiveInterval=100000 --smtAdaptiveLimit=0.9'
    ;;
  *)
    echo "wrong scenario"
    exit
esac

outdir=results/$1__$2__$3

cd $WORKLOAD
echo running scenario $1
echo running: $bin1 $args1 xx $bin2 $args2
echo 
echo with gem5 args:
echo $simopts

mkdir $outdir
mkdir $outdir/checkpoints
$PROJ/gem5/build/X86/gem5.fast  --outdir $outdir --stats-file=stat-file.txt $PROJ/gem5/configs/example/se.py -c "$bin2;$bin1" --options="$args2;$args1" -I 100000000 --caches --cpu-type=Skylake_3 --l2cache --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8  --smt $simopts --maxinsts_threadID=0
echo $PROJ

echo done running: $bin1 $args1 XXX  $bin2 $args2
