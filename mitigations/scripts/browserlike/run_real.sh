PROJ="$HOME/src"
bin1="$PROJ/wolfssl-4.5.0/wolfcrypt/benchmark/benchmark"
args1="-rsa"
bin2="$PROJ/duktape-2.6.0/duk"
args2=`ls *.js`
time $bin1 $args1
time $bin2 $args2
