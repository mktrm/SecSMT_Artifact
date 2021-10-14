#!/bin/bash
#
# Parameters:
#
# nopsize:   Size of NOP instructions (1 - 15)
# 
# 
# repeat1:   Number of loop repetitions
#
# repeat2:   Number of NOPs in loop
#
# nthreads:  Number of simultaneous threads


. ../vars.sh

# Compile A file if modified
if [ ../PMCTestA.cpp -nt a64.o ] ; then
    g++ -O2 -c -m64 -oa64.o ../PMCTestA.cpp
fi

echo -e "Measure, #nops, T1/T2, T2/T1, T1, T2,"

for cts in 2
do
for sender in {1..13..1}
do

echo $sender

nasm -f elf64 -o b64.o -i../ \
    -Dsender=$sender \
    -Dcounters=$cts \
    -Pcovert.nasm \
    ../TemplateB64.nasm
if [ $? -ne 0 ] ; then exit ; fi

g++ -m64 a64.o b64.o -oreceiver_$sender -lpthread
if [ $? -ne 0 ] ; then exit ; fi

#./receiver_$sender 
nasm -f elf64 -o b64.o -i../ \
    -Dsender=$sender \
    -Dcounters=$cts \
    -Pcovert.send.nasm \
    ../TemplateB64.nasm
if [ $? -ne 0 ] ; then exit ; fi

g++ -m64 a64.o b64.o -osender_$sender -lpthread
if [ $? -ne 0 ] ; then exit ; fi

#./sender_$sender   

done
done 

