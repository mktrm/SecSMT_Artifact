# format: run.sh type cryptotype jstype
#for type in adaptive partitioned symmetric shared;
for type in  adaptive asymmetric partitioned shared;
do
  for cryptotype in -aes-gcm -rsa;
  do
    for jstype in 3d-raytrace.js controlflow-recursive.js math-cordic.js regexp-dna.js string-base64.js;
    do
      echo starting $type $cryptotype $jstype
      ./run.sh $type $cryptotype $jstype 2&>1 > ${type}__${cryptotype}__${jstype}.stdoutsderr &
    done
  done
done
