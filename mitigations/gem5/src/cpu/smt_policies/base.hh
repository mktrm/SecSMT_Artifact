/*
 * Copyright (c) 2020 Kazem Taram
 * All rights reserved
*/
#ifndef __CPU_O3_SMT_STATEFUL_RESOURCE_HH__
#define __CPU_O3_SMT_STATEFUL_RESOURCE_HH__

#include <string>
#include <vector>

#include "base/logging.hh"
#include "base/types.hh"
#include "cpu/base.hh"
#include "params/BaseSMTPolicy.hh"
#include "sim/sim_object.hh"

class BaseSMTPolicy : public SimObject
{

  public:
    typedef BaseSMTPolicyParams Params;
  protected:

    /** the number of entries in the stateful resource */
    int size;

    /** the number of threads sharing this resource */
    int numThreads;


    ClockedObject* cpu;

    const Params* saved_p; 

  public:
    /** Constructs a stateful resource */
    BaseSMTPolicy(const Params *p) : SimObject(p)
    , size(p->size), numThreads(p->numThreads), saved_p(p)  {};

    /** Destructor. */
    virtual ~BaseSMTPolicy() {};

    virtual bool borrowedFrom (ThreadID tid){
      return false;
    }
    /** should be called when a thread tid uses one element of the
     *  stateful resource    */
    virtual void consume(ThreadID tid) = 0;

    /** should be called when a thread tid frees up one element of the
     *  stateful resource    */
    virtual void free(ThreadID tid) = 0;

    /** resets the state */
    virtual void reset() = 0;

    /** returns true if the thread tid can not use up more resources */
    virtual bool isFull(ThreadID tid) = 0;

    /** returns number of available entries */
    virtual unsigned numFree(ThreadID tid) = 0;

    virtual void init (int _size, int _numThreads, ClockedObject* _cpu) = 0;

    virtual BaseSMTPolicy* clone () = 0;

    /** returns a mask where the ids of the threads that can be
     * evicted when we want to insert an entry for thread tid */
    virtual unsigned evictMask(ThreadID tid) = 0;

};



#endif
