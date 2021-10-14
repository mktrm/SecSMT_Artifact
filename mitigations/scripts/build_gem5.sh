cd `dirname "$0"`/../gem5
scons USE_HDF5=0 build/X86/gem5.fast -j 8