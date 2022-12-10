/*
 * Copyright (C) 2004-2021 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

#include <iostream>
#include <fstream>
#include<vector>
#include <cstdlib>
#include <cstddef>
#include <unistd.h>
#include "pin.H"
typedef UINT32 CACHE_STATS; 
#include "pin_cache.H"

using std::cerr;
using std::endl;
using std::ios;
using std::ofstream;
using std::string;
using std::vector;

FILE* trace;

 __inline__ uint64_t rdtsc(void)
   {
uint32_t lo, hi;
__asm__ __volatile__ (
        "xorl %%eax,%%eax \n        cpuid"
        ::: "%rax", "%rbx", "%rcx", "%rdx");
__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
return (uint64_t)hi << 32 | lo;
 }

namespace ITLB
{
// instruction TLB: 4 kB pages, 32 entries, fully associative
const UINT32 lineSize                          = 4 * KILO;
const UINT32 cacheSize                         = 64 * lineSize;
const UINT32 associativity                     = 4;
const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;

const UINT32 max_sets          = cacheSize / (lineSize * associativity);
const UINT32 max_associativity = associativity;

typedef CACHE_ROUND_ROBIN(max_sets, max_associativity, allocation) CACHE;
} // namespace ITLB
static ITLB::CACHE itlb("ITLB", ITLB::cacheSize, ITLB::lineSize, ITLB::associativity);

namespace DTLB
{
// data TLB: 4 kB pages, 32 entries, fully associative
const UINT32 lineSize                          = 4 * KILO;
const UINT32 cacheSize                         = 64 * lineSize;
const UINT32 associativity                     = 4;
const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;

const UINT32 max_sets          = cacheSize / (lineSize * associativity);
const UINT32 max_associativity = associativity;

typedef CACHE_ROUND_ROBIN(max_sets, max_associativity, allocation) CACHE;
} // namespace DTLB
static DTLB::CACHE dtlb("DTLB", DTLB::cacheSize, DTLB::lineSize, DTLB::associativity);

namespace IL1
{
// 1st level instruction cache: 32 kB, 32 B lines, 32-way associative
const UINT32 cacheSize                         = 32 * KILO;
const UINT32 lineSize                          = 64;
const UINT32 associativity                     = 1;
const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_NO_ALLOCATE;

const UINT32 max_sets          = cacheSize / (lineSize * associativity);
const UINT32 max_associativity = associativity;

typedef CACHE_ROUND_ROBIN(max_sets, max_associativity, allocation) CACHE;
} // namespace IL1
static IL1::CACHE il1("L1 Instruction Cache", IL1::cacheSize, IL1::lineSize, IL1::associativity);

namespace DL1
{
// 1st level data cache: 32 kB, 32 B lines, 32-way associative
const UINT32 cacheSize                         = 32 * KILO;
const UINT32 lineSize                          = 64;
const UINT32 associativity                     = 8;
const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_NO_ALLOCATE;

const UINT32 max_sets          = cacheSize / (lineSize * associativity);
const UINT32 max_associativity = associativity;

typedef CACHE_ROUND_ROBIN(max_sets, max_associativity, allocation) CACHE;
} // namespace DL1
static DL1::CACHE dl1("L1 Data Cache", DL1::cacheSize, DL1::lineSize, DL1::associativity);

namespace UL2
{
// 2nd level unified cache: 2 MB, 64 B lines, direct mapped
const UINT32 cacheSize                         = 256 * KILO;
const UINT32 lineSize                          = 64;
const UINT32 associativity                     = 4;
const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;

const UINT32 max_sets = cacheSize / (lineSize * associativity);
const UINT32 max_associativity = associativity;

typedef CACHE_ROUND_ROBIN(max_sets, max_associativity, allocation) CACHE;
} // namespace UL2
static UL2::CACHE ul2("L2 Unified Cache", UL2::cacheSize, UL2::lineSize, UL2::associativity);

namespace UL3
{
// 3rd level unified cache: 16 MB, 64 B lines, direct mapped
const UINT32 cacheSize                         = 8 * MEGA;
const UINT32 lineSize                          = 64;
const UINT32 associativity                     = 16;
const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;

const UINT32 max_sets = cacheSize / (lineSize * associativity);
const UINT32 max_associativity = associativity;

typedef CACHE_ROUND_ROBIN(max_sets, max_associativity, allocation) CACHE;
} // namespace UL3
static UL3::CACHE ul3("L3 Unified Cache", UL3::cacheSize, UL3::lineSize, UL3::associativity);

VOID call(ADDRINT addr, int a){
    char buff[6] = "WRITE";
    if(a){
        sprintf(buff, "READ");
    }
	fprintf(trace,"%p %s %lu\n", (void *)(addr&((2ull << 34) - 1)), buff, rdtsc());
}

VOID Ul2Access(ADDRINT addr, UINT32 size, CACHE_BASE::ACCESS_TYPE accessType)
{
    // second level unified cache
    const BOOL ul2Hit = ul2.Access(addr, size, accessType);
    int value = 0;
    if(accessType == CACHE_BASE::ACCESS_TYPE_LOAD){
        value = 1;
    }
    // third level unified cache
    if ( ! ul2Hit && !ul3.Access(addr, size, accessType))
		call(addr, value);
}

 VOID InsRef(ADDRINT addr)
{
    const UINT32 size = 1; // assuming access does not cross cache lines
    const CACHE_BASE::ACCESS_TYPE accessType = CACHE_BASE::ACCESS_TYPE_LOAD;

    // ITLB
    if(!itlb.AccessSingleLine(addr, accessType)){
    	call(addr, 1);
			return;
    }

    // first level I-cache
    const BOOL il1Hit = il1.AccessSingleLine(addr, accessType);

    // second level unified Cache
    if ( ! il1Hit) Ul2Access(addr, size, accessType);
}

 VOID MemRefMulti(ADDRINT addr, UINT32 size, CACHE_BASE::ACCESS_TYPE accessType)
{
    // DTLB
    if (!dtlb.AccessSingleLine(addr, CACHE_BASE::ACCESS_TYPE_LOAD)) {
		call(addr, 1);
			return;
	}
    // first level D-cache
    const BOOL dl1Hit = dl1.Access(addr, size, accessType);

    // second level unified Cache
    if ( ! dl1Hit) Ul2Access(addr, size, accessType);
}

 VOID MemRefSingle(ADDRINT addr, UINT32 size, CACHE_BASE::ACCESS_TYPE accessType)
{
    // DTLB
    if (!dtlb.AccessSingleLine(addr, CACHE_BASE::ACCESS_TYPE_LOAD)) {
		call(addr, 1);
			return;
	}

    // first level D-cache
    const BOOL dl1Hit = dl1.AccessSingleLine(addr, accessType);

    // second level unified Cache
    if ( ! dl1Hit) Ul2Access(addr, size, accessType);
}

static VOID Instruction(INS ins, VOID* v)
{
    // all instruction fetches access I-cache
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)InsRef, IARG_INST_PTR, IARG_END);

    if (!INS_IsStandardMemop(ins)) return;
    if (INS_MemoryOperandCount(ins) == 0) return;
    ;

    UINT32 readSize = 0, writeSize = 0;
    UINT32 readOperandCount = 0, writeOperandCount = 0;

    for (UINT32 opIdx = 0; opIdx < INS_MemoryOperandCount(ins); opIdx++)
    {
        if (INS_MemoryOperandIsRead(ins, opIdx))
        {
            readSize = INS_MemoryOperandSize(ins, opIdx);
            readOperandCount++;
            break;
        }
        if (INS_MemoryOperandIsWritten(ins, opIdx))
        {
            writeSize = INS_MemoryOperandSize(ins, opIdx);
            writeOperandCount++;
            break;
        }
    }

    if (readOperandCount > 0)
    {
        const AFUNPTR countFun = (readSize <= 4 ? (AFUNPTR)MemRefSingle : (AFUNPTR)MemRefMulti);

        // only predicated-on memory instructions access D-cache
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, countFun, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_UINT32,
                                 CACHE_BASE::ACCESS_TYPE_LOAD, IARG_END);
    }

    if (writeOperandCount > 0)
    {
        const AFUNPTR countFun = (writeSize <= 4 ? (AFUNPTR)MemRefSingle : (AFUNPTR)MemRefMulti);

        // only predicated-on memory instructions access D-cache
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, countFun, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_UINT32,
                                 CACHE_BASE::ACCESS_TYPE_STORE, IARG_END);
    }
}

// KNOB< string > KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "traces.out", "specify output file name");

// This function is called when the application exits
VOID Fini(INT32 code, VOID* v)
{
       
	fclose(trace);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool counts the number of dynamic instructions executed" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*   argc, argv are the entire command line: pin -t <toolname> -- ...    */
/* ===================================================================== */

int main(int argc, char* argv[])
{
    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    //OutFile.open(KnobOutputFile.Value().c_str());
	trace = fopen("tracess.out", "w+");
    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}

