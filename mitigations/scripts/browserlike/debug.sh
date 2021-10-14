 /u/mtaram//gem5-smt/build/X86/gem5.fast --outdir debug_asymmetric --stats-file=stat-file2.txt /u/mtaram//gem5-smt/configs/example/se.py \
    -c '/u/xr5ry//src/duktape-2.6.0/duk;/u/xr5ry//src/wolfssl-4.5.0/wolfcrypt/benchmark/benchmark' '--options=3d-raytrace.js;-aes-gcm' -I 100000000 \
    --caches --cpu-type=Skylake_3 --l2cache --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --smt --smtFetchPolicy=Asymmetric --smtBTBPolicy=AsymmetricSMTPolicy \
    --smtCachePolicy=AsymmetricSMTPolicy --smtROBPolicy=AdaptiveSMTPolicy --smtIQPolicy=AdaptiveSMTPolicy --smtIQPolicy-limit=0.8 --smtTLBPolicy=AdaptiveSMTPolicy --smtPhysRegPolicy=AdaptiveSMTPolicy \
    --smtSQPolicy=AdaptiveSMTPolicy --smtLQPolicy=AdaptiveSMTPolicy --smtIssuePolicy=Asymmetric --smtIssuePolicy-limit=10 --smtAdaptiveInterval=100000 --smtCommitPolicy=Adaptive --smtAdaptiveLimit=0.9 --maxinsts_threadID=0  

exit 0 

 /u/mtaram//gem5-smt/build/X86/gem5.opt --debug-start=20000018098261500 --debug-flags=Fetch,Exec  --outdir debug_adaptive --stats-file=stat-file2.txt /u/mtaram//gem5-smt/configs/example/se.py \
    -c '/u/xr5ry//src/duktape-2.6.0/duk;/u/xr5ry//src/wolfssl-4.5.0/wolfcrypt/benchmark/benchmark' '--options=3d-raytrace.js;-aes-gcm' -I 100000000 \
    --caches --cpu-type=Skylake_3 --l2cache --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --smt --smtFetchPolicy=Adaptive --smtBTBPolicy=AsymmetricSMTPolicy \
    --smtCachePolicy=AsymmetricSMTPolicy --smtROBPolicy=AdaptiveSMTPolicy --smtIQPolicy=AdaptiveSMTPolicy --smtTLBPolicy=AdaptiveSMTPolicy --smtPhysRegPolicy=AdaptiveSMTPolicy \
    --smtSQPolicy=AdaptiveSMTPolicy --smtLQPolicy=AdaptiveSMTPolicy --smtIssuePolicy=Asymmetric --smtAdaptiveInterval=100000 --smtAdaptiveLimit=0.9 --maxinsts_threadID=0  > adaptive.log &

 #/u/mtaram//gem5-smt/build/X86/gem5.opt --debug-start=0  --outdir C3symmetric3__-aes-gcm__3d-raytrace.js --stats-file=stat-file.txt /u/mtaram//gem5-smt/configs/example/se.py -c '/u/xr5ry//src/duktape-2.6.0/duk;/u/xr5ry//src/wolfssl-4.5.0/wolfcrypt/benchmark/benchmark' '--options=3d-raytrace.js;-aes-gcm' -I 2000000 --caches --cpu-type=Skylake_3 --l2cache --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --smt --smtFetchPolicy=Adaptive --smtBTBPolicy=AsymmetricSMTPolicy --smtCachePolicy=AdaptiveSMTPolicy --smtROBPolicy=AdaptiveSMTPolicy --smtIQPolicy=AdaptiveSMTPolicy --smtTLBPolicy=AdaptiveSMTPolicy --smtPhysRegPolicy=AdaptiveSMTPolicy --smtSQPolicy=AdaptiveSMTPolicy --smtLQPolicy=AdaptiveSMTPolicy --smtIssuePolicy=Asymmetric --smtAdaptiveInterval=2000000 --smtAdaptiveLimit=0.9 --maxinsts_threadID=0 > adaptive.log 
 #/u/mtaram//gem5-smt/build/X86/gem5.opt --debug-start=2881812500 --debug-flags=Exec --outdir Cshared__-aes-gcm__3d-raytrace.js --stats-file=stat-file.txt /u/mtaram//gem5-smt/configs/example/se.py -c '/u/xr5ry//src/duktape-2.6.0/duk;/u/xr5ry//src/wolfssl-4.5.0/wolfcrypt/benchmark/benchmark' '--options=3d-raytrace.js;-aes-gcm' -I 20000000 --caches --cpu-type=Skylake_3 --l2cache --l1d_size=32kB --l1d_assoc=8 --l1i_size=32kB --l1i_assoc=8 --smt --maxinsts_threadID=0 > shared.log 
 #O3CPUAll,LSQUnit,LSQ,CacheAll,SMTPolicies2230368500
