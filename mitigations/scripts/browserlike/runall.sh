# format: run.sh type cryptotype jstype
#for type in adaptive partitioned symmetric shared;

BASE=`realpath  "$0"`
BASE="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

mkdir -p logs
for type in  adaptive asymmetric partitioned shared;
do
  for cryptotype in -aes-gcm -rsa;
  do
    for jstype in 3d-raytrace.js controlflow-recursive.js math-cordic.js regexp-dna.js string-base64.js;
    do
      echo starting $type $cryptotype $jstype 
      if [ -z "$1" ]; then
        $BASE/run.sh $BASE $type $cryptotype $jstype  > logs/${type}__${cryptotype}__${jstype}.stdoutsderr 2>&1 &
      else 
        sbatch  --partition=main -n 1 -c 1 -N 1  -o logs/${type}__${cryptotype}__${jstype}.stdout -e logs/${type}__${cryptotype}__${jstype}.stderr --job-name=${type}__${cryptotype}__${jstype} $BASE/run.sh $BASE $type $cryptotype $jstype 
      fi
    done
  done
done
