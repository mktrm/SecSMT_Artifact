/*
 * Copyright (c) 2004-2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cpu/pred/assoc_btb.hh"

#include "base/intmath.hh"
#include "base/trace.hh"
#include "debug/Fetch.hh"

AssocBTB::AssocBTB(unsigned _numEntries,
                       unsigned _tagBits,
                       unsigned _instShiftAmt,
                       unsigned _num_threads,
                       BaseSMTPolicy* _smt_policy,
                       ClockedObject* _bp_unit)
    : numEntries(_numEntries),
      tagBits(_tagBits),
      instShiftAmt(_instShiftAmt),
      log2NumThreads(floorLog2(_num_threads)),
      bp_unit(_bp_unit)
{
    DPRINTF(Fetch, "BTB: Creating BTB object.\n");

    if (!isPowerOf2(numEntries)) {
        fatal("BTB entries is not a power of 2!");
    }

    dead_threshold = 10000;

    numWays = 4; 
    numSets = numEntries/numWays;

     if (!isPowerOf2(numSets)) {
        fatal("BTB sets is not a power of 2!");
    }

    btb.resize(numSets);
    for (int i = 0; i < numSets; i++){
        btb[i].resize(numWays+1);
        BaseSMTPolicy *policy = _smt_policy->clone();
        policy->init(numWays, _num_threads, _bp_unit);
        policy->reset(); 
        SMTPolicy.push_back(policy);
    }

    for (unsigned i = 0; i < numSets; ++i) {
        for (unsigned j = 0; j < numWays; ++j)
            btb[i][j].valid = false;
    }

    idxMask = numSets - 1;

    tagMask = (1 << tagBits) - 1;

    tagShiftAmt = instShiftAmt + floorLog2(numEntries);
}

void
AssocBTB::reset()
{
    for (unsigned i = 0; i < numSets; ++i) {
        for (unsigned j = 0; j < numWays; ++j)
            btb[i][j].valid = false;
    }
}

inline
unsigned
AssocBTB::getIndex(Addr instPC, ThreadID tid)
{
    // Need to shift PC over by the word offset.
    return (instPC >> instShiftAmt & idxMask);

    return ((instPC >> instShiftAmt)
            ^ (tid << (tagShiftAmt - instShiftAmt - log2NumThreads)))
            & idxMask;
}

inline
Addr
AssocBTB::getTag(Addr instPC)
{
    return (instPC >> tagShiftAmt) & tagMask;
}

bool
AssocBTB::valid(Addr instPC, ThreadID tid)
{
    unsigned btb_idx = getIndex(instPC, tid);

    Addr inst_tag = getTag(instPC);

    assert(btb_idx < numSets);

    
    for (unsigned j = 0; j < numWays; ++j) {
       if (btb[btb_idx][j].valid && 
           bp_unit->curCycle() - btb[btb_idx][j].lastAccessTime > dead_threshold){
            
            btb[btb_idx][j].valid = false;
            SMTPolicy[btb_idx]->free(btb[btb_idx][j].tid);
        }
    }

    for (unsigned j = 0; j < numWays; ++j){
        if (btb[btb_idx][j].valid
            && inst_tag == btb[btb_idx][j].tag
            && btb[btb_idx][j].tid == tid) {
            return true;
        } 
    }
    return false;
}

// @todo Create some sort of return struct that has both whether or not the
// address is valid, and also the address.  For now will just use addr = 0 to
// represent invalid entry.
TheISA::PCState
AssocBTB::lookup(Addr instPC, ThreadID tid)
{
    unsigned btb_idx = getIndex(instPC, tid);

    Addr inst_tag = getTag(instPC);

    assert(btb_idx < numEntries);
    for (unsigned j = 0; j < numWays; ++j) {
       if (btb[btb_idx][j].valid && 
           bp_unit->curCycle() - btb[btb_idx][j].lastAccessTime > dead_threshold){
            
            btb[btb_idx][j].valid = false;
            SMTPolicy[btb_idx]->free(btb[btb_idx][j].tid);
        }
    }

    for (unsigned j = 0; j < numWays; ++j) {
        if (btb[btb_idx][j].valid
            && inst_tag == btb[btb_idx][j].tag
            && btb[btb_idx][j].tid == tid) {
            btb[btb_idx][j].lastAccessTime = bp_unit->curCycle();
            return btb[btb_idx][j].target;
        } 
    
    }
    return 0;
}

void
AssocBTB::update(Addr instPC, const TheISA::PCState &target, ThreadID tid)
{
    unsigned btb_idx = getIndex(instPC, tid);
    assert(btb_idx < numSets);

    Addr inst_tag = getTag(instPC);

    //check if we can find it in BTB
    for (unsigned j = 0; j < numWays; ++j) {
        if (btb[btb_idx][j].valid == true)
         if (btb[btb_idx][j].tag == inst_tag)
            if (btb[btb_idx][j].tid == tid) {
            //update the target
            btb[btb_idx][j].target = target;
            btb[btb_idx][j].lastAccessTime = bp_unit->curCycle();
            return;
        } 
    }
    
    //need to allocate a new entry

    //every possible victim
    std::vector<int> victims; 
    for (unsigned j = 0; j < numWays; ++j) {
        victims.push_back(j);
    } 

    if (SMTPolicy[btb_idx]->isFull(tid))
    {
        // if it's full, we are the main thread
        // and the other thread has borrowed from us
        // need to take our block back!
        if (SMTPolicy[btb_idx]->borrowedFrom(tid)) //only Asymmetric
        {
            // Remove any block that belongs to us from possible victim list
            // so the only blocks that can be evicted are the ones that belong
            // to the borrowing thread
            auto i = std::begin(victims);
            while (i != std::end(victims))
            {
                if (btb[btb_idx][*i].tid == tid &&
                    btb[btb_idx][*i].valid)
                    i = victims.erase(i);
                else
                    i++;
            }
        }
        else
        {

            auto i = std::begin(victims);
            while (i != std::end(victims))
            {
                bool remove_i = false;

                // If full remove anything that does not belong to
                // this context from the list of possible victims
                // in other words, only evict this thread's blocks
                if (!(SMTPolicy[btb_idx]->evictMask(tid) & (1 << btb[btb_idx][*i].tid)))
                {
                    remove_i = true;
                }
                else
                {
                    // The set is not full for this context so we can
                    // evict and invalid line. But can we evict another
                    // context's blocks? think about it.
                    if (!btb[btb_idx][*i].valid)
                    {
                        remove_i = true;
                    }
                }
                // Actual remove
                if (remove_i)
                    i = victims.erase(i);
                else
                    i++;
            }
        }
    }



    //need to replace a block
    int LRU = 0;
    int LRU_time = bp_unit->curCycle();
    auto i = std::begin(victims);
    while (i != std::end(victims))
    {
        if (LRU_time > btb[btb_idx][*i].lastAccessTime)
        {
            LRU_time = btb[btb_idx][*i].lastAccessTime;
            LRU = *i;
        }
        i++;
    }

    if(btb[btb_idx][LRU].valid)
        SMTPolicy[btb_idx]->free(btb[btb_idx][LRU].tid);

    // the LRU var now points to the LRU
    btb[btb_idx][LRU].tid = tid;
    btb[btb_idx][LRU].valid = true;
    btb[btb_idx][LRU].target = target;
    btb[btb_idx][LRU].tag = getTag(instPC);
    btb[btb_idx][LRU].lastAccessTime = bp_unit->curCycle();
    SMTPolicy[btb_idx]->consume(tid);
}
