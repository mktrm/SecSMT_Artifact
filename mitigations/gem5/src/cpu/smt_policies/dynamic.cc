/*
 * Copyright (c) 2020 Kazem Taram
 * All rights reserved
*/
#include "cpu/smt_policies/dynamic.hh"

DynamicSMTPolicy::DynamicSMTPolicy(const Params* p)
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
DynamicSMTPolicy::consume(ThreadID tid)
{
    //assert(occupiedEntries[tid] < size / 2);
    occupiedEntries[tid]++;
}

void
DynamicSMTPolicy::free(ThreadID tid)
{
    //assert(occupiedEntries[tid] > 0);
    occupiedEntries[tid]--;
}

void
DynamicSMTPolicy::reset()
{
    for (int i = 0; i < numThreads; i++)
    {
        occupiedEntries[i] = 0;
    }
}

int
DynamicSMTPolicy::sumFull()
{
    int sumFull = 0;
    for (int tid = 0; tid < numThreads; tid++)
        sumFull += occupiedEntries [tid];
    return sumFull;
}

bool
DynamicSMTPolicy::isFull(ThreadID tid)
{
    if (sumFull() >= size)
        return true;
    return false;
}

unsigned
DynamicSMTPolicy::numFree(ThreadID tid)
{
    assert((size - sumFull()) >= 0);
    return size - sumFull();
}

unsigned
DynamicSMTPolicy::evictMask(ThreadID tid)
{
    return (0x1<<numThreads)-1;
}


void
DynamicSMTPolicy::init (int _size, int _numThreads, ClockedObject* _cpu)
{
    size = _size;
    numThreads = _numThreads;
    cpu = _cpu;
    occupiedEntries = new int[numThreads];
    for (int i = 0; i < numThreads; i++)
    {
        occupiedEntries[i] = 0;
    }
}

DynamicSMTPolicy*
DynamicSMTPolicyParams::create()
{
    return new DynamicSMTPolicy(this);
}

