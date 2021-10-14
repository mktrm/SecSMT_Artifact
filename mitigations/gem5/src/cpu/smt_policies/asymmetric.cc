/*
 * Copyright (c) 2020 Kazem Taram
 * All rights reserved
*/
#include "cpu/smt_policies/asymmetric.hh"

AsymmetricSMTPolicy::AsymmetricSMTPolicy(const Params* p)
    : BaseSMTPolicy(p), step(p->step), limit(p->limit),
    interval(p->interval), mainThread(p->main_thread),
    maxBorrow(p->max_borrow), minFree(p->min_free)
{
    assert(numThreads);
    occupiedEntries = new int[numThreads];
    fullCounts = new int[numThreads];
    maxEntries = new int[numThreads];
    for (int i = 0; i < numThreads; i++)
    {
        occupiedEntries[i] = 0;

        //uniformly partition by number of threads
        maxEntries[i] = size/numThreads;
        fullCounts[i] = 0;
    }

    lastAdaptationCycle = Cycles(0);

    //make sure not to limit for single thread
    if (numThreads == 1)
        limit = 1.0;
    cpu = NULL;
    numBorrowed = 0;
    squashing = 0;
}

void
AsymmetricSMTPolicy::adapt()
{
    if (!cpu) return;
    DPRINTF(SMTPolicies, "check adaptation period. last=%llu now=%llu\n", 
                                lastAdaptationCycle, cpu->curCycle());
    DPRINTF(SMTPolicies, "adapt size=%d numThreads=%d period=%d\n", size, numThreads, interval);
    if (lastAdaptationCycle == 0)
        lastAdaptationCycle = cpu->curCycle();
    if ( cpu->curCycle()  < lastAdaptationCycle + interval)
        return;

    DPRINTF(SMTPolicies, "adapting fullCounts[%d]=%d fullCounts[%d]=%d\n",
            0, fullCounts[0], 1, fullCounts[1]);
    DPRINTF(SMTPolicies, "adapting occupiedEntries[%d]=%d"
            "occupiedEntries[%d]=%d\n", 0, occupiedEntries[0],
             1, occupiedEntries[1]);
    DPRINTF(SMTPolicies, "adapting maxEntries[%d]=%d maxEntries[%d]=%d\n",
            0, maxEntries[0], 1, maxEntries[1]);
    //should move maxEnries here based on counters:
    //Currently only supports 2 threads
    assert (numThreads == 2);
    lastAdaptationCycle = cpu->curCycle();

    // do not need any adaptation
    if (fullCounts[0] == fullCounts[1])
        return;

    //find which parition to expand
    int to_be_shrunk = 0;
    int to_be_expanded = 1;
    if (fullCounts[0] > fullCounts[1])
    {
        to_be_shrunk = 1;
        to_be_expanded = 0;
    }
    fullCounts[0] = fullCounts[1] = 0;

    int max_allowed = limit*size;
    DPRINTF(SMTPolicies, "adapting maxEntries[%d]=%d ?> %d\n",
        to_be_expanded, maxEntries[to_be_expanded], max_allowed);
    // if there is room, expand the partition
    if ( maxEntries[to_be_expanded] + step < max_allowed &&
         maxEntries[to_be_shrunk]   - step >= 0 &&
         occupiedEntries[to_be_shrunk] <= maxEntries[to_be_shrunk] - step)
    {
        maxEntries[to_be_shrunk] -= step;
        maxEntries[to_be_expanded] += step;
        if (numBorrowed > 0){
            if (to_be_expanded != mainThread)
                numBorrowed-=step;
            if (numBorrowed < 0)
                numBorrowed = 0;
            //think about the other case
        }
        DPRINTF(SMTPolicies, "shrinking partition=%d and "
                             "expanding partition=%d\n",
                             to_be_shrunk, to_be_expanded);
    }

    DPRINTF(SMTPolicies, "adapting: maxEntries[%d]=%d  "
                         "maxEntries[%d]=%d\n",
                         to_be_shrunk, maxEntries[to_be_shrunk],
                         to_be_expanded, maxEntries[to_be_expanded]);
    //make sure the size of each partition make sense
    assert(maxEntries[to_be_shrunk] >= 0);
    assert(maxEntries[to_be_expanded] + maxEntries[to_be_shrunk] <= size);
}

void
AsymmetricSMTPolicy::consume(ThreadID tid)
{
    DPRINTF(SMTPolicies, "Asymmetric consume tid=%d maxEnt=%d occupied=%d"
                          "minFree=%d maxBorrow=%d numBorrowed=%d\n", 
                        tid, maxEntries[tid], occupiedEntries[tid], minFree, 
                        maxBorrow, numBorrowed);
    assert(occupiedEntries[tid] <= maxEntries[tid] + maxBorrow+1);

    // can borrow
    if(tid != mainThread){
        
        // if full
        if (occupiedEntries[tid] >= maxEntries[tid]){
            numBorrowed ++;
            totalBorrowed++;// increease Borrowed Stats
        }

    } else { //cannot borrow
        //
        if (numBorrowed > 0 && squashing==0){
            numSquashed++; //increase Squashed Stats;
            squashing = 1;
        }
    } 

    //occupy one entry
    occupiedEntries[tid]++;

    //update the adaptive full counters
    if (occupiedEntries[tid] == maxEntries[tid])
        fullCounts[tid]++;
    
    
}

void
AsymmetricSMTPolicy::free(ThreadID tid)
{
    DPRINTF(SMTPolicies, "free tid=%d maxEnt=%d occupied=%d maxBorrow=%d\n", 
                tid, maxEntries[tid], occupiedEntries[tid], maxBorrow);
    assert(occupiedEntries[tid] > 0);
    occupiedEntries[tid]--;
    if (occupiedEntries[tid]<0)
        occupiedEntries = 0;
    if(tid != mainThread){
        if (numBorrowed > 0) 
            numBorrowed--;
        else
            squashing = 0;
    } 
    adapt();
}

void
AsymmetricSMTPolicy::reset()
{
    DPRINTF(SMTPolicies, "reset \n");
    for (int i = 0; i < numThreads; i++)
    {
        occupiedEntries[i] = 0;
        maxEntries[i] = size/numThreads;
        fullCounts[i] = 0;
    }
    lastAdaptationCycle = Cycles(0);


}

bool
AsymmetricSMTPolicy::isFull(ThreadID tid)
{
    DPRINTF(SMTPolicies, "isFull tid=%d\n", tid);
    assert(numThreads == 2);
    assert (maxEntries[0] +  maxEntries[1] <= size);

    // //say it is full if we are shrinking but havn't freed
    // for (int i = 0; i < numThreads; i++){
    //     if (occupiedEntries[i] > maxEntries[i])
    //         return true;
    // }

    // we are full 
    if (occupiedEntries[tid] >= maxEntries[tid])
    {
        fullCounts[tid]++;

        // if this thread can borrow
        if (tid != mainThread)
        {
            // make sure we don't use more than maxBorrow
            if (numBorrowed < maxBorrow)
            {
                // make sure the other thread has at least min_free free items
                int frees = maxEntries[mainThread] - occupiedEntries[mainThread];
                if (frees > minFree)
                    return false;
            }
        } else {
            // the other thread has borrowed from us
            // and we are taking it back!
            if (numBorrowed > 0)
                numSquashed ++;
        }


        // if this thread cannot borrow 
        // or borrowing conditions are not met
        return true;
    }
    
    // if we reach here, it's not full
    return false;
}

bool
AsymmetricSMTPolicy::borrowedFrom(ThreadID tid)
{
   DPRINTF(SMTPolicies, "check if someone has borrowed form tid=%d \n", tid); 
   DPRINTF(SMTPolicies, "borrowedFrom=%d\n", tid == mainThread && numBorrowed>0); 
   return  ( tid == mainThread && numBorrowed>0 );
}

unsigned
AsymmetricSMTPolicy::numFree(ThreadID tid)
{
    assert(numThreads>0);
    if (occupiedEntries[tid] >= maxEntries[tid]) fullCounts[tid]++;

    // return 0 if we are shrinking
    // for (int i = 0; i < numThreads; i++){
    //     if (occupiedEntries[i] > maxEntries[i])
    //         return 0;
    // }

    if (tid != mainThread){
        
        //make sure we don't use more than maxBorrow
        if (numBorrowed < maxBorrow){
            int frees = maxEntries[mainThread] - occupiedEntries[mainThread];

            //make sure we spare at least 10 entries for the other thread
            frees -= minFree;
            if (frees < 0)
                frees =0;
            
            int to_return =(maxEntries[tid] - occupiedEntries[tid] + frees);
            if (to_return < 0) 
                to_return = 0;
            DPRINTF(SMTPolicies, "numFree [tid:%d]=%d\n", tid, to_return);
            return to_return;

        } else {
            DPRINTF(SMTPolicies, "numFree [tid:%d]=%d\n", tid, 0);
            return 0;
        }
    }
    int to_return = maxEntries[tid]  - occupiedEntries[tid] - numBorrowed;
    if (to_return < 0)
        to_return = 0;
    DPRINTF(SMTPolicies, "numFree [tid:%d]=%d\n", tid, to_return);
    if (numBorrowed > 0 &&  to_return <= 0)
        numSquashed++;

    return to_return;
}

unsigned
AsymmetricSMTPolicy::evictMask(ThreadID tid)
{
    assert(tid<numThreads);
    return (0x1<<(tid));
}

void
AsymmetricSMTPolicy::init (int _size, int _numThreads, ClockedObject* _cpu)
{
    size = _size; 
    numThreads = _numThreads;
    cpu = _cpu;

    occupiedEntries = new int[numThreads];
    maxEntries = new int[numThreads];
    fullCounts = new int[numThreads];
    for (int i = 0; i < numThreads; i++)
    {
        occupiedEntries[i] = 0;
        fullCounts[i] = 0;
        maxEntries[i] = size/numThreads;
    }
    lastAdaptationCycle = Cycles(0);

    //make sure not to limit for single thread
    if (numThreads == 1)
        limit = 1.0;

    numBorrowed = 0;
    DPRINTF(SMTPolicies, "init size=%d numThreads=%d interval=%d limit=%d minFree=%d \n", _size, _numThreads, interval, limit, minFree);
} 

void
AsymmetricSMTPolicy::regStats(){
   using namespace Stats;
    numSquashed
        .name(name() + ".numSquashed")
        .desc("Number of time borrowing caused squash")
        .prereq(numSquashed);
    
    totalBorrowed
        .name(name() + ".totalBorrowed")
        .desc("Number of time borrowing ")
        .prereq(totalBorrowed);
}

AsymmetricSMTPolicy*
AsymmetricSMTPolicyParams::create()
{
    return new AsymmetricSMTPolicy(this);
}
