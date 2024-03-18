SHARE_MEM
=========

version 1. - nov 2012
cyrille henry - nicolas montgermont
windows version : A.Villeret

INFOS
=====

share_mem allows the usage of shared memory in Pd.

BUILD & INSTALL
===============


To build you need `cmake` and a compiling toolchain. Then run those steps from the repo folder : 

    cmake -S . -B build 
    cmake --build build
    cmake --build build --target install

Then you'll find the `shmem` package folder under `build/package/shmem`.
