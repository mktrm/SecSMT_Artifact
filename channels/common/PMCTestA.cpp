//                       PMCTestA.cpp                2018-04-20 Agner Fog
//
//          Multithread PMC Test program for Windows and Linux
//
//
// This program is intended for testing the performance of a little piece of 
// code written in C, C++ or assembly. 
// The code to test is inserted at the place marked "Test code start" in
// PMCTestB.cpp, PMCTestB32.asm or PMCTestB64.asm.
// 
// In 64-bit Windows: Run as administrator, with driver signature enforcement
// off.
//
// See PMCTest.txt for further instructions.
//
// To turn on counters for use in another program, run with command line option
//     startcounters
// To turn counters off again, use command line option 
//     stopcounters
//
// � 2000-2018 GNU General Public License v. 3. www.gnu.org/licenses
//////////////////////////////////////////////////////////////////////////////

#include "PMCTest.h"

// #include <math.h> // for warmup only

int diagnostics = 0; // 1 for output of CPU model and PMC scheme


//////////////////////////////////////////////////////////////////////
//
//        Thread synchronizer
//
//////////////////////////////////////////////////////////////////////

union USync {
#if MAXTHREADS > 4
    int64 allflags;                     // for MAXTHREADS = 8
#else
    int allflags;                       // for MAXTHREADS = 4
#endif
    char flag[MAXTHREADS];
};
volatile USync TSync = {0};

// processornumber for each thread
int ProcNum[MAXTHREADS+64] = {0};

// clock correction factor for AMD Zen processor
double clockFactor[MAXTHREADS] = {0};

// number of repetitions in each thread
int repetitions;

// Create CCounters instance
CCounters MSRCounters;


//////////////////////////////////////////////////////////////////////
//
//        Thread procedure
//
//////////////////////////////////////////////////////////////////////

ThreadProcedureDeclaration(ThreadProc1) {
    //DWORD WINAPI ThreadProc1(LPVOID parm) {
    // check thread number
    unsigned int threadnum = *(unsigned int*)parm;

    if (threadnum >= (unsigned int)NumThreads) {
        printf("\nThread number out of range %i", threadnum);
        return 0;
    }

    // get desired processornumber
    int ProcessorNumber = ProcNum[threadnum];

    // Lock process to this processor number
    SyS::SetProcessMask(ProcessorNumber);

    // Start MSR counters
    MSRCounters.StartCounters(threadnum);

    // Wait for rest of timeslice
    SyS::Sleep0();

    // wait for other threads
    // Initialize synchronizer
    USync WaitTo;
    WaitTo.allflags = 0;
    for (int t = 0; t < NumThreads; t++) WaitTo.flag[t] = 1;
    // flag for this thead ready
    TSync.flag[threadnum] = 1;
    // wait for other threads to be ready
    while (TSync.allflags != WaitTo.allflags) {} // Note: will wait forever if a thread is not created

    // Run the test code
    repetitions = TestLoop(threadnum);

    // Wait for rest of timeslice
    SyS::Sleep0();

    // Start MSR counters
    MSRCounters.StopCounters(threadnum);

    return 0;
};

//////////////////////////////////////////////////////////////////////
//
//        Start counters and leave them on, or stop counters
//
//////////////////////////////////////////////////////////////////////
int setcounters(int argc, char* argv[]) {
    int i, cnt, thread;
    int countthreads = 0;
    int command = 0;                 // 1: start counters, 2: stop counters

    if (strstr(argv[1], "startcounters")) command = 1;
    else if (strstr(argv[1], "stopcounters")) command = 2;
    else {
        printf("\nUnknown command line parameter %s\n", argv[1]);
        return 1;
    }

    // find counter definitions on command line, if any
    if (argc > 2) {
        for (i = 0; i < MAXCOUNTERS; i++) {
            cnt = 0;
            if (command == 2) cnt = 100;  // dummy value that is valid for all CPUs
            if (i + 2 < argc) cnt = atoi(argv[i+2]);
            CounterTypesDesired[i] = cnt;
        }
    }            

    // Get mask of possible CPU cores
    SyS::ProcMaskType ProcessAffMask = SyS::GetProcessMask();

    // find all possible CPU cores
    NumThreads = (int)sizeof(void*)*8;
    if (NumThreads > 64) NumThreads = 64;

    for (thread = 0; thread < NumThreads; thread++) {
        if (SyS::TestProcessMask(thread, &ProcessAffMask)) {
            ProcNum[thread] = thread;
            countthreads++;
        }
        else {
            ProcNum[thread] = -1;
        }
    }

    // Lock processor number for each thread
    MSRCounters.LockProcessor();

    // Find counter defitions and put them in queue for driver
    MSRCounters.QueueCounters();

    // Install and load driver
    int e = MSRCounters.StartDriver();
    if (e) return e;

    // Start MSR counters
    for (thread = 0; thread < NumThreads; thread++) {
        if (ProcNum[thread] >= 0) {

#if defined(__unix__) || defined(__linux__)
            // get desired processornumber
            int ProcessorNumber = ProcNum[thread];
            // Lock process to this processor number
            SyS::SetProcessMask(ProcessorNumber);
#else
            // In Windows, the thread number needs only be fixed inside the driver
#endif

            if (command == 1) {
                MSRCounters.StartCounters(thread);
            }
            else  {
                MSRCounters.StopCounters(thread);
            }
        }
    }

    // Clean up driver
    MSRCounters.CleanUp();

    // print output
    if (command == 1) {
        printf("\nEnabled %i counters in each of %i CPU cores", NumCounters, countthreads);
        printf("\n\nPMC number:   Counter name:");
        for (i = 0; i < NumCounters; i++) {                
            printf("\n0x%08X    %-10s ", Counters[i], MSRCounters.CounterNames[i]);
        }
    }
    else {
        printf("\nDisabled %i counters in each of %i CPU cores", NumCounters, countthreads);
    }
    printf("\n");
    return 0;
}


//////////////////////////////////////////////////////////////////////
//
//        Main
//
//////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
    int repi;                           // repetition counter
    int i;                              // loop counter
    int t;                              // thread counter
    int e;                              // error number
    int procthreads;                    // number of threads supported by processor

    if (argc > 1) {
        // Interpret command line parameters
        if (strstr(argv[1], "diagnostics")) diagnostics = 1;
        else if (strstr(argv[1], "counters")) {
            // not running test. setting or resetting PMC counters        
            return setcounters(argc, argv);
        }
        else {
            //printf("\nUnknown command line parameter %s\n", argv[1]);
            //return 1;
        }
    }

    // Limit number of threads
    if (NumThreads > MAXTHREADS) {
        NumThreads = MAXTHREADS;
        printf("\nToo many threads");
    }
    if (NumThreads < 1) NumThreads = 1;

    // Get mask of possible CPU cores
    SyS::ProcMaskType ProcessAffMask = SyS::GetProcessMask();
    // Count possible threads
    int maxProcThreads = (int)sizeof(void*)*8;
    if (maxProcThreads > 64) maxProcThreads = 64;
    for (procthreads = i = 0; i < maxProcThreads; i++) {
        if (SyS::TestProcessMask(i, &ProcessAffMask)) procthreads++;
    }

    // Fix a processornumber for each thread
    /*
    for (t = 0, i = NumThreads-1; t < NumThreads; t++, i--) {
        // make processornumbers different, and last thread = MainThreadProcNum:
        // ProcNum[t] = MainThreadProcNum ^ i;
        if (procthreads < 4) {        
            ProcNum[t] = i;
        }
        else {        
            ProcNum[t] = (i % 2) * (procthreads/2) + i / 2;
            //printf("ProcNum[%d]=%d\n",t,ProcNum[t]);
        }
        if (!SyS::TestProcessMask(ProcNum[t], &ProcessAffMask)) {
            // this processor core is not available
            printf("\nProcessor %i not available. Processors available:\n", ProcNum[t]);
            for (int p = 0; p < MAXTHREADS; p++) {
                if (SyS::TestProcessMask(p, &ProcessAffMask)) printf("%i  ", p);
            }
            printf("\n");
            return 1;
        }
    }
    */
    for(int i=0; i<NumThreads; i++)
        ProcNum[i] = ProcAffinity[i];
      
    /*
    printf("ProcNum[%d]=%d\n",0,ProcNum[0]);
    printf("ProcNum[%d]=%d\n",1,ProcNum[1]);
    printf("ProcNum[%d]=%d\n",2,ProcNum[2]);
    printf("ProcNum[%d]=%d\n",3,ProcNum[3]);
    */ 

    /*!!
    //warmup
    volatile double v = 1;
    double x = v;
    for (int w = 0; w < 10000000; w++) {
        x = cos(x);
    }
    v = x;
    */    


    // Make program and driver use the same processor number
    MSRCounters.LockProcessor();

    // Find counter defitions and put them in queue for driver
    MSRCounters.QueueCounters();

    // Install and load driver
    e = MSRCounters.StartDriver();
    if (e) return e;

    // Set high priority to minimize risk of interrupts during test
    SyS::SetProcessPriorityHigh();

    // Make multiple threads
    ThreadHandler Threads;
    Threads.Start(NumThreads);

    // Stop threads
    Threads.Stop();

    // Set priority back normal
    SyS::SetProcessPriorityNormal();

    // Clean up
    MSRCounters.CleanUp();

    // Print results
    for (repi = 0; repi < repetitions; repi++) {
        for (i = 0; i < NumCounters+1; i++) {

            if(i == NumCounters)
                printf("     Clock, ");
            else
                printf("%10s, ", MSRCounters.CounterNames[i]);


            //print parameters of the experimnet
            for(int arg = diagnostics+1; arg< argc; arg++)
                printf("%10s, ", argv[arg]);


            for (t = 0; t < NumThreads; t++) {
                int TOffset = t * (ThreadDataSize / sizeof(int));
                int ClockOS = ClockResultsOS / sizeof(int);
                int PMCOS   = PMCResultsOS / sizeof(int);

                int tscClock = PThreadData[repi+TOffset+ClockOS];
                if (i == NumCounters){
                    if (UsePMC) {
                        if (MSRCounters.MScheme == S_AMD2) {
                            printf("%10i, ", int(tscClock * clockFactor[t] + 0.5)); // Calculated core clock count
                        }
                        else{
                            printf("%10i, ", tscClock);
                        }
                    } else {
                        printf("%10i, ", tscClock);
                    }
                } else{
                    if (UsePMC) 
                        printf("%10i, ", PThreadData[repi+i*repetitions+TOffset+PMCOS]);

                }
            }
            printf("\n");
        }

    }
    /*
       for (t = 0; t < NumThreads; t++) {
// calculate offsets into ThreadData[]
int TOffset = t * (ThreadDataSize / sizeof(int));
int ClockOS = ClockResultsOS / sizeof(int);
int PMCOS   = PMCResultsOS / sizeof(int);

// print column headings
if (NumThreads > 1) printf("\nProcessor %i", ProcNum[t]);
printf("\n     Clock ");
if (UsePMC) {
if (MSRCounters.MScheme == S_AMD2) {
printf("%10s ", "Corrected");
}
for (i = 0; i < NumCounters; i++) {
printf("%10s ", MSRCounters.CounterNames[i]);
}
}
if (RatioOut[0]) printf("%10s ", RatioOutTitle ? RatioOutTitle : "Ratio");
if (TempOut) printf("%10s ", TempOutTitle ? TempOutTitle : "Extra out");

// print counter outputs
for (repi = 0; repi < repetitions; repi++) {
int tscClock = PThreadData[repi+TOffset+ClockOS];
printf("\n%10i ", tscClock);
if (UsePMC) {
if (MSRCounters.MScheme == S_AMD2) {
printf("%10i ", int(tscClock * clockFactor[t] + 0.5)); // Calculated core clock count
}
for (i = 0; i < NumCounters; i++) {         
printf("%10i ", PThreadData[repi+i*repetitions+TOffset+PMCOS]);
}
}
// optional ratio output
if (RatioOut[0]) {
union {
int i;
float f;
} factor;
factor.i = RatioOut[3];
int a, b;
if (RatioOut[1] == 0) {
a = PThreadData[repi+TOffset+ClockOS];
if (MSRCounters.MScheme == S_AMD2) a = int(a * clockFactor[t] + 0.5); // Calculated core clock count
}
else if ((unsigned int)RatioOut[1] <= (unsigned int)NumCounters) {
a = PThreadData[repi+(RatioOut[1]-1)*repetitions+TOffset+PMCOS];
}
else {
a = 1;
}
if (RatioOut[2] == 0) {
b = PThreadData[repi+TOffset+ClockOS];
if (MSRCounters.MScheme == S_AMD2) a = int(a * clockFactor[t] + 0.5); // Calculated core clock count
}
else if ((unsigned int)RatioOut[2] <= (unsigned int)NumCounters) {
b = PThreadData[repi+(RatioOut[2]-1)*repetitions+TOffset+PMCOS];
}
else {
b = 1;
}
if (b == 0) {
printf("%10s", "inf");
}
else if (RatioOut[0] == 1) {
printf("%10i ", factor.i * a / b);
}
else {
printf("%10.6f ", factor.f * (double)a / (double)b);
}
}
// optional arbitrary output
if (TempOut) {
    union {
        int * pi;
        int64 * pl;
        float * pf;
        double * pd;
    } pu;
    pu.pi = PThreadData + repi + TOffset;      // pointer to CountTemp
    if (TempOut & 1) pu.pi += repi;            // double size
                switch (TempOut) {
                case 2:    // int
                    printf("%10i", *pu.pi);  break;
                case 3:    // 64 bit int
                    printf("%10lli", *pu.pl);  break;
                case 4:    // hexadecimal int
                    printf("0x%08X", *pu.pi);  break;
                case 5:    // hexadecimal 64-bit int
                    printf("0x%08X%08X", pu.pi[1], pu.pi[0]);  break;
                case 6:    // float
                    printf("%10.6f", *pu.pf);  break;
                case 7:    // double
                    printf("%10.6f", *pu.pd);  break;
                default:
                    printf("unknown TempOut %i", TempOut);
                }
            }
        }
        if (MSRCounters.MScheme == S_AMD2) {
            printf("\nClock factor %.4f", clockFactor[t]);
        }
    }
    */

    printf("\n");
    // Optional: wait for key press
    //printf("\npress any key");
    //getch();

    // Exit
    return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
//        CMSRInOutQue class member functions
//
//////////////////////////////////////////////////////////////////////////////

// Constructor
CMSRInOutQue::CMSRInOutQue() {
    n = 0;
    for (int i = 0; i < MAX_QUE_ENTRIES+1; i++) {
        queue[i].msr_command = MSR_STOP;
    }
}

// Put data record in queue
int CMSRInOutQue::put (EMSR_COMMAND msr_command, unsigned int register_number, unsigned int value_lo, unsigned int value_hi) {

    if (n >= MAX_QUE_ENTRIES) return -10;

    queue[n].msr_command = msr_command;
    queue[n].register_number = register_number;
    queue[n].val[0] = value_lo;
    queue[n].val[1] = value_hi;
    n++;
    return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
//        CCounters class member functions
//
//////////////////////////////////////////////////////////////////////////////

// Constructor
CCounters::CCounters() {
    // Set everything to zero
    MVendor = VENDOR_UNKNOWN;
    MFamily = PRUNKNOWN;
    MScheme = S_UNKNOWN;
    NumPMCs = 0;
    NumFixedPMCs = 0;
    ProcessorNumber = 0;
    for (int i = 0; i < MAXCOUNTERS; i++) CounterNames[i] = 0;
}

void CCounters::QueueCounters() {
    // Put counter definitions in queue
    int n = 0, CounterType; 
    const char * err;
    while (CounterDefinitions[n].ProcessorFamily || CounterDefinitions[n].CounterType) n++;
    NumCounterDefinitions = n;

    // Get processor information
    GetProcessorVendor();               // get microprocessor vendor
    GetProcessorFamily();               // get microprocessor family
    GetPMCScheme();                     // get PMC scheme

    if (UsePMC) {   
        // Get all counter requests
        for (int i = 0; i < MaxNumCounters; i++) {
            CounterType = CounterTypesDesired[i];
            err = DefineCounter(CounterType);
            if (err) {
                printf("\nCannot make counter %i. %s\n", i+1, err);
            }
        }

        if (MScheme == S_AMD2) {
            // AMD Zen processor has a core clock counter called APERF 
            // which is only accessible in the driver.
            // Read TSC and APERF in the driver before and after the test.
            // This value is used for adjusting the clock count
            for (int thread=0; thread < NumThreads; thread++) {
                queue1[thread].put(MSR_READ, rTSCounter, thread);
                queue1[thread].put(MSR_READ, rCoreCounter, thread);
                //queue1[thread].put(MSR_READ, rMPERF, 0);
                queue2[thread].put(MSR_READ, rTSCounter, thread);
                queue2[thread].put(MSR_READ, rCoreCounter, thread);
                //queue2[thread].put(MSR_READ, rMPERF, 0);
            }
        }
    }
}

void CCounters::LockProcessor() {
    // Make program and driver use the same processor number if multiple processors
    // Enable RDMSR instruction
    int thread, procnum;

    // We must lock the driver call to the desired processor number
    for (thread = 0; thread < NumThreads; thread++) {
        procnum = ProcNum[thread];
        if (procnum >= 0) {
            // lock driver to the same processor number as thread
            queue1[thread].put(PROC_SET, 0, procnum);
            queue2[thread].put(PROC_SET, 0, procnum);
            // enable readpmc instruction (for this processor number)
            queue1[thread].put(PMC_ENABLE, 0, 0);
            // disable readpmc instruction after run
            queue2[thread].put(PMC_DISABLE, 0, 0);     // This causes segmentation fault on AMD when thread hopping. Why is the processor not properly locked?!!
        }
    }
}

int CCounters::StartDriver() {
    // Install and load driver
    // return error code
    int ErrNo = 0;

    if (UsePMC && !diagnostics) {
        // Load driver
        ErrNo = msr.LoadDriver();
    }

    return ErrNo;
}

void CCounters::CleanUp() {
    // Things to do after measuring

    if (MScheme == S_AMD2) {
        // Calculate clock correction factors for AMD Zen
        for (int thread = 0; thread < NumThreads; thread++) {
            long long tscount, corecount;
            tscount = MSRCounters.read2(rTSCounter, thread) - MSRCounters.read1(rTSCounter, thread);
            corecount = MSRCounters.read2(rCoreCounter, thread) - MSRCounters.read1(rCoreCounter, thread);
            clockFactor[thread] = double(corecount) / double(tscount);
        }
    }

    // Any required cleanup of driver etc
    // Optionally unload driver
    //msr.UnloadDriver();
    //msr.UnInstallDriver();
}

// put record into multiple start queues
void CCounters::Put1 (int num_threads,
    EMSR_COMMAND msr_command, unsigned int register_number,
    unsigned int value_lo, unsigned int value_hi) {
    for (int t = 0; t < num_threads; t++) {
        queue1[t].put(msr_command, register_number, value_lo, value_hi);
    }
}

// put record into multiple stop queues
void CCounters::Put2 (int num_threads,
    EMSR_COMMAND msr_command, unsigned int register_number,
    unsigned int value_lo, unsigned int value_hi) {
    for (int t = 0; t < num_threads; t++) {
        queue2[t].put(msr_command, register_number, value_lo, value_hi);
    }
}

// get value from previous MSR_READ command in queue1
long long CCounters::read1(unsigned int register_number, int thread) {
    for(int i=0; i < queue1[thread].GetSize(); i++) {
        if (queue1[thread].queue[i].msr_command == MSR_READ && queue1[thread].queue[i].register_number == register_number) {
            return queue1[thread].queue[i].value;
        }
    }
    return 0;  // not found
}

// get value from previous MSR_READ command in queue1
long long CCounters::read2(unsigned int register_number, int thread) {
    for(int i=0; i < queue2[thread].GetSize(); i++) {
        if (queue2[thread].queue[i].msr_command == MSR_READ && queue1[thread].queue[i].register_number == register_number) {
            return queue2[thread].queue[i].value;
        }
    }
    return 0;  // not found
}


// Start counting
void CCounters::StartCounters(int ThreadNum) {
    if (UsePMC) {
        msr.AccessRegisters(queue1[ThreadNum]);
    }
}

// Stop and reset counters
void CCounters::StopCounters(int ThreadNum) {
    if (UsePMC) {
        msr.AccessRegisters(queue2[ThreadNum]);
    }
}

void CCounters::GetProcessorVendor() {
    // get microprocessor vendor
    int CpuIdOutput[4];

    // Call cpuid function 0
    Cpuid(CpuIdOutput, 0);

    // Interpret vendor string
    MVendor = VENDOR_UNKNOWN;
    if (CpuIdOutput[2] == 0x6C65746E) MVendor = INTEL;  // Intel "GenuineIntel"
    if (CpuIdOutput[2] == 0x444D4163) MVendor = AMD;    // AMD   "AuthenticAMD"
    if (CpuIdOutput[1] == 0x746E6543) MVendor = VIA;    // VIA   "CentaurHauls"

    if (diagnostics) {
        printf("\nVendor = %X", MVendor);
    }
}

void CCounters::GetProcessorFamily() {
    // get microprocessor family
    int CpuIdOutput[4];
    int Family, Model;

    MFamily = PRUNKNOWN;                // default = unknown

    // Call cpuid function 0
    Cpuid(CpuIdOutput, 0);
    if (CpuIdOutput[0] == 0) return;     // cpuid function 1 not supported

    // call cpuid function 1 to get family and model number
    Cpuid(CpuIdOutput, 1);
    Family = ((CpuIdOutput[0] >> 8) & 0x0F) + ((CpuIdOutput[0] >> 20) & 0xFF);   // family code
    Model  = ((CpuIdOutput[0] >> 4) & 0x0F) | ((CpuIdOutput[0] >> 12) & 0xF0);   // model code
    // printf("\nCPU family 0x%X, model 0x%X\n", Family, Model);

    if (MVendor == INTEL)  {
        // Intel processor
        if (Family <  5)    MFamily = PRUNKNOWN;           // less than Pentium
        if (Family == 5)    MFamily = INTEL_P1MMX;         // pentium 1 or mmx
        if (Family == 0x0F) MFamily = INTEL_P4;            // pentium 4 or other netburst
        if (Family == 6) {
            switch(Model) {  // list of known Intel families with different performance monitoring event tables
            case 0x09:  case 0x0D:
                MFamily = INTEL_PM;  break;                // Pentium M
            case 0x0E: 
                MFamily = INTEL_CORE;  break;              // Core 1
            case 0x0F: case 0x16: 
                MFamily = INTEL_CORE2;  break;             // Core 2, 65 nm
            case 0x17: case 0x1D:
                MFamily = INTEL_CORE2;  break;             // Core 2, 45 nm
            case 0x1A: case 0x1E: case 0x1F: case 0x2E:
                MFamily = INTEL_7;  break;                 // Nehalem
            case 0x25: case 0x2C: case 0x2F:
                MFamily = INTEL_7;  break;                 // Westmere
            case 0x2A: case 0x2D:
                MFamily = INTEL_IVY;  break;               // Sandy Bridge
            case 0x3A: case 0x3E:
                MFamily = INTEL_IVY;  break;               // Ivy Bridge
            case 0x3C: case 0x3F: case 0x45: case 0x46:
                MFamily = INTEL_HASW;  break;              // Haswell
            case 0x3D: case 0x47: case 0x4F: case 0x56:
                MFamily = INTEL_HASW;  break;              // Broadwell
            case 0x5E: case 0x55:
                MFamily = INTEL_SKYL;  break;              // Skylake
            // low power processors:
            case 0x1C: case 0x26: case 0x27: case 0x35: case 0x36:
                MFamily = INTEL_ATOM;  break;              // Atom
            case 0x37: case 0x4A: case 0x4D:
                MFamily = INTEL_SILV;  break;              // Silvermont
            case 0x5C: case 0x5F: case 0x7A:
                MFamily = INTEL_GOLDM;  break;             // Goldmont                
            case 0x57:
                MFamily = INTEL_KNIGHT; break;             // Knights Landing
            // unknown and future
            default:
                MFamily = INTEL_P23;                       // Pentium 2 or 3
                if (Model >= 0x3C) MFamily = INTEL_HASW;   // Haswell
                if (Model >= 0x5E) MFamily = INTEL_SKYL;   // Skylake
            }
        }
    }

    if (MVendor == AMD)  {
        // AMD processor
        MFamily = PRUNKNOWN;                               // old or unknown AMD
        if (Family == 6)    MFamily = AMD_ATHLON;          // AMD Athlon
        if (Family >= 0x0F && Family <= 0x14) {
            MFamily = AMD_ATHLON64;                        // Athlon 64, Opteron, etc
        }
        if (Family >= 0x15) MFamily = AMD_BULLD;           // Family 15h
        if (Family >= 0x17) MFamily = AMD_ZEN;             // Family 17h
    }

    if (MVendor == VIA)  {
        // VIA processor
        if (Family == 6 && Model >= 0x0F) MFamily = VIA_NANO; // VIA Nano
    }
    if (diagnostics) {
        printf(" Family %X, Model %X, MFamily %X", Family, Model, MFamily);
    }
}

void CCounters::GetPMCScheme() {
    // get PMC scheme
    // Default values
    MScheme = S_UNKNOWN;
    NumPMCs = 2;
    NumFixedPMCs = 0;

    if (MVendor == AMD)  {
        // AMD processor
        MScheme = S_AMD;
        NumPMCs = 4;
        int CpuIdOutput[4];        
        Cpuid(CpuIdOutput, 6);                   // Call cpuid function 6
        if (CpuIdOutput[2] & 1) {                // APERF AND MPERF counters present
            Cpuid(CpuIdOutput, 0x80000001);      // Call cpuid function 0x80000001
            if (CpuIdOutput[2] & (1 << 28)) {    // L3 performance counter extensions            
                MScheme = S_AMD2;                // AMD Zen scheme
                NumPMCs = 6;                     // 6 counters
                rTSCounter = 0x00000010;         // PMC register number of time stamp counter in S_AMD2 scheme
                rCoreCounter = 0xC00000E8;       // PMC register number of core clock counter in S_AMD2 scheme
                // rMPERF = 0xC00000E7
            }
        }
    }

    if (MVendor == VIA)  {
        // VIA processor
        MScheme = S_VIA;
    }

    if (MVendor == INTEL)  {
        // Intel processor
        int CpuIdOutput[4];

        // Call cpuid function 0
        Cpuid(CpuIdOutput, 0);
        if (CpuIdOutput[0] >= 0x0A) {
            // PMC scheme defined by cpuid function A
            Cpuid(CpuIdOutput, 0x0A);
            if (CpuIdOutput[0] & 0xFF) {
                MScheme = EPMCScheme(S_ID1 << ((CpuIdOutput[0] & 0xFF) - 1));
                NumPMCs = (CpuIdOutput[0] >> 8) & 0xFF;
                //NumFixedPMCs = CpuIdOutput[0] & 0x1F;
                NumFixedPMCs = CpuIdOutput[3] & 0x1F;
                // printf("\nCounters:\nMScheme = 0x%X, NumPMCs = %i, NumFixedPMCs = %i\n\n", MScheme, NumPMCs, NumFixedPMCs);
            }
        }

        if (MScheme == S_UNKNOWN) {
            // PMC scheme not defined by cpuid 
            switch (MFamily) {
            case INTEL_P1MMX:
                MScheme = S_P1; break;
            case INTEL_P23: case INTEL_PM:
                MScheme = S_P2; break;
            case INTEL_P4:
                MScheme = S_P4; break;
            case INTEL_CORE:
                MScheme = S_ID1; break;
            case INTEL_CORE2:
                MScheme = S_ID2; break;
            case INTEL_7: case INTEL_ATOM: case INTEL_SILV:
                MScheme = S_ID3; break;
            }
        }
    }
    if (diagnostics) {

        if (MVendor == INTEL)  printf(", NumPMCs %X, NumFixedPMCs %X", NumPMCs, NumFixedPMCs);
        printf(", MScheme %X\n", MScheme);
    }
}

// Request a counter setup
// (return value is error message)
const char * CCounters::DefineCounter(int CounterType) {
    if (CounterType == 0) return NULL;
    int i;
    SCounterDefinition * p;

    // Search for matching counter definition
    for (i=0, p = CounterDefinitions; i < NumCounterDefinitions; i++, p++) {
        if (p->CounterType == CounterType && (p->PMCScheme & MScheme) && (p->ProcessorFamily & MFamily)) {
            // Match found
            break;
        }
    }
    if (i >= NumCounterDefinitions) {
        //printf("\nCounterType = %X, MScheme = %X, MFamily = %X\n", CounterType, MScheme, MFamily);
        return "No matching counter definition found"; // not found in list
    }
    return DefineCounter(*p);
}

// Request a counter setup
// (return value is error message)
const char * CCounters::DefineCounter(SCounterDefinition & CDef) {
    int i, counternr, a, b, reg, eventreg, tag;
    static int CountersEnabled = 0, FixedCountersEnabled = 0;

    if ( !(CDef.ProcessorFamily & MFamily)) return "Counter not defined for present microprocessor family";
    if (NumCounters >= MaxNumCounters) return "Too many counters";

    if (CDef.CounterFirst & 0x40000000) { 
        // Fixed function counter
        counternr = CDef.CounterFirst;
    }
    else {
        // check CounterLast
        if (CDef.CounterLast < CDef.CounterFirst) {
            CDef.CounterLast = CDef.CounterFirst;
        }
        if (CDef.CounterLast >= NumPMCs && (MScheme & S_INTL)) {
        }   

        // Find vacant counter
        for (counternr = CDef.CounterFirst; counternr <= CDef.CounterLast; counternr++) {
            // Check if this counter register is already in use
            for (i = 0; i < NumCounters; i++) {
                if (counternr == Counters[i]) {
                    // This counter is already in use, find another
                    goto USED;
                }
            }
            if (MFamily == INTEL_P4) {
                // Check if the corresponding event register ESCR is already in use
                eventreg = GetP4EventSelectRegAddress(counternr, CDef.EventSelectReg); 
                for (i = 0; i < NumCounters; i++) {
                    if (EventRegistersUsed[i] == eventreg) {
                        goto USED;
                    }
                }
            }

            // Vacant counter found. stop searching
            break;

        USED:;
            // This counter is occupied. keep searching
        }

        if (counternr > CDef.CounterLast) {
            // No vacant counter found
            return "Counter registers are already in use";
        }
    }

    // Vacant counter found. Save name   
    CounterNames[NumCounters] = CDef.Description;

    // Put MSR commands for this counter in queues
    switch (MScheme) {

    case S_P1:
        // Pentium 1 and Pentium MMX
        a = CDef.Event | (CDef.EventMask << 6);
        if (counternr == 1) a = EventRegistersUsed[0] | (a << 16);
        Put1(NumThreads, MSR_WRITE, 0x11, a);
        Put2(NumThreads, MSR_WRITE, 0x11, 0);
        Put1(NumThreads, MSR_WRITE, 0x12+counternr, 0);
        Put2(NumThreads, MSR_WRITE, 0x12+counternr, 0);
        EventRegistersUsed[0] = a;
        break;

    case S_ID2: case S_ID3: case S_ID4:
        // Intel Core 2 and later
        if (counternr & 0x40000000) {
            // This is a fixed function counter
            if (!(FixedCountersEnabled++)) {
                // Enable fixed function counters
                for (a = i = 0; i < NumFixedPMCs; i++) {
                    b = 2;  // 1=privileged level, 2=user level, 4=any thread
                    a |= b << (4*i);
                }
                // Set MSR_PERF_FIXED_CTR_CTRL
                Put1(NumThreads, MSR_WRITE, 0x38D, a); 
                Put2(NumThreads, MSR_WRITE, 0x38D, 0);
            }
            break;
        }
        if (!(CountersEnabled++)) {
            // Enable counters
            a = (1 << NumPMCs) - 1;      // one bit for each pmc counter
            b = (1 << NumFixedPMCs) - 1; // one bit for each fixed counter
            // set MSR_PERF_GLOBAL_CTRL
            Put1(NumThreads, MSR_WRITE, 0x38F, a, b);
            Put2(NumThreads, MSR_WRITE, 0x38F, 0);
        }
        // All other counters continue in next case:

    case S_P2: case S_ID1:
        // Pentium Pro, Pentium II, Pentium III, Pentium M, Core 1, (Core 2 continued):


        a = CDef.Event | (CDef.EventMask << 8) | (1 << 16) | (1 << 22) | (CDef.CMask << 24) | (CDef.Inv << 23);
        if (MScheme == S_ID1) a |= (1 << 14);  // Means this core only
        //if (MScheme == S_ID3) a |= (1 << 22);  // Means any thread in this core!

        eventreg = 0x186 + counternr;             // IA32_PERFEVTSEL0,1,..
        reg = 0xc1 + counternr;                   // IA32_PMC0,1,..
        Put1(NumThreads, MSR_WRITE, eventreg, a);
        Put2(NumThreads, MSR_WRITE, eventreg, 0);
        Put1(NumThreads, MSR_WRITE, reg, 0);
        Put2(NumThreads, MSR_WRITE, reg, 0);
        break;

    case S_P4:
        // Pentium 4 and Pentium 4 with EM64T
        // ESCR register
        eventreg = GetP4EventSelectRegAddress(counternr, CDef.EventSelectReg); 
        tag = 1;
        a = 0x1C | (tag << 5) | (CDef.EventMask << 9) | (CDef.Event << 25);
        Put1(NumThreads, MSR_WRITE, eventreg, a);
        Put2(NumThreads, MSR_WRITE, eventreg, 0);
        // Remember this event register is used
        EventRegistersUsed[NumCounters] = eventreg;
        // CCCR register
        reg = counternr + 0x360;
        a = (1 << 12) | (3 << 16) | (CDef.EventSelectReg << 13);
        Put1(NumThreads, MSR_WRITE, reg, a);
        Put2(NumThreads, MSR_WRITE, reg, 0);
        // Reset counter register
        reg = counternr + 0x300;
        Put1(NumThreads, MSR_WRITE, reg, 0);
        Put2(NumThreads, MSR_WRITE, reg, 0);
        // Set high bit for fast readpmc
        counternr |= 0x80000000;
        break;

    case S_AMD:
        // AMD
        a = CDef.Event | (CDef.EventMask << 8) | (1 << 16) | (1 << 22);
        eventreg = 0xc0010000 + counternr;
        reg = 0xc0010004 + counternr;
        Put1(NumThreads, MSR_WRITE, eventreg, a);
        Put2(NumThreads, MSR_WRITE, eventreg, 0);
        Put1(NumThreads, MSR_WRITE, reg, 0);
        Put2(NumThreads, MSR_WRITE, reg, 0);
        break;

    case S_AMD2:
        // AMD Zen
        reg = 0xC0010200 + counternr * 2;
        b = CDef.Event | (CDef.EventMask << 8) | (1 << 16) | (1 << 22);
        Put1(NumThreads, MSR_WRITE, reg, b);
        Put2(NumThreads, MSR_WRITE, reg, 0);
        break;

    case S_VIA:
        // VIA Nano. Undocumented!
        a = CDef.Event | (1 << 16) | (1 << 22);
        eventreg = 0x186 + counternr;
        reg = 0xc1 + counternr;
        Put1(NumThreads, MSR_WRITE, eventreg, a);
        Put2(NumThreads, MSR_WRITE, eventreg, 0);
        Put1(NumThreads, MSR_WRITE, reg, 0);
        Put2(NumThreads, MSR_WRITE, reg, 0);
        break;

    default:
        return "No counters defined for present microprocessor family";
    }

    // Save counter register number in Counters list
    Counters[NumCounters++] = counternr;

    return NULL; // NULL = success
}


// Translate event select register number to register address for P4 processor
int CCounters::GetP4EventSelectRegAddress(int CounterNr, int EventSelectNo) {
    // On Pentium 4 processors, the Event Select Control Registers (ESCR) are
    // identified in a very confusing way. Each ESCR has both an ESCRx-number which 
    // is part of its name, an event select number to specify in the Counter 
    // Configuration Control Register (CCCR), and a register address to specify 
    // in the WRMSR instruction.
    // This function gets the register address based on table 15-6 in Intel manual 
    // 25366815, IA-32 Intel Architecture Software Developer's Manual Volume 3: 
    // System Programming Guide, 2005.
    // Returns -1 if error.
    static int TranslationTables[4][8] = {
        {0x3B2, 0x3B4, 0x3AA, 0x3B6, 0x3AC, 0x3C8, 0x3A2, 0x3A0}, // counter 0-3
        {0x3C0, 0x3C4, 0x3C2,    -1,    -1,    -1,    -1,    -1}, // counter 4-7
        {0x3A6, 0x3A4, 0x3AE, 0x3B0,    -1, 0x3A8,    -1,    -1}, // counter 8-11
        {0x3BA, 0x3CA, 0x3BC, 0x3BE, 0x3B8, 0x3CC, 0x3E0,    -1}};// counter 12-17

        unsigned int n = CounterNr;
        if (n > 17) return -1;
        if (n > 15) n -= 3;
        if ((unsigned int)EventSelectNo > 7) return -1;

        int a = TranslationTables[n/4][EventSelectNo];
        if (a < 0) return a;
        if (n & 2) a++;
        return a;
}


//////////////////////////////////////////////////////////////////////////////
//
//             list of counter definitions
//
//////////////////////////////////////////////////////////////////////////////
// How to add new entries to this list:
//
// Warning: Be sure to save backup copies of your files before you make any 
// changes here. A wrong register number can result in a crash in the driver.
// This results in a blue screen and possibly loss of your most recently
// modified file.
//
// Set CounterType to any vacant id number. Use the same id for similar events
// in different processor families. The other fields depend on the processor 
// family as follows:
//
// Pentium 1 and Pentium MMX:
//    Set ProcessorFamily = INTEL_P1MMX.
//    CounterFirst = 0, CounterLast = 1, Event = Event number,
//    EventMask = Counter control code.
//
// Pentium Pro, Pentium II, Pentium III, Pentium M, Core Solo/Duo
//    Set ProcessorFamily = INTEL_P23M for events that are valid for all these
//    processors or INTEL_PM or INTEL_CORE for events that only apply to 
//    one processor.
//    CounterFirst = 0, CounterLast = 1, 
//    Event = Event number, EventMask = Unit mask.
//
// Core 2
//    Set ProcessorFamily = INTEL_CORE2.
//    Fixed function counters:
//    CounterFirst = 0x40000000 + MSR Address - 0x309. (Intel manual Oct. 2006 is wrong)
//    All other counters:
//    CounterFirst = 0, CounterLast = 1, 
//    Event = Event number, EventMask = Unit mask.
//
// Pentium 4 and Pentium 4 with EM64T (Netburst):
//    Set ProcessorFamily = INTEL_P4.
//    Look in Software Developer's Manual vol. 3 appendix A, table of 
//    Performance Monitoring Events.
//    Set CounterFirst and CounterLast to the range of possible counter
//    registers listed under "Counter numbers per ESCR".
//    Set EventSelectReg to the value listed for "CCCR Select".
//    Set Event to the value indicated for "ESCR Event Select".
//    Set EventMask to a combination of the relevant bits for "ESCR Event Mask".
//    You don't need the table named "Performance Counter MSRs and Associated
//    CCCR and ESCR MSRs". This table is already implemented in the function
//    CCounters::GetP4EventSelectRegAddress.
//
// AMD Athlon 64, Opteron
//    Set ProcessorFamily = AMD_ATHLON64.
//    CounterFirst = 0, CounterLast = 3, Event = Event mask,
//    EventMask = Unit mask.
//

SCounterDefinition CounterDefinitions[] = {
    //  id   scheme cpu    countregs eventreg event  mask   name

    // Nehalem, Sandy Bridge, Ivy Bridge
    // The first three counters are fixed-function counters having their own register,
    // The rest of the counters are competing for the same counter registers.
    // id   scheme  cpu       countregs eventreg event  mask   name
    {1,   S_ID3,  INTEL_7I,  0x40000001,  0,0,   0,      0,   0, 0, "Core cyc"   }, // core clock cycles
    {2,   S_ID3,  INTEL_7I,  0x40000002,  0,0,   0,      0,   0, 0, "Ref cyc"    }, // Reference clock cycles
    {9,   S_ID3,  INTEL_7I,  0x40000000,  0,0,   0,      0,   0, 0, "Instruct"   }, // Instructions (reference counter)
    {10,  S_ID3,  INTEL_7I,  0,   3,     0,   0xc0,     0x01, 0, 0, "Instruct"   }, // Instructions
    {22,  S_ID3,  INTEL_7I,  0,   3,     0,   0x87,      0,   0, 0, "ILenStal"   }, // instruction length decoder stalls (length changing prefix)
    {24,  S_ID3,  INTEL_7I,  0,   3,     0,   0xA8,     0x01, 0, 0, "Loop uops"  }, // uops from loop stream detector
    {25,  S_ID3,  INTEL_7I,  0,   3,     0,   0x79,     0x04, 0, 0, "Dec uops"   }, // uops from decoders. (MITE = Micro-instruction Translation Engine)
    {26,  S_ID3,  INTEL_7I,  0,   3,     0,   0x79,     0x08, 0, 0, "Cach uops"  }, // uops from uop cache. (DSB = Decoded Stream Buffer)
    {100, S_ID3,  INTEL_7I,  0,   3,     0,   0xc2,     0x01, 0, 0, "Uops"       }, // uops retired, unfused domain
    {103, S_ID3,  INTEL_7I,  0,   3,     0,   0xc2,     0x04, 0, 0, "Macrofus"   }, // macrofused uops, Sandy Bridge
    {104, S_ID3,  INTEL_7I,  0,   3,     0,   0x0E,     0x01, 0, 0, "Uops F.D."  }, // uops, fused domain, Sandy Bridge
    {105, S_ID3,  INTEL_7,   0,   3,     0,   0x0E,     0x02, 0, 0, "fused uop"  }, // microfused uops 
    {110, S_ID3,  INTEL_7,   0,   3,     0,   0xa0,        0, 0, 0, "Uops UFD?"  }, // uops dispatched, unfused domain. Imprecise, Sandy Bridge
    {111, S_ID3,  INTEL_7I,  0,   3,     0,   0xa2,        1, 0, 0, "res.stl."   }, // any resource stall
    {121, S_ID3,  INTEL_7,   0,   3,     0,   0xd2,     0x02, 0, 0, "Part.reg"   }, // Partial register synchronization, clock cycles, Sandy Bridge
    {122, S_ID3,  INTEL_7,   0,   3,     0,   0xd2,     0x01, 0, 0, "part.flag"  }, // partial flags stall, clock cycles, Sandy Bridge
    {123, S_ID3,  INTEL_7,   0,   3,     0,   0xd2,     0x04, 0, 0, "R Rd stal"  }, // ROB register read stall, Sandy Bridge
    {124, S_ID3,  INTEL_7,   0,   3,     0,   0xd2,     0x0F, 0, 0, "RAT stal"   }, // RAT stall, any, Sandy Bridge
    {150, S_ID3,  INTEL_7,   0,   3,     0,   0xb1,     0x01, 0, 0, "uop p0"     }, // uops port 0.
    {151, S_ID3,  INTEL_7,   0,   3,     0,   0xb1,     0x02, 0, 0, "uop p1"     }, // uops port 1.
    {152, S_ID3,  INTEL_7,   0,   3,     0,   0xb1,     0x04, 0, 0, "uop p2"     }, // uops port 2.
    {153, S_ID3,  INTEL_7,   0,   3,     0,   0xb1,     0x08, 0, 0, "uop p3"     }, // uops port 3.
    {154, S_ID3,  INTEL_7,   0,   3,     0,   0xb1,     0x10, 0, 0, "uop p4"     }, // uops port 4.
    {155, S_ID3,  INTEL_7,   0,   3,     0,   0xb1,     0x20, 0, 0, "uop p5"     }, // uops port 5.
    {156, S_ID3,  INTEL_7,   0,   3,     0,   0xb1,     0x40, 0, 0, "uop p015"   }, // uops port 0,1,5.
    {157, S_ID3,  INTEL_7,   0,   3,     0,   0xb1,     0x80, 0, 0, "uop p234"   }, // uops port 2,3,4.
    {150, S_ID3,  INTEL_IVY, 0,   3,     0,   0xa1,     0x01, 0, 0, "uopp0"     }, // uops port 0
    {151, S_ID3,  INTEL_IVY, 0,   3,     0,   0xa1,     0x02, 0, 0, "uopp1"     }, // uops port 1
    {152, S_ID3,  INTEL_IVY, 0,   3,     0,   0xa1,     0x0c, 0, 0, "uopp2"     }, // uops port 2
    {153, S_ID3,  INTEL_IVY, 0,   3,     0,   0xa1,     0x30, 0, 0, "uopp3"     }, // uops port 3
    {154, S_ID3,  INTEL_IVY, 0,   3,     0,   0xa1,     0x40, 0, 0, "uopp4"     }, // uops port 4
    {155, S_ID3,  INTEL_IVY, 0,   3,     0,   0xa1,     0x80, 0, 0, "uopp5"     }, // uops port 5
    {160, S_ID3,  INTEL_IVY, 0,   3,     0,   0xa1,     0xFF, 0, 0, "uopp05"    }, // uops port 0-5
    {201, S_ID3,  INTEL_IVY, 0,   1,     0,   0xc4,     0x20, 0, 0, "BrTaken"    }, // branches taken. (Mask: 1=pred.not taken, 2=mispred not taken, 4=pred.taken, 8=mispred taken)
    {204, S_ID3,  INTEL_7,   0,   3,     0,   0xc5,     0x0a, 0, 0, "BrMispred"  }, // mispredicted branches
    {207, S_ID3,  INTEL_7I,  0,   3,     0,   0xc5,     0x0,  0, 0, "BrMispred"  }, // mispredicted branches
    {205, S_ID3,  INTEL_7,   0,   3,     0,   0xe6,      2,   0, 0, "BTBMiss"    }, // static branch prediction made, Sandy Bridge
    {205, S_ID3,  INTEL_7I,  0,   3,     0,   0xe6,      2,   0, 0, "BTBMiss"    }, // static branch prediction made, Sandy Bridge
    {220, S_ID3,  INTEL_IVY, 0,   3,     0,   0x58,     0x03, 0, 0, "Mov elim"   }, // register moves eliminated
    {221, S_ID3,  INTEL_IVY, 0,   3,     0,   0x58,     0x0C, 0, 0, "Mov elim-"  }, // register moves elimination unsuccessful
    {311, S_ID3,  INTEL_7I,  0,   3,     0,   0x28,     0x0f, 0, 0, "L1D Miss"   }, // level 1 data cache miss
    {312, S_ID3,  INTEL_7,   0,   3,     0,   0x24,     0x0f, 0, 0, "L1Miss"    }, // level 2 cache requests
    {313, S_ID3,  INTEL_IVY, 0,   3,     0,   0x87,     0x04, 0, 0, "IQFULL"    }, // Stall cycles because IQ is full.
    {314, S_ID3,  INTEL_IVY, 0,   3,     0,   0x5B,     0x4f, 0, 0, "OOOFull"    }, // Resource stalls out of order resources full.
    {315, S_ID3,  INTEL_IVY, 0,   3,     0,   0x9C,     0x01, 0, 0, "frnt-bnd"    }, // uop not delvd. to backend per cycle when back-end not stalled.
    {316, S_ID3,  INTEL_IVY, 0,   3,     0,   0xa2,     0x01, 0, 0, "res.stl."    }, // resource relaged stall cycles: resource_stalls.any
    {317, S_ID3,  INTEL_IVY, 0,   3,     0,   0xb1,     0x01, 1, 1, "dpch.stl"    }, // resource relaged stall cycles: resource_stalls.any
    {318, S_ID3,  INTEL_IVY, 0,   3,     0,   0xc2,     0x02, 0, 0, "retrd.slt"    }, // resource relaged stall cycles: resource_stalls.any
    {319, S_ID3,  INTEL_IVY, 0,   3,     0,   0x0e,     0x01, 1, 1, "rat.stall"    }, // resource relaged stall cycles: resource_stalls.any
    {320, S_ID3,  INTEL_IVY, 0,   3,     0,   0xa2,     0x04, 0, 0, "rs.stl."    }, // resource relaged stall cycles: resource_stalls.any
    {321, S_ID3,  INTEL_IVY, 0,   3,     0,   0xa2,     0x10, 0, 0, "rob.stl."    }, // resource relaged stall cycles: resource_stalls.any
    {322, S_ID3,  INTEL_IVY, 0,   3,     0,   0xb1,     0x02, 1, 1, "dpch.stl2"    }, // resource relaged stall cycles: resource_stalls.any
    {323, S_ID3,  INTEL_IVY, 0,   3,     0,   0x79,     0x08, 0, 0, "ucacheUps"    }, // resource relaged stall cycles: resource_stalls.any
    {324, S_ID3,  INTEL_IVY, 0,   3,     0,   0x87,     0x01, 0, 0, "LCPstall"    }, // Stall cycles because IQ is full.
    {325, S_ID3,  INTEL_IVY, 0,   3,     0,   0xa8,     0x01, 0, 0, "LSD.Uops"    }, // Stall cycles because IQ is full.
    {326, S_ID3,  INTEL_IVY, 0,   3,     0,   0xab,     0x02, 0, 0, "dsb2mite"    }, // Stall cycles because IQ is full.
    {327, S_ID3,  INTEL_IVY, 0,   3,     0,   0x80,     0x02, 0, 0, "ichmiss"    }, // Stall cyyycles because IQ is full.
    {328, S_ID3,  INTEL_IVY, 0,   3,     0,   0x53,     0x01, 0, 0, "RS_EVENTS.EMPTY_CYCLES"    }, // Stall cycles because IQ is full.
    {329, S_ID3,  INTEL_IVY, 0,   3,     0,   0x58,     0x01, 0, 0, "MOVE_ELIMINATION.ELIMINATED"    }, // Stall cycles because IQ is full.
    {330, S_ID3,  INTEL_IVY, 0,   3,     0,   0x79,     0x24, 0, 4, "IDQ.ALL_MITE_CYCLES_4_UOPS"    }, // Stall cycles because IQ is full.
    {331, S_ID3,  INTEL_IVY, 0,   3,     0,   0x79,     0x24, 0, 1, "IDQ.ALL_MITE_CYCLES_ANY_UOPS"    }, // Stall cycles because IQ is full.
    {332, S_ID3,  INTEL_IVY, 0,   3,     0,   0x80,     0x04, 0, 0, "ICACHE.IFETCH_STALL"    }, // Stall cycles because IQ is full.
    {333, S_ID3,  INTEL_IVY, 0,   3,     0,   0x5B,     0x0c, 0, 0, "FL_EMPTY"    }, // Resource stalls out of order resources full.
    {334, S_ID3,  INTEL_IVY, 0,   3,     0,   0x79,     0x0c, 0, 0, "msrom"    }, // Resource stalls out of order resources full.
    {335, S_ID3,  INTEL_IVY, 0,   3,     0,   0x79,     0x0c, 1, 1, "MS_SWITCH"    }, // Resource stalls out of order resources full.
    // Haswell
    // The first three counters are fixed-function counters having their own register,
    // The rest of the counters are competing for the same four counter registers.
    // id   scheme  cpu       countregs eventreg event  mask   name
    {1,   S_ID3,  INTEL_HASW, 0x40000001,  0,0,   0,     0,   0, 0, "Core cyc"   }, // core clock cycles
    {2,   S_ID3,  INTEL_HASW, 0x40000002,  0,0,   0,     0,   0, 0, "Ref cyc"    }, // Reference clock cycles
    {9,   S_ID3,  INTEL_HASW, 0x40000000,  0,0,   0,     0,   0, 0, "Instruct"   }, // Instructions (reference counter)
    {10,  S_ID3,  INTEL_HASW, 0,  3,     0,   0xc0,     0x01, 0, 0, "Instruct"   }, // Instructions
    {22,  S_ID3,  INTEL_HASW, 0,  3,     0,   0x87,     0x01, 0, 0, "ILenStal"   }, // instruction length decoder stall due to length changing prefix
    {24,  S_ID3,  INTEL_HASW, 0,  3,     0,   0xA8,     0x01, 0, 0, "Loop uops"  }, // uops from loop stream detector
    {25,  S_ID3,  INTEL_HASW, 0,  3,     0,   0x79,     0x04, 0, 0, "Dec uops"   }, // uops from decoders. (MITE = Micro-instruction Translation Engine)
    {26,  S_ID3,  INTEL_HASW, 0,  3,     0,   0x79,     0x08, 0, 0, "Cach uops"  }, // uops from uop cache. (DSB = Decoded Stream Buffer)
    {100, S_ID3,  INTEL_HASW, 0,  3,     0,   0xc2,     0x01, 0, 0, "Uops"       }, // uops retired, unfused domain
    {104, S_ID3,  INTEL_HASW, 0,  3,     0,   0x0e,     0x01, 0, 0, "uops RAT"   }, // uops from RAT to RS
    {111, S_ID3,  INTEL_HASW, 0,  3,     0,   0xa2,     0x01, 0, 0, "res.stl."   }, // any resource stall
    {131, S_ID3,  INTEL_HASW, 0,  3,     0,   0xC1,     0x18, 0, 0, "AVX trans"  }, // VEX - non-VEX transition penalties
    {150, S_ID3,  INTEL_HASW, 0,  3,     0,   0xa1,     0x01, 0, 0, "uop p0"     }, // uops port 0.
    {151, S_ID3,  INTEL_HASW, 0,  3,     0,   0xa1,     0x02, 0, 0, "uop p1"     }, // uops port 1.
    {152, S_ID3,  INTEL_HASW, 0,  3,     0,   0xa1,     0x04, 0, 0, "uop p2"     }, // uops port 2.
    {153, S_ID3,  INTEL_HASW, 0,  3,     0,   0xa1,     0x08, 0, 0, "uop p3"     }, // uops port 3.
    {154, S_ID3,  INTEL_HASW, 0,  3,     0,   0xa1,     0x10, 0, 0, "uop p4"     }, // uops port 4.
    {155, S_ID3,  INTEL_HASW, 0,  3,     0,   0xa1,     0x20, 0, 0, "uop p5"     }, // uops port 5.
    {156, S_ID3,  INTEL_HASW, 0,  3,     0,   0xa1,     0x40, 0, 0, "uop p6"     }, // uops port 6.
    {157, S_ID3,  INTEL_HASW, 0,  3,     0,   0xa1,     0x80, 0, 0, "uop p7"     }, // uops port 7.
    {160, S_ID3,  INTEL_HASW, 0,  3,     0,   0xa1,     0xFF, 0, 0, "uop p07"    }, // uops port 0-7
    {201, S_ID3,  INTEL_HASW, 0,  3,     0,   0xC4,     0x20, 0, 0, "BrTaken"    }, // branches taken
    {207, S_ID3,  INTEL_HASW, 0,  3,     0,   0xc5,     0x00, 0, 0, "BrMispred"  }, // mispredicted branches
    {220, S_ID3,  INTEL_HASW, 0,  3,     0,   0x58,     0x03, 0, 0, "Mov elim"   }, // register moves eliminated
    {221, S_ID3,  INTEL_HASW, 0,  3,     0,   0x58,     0x0C, 0, 0, "Mov elim-"  }, // register moves elimination unsuccessful
    {310, S_ID3,  INTEL_HASW, 0,  3,     0,   0x80,     0x02, 0, 0, "CodeMiss"   }, // code cache misses
    {311, S_ID3,  INTEL_HASW, 0,  3,     0,   0x24,     0xe1, 0, 0, "L1D Miss"   }, // level 1 data cache miss
    {320, S_ID3,  INTEL_HASW, 0,  3,     0,   0x24,     0x27, 0, 0, "L2 Miss"    }, // level 2 cache misses

    // Skylake
    // The first three counters are fixed-function counters having their own register,
    // The rest of the counters are competing for the same four counter registers.
    // id   scheme  cpu       countregs eventreg event  mask   name
    {1,   S_ID4,  INTEL_SKYL, 0x40000001,  0,0,   0,     0,   0, 0, "Core cyc"   }, // core clock cycles
    {2,   S_ID4,  INTEL_SKYL, 0x40000002,  0,0,   0,     0,   0, 0, "Ref cyc"    }, // Reference clock cycles
    {9,   S_ID4,  INTEL_SKYL, 0x40000000,  0,0,   0,     0,   0, 0, "Instruct"   }, // Instructions (reference counter)
    {10,  S_ID4,  INTEL_SKYL, 0,  3,     0,   0xc0,     0x01, 0, 0, "Instruct"   }, // Instructions
    {22,  S_ID4,  INTEL_SKYL, 0,  3,     0,   0x87,     0x01, 0, 0, "ILenStal"   }, // instruction length decoder stall due to length changing prefix
    {24,  S_ID4,  INTEL_SKYL, 0,  3,     0,   0xA8,     0x01, 0, 0, "Loop uops"  }, // uops from loop stream detector
    {25,  S_ID4,  INTEL_SKYL, 0,  3,     0,   0x79,     0x04, 0, 0, "Dec uops"   }, // uops from decoders. (MITE = Micro-instruction Translation Engine)
    {26,  S_ID4,  INTEL_SKYL, 0,  3,     0,   0x79,     0x08, 0, 0, "Cach uops"  }, // uops from uop cache. (DSB = Decoded Stream Buffer)
    {100, S_ID4,  INTEL_SKYL, 0,  3,     0,   0xc2,     0x01, 0, 0, "Uops"       }, // uops retired, unfused domain
    {104, S_ID4,  INTEL_SKYL, 0,  3,     0,   0x0e,     0x01, 0, 0, "uops RAT"   }, // uops from RAT to RS
    {111, S_ID4,  INTEL_SKYL, 0,  3,     0,   0xa2,     0x01, 0, 0, "res.stl."   }, // any resource stall
    {131, S_ID4,  INTEL_SKYL, 0,  3,     0,   0xC1,     0x18, 0, 0, "AVX trans"  }, // VEX - non-VEX transition penalties
    {150, S_ID4,  INTEL_SKYL, 0,  3,     0,   0xa1,     0x01, 0, 0, "uop p0"     }, // uops port 0.
    {151, S_ID4,  INTEL_SKYL, 0,  3,     0,   0xa1,     0x02, 0, 0, "uop p1"     }, // uops port 1.
    {152, S_ID4,  INTEL_SKYL, 0,  3,     0,   0xa1,     0x04, 0, 0, "uop p2"     }, // uops port 2.
    {153, S_ID4,  INTEL_SKYL, 0,  3,     0,   0xa1,     0x08, 0, 0, "uop p3"     }, // uops port 3.
    {154, S_ID4,  INTEL_SKYL, 0,  3,     0,   0xa1,     0x10, 0, 0, "uop p4"     }, // uops port 4.
    {155, S_ID4,  INTEL_SKYL, 0,  3,     0,   0xa1,     0x20, 0, 0, "uop p5"     }, // uops port 5.
    {156, S_ID4,  INTEL_SKYL, 0,  3,     0,   0xa1,     0x40, 0, 0, "uop p6"     }, // uops port 6.
    {157, S_ID4,  INTEL_SKYL, 0,  3,     0,   0xa1,     0x80, 0, 0, "uop p7"     }, // uops port 7.
    {160, S_ID4,  INTEL_SKYL, 0,  3,     0,   0xa1,     0xFF, 0, 0, "uop p07"    }, // uops port 0-7
    {201, S_ID4,  INTEL_SKYL, 0,  3,     0,   0xC4,     0x20, 0, 0, "BrTaken"    }, // branches taken
    {202, S_ID4,  INTEL_SKYL, 0,  3,     0,   0xE6,     0x01, 0, 0, "BACLEARS"    }, // branches taken
    {205, S_ID4,  INTEL_SKYL, 0,  3,     0,   0xe6,      2,   0, 0, "BTBMiss"    }, // static branch prediction made, Sandy Bridge
    {207, S_ID4,  INTEL_SKYL, 0,  3,     0,   0xC5,     0x00, 0, 0, "BrMispred"  }, // mispredicted branches
    {220, S_ID4,  INTEL_SKYL, 0,  3,     0,   0x58,     0x03, 0, 0, "Mov elim"   }, // register moves eliminated
    {221, S_ID4,  INTEL_SKYL, 0,  3,     0,   0x58,     0x0C, 0, 0, "Mov elim-"  }, // register moves elimination unsuccessful
    {311, S_ID4,  INTEL_SKYL, 0,  3,     0,   0x24,     0xe1, 0, 0, "L1D Miss"   }, // level 1 data cache miss
    {313, S_ID4,  INTEL_SKYL, 0,   3,     0,   0x87,     0x04, 0, 0, "IQFULL"    }, // Stall cycles because IQ is full.
    {314, S_ID4,  INTEL_SKYL, 0,   3,     0,   0x5B,     0x4f, 0, 0, "OOOFull"    }, // Resource stalls out of order resources full.
    {315, S_ID4,  INTEL_SKYL, 0,   3,     0,   0x9C,     0x01, 0, 0, "frnt-bnd"    }, // uop not delvd. to backend per cycle when back-end not stalled.
    {316, S_ID4,  INTEL_SKYL, 0,   3,     0,   0xa2,     0x01, 0, 0, "res.stl."    }, // resource relaged stall cycles: resource_stalls.any
    {317, S_ID4,  INTEL_SKYL, 0,   3,     0,   0xb1,     0x01, 1, 1, "dpch.stl"    }, // resource relaged stall cycles: resource_stalls.any
    {318, S_ID4,  INTEL_SKYL, 0,   3,     0,   0xc2,     0x02, 0, 0, "retrd.slt"    }, // resource relaged stall cycles: resource_stalls.any
    {319, S_ID4,  INTEL_SKYL, 0,   3,     0,   0x0e,     0x01, 1, 1, "rat.stall"    }, // resource relaged stall cycles: resource_stalls.any
    {320, S_ID4,  INTEL_SKYL, 0,   3,     0,   0xa2,     0x04, 0, 0, "rs.stl."    }, // resource relaged stall cycles: resource_stalls.any
    {321, S_ID4,  INTEL_SKYL, 0,   3,     0,   0xa2,     0x10, 0, 0, "rob.stl."    }, // resource relaged stall cycles: resource_stalls.any
    {322, S_ID4,  INTEL_SKYL, 0,   3,     0,   0xb1,     0x02, 1, 1, "dpch.stl2"    }, // resource relaged stall cycles: resource_stalls.any
    {323, S_ID4,  INTEL_SKYL, 0,   3,     0,   0x79,     0x08, 0, 0, "ucacheUps"    }, // resource relaged stall cycles: resource_stalls.any
    {324, S_ID4,  INTEL_SKYL, 0,   3,     0,   0x87,     0x01, 0, 0, "LCPstall"    }, // Stall cycles because IQ is full.
    {325, S_ID4,  INTEL_SKYL, 0,   3,     0,   0xa8,     0x01, 0, 0, "LSD.Uops"    }, // Stall cycles because IQ is full.
    {326, S_ID4,  INTEL_SKYL, 0,   3,     0,   0xab,     0x02, 0, 0, "dsb2mite"    }, // Stall cycles because IQ is full.
    {327, S_ID4,  INTEL_SKYL, 0,   3,     0,   0x80,     0x02, 0, 0, "ichmiss"    }, // Stall cyyycles because IQ is full.
    {328, S_ID4,  INTEL_SKYL, 0,   3,     0,   0x53,     0x01, 0, 0, "RS_EVENTS.EMPTY_CYCLES"    }, // Stall cycles because IQ is full.
    {329, S_ID4,  INTEL_SKYL, 0,   3,     0,   0x58,     0x01, 0, 0, "MOVE_ELIMINATION.ELIMINATED"    }, // Stall cycles because IQ is full.
    {330, S_ID4,  INTEL_SKYL, 0,   3,     0,   0x79,     0x24, 0, 4, "IDQ.ALL_MITE_CYCLES_4_UOPS"    }, // Stall cycles because IQ is full.
    {331, S_ID4,  INTEL_SKYL, 0,   3,     0,   0x79,     0x24, 0, 1, "IDQ.ALL_MITE_CYCLES_ANY_UOPS"    }, // Stall cycles because IQ is full.
    {332, S_ID4,  INTEL_SKYL, 0,   3,     0,   0x80,     0x04, 0, 0, "ICACHE.IFETCH_STALL"    }, // Stall cycles because IQ is full.
    {333, S_ID4,  INTEL_SKYL, 0,   3,     0,   0x24,     0x27, 0, 0, "L2 Miss"    }, // level 2 cache misses

    {  9, S_AMD2, AMD_ZEN,     0,   5,     0,   0xc0,      1, 0, 0,   "Instruct" }, // x86 instructions executed
    {100, S_AMD2, AMD_ZEN,     0,   5,     0,   0xc1,      0, 0, 0,   "Uops"     }, // microoperations
    {110, S_AMD2, AMD_ZEN,     0,   5,     0,   0x04,   0x0a, 0, 0,   "UopsElim" }, // move eliminations and scalar op optimizations
    {150, S_AMD2, AMD_ZEN,     0,   5,     0,   0x00,   0x01, 0, 0,   "UopsFP0"  }, // microoperations in FP pipe 0
    {151, S_AMD2, AMD_ZEN,     0,   5,     0,   0x00,   0x02, 0, 0,   "UopsFP1"  }, // microoperations in FP pipe 1
    {152, S_AMD2, AMD_ZEN,     0,   5,     0,   0x00,   0x04, 0, 0,   "UopsFP2"  }, // microoperations in FP pipe 2
    {153, S_AMD2, AMD_ZEN,     0,   5,     0,   0x00,   0x08, 0, 0,   "UopsFP3"  }, // microoperations in FP pipe 3
    {158, S_AMD2, AMD_ZEN,     0,   5,     0,   0x00,   0xF0, 0, 0,   "UMultiP"  }, // microoperations using multiple FP pipes
    {159, S_AMD2, AMD_ZEN,     0,   5,     0,   0x00,   0x0F, 0, 0,   "UopsFP"   }, // microoperations in FP
    {120, S_AMD2, AMD_ZEN,     0,   5,     0,   0x35,   0x01, 0, 0,   "Forw"     }, // load-to-store forwards
    {160, S_AMD2, AMD_ZEN,     0,   5,     0,   0x02,   0x07, 0, 0,   "x87"      }, // FP x87 instructions
    {162, S_AMD2, AMD_ZEN,     0,   5,     0,   0x03,   0xFF, 0, 0,   "Vect"     }, // XMM and YMM instructions
    {204, S_AMD2, AMD_ZEN,     0,   5,     0,   0xc3,      0, 0, 0,   "BrMispred"}, // mispredicted branches
    {201, S_AMD2, AMD_ZEN,     0,   5,     0,   0xc4,   0x00, 0, 0,   "BrTaken"  }, // branches taken

    {202, S_AMD2, AMD_ZEN,     0,   5,     0,   0x8a,   0x00, 0, 0,   "btbL1hit"  }, 
    {203, S_AMD2, AMD_ZEN,     0,   5,     0,   0x8b,   0x00, 0, 0,   "btbL2hit"  }, 
    {205, S_AMD2, AMD_ZEN,     0,   5,     0,   0x91,   0x00, 0, 0,   "DecCorrBr"  }, // #Decode corrected branches

    {310, S_AMD2, AMD_ZEN,     0,   5,     0,   0x81,      0, 0, 0,   "CodeMiss" }, // instruction cache misses
    {315, S_AMD2, AMD_ZEN,     0,   5,     0,   0x45,      1, 0, 0,   "dTLBMiss" }, // instruction cache misses
    {316, S_AMD2, AMD_ZEN,     0,   5,     0,   0x85,      0, 0, 0,   "iTLBMiss" }, // instruction cache misses
    {317, S_AMD2, AMD_ZEN,     0,   5,     0,   0x84,      0, 0, 0,   "iTLBL2hit" }, // instruction cache misses
    {320, S_AMD2, AMD_ZEN,     0,   5,     0,   0x60,   0xFF, 0, 0,   "L2 req."  }, // L2 cache requests
    {320, S_AMD2, AMD_ZEN,     0,   5,     0,   0x60,   0xFF, 0, 0,   "L2 req."  }, // L2 cache requests
    {321, S_AMD2, AMD_ZEN,     0,   5,     0,   0xaa,   0x02, 0, 0,   "ucacheUps"  }, // L2 cache requests
    {322, S_AMD2, AMD_ZEN,     0,   5,     0,   0xae,   0x40, 0, 0,   "FP-SchStl"  }, // FP scheduler resource stall
    {323, S_AMD2, AMD_ZEN,     0,   5,     0,   0xae,   0x01, 0, 0,   "I-RFStl"  }, // Integer Physical Register File resource stall. 
    {324, S_AMD2, AMD_ZEN,     0,   5,     0,   0xae,   0x08, 0, 0,   "I-MiscStl"  }, // Integer Physical Register File resource stall.
    {325, S_AMD2, AMD_ZEN,     0,   5,     0,   0xae,   0x80, 0, 0,   "FP-MiscStl"  }, // FP Miscellaneous resource unavailable.
    {326, S_AMD2, AMD_ZEN,     0,   5,     0,   0xae,   0x20, 0, 0,   "FP-RFStl"  }, // FP Miscellaneous resource unavailable.
    {327, S_AMD2, AMD_ZEN,     0,   5,     0,   0xae,   0x02, 0, 0,   "LQStl"  }, // FP Miscellaneous resource unavailable.
    {328, S_AMD2, AMD_ZEN,     0,   5,     0,   0xaf,   0x10, 0, 0,   "AGUStl"  }, // FP Miscellaneous resource unavailable.
    {329, S_AMD2, AMD_ZEN,     0,   5,     0,   0xaf,   0x04, 0, 0,   "ALSQ03"  }, // FP Miscellaneous resource unavailable.
    {330, S_AMD2, AMD_ZEN,     0,   5,     0,   0xaf,   0x02, 0, 0,   "ALSQ2"  }, // FP Miscellaneous resource unavailable.
    {331, S_AMD2, AMD_ZEN,     0,   5,     0,   0xaf,   0x01, 0, 0,   "ALSQ1"  }, // FP Miscellaneous resource unavailable.




    //  end of list   
    {0, S_UNKNOWN, PRUNKNOWN, 0,  0,     0,      0,     0,    0, 0, 0     }  // list must end with a record of all 0
};
