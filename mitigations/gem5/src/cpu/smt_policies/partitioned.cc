/*
 * Copyright (c) 2020 Kazem Taram
 * All rights reserved
*/
#include "cpu/smt_policies/partitioned.hh"

#include "debug/SMTPolicies.hh"

PartitionedSMTPolicy::PartitionedSMTPolicy(const Params* p)
    : BaseSMTPolicy(p)
{
    assert(numThreads);
    occupiedEntries = new int[numThreads];
    for (int i = 0; i < numThreads; i++)
    {
        occupiedEntries[i] = 0;
    }
}

void
PartitionedSMTPolicy::consume(ThreadID tid)
{
    DPRINTF(SMTPolicies, "consume tid=%d\n", tid);
    assert(occupiedEntries[tid] <= size / numThreads);
    occupiedEntries[tid]++;
}

void
PartitionedSMTPolicy::free(ThreadID tid)
{
    DPRINTF(SMTPolicies, "free tid=%d\n", tid);
    assert(occupiedEntries[tid] > 0);
    occupiedEntries[tid]--;
    if (occupiedEntries[tid]<0)
        occupiedEntries = 0;
}

void
PartitionedSMTPolicy::reset()
{
    for (int i = 0; i < numThreads; i++)
    {
        occupiedEntries[i] = 0;
    }
}

bool
PartitionedSMTPolicy::isFull(ThreadID tid)
{
    DPRINTF(SMTPolicies, "isFull tid=%d occupied=%d size=%d\n", 
                            tid, occupiedEntries[tid], size/numThreads );
    if (occupiedEntries[tid] >= size / numThreads)
        return true;
    return false;
}

unsigned
PartitionedSMTPolicy::numFree(ThreadID tid)
{
    DPRINTF(SMTPolicies, "numFree tid=%d occupied=%d size=%d\n", 
                            tid, occupiedEntries[tid], size/numThreads );
    assert(numThreads>0);
    assert((size / numThreads - occupiedEntries[tid]) >= 0);
    return size / numThreads - occupiedEntries[tid];
}

unsigned
PartitionedSMTPolicy::evictMask(ThreadID tid)
{
    assert(tid<numThreads);
    return (0x1<<(tid));
}

void
PartitionedSMTPolicy::init (int _size, int _numThreads, ClockedObject* _cpu)
{
    size = _size;
    numThreads = _numThreads;
    cpu = _cpu;
    occupiedEntries = new int[numThreads];
    for (int i = 0; i < numThreads; i++)
    {
        occupiedEntries[i] = 0;
    }

    DPRINTF(SMTPolicies, "init size=%d numThreads=%d\n", _size, _numThreads);
}


PartitionedSMTPolicy*
PartitionedSMTPolicyParams::create()
{
    return new PartitionedSMTPolicy(this);
}
