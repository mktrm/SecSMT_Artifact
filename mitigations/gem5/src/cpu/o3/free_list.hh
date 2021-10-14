/*
 * Copyright (c) 2016-2018 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder. You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2004-2005 The Regents of The University of Michigan
 * Copyright (c) 2013 Advanced Micro Devices, Inc.
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

#ifndef __CPU_O3_FREE_LIST_HH__
#define __CPU_O3_FREE_LIST_HH__

#include <iostream>
#include <queue>
#include <set>
#include <vector>

#include "base/logging.hh"
#include "base/trace.hh"
#include "cpu/o3/comm.hh"
#include "cpu/o3/regfile.hh"
#include "cpu/smt_policies/base.hh"
#include "debug/FreeList.hh"

/**
 * Free list for a single class of registers (e.g., integer
 * or floating point).  Because the register class is implicitly
 * determined by the rename map instance being accessed, all
 * architectural register index parameters and values in this class
 * are relative (e.g., %fp2 is just index 2).
 */
class SimpleFreeList
{
  private:

    /** The actual free list */
    std::queue<PhysRegIdPtr> freeRegs;
    std::set<PhysRegIdPtr> freeSet;

    BaseSMTPolicy * policy;
  public:

    SimpleFreeList(BaseSMTPolicy * _policy) : policy(_policy)
    {};

    /** Add a physical register to the free list */
    void addReg(PhysRegIdPtr reg, ThreadID tid) {
        if (freeSet.find(reg) == freeSet.end()){
            freeRegs.push(reg);
            freeSet.insert((reg));
            if (policy)
                policy->free(tid);
            DPRINTF(FreeList,"Actually Freeing register %i (%s).\n",
                    reg->index(), reg->className());
        }
    }

    /** Add physical registers to the free list */
    /** Big assumption for SMT: this function is the only place
     * for initialization && all the initialization only go through
     * this function. So, we do not consider it as resource consumption */
    template<class InputIt>
    void
    addRegs(InputIt first, InputIt last) {
        std::for_each(first, last, [this](typename InputIt::value_type& reg) {
            if (freeSet.find(&reg) == freeSet.end()){
                freeRegs.push(&reg);
                freeSet.insert(&reg);
            }
        });
    }

    /** Get the next available register from the free list */
    PhysRegIdPtr getReg(ThreadID tid)
    {
        assert(!freeRegs.empty());
        PhysRegIdPtr free_reg = freeRegs.front();
        freeRegs.pop();
        freeSet.erase((free_reg));
        if (policy)
            policy->consume(tid);
        DPRINTF(FreeList,"Removing Free register %i (%s).\n",
                free_reg->index(), free_reg->className());
        return free_reg;
    }

    /** Return the number of free registers on the list. */
    unsigned numFreeRegs(ThreadID tid) const {
        if (policy)
            return policy->numFree(tid);
        else
            return freeRegs.size();
    }

    /** True iff there are free registers on the list. */
    bool hasFreeRegs(ThreadID tid) const {
        if (policy)
            return !policy->isFull(tid);
        else
            return !freeRegs.empty();
    }
};


/**
 * FreeList class that simply holds the list of free integer and floating
 * point registers.  Can request for a free register of either type, and
 * also send back free registers of either type.  This is a very simple
 * class, but it should be sufficient for most implementations.  Like all
 * other classes, it assumes that the indices for the floating point
 * registers starts after the integer registers end.  Hence the variable
 * numPhysicalIntRegs is logically equivalent to the baseFP dependency.
 * Note that while this most likely should be called FreeList, the name
 * "FreeList" is used in a typedef within the CPU Policy, and therefore no
 * class can be named simply "FreeList".
 * @todo: Give a better name to the base FP dependency.
 */
class UnifiedFreeList
{
  private:

    /** The object name, for DPRINTF.  We have to declare this
     *  explicitly because Scoreboard is not a SimObject. */
    const std::string _name;

    /** The list of free integer registers. */
    SimpleFreeList* intList;

    /** The list of free floating point registers. */
    SimpleFreeList* floatList;

    /** The following two are exclusive interfaces. */
    /** @{ */
    /** The list of free vector registers. */
    SimpleFreeList* vecList;

    /** The list of free vector element registers. */
    SimpleFreeList* vecElemList;
    /** @} */

    /** The list of free predicate registers. */
    SimpleFreeList* predList;

    /** The list of free condition-code registers. */
    SimpleFreeList* ccList;

    /**
     * The register file object is used only to distinguish integer
     * from floating-point physical register indices.
     */
    PhysRegFile *regFile;

    /*
     * We give UnifiedRenameMap internal access so it can get at the
     * internal per-class free lists and associate those with its
     * per-class rename maps. See UnifiedRenameMap::init().
     */
    friend class UnifiedRenameMap;

  public:
    /** Constructs a free list.
     *  @param _numPhysicalIntRegs Number of physical integer registers.
     *  @param reservedIntRegs Number of integer registers already
     *                         used by initial mappings.
     *  @param _numPhysicalFloatRegs Number of physical fp registers.
     *  @param reservedFloatRegs Number of fp registers already
     *                           used by initial mappings.
     */
    UnifiedFreeList(const std::string &_my_name, PhysRegFile *_regFile,
                     BaseSMTPolicy* intPolicy,
                     BaseSMTPolicy* floatPolicy,
                     BaseSMTPolicy* vecPolicy);

    /** Gives the name of the freelist. */
    std::string name() const { return _name; };

    /** Returns a pointer to the condition-code free list */
    SimpleFreeList *getCCList() { return ccList; }

    /** Gets a free integer register. */
    PhysRegIdPtr getIntReg(ThreadID tid) { return intList->getReg(tid); }

    /** Gets a free fp register. */
    PhysRegIdPtr getFloatReg(ThreadID tid) { return floatList->getReg(tid); }

    /** Gets a free vector register. */
    PhysRegIdPtr getVecReg(ThreadID tid) { return vecList->getReg(tid); }

    /** Gets a free vector elemenet register. */
    PhysRegIdPtr getVecElem(ThreadID tid) { return vecElemList->getReg(tid); }

    /** Gets a free predicate register. */
    PhysRegIdPtr getVecPredReg(ThreadID tid) { return predList->getReg(tid); }

    /** Gets a free cc register. */
    PhysRegIdPtr getCCReg(ThreadID tid) { return ccList->getReg(tid); }

    /** Adds a register back to the free list. */
    void addReg(PhysRegIdPtr freed_reg, ThreadID tid);

    /** Adds a register back to the free list. */
    template<class InputIt>
    void addRegs(InputIt first, InputIt last);

    /** Adds an integer register back to the free list. */
    void addIntReg(PhysRegIdPtr freed_reg, ThreadID tid)
    {
        intList->addReg(freed_reg, tid);
    }

    /** Adds a fp register back to the free list. */
    void addFloatReg(PhysRegIdPtr freed_reg, ThreadID tid)
    {
        floatList->addReg(freed_reg, tid);
    }

    /** Adds a vector register back to the free list. */
    void addVecReg(PhysRegIdPtr freed_reg, ThreadID tid)
    {
        vecList->addReg(freed_reg, tid);
    }

    /** Adds a vector element register back to the free list. */
    void addVecElem(PhysRegIdPtr freed_reg, ThreadID tid)
    {
        vecElemList->addReg(freed_reg, tid);
    }

    /** Adds a predicate register back to the free list. */
    void addVecPredReg(PhysRegIdPtr freed_reg, ThreadID tid)
    {
        predList->addReg(freed_reg, tid);
    }

    /** Adds a cc register back to the free list. */
    void addCCReg(PhysRegIdPtr freed_reg, ThreadID tid)
    {
        ccList->addReg(freed_reg, tid);
    }

    /** Checks if there are any free integer registers. */
    bool hasFreeIntRegs(ThreadID tid) const
    {
        return intList->hasFreeRegs(tid);
    }

    /** Checks if there are any free fp registers. */
    bool hasFreeFloatRegs(ThreadID tid) const
    {
        return floatList->hasFreeRegs(tid);
    }

    /** Checks if there are any free vector registers. */
    bool hasFreeVecRegs(ThreadID tid) const
    {
         return vecList->hasFreeRegs(tid);
    }

    /** Checks if there are any free vector registers. */
    bool hasFreeVecElems(ThreadID tid) const
    {
        return vecElemList->hasFreeRegs(tid);
    }

    /** Checks if there are any free predicate registers. */
    bool hasFreeVecPredRegs(ThreadID tid) const
    {
        return predList->hasFreeRegs(tid);
    }

    /** Checks if there are any free cc registers. */
    bool hasFreeCCRegs(ThreadID tid) const
    {
        return ccList->hasFreeRegs(tid);
    }

    /** Returns the number of free integer registers. */
    unsigned numFreeIntRegs(ThreadID tid) const
    {
        return intList->numFreeRegs(tid);
    }

    /** Returns the number of free fp registers. */
    unsigned numFreeFloatRegs(ThreadID tid) const
    {
        return floatList->numFreeRegs(tid);
    }

    /** Returns the number of free vector registers. */
    unsigned numFreeVecRegs(ThreadID tid) const
    {
        return vecList->numFreeRegs(tid);
    }

    /** Returns the number of free vector registers. */
    unsigned numFreeVecElems(ThreadID tid) const
    {
        return vecElemList->numFreeRegs(tid);
    }

    /** Returns the number of free predicate registers. */
    unsigned numFreeVecPredRegs(ThreadID tid) const
    {
        return predList->numFreeRegs(tid);
    }

    /** Returns the number of free cc registers. */
    unsigned numFreeCCRegs(ThreadID tid) const
    {
        return ccList->numFreeRegs(tid);
    }
};

template<class InputIt>
inline void
UnifiedFreeList::addRegs(InputIt first, InputIt last)
{
    // Are there any registers to add?
    if (first == last)
        return;

    panic_if((first != last) &&
            first->classValue() != (last-1)->classValue(),
            "Attempt to add mixed type regs: %s and %s",
            first->className(),
            (last-1)->className());
    switch (first->classValue()) {
        case IntRegClass:
            intList->addRegs(first, last);
            break;
        case FloatRegClass:
            floatList->addRegs(first, last);
            break;
        case VecRegClass:
            vecList->addRegs(first, last);
            break;
        case VecElemClass:
            vecElemList->addRegs(first, last);
            break;
        case VecPredRegClass:
            predList->addRegs(first, last);
            break;
        case CCRegClass:
            ccList->addRegs(first, last);
            break;
        default:
            panic("Unexpected RegClass (%s)",
                                   first->className());
    }

}

inline void
UnifiedFreeList::addReg(PhysRegIdPtr freed_reg, ThreadID tid)
{
    DPRINTF(FreeList,"Freeing register %i (%s).\n", freed_reg->index(),
            freed_reg->className());
    //Might want to add in a check for whether or not this register is
    //already in there.  A bit vector or something similar would be useful.
    switch (freed_reg->classValue()) {
        case IntRegClass:
            intList->addReg(freed_reg, tid);
            break;
        case FloatRegClass:
            floatList->addReg(freed_reg, tid);
            break;
        case VecRegClass:
            vecList->addReg(freed_reg, tid);
            break;
        case VecElemClass:
            vecElemList->addReg(freed_reg, tid);
            break;
        case VecPredRegClass:
            predList->addReg(freed_reg, tid);
            break;
        case CCRegClass:
            ccList->addReg(freed_reg, tid);
            break;
        default:
            panic("Unexpected RegClass (%s)",
                                   freed_reg->className());
    }

    // These assert conditions ensure that the number of free
    // registers are not more than the # of total Physical  Registers.
    // If this were false, it would mean that registers
    // have been freed twice, overflowing the free register
    // pool and potentially crashing SMT workloads.
    // ----
    // Comment out for now so as to not potentially break
    // CMP and single-threaded workloads
    // ----
    // assert(freeIntRegs.size() <= numPhysicalIntRegs);
    // assert(freeFloatRegs.size() <= numPhysicalFloatRegs);
}


#endif // __CPU_O3_FREE_LIST_HH__
