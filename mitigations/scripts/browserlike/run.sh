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

HOME=/u/xr5ry/
PROJ="/u/mtaram/"
bin1="$HOME/src/wolfssl-4.5.0/wolfcrypt/benchmark/benchmark"
args1=$2
bin2="$HOME/src/duktape-2.6.0/duk"
args2=$3
args2list="3d-raytrace.js controlflow-recursive.js math-cordic.js regexp-dna.js string-base64.js"

case "$1" in
  
  shared)
#    outdir=shared
    simpots=''
    ;;
  partitioned)
#    outdir=partitioned
    simopts='--smtFetchPolicy=StrictRoundRobin --smtBTBPolicy=PartitionedSMTPolicy --smtCachePolicy=PartitionedSMTPolicy --smtROBPolicy=PartitionedSMTPolicy --smtIQPolicy=PartitionedSMTPolicy --smtTLBPolicy=PartitionedSMTPolicy --smtPhysRegPolicy=PartitionedSMTPolicy --smtSQPolicy=PartitionedSMTPolicy --smtLQPolicy=PartitionedSMTPolicy --smtCommitPolicy=StrictRoundRobin --smtIssuePolicy=MultiplexingFUs'
    ;;
  adaptive)
#    outdir=adaptive
    simopts='--smtFetchPolicy=Adaptive --smtBTBPolicy=AdaptiveSMTPolicy --smtCachePolicy=AdaptiveSMTPolicy --smtROBPolicy=AdaptiveSMTPolicy --smtIQPolicy=AdaptiveSMTPolicy --smtTLBPolicy=AdaptiveSMTPolicy --smtPhysRegPolicy=AdaptiveSMTPolicy  --smtSQPolicy=AdaptiveSMTPolicy --smtLQPolicy=AdaptiveSMTPolicy --smtIssuePolicy=Adaptive --smtCommitPolicy=Adaptive --smtAdaptiveInterval=100000 --smtAdaptiveLimit=0.8'
    ;;
  symmetric)
#    outdir=symmetric
    simopts='--smtFetchPolicy=Adaptive --smtBTBPolicy=AdaptiveSMTPolicy --smtCachePolicy=AdaptiveSMTPolicy  --smtROBPolicy=AdaptiveSMTPolicy --smtIQPolicy=AdaptiveSMTPolicy --smtTLBPolicy=AdaptiveSMTPolicy --smtPhysRegPolicy=AdaptiveSMTPolicy --smtSQPolicy=AdaptiveSMTPolicy --smtLQPolicy=AdaptiveSMTPolicy  --smtIssuePolicy=Asymmetric --smtCommitPolicy=Asymmetric--smtAdaptiveInterval=100000 --smtAdaptiveLimit=0.9'
    ;;
  symmetric2)
#    outdir=symmetric
    simopts='--smtFetchPolicy=Adaptive --smtBTBPolicy=AsymmetricSMTPolicy --smtCachePolicy=AdaptiveSMTPolicy  --smtROBPolicy=AdaptiveSMTPolicy --smtIQPolicy=AdaptiveSMTPolicy --smtTLBPolicy=AdaptiveSMTPolicy --smtPhysRegPolicy=AdaptiveSMTPolicy --smtSQPolicy=AdaptiveSMTPolicy  --smtLQPolicy=AdaptiveSMTPolicy  --smtIssuePolicy=Asymmetric --smtCommitPolicy=Asymmetric --smtAdaptiveInterval=100000 --smtAdaptiveLimit=0.9'
    ;;
  Csymmetric3)
#    outdir=symmetric
    simopts='--smtFetchPolicy=Adaptive --smtBTBPolicy=AsymmetricSMTPolicy --smtCachePolicy=AsymmetricSMTPolicy  --smtROBPolicy=AdaptiveSMTPolicy --smtIQPolicy=AdaptiveSMTPolicy --smtTLBPolicy=AdaptiveSMTPolicy --smtPhysRegPolicy=AdaptiveSMTPolicy --smtSQPolicy=AdaptiveSMTPolicy  --smtLQPolicy=AdaptiveSMTPolicy  --smtIssuePolicy=Asymmetric --smtCommitPolicy=Asymmetric --smtAdaptiveInterval=100000 --smtAdaptiveLimit=0.9'
    ;;
  asymmetric)
#    outdir=symmetric
    simopts='--smtFetchPolicy=Asymmetric --smtBTBPolicy=AsymmetricSMTPolicy --smtCachePolicy=AsymmetricSMTPolicy  --smtROBPolicy=AdaptiveSMTPolicy --smtIQPolicy=AdaptiveSMTPolicy --smtTLBPolicy=AdaptiveSMTPolicy --smtPhysRegPolicy=AdaptiveSMTPolicy --smtSQPolicy=AdaptiveSMTPolicy  --smtLQPolicy=AdaptiveSMTPolicy  --smtIssuePolicy=Asymmetric --smtCommitPolicy=Asymmetric  --smtAdaptiveInterval=100000 --smtAdaptiveLimit=0.9'
    ;;
  *)
    echo "wrong scenario"
    exit
esac

outdir=$1__$2__$3

echo running scenario $1
echo running: $bin1 $args1 xx $bin2 $args2
echo 
echo with gem5 args:
echo $simopts

mkdir $outdir
mkdir $outdir/checkpoints
# simulate for some 6~8 hours and take 100 checkpoints over that time
$PROJ/gem5-smt/build/X86/gem5.fast  --outdir $outdir --stats-file=stat-file.txt $PROJ/gem5-smt/configs/example/se.py -c "$bin2;$bin1" --options="$args2;$args1" -I 100000000 --caches --cpu-type=Skylake_3 --l2cache --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8  --smt $simopts --maxinsts_threadID=0
echo $PROJ

echo done running: $bin1 $args1 XXX  $bin2 $args2
