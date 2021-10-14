/*
Author: Kazem Taram

*/
#ifndef __CPU_O3_SMT_DYNAMIC_HH__
#define __CPU_O3_SMT_DYNAMIC_HH__

#include "cpu/smt_policies/base.hh"
#include "params/DynamicSMTPolicy.hh"

class DynamicSMTPolicy : public BaseSMTPolicy
{
  private:
    /* number of entries that are occupied by each thread */
    int* occupiedEntries;

    /* number of full entries across all the threads */
    int  sumFull();

  public:

    typedef DynamicSMTPolicyParams Params;

    DynamicSMTPolicy(const Params*  p);

    /** Destructor. */
    ~DynamicSMTPolicy() {};

    /** should be called when a thread tid uses one element of the
     *  stateful resource    */
    void consume(ThreadID tid) override;

    /** should be called when a thread tid frees up one element of the
     *  stateful resource    */
    void free(ThreadID tid) override;

    /** resets the state */
    void reset() override;

    /** returns true if the thread tid can not use up more resources */
    bool isFull(ThreadID tid) override;

    /** returns number of available entries */
    unsigned numFree(ThreadID tid) override;

    /** returns a mask where the ids of the threads that can be
     * evicted when we want to insert an entry for thread tid */
    unsigned evictMask(ThreadID tid) override;

    void init(int _size, int _numThreads, ClockedObject* cpu) override;
    
    BaseSMTPolicy * clone () 
    {
      return new DynamicSMTPolicy( (Params*) saved_p);
    }
};

#endif
