/*
 * Copyright (c) 2012-2013 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2006 The Regents of The University of Michigan
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

#include "cpu/o3/fu_pool.hh"

#include <sstream>

#include "base/debug.hh"
#include "base/trace.hh"
#include "cpu/func_unit.hh"
#include "debug/FUPool.hh"

using namespace std;

////////////////////////////////////////////////////////////////////////////
//
//  A pool of function units
//

inline void
FUPool::FUIdxQueue::addFU(int fu_idx)
{
    funcUnitsIdx.push_back(fu_idx);
    ++size;
}

inline int
FUPool::FUIdxQueue::getFU()
{
    int retval = funcUnitsIdx[idx++];

    if (idx == size)
        idx = 0;

    return retval;
}

FUPool::~FUPool()
{
    fuListIterator i = funcUnits.begin();
    fuListIterator end = funcUnits.end();
    for (; i != end; ++i)
        delete *i;
}


// Constructor
FUPool::FUPool(const Params *p)
    : SimObject(p)
{


    numFU = 0;

    funcUnits.clear();

    maxOpLatencies.fill(Cycles(0));
    pipelined.fill(true);

    //
    //  Iterate through the list of FUDescData structures
    //
    const vector<FUDesc *> &paramList =  p->FUList;
    for (FUDDiterator i = paramList.begin(); i != paramList.end(); ++i) {

        //
        //  Don't bother with this if we're not going to create any FU's
        //
        if ((*i)->number) {
            //
            //  Create the FuncUnit object from this structure
            //   - add the capabilities listed in the FU's operation
            //     description
            //
            //  We create the first unit, then duplicate it as needed
            //
            FuncUnit *fu = new FuncUnit;

            OPDDiterator j = (*i)->opDescList.begin();
            OPDDiterator end = (*i)->opDescList.end();
            for (; j != end; ++j) {
                // indicate that this pool has this capability
                capabilityList.set((*j)->opClass);

                // Add each of the FU's that will have this capability to the
                // appropriate queue.
                for (int k = 0; k < (*i)->number; ++k)
                    fuPerCapList[(*j)->opClass].addFU(numFU + k);

                // indicate that this FU has the capability
                fu->addCapability((*j)->opClass, (*j)->opLat, (*j)->pipelined);

                if ((*j)->opLat > maxOpLatencies[(*j)->opClass])
                    maxOpLatencies[(*j)->opClass] = (*j)->opLat;

                if (!(*j)->pipelined)
                    pipelined[(*j)->opClass] = false;
            }

            numFU++;

            //  Add the appropriate number of copies of this FU to the list
            fu->name = (*i)->name() + "(0)";
            funcUnits.push_back(fu);

            for (int c = 1; c < (*i)->number; ++c) {
                ostringstream s;
                numFU++;
                FuncUnit *fu2 = new FuncUnit(*fu);

                s << (*i)->name() << "(" << c << ")";
                fu2->name = s.str();
                funcUnits.push_back(fu2);
            }
        }
    }

    unitBusy.resize(numFU);

    for (int i = 0; i < numFU; i++) {
        unitBusy[i] = false;
    }
    smt_last_adaptation_cycle = Cycles(0);

    for (int i = 0; i < numFU; i++) {
        smt_t0_period.push_back(1);
        smt_period.push_back(2);
        smt_full_count[0].push_back(0);
        smt_full_count[1].push_back(0);
    }
}


void
FUPool::smtAdapt(Cycles currCyc)
{
    /*
        ... 1/4 1/3 1/2 2/3 2/4 ...
    */

    //init the last cycle
    DPRINTF(FUPool, "last adaptation cycle was = %d, this cycle=%d "
                    "smtadaptiation_inerval=%d\n", smt_last_adaptation_cycle,
                     currCyc, smt_adaptation_interval);
    if (smt_last_adaptation_cycle == 0)
        smt_last_adaptation_cycle = currCyc;

    //have not reach to adaptation cycle yet
    if (currCyc < smt_last_adaptation_cycle +  smt_adaptation_interval)
        return;

    //update the last cycle
    smt_last_adaptation_cycle = currCyc;

    for (int i = 0; i < numFU; i++) {


        //adapt the period
        if (smt_full_count[0][i] >smt_adaptation_mult * smt_full_count[1][i])
        {
            if (smt_t0_period[i] > 1 || smt_period[i] / smt_t0_period[i] == 2)
            {
                if (smt_period[i] > smt_adaptation_limit)
                    continue;
                smt_t0_period[i]++;
                smt_period[i]++;
            }
            else
            {
                assert(smt_t0_period[i] == 1);
                smt_period[i]--;
            }
        }
        else if (smt_full_count[0][i] * smt_adaptation_mult < smt_full_count[1][i])
        {
            if (smt_t0_period[i] > 1)
            {
                smt_t0_period[i]--;
                smt_period[i]--;
            }
            else
            {
                assert(smt_t0_period[i] == 1);
                if (smt_period[i] > smt_adaptation_limit)
                    continue;
                smt_period[i]++;
            }
        }

        DPRINTF(FUPool, "fu[%d] smt_t0_period=%d/%d smt_full_count=%d-%d\n",
                i, smt_t0_period[i], smt_period[i], smt_full_count[0][i],
                smt_full_count[1][i]);
        //restart the counters
        smt_full_count[0][i] = 0;
        smt_full_count[1][i] = 0;

        //sanity check
        assert(smt_t0_period[i] < smt_period[i]);
        assert(smt_t0_period[i] > 0);
        assert(smt_period[i] > 0);
    }

}

bool
FUPool::smtCanUse(int fu_idx, ThreadID tid, Cycles curCyc, int attempt, OpClass capablity)
{
    uint64_t corr_curCyc = curCyc;
    if (!isPipelined(capablity)){
        corr_curCyc = (corr_curCyc / getOpLatency(capablity)); 
    }
    if (smtPolicy == SMTIssuePolicy::SingleThread)
        return true;

    if (smtPolicy == SMTIssuePolicy::Asymmetric &&
        attempt == 1 &&
        tid != smt_main_thread) //@TODO: smt_main thread should be inversed!
    {
        return true;
    }

    assert (numThreads > 0);
    if (smtPolicy == SMTIssuePolicy::MultiplexingThreads){
        if (corr_curCyc % numThreads == tid)
            return true;
    }

    if (smtPolicy == SMTIssuePolicy::MultiplexingFUs){
        if (corr_curCyc % numThreads) // odd cycles
        {
            if (fu_idx % numThreads) // odd FUs
                return tid == 1;
            else 
                return tid == 0;

        } else { //even cycles
            if (fu_idx % numThreads) // odd FUs
                return tid == 0;
            else 
                return tid == 1;
        }
        return false;
    }

    if (smtPolicy == SMTIssuePolicy::Adaptive ||
        smtPolicy == SMTIssuePolicy::Asymmetric){
        //we are servicing only to t0 for this cycle
        if (((corr_curCyc+fu_idx) % smt_period[fu_idx]) < smt_t0_period[fu_idx]){
            if (tid==0)
                return true;
        } else {
            if (tid==1)
                return true;
        }
    }

    return false;

}
int
FUPool::getUnit(OpClass capability, ThreadID tid, Cycles curCyc, int attempt)
{
    if (smtPolicy == SMTIssuePolicy::Adaptive || 
        smtPolicy == SMTIssuePolicy::Asymmetric)
        smtAdapt(curCyc);

    //  If this pool doesn't have the specified capability,
    //  return this information to the caller
    if (!capabilityList[capability])
        return -2;


    int fu_idx = fuPerCapList[capability].getFU();
    int start_idx = fu_idx;




    // Iterate through the circular queue if needed, stopping if we've reached
    // the first element again.
    while (unitBusy[fu_idx] || !smtCanUse(fu_idx, tid, curCyc, attempt, capability)) {
        DPRINTF(FUPool, "Trying to find an available FU"
                        "unitBusy[%d]=%d, tid=%d, curCyc=%d, attempt=%d\n",
                        fu_idx, unitBusy[fu_idx], tid, curCyc, attempt);
        fu_idx = fuPerCapList[capability].getFU();
        if (fu_idx == start_idx) {
            // No FU available

            //reiterate and increase all the full counts
            fu_idx = fuPerCapList[capability].getFU();
            smt_full_count[tid][fu_idx]++;
            while ( fu_idx != start_idx ){ 
                smt_full_count[tid][fu_idx]++;
                fu_idx = fuPerCapList[capability].getFU();
            }
            DPRINTF(FUPool, "Coudn't Find an available FU\n");
            return -1;
        }
    }

    assert(fu_idx < numFU);

    assert( smtPolicy == SMTIssuePolicy:: Asymmetric || attempt==0);
    unitBusy[fu_idx] = true;
    DPRINTF(FUPool, "Functional Unit is selected idx=%d\n", fu_idx);

    return fu_idx;
}

void
FUPool::freeUnitNextCycle(int fu_idx)
{
    assert(unitBusy[fu_idx]);
    unitsToBeFreed.push_back(fu_idx);
}

void
FUPool::processFreeUnits()
{
    while (!unitsToBeFreed.empty()) {
        int fu_idx = unitsToBeFreed.back();
        unitsToBeFreed.pop_back();

        DPRINTF(FUPool, "Freeing Functional Unit [idx=%d]\n", fu_idx);
        assert(unitBusy[fu_idx]);

        unitBusy[fu_idx] = false;
    }
}

void
FUPool::dump()
{
    cout << "Function Unit Pool (" << name() << ")\n";
    cout << "======================================\n";
    cout << "Free List:\n";

    for (int i = 0; i < numFU; ++i) {
        if (unitBusy[i]) {
            continue;
        }

        cout << "  [" << i << "] : ";

        cout << funcUnits[i]->name << " ";

        cout << "\n";
    }

    cout << "======================================\n";
    cout << "Busy List:\n";
    for (int i = 0; i < numFU; ++i) {
        if (!unitBusy[i]) {
            continue;
        }

        cout << "  [" << i << "] : ";

        cout << funcUnits[i]->name << " ";

        cout << "\n";
    }
}

bool
FUPool::isDrained() const
{
    bool is_drained = true;
    for (int i = 0; i < numFU; i++)
        is_drained = is_drained && !unitBusy[i];

    return is_drained;
}

//

////////////////////////////////////////////////////////////////////////////
//
//  The SimObjects we use to get the FU information into the simulator
//
////////////////////////////////////////////////////////////////////////////

//
//    FUPool - Contails a list of FUDesc objects to make available
//

//
//  The FuPool object
//
FUPool *
FUPoolParams::create()
{
    return new FUPool(this);
}
