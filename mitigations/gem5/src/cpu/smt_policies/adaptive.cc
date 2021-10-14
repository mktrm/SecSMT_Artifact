/*
 * Copyright (c) 2020 Kazem Taram
 * All rights reserved
*/
#include "cpu/smt_policies/adaptive.hh"

AdaptiveSMTPolicy::AdaptiveSMTPolicy(const Params* p)
    : BaseSMTPolicy(p), step(p->step), limit(p->limit),
    interval(p->interval)
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
}

void
AdaptiveSMTPolicy::adapt()
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
AdaptiveSMTPolicy::consume(ThreadID tid)
{
    DPRINTF(SMTPolicies, "consume tid=%d occupied=%d\n", tid, occupiedEntries[tid]);
    assert(occupiedEntries[tid] <= maxEntries[tid]);
    occupiedEntries[tid]++;

    if (occupiedEntries[tid] == maxEntries[tid])
        fullCounts[tid]++;
}

void
AdaptiveSMTPolicy::free(ThreadID tid)
{
    DPRINTF(SMTPolicies, "free tid=%d occupied=%d\n", tid, occupiedEntries[tid]);
    assert(occupiedEntries[tid] > 0);
    occupiedEntries[tid]--;
    if (occupiedEntries[tid]<0)
        occupiedEntries = 0;
    adapt();
}

void
AdaptiveSMTPolicy::reset()
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
AdaptiveSMTPolicy::isFull(ThreadID tid)
{
    DPRINTF(SMTPolicies, "isFull tid=%d occupied=%d\n", tid, occupiedEntries[tid]);
    assert(numThreads == 2);
    assert (maxEntries[0] +  maxEntries[1] <= size);

    // //say it is full if we are shrinking but havn't freed
    // for (int i = 0; i < numThreads; i++){
    //     if (occupiedEntries[i] > maxEntries[i])
    //         return true;
    // }

    if (occupiedEntries[tid] >= maxEntries[tid])
    {
        fullCounts[tid]++;
        return true;
    }
    return false;
}

unsigned
AdaptiveSMTPolicy::numFree(ThreadID tid)
{
    DPRINTF(SMTPolicies, "numFree tid=%d\n", tid);
    assert(numThreads>0);
    if (occupiedEntries[tid] >= maxEntries[tid]) fullCounts[tid]++;

    // return 0 if we are shrinking
    // for (int i = 0; i < numThreads; i++){
    //     if (occupiedEntries[i] > maxEntries[i])
    //         return 0;
    // }

    return maxEntries[tid]  - occupiedEntries[tid];
}

unsigned
AdaptiveSMTPolicy::evictMask(ThreadID tid)
{
    assert(tid<numThreads);
    return (0x1<<(tid));
}

void
AdaptiveSMTPolicy::init (int _size, int _numThreads, ClockedObject* _cpu)
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

    DPRINTF(SMTPolicies, "init size=%d numThreads=%d period=%d\n", _size, _numThreads, interval);
}


AdaptiveSMTPolicy*
AdaptiveSMTPolicyParams::create()
{
    return new AdaptiveSMTPolicy(this);
}
