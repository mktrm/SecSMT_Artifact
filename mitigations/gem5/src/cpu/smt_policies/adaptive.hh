/*
Author: Kazem Taram

*/
#ifndef __CPU_O3_SMT_ADAPTIVE_HH__
#define __CPU_O3_SMT_ADAPTIVE_HH__

#include "base/logging.hh"
#include "base/trace.hh"
#include "cpu/smt_policies/base.hh"
#include "debug/SMTPolicies.hh"
#include "params/AdaptiveSMTPolicy.hh"

class AdaptiveSMTPolicy : public BaseSMTPolicy
{
  private:

    /* number of entries that are occupied by each thread */
    int* occupiedEntries;

    /* the step size by which we change size of the partiiones
    every adaptation interval */
    int step;

    /* the maximum size of one partition over the number of entries */
    float limit;

    /* the adaptation period */
    int interval;


    Cycles lastAdaptationCycle;

    /* current partiion */
    int* maxEntries;

    /* counts the number of times a thread filled the entire partition */
    int* fullCounts;

    void adapt();

  public:
    typedef AdaptiveSMTPolicyParams Params;

    AdaptiveSMTPolicy(const Params*  p);

    /** Destructor. */
    ~AdaptiveSMTPolicy() {};

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


    void init (int _size, int _numThreads, ClockedObject* _cpu) override;

    BaseSMTPolicy * clone () 
    {
      return new AdaptiveSMTPolicy( (Params*) saved_p);
    }

};

#endif
