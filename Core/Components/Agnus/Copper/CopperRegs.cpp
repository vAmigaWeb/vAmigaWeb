// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Copper.h"
#include "Agnus.h"

namespace vamiga {

void
Copper::pokeCOPCON(u16 value)
{
    trace(COPREG_DEBUG, "pokeCOPCON(%04X)\n", value);
    
    /* "This is a 1-bit register that when set true, allows the Copper to
     *  access the blitter hardware. This bit is cleared by power-on reset, so
     *  that the Copper cannot access the blitter hardware." [HRM]
     */
    cdang = (value & 0b10) != 0;
}

template <Accessor s> void
Copper::pokeCOPJMP1()
{
    trace(COPREG_DEBUG, "pokeCOPJMP1: Jumping to %X\n", cop1lc);

    if constexpr (s == Accessor::AGNUS) {

        fatalError;

    }
    if constexpr (s == Accessor::CPU) {

        if (agnus.blitter.isActive()) {
            xfiles("pokeCOPJMP1: Blitter is running\n");
        }
        switchToCopperList(1);
    }
}

template <Accessor s> void
Copper::pokeCOPJMP2()
{
    trace(COPREG_DEBUG, "pokeCOPJMP2(): Jumping to %X\n", cop2lc);

    if constexpr (s == Accessor::AGNUS) {

        fatalError;

    }
    if constexpr (s == Accessor::CPU) {

        if (agnus.blitter.isActive()) {
            xfiles("pokeCOPJMP2: Blitter is running\n");
        }
        switchToCopperList(2);
    }
}

void
Copper::pokeCOPINS(u16 value)
{
    trace(COPREG_DEBUG, "COPPC: %X pokeCOPINS(%04X)\n", coppc0, value);

    // Manually writing into COPINS seems to have no noticeable effect
    xfiles("Write to COPINS (%x)\n", value);
}

void
Copper::pokeCOP1LCH(u16 value)
{
    trace(COPREG_DEBUG, "pokeCOP1LCH(%04X)\n", value);

    if (HI_WORD(cop1lc) != value) {
        
        cop1lc = REPLACE_HI_WORD(cop1lc, value);

        if (!activeInThisFrame && copList == 1) {
            setPC(cop1lc);
        }
    }
}

void
Copper::pokeCOP1LCL(u16 value)
{
    trace(COPREG_DEBUG, "pokeCOP1LCL(%04X)\n", value);

    value &= 0xFFFE;

    if (LO_WORD(cop1lc) != value) {
        
        cop1lc = REPLACE_LO_WORD(cop1lc, value);
        
        if (!activeInThisFrame && copList == 1) {
            setPC(cop1lc);
        }
    }
}

void
Copper::pokeCOP2LCH(u16 value)
{
    trace(COPREG_DEBUG, "pokeCOP2LCH(%04X)\n", value);

    if (HI_WORD(cop2lc) != value) {
        
        cop2lc = REPLACE_HI_WORD(cop2lc, value);

        if (!activeInThisFrame && copList == 2) {
            setPC(cop2lc);
        }
    }
}

void
Copper::pokeCOP2LCL(u16 value)
{
    trace(COPREG_DEBUG, "pokeCOP2LCL(%04X)\n", value);

    value &= 0xFFFE;

    if (LO_WORD(cop2lc) != value) {

        cop2lc = REPLACE_LO_WORD(cop2lc, value);
        
        if (!activeInThisFrame && copList == 2) {
            setPC(cop2lc);
        }
    }
}

void
Copper::pokeNOOP(u16 value)
{
    trace(COPREG_DEBUG, "pokeNOOP(%04X)\n", value);
}

template void Copper::pokeCOPJMP1<Accessor::CPU>();
template void Copper::pokeCOPJMP1<Accessor::AGNUS>();
template void Copper::pokeCOPJMP2<Accessor::CPU>();
template void Copper::pokeCOPJMP2<Accessor::AGNUS>();

}
