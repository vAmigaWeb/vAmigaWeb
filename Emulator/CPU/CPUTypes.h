// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "Aliases.h"
#include "Reflection.h"

#ifdef __cplusplus
#include "RingBuffer.h"
#endif

#define CPUINFO_INSTR_COUNT 256


//
// Enumerations
//

enum_long(CPU_REVISION)
{
    CPU_MC68000
};
typedef CPU_REVISION CPURevision;

#ifdef __cplusplus
struct CPURevisionEnum : util::Reflection<CPURevisionEnum, CPURevision>
{
    static constexpr long minVal = 0;
    static constexpr long maxVal = CPU_MC68000;
    static bool isValid(auto val) { return val >= minVal && val <= maxVal; }

    static const char *prefix() { return "CPU"; }
    static const char *key(CPURevision value)
    {
        switch (value) {

            case CPU_MC68000:   return "MC68000";
        }
        return "???";
    }
};
#endif


//
// Structures
//

typedef struct
{
    CPURevision revision;
    isize overclocking;
    u32 regResetVal;
}
CPUConfig;

typedef struct
{
    u32 pc0;
    u32 d[8];
    u32 a[8];
    u32 usp;
    u32 ssp;
    u16 sr;
}
CPUInfo;

#ifdef __cplusplus
struct CallStackEntry
{
    // Opcode of the branch instruction
    u16 opcode;
    
    // Program counter and subroutine address
    u32 oldPC;
    u32 newPC;
    
    // Register contents
    u32 d[8];
    u32 a[8];

    template <class W>
    void operator<<(W& worker)
    {
        worker << opcode << oldPC << newPC << d << a;
    }
};

struct CallstackRecorder : public util::SortedRingBuffer<CallStackEntry, 64>
{
    template <class W>
    void operator<<(W& worker)
    {
        worker >> this->elements << this->r << this->w << this->keys;
    }
};
#endif
