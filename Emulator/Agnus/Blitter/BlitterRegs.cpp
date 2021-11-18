// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Blitter.h"
#include "Agnus.h"

void
Blitter::pokeBLTCON0(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTCON0(%X)\n", value);

    agnus.recordRegisterChange(DMA_CYCLES(2), SET_BLTCON0, value);
}

void
Blitter::setBLTCON0(u16 value)
{
    if (BLT_GUARD && running) {
        trace(true, "BLTCON0 written while Blitter is running\n");
    }

    bltcon0 = value;
}

void
Blitter::pokeBLTCON0L(u16 value)
{
    debug(BLTREG_DEBUG || ECSREG_DEBUG, "pokeBLTCON0L(%X)\n", value);

    // ECS only register
    if (agnus.isOCS()) return;

    agnus.recordRegisterChange(DMA_CYCLES(2), SET_BLTCON0L, value);
}

void
Blitter::setBLTCON0L(u16 value)
{
    if (BLT_GUARD && running) {
        trace(true, "BLTCON0L written while Blitter is running\n");
    }
    
    bltcon0 = HI_LO(HI_BYTE(bltcon0), LO_BYTE(value));
}

void
Blitter::setASH(u16 ash)
{
    assert(ash <= 0xF);
    
    bltcon0 = (u16)((bltcon0 & 0x0FFF) | ash << 12);
}

bool
Blitter::incASH()
{
    if ((bltcon0 & 0xF000) == 0xF000) {
        
        bltcon0 &= 0x0FFF;
        return true;
        
    } else {
        
        bltcon0 += 0x1000;
        return false;
    }
}

bool
Blitter::decASH()
{
    if ((bltcon0 & 0xF000) == 0x0000) {
        
        bltcon0 |= 0xF000;
        return true;
        
    } else {
        
        bltcon0 -= 0x1000;
        return false;
    }
}

void
Blitter::pokeBLTCON1(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTCON1(%X)\n", value);
    agnus.recordRegisterChange(DMA_CYCLES(2), SET_BLTCON1, value);
}

void
Blitter::setBLTCON1(u16 value)
{
    if (BLT_GUARD && running) {
        trace(true, "BLTCON1 written while Blitter is running\n");
    }
    
    bltcon1 = value;
}

void
Blitter::setBSH(u16 bsh)
{
    assert(bsh <= 0xF);
    
    bltcon1 = (u16)((bltcon1 & 0x0FFF) | bsh << 12);
}

bool
Blitter::incBSH()
{
    if ((bltcon1 & 0xF000) == 0xF000) {
        
        bltcon1 &= 0x0FFF;
        return true;
        
    } else {
        
        bltcon1 += 0x1000;
        return false;
    }
}

bool
Blitter::decBSH()
{
    if ((bltcon1 & 0xF000) == 0x0000) {
        
        bltcon1 |= 0xF000;
        return true;
        
    } else {
        
        bltcon1 -= 0x1000;
        return false;
    }
}

void
Blitter::pokeBLTAPTH(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTAPTH(%X)\n", value);

    if (BLT_GUARD && running) {
        trace(true, "BLTAPTH written while Blitter is running\n");
    }

    bltapt = REPLACE_HI_WORD(bltapt, value);

    if (BLT_GUARD && (bltapt & ~agnus.ptrMask)) {
        trace(true, "BLTAPT out of range: %x\n", bltapt);
    }
}

void
Blitter::pokeBLTAPTL(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTAPTL(%X)\n", value);
    
    if(BLT_GUARD && running) {
        trace(true, "BLTAPTL written while Blitter is running\n");
    }

    bltapt = REPLACE_LO_WORD(bltapt, value & 0xFFFE);
}

void
Blitter::pokeBLTBPTH(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTBPTH(%X)\n", value);
    
    if(BLT_GUARD && running) {
        trace(true, "BLTBPTH written while Blitter is running\n");
    }
    
    bltbpt = REPLACE_HI_WORD(bltbpt, value);
    
    if (BLT_GUARD && (bltbpt & ~agnus.ptrMask)) {
        trace(true, "BLTBPT out of range: %x\n", bltbpt);
    }
}

void
Blitter::pokeBLTBPTL(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTBPTL(%X)\n", value);
    
    if(BLT_GUARD && running) {
        trace(true, "BLTBPTL written while Blitter is running\n");
    }

    bltbpt = REPLACE_LO_WORD(bltbpt, value & 0xFFFE);
}

void
Blitter::pokeBLTCPTH(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTCPTH(%X)\n", value);
    
    if(BLT_GUARD && running) {
        trace(true, "BLTCPTH written while Blitter is running\n");
    }
    
    bltcpt = REPLACE_HI_WORD(bltcpt, value);
    
    if (BLT_GUARD && (bltcpt & ~agnus.ptrMask)) {
        trace(true, "BLTCPT out of range: %x\n", bltcpt);
    }
}

void
Blitter::pokeBLTCPTL(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTCPTL(%X)\n", value);
    
    if(BLT_GUARD && running) {
        trace(true, "BLTCPTL written while Blitter is running\n");
    }

    bltcpt = REPLACE_LO_WORD(bltcpt, value & 0xFFFE);
}

void
Blitter::pokeBLTDPTH(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTDPTH(%X)\n", value);
    
    if (BLT_GUARD && running) {
        trace(true, "BLTDPTH written while Blitter is running\n");
    }

    bltdpt = REPLACE_HI_WORD(bltdpt, value);
    
    if (BLT_GUARD && (bltdpt & ~agnus.ptrMask)) {
        trace(true, "BLTDPT out of range: %x\n", bltdpt);
    }
}

void
Blitter::pokeBLTDPTL(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTDPTL(%X)\n", value);
    
    if (BLT_GUARD && running) {
        trace(true, "BLTDPTL written while Blitter is running\n");
    }
    
    bltdpt = REPLACE_LO_WORD(bltdpt, value & 0xFFFE);
}

void
Blitter::pokeBLTAFWM(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTAFWM(%X)\n", value);
    
    if (BLT_GUARD && running) {
        trace(true, "BLTAFWM written while Blitter is running\n");
    }

    bltafwm = value;
}

void
Blitter::pokeBLTALWM(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTALWM(%X)\n", value);
    
    if (BLT_GUARD && running) {
        trace(true, "BLTALWM written while Blitter is running\n");
    }

    bltalwm = value;
}

template <Accessor s> void
Blitter::pokeBLTSIZE(u16 value)
{
    debug(BLTTIM_DEBUG, "(%zd,%zd) BLTSIZE(%x)\n", agnus.pos.v, agnus.pos.h, value);
    debug(BLTREG_DEBUG, "pokeBLTSIZE(%X)\n", value);

    if (s == ACCESSOR_AGNUS) {
        agnus.recordRegisterChange(DMA_CYCLES(1), SET_BLTSIZE, value);
    } else {
        blitter.setBLTSIZE(value);
    }
}

void
Blitter::setBLTSIZE(u16 value)
{
    debug(BLTREG_DEBUG, "setBLTSIZE(%X)\n", value);
    
    if (BLT_GUARD && running) {
        trace(true, "BLTSIZE written while Blitter is running\n");
    }

    // Execute pending event if the Blitter is still running (Chaosland, #437)
    if (running && scheduler.hasEvent<SLOT_BLT>()) serviceEvent();
    
    // 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    // h9 h8 h7 h6 h5 h4 h3 h2 h1 h0 w5 w4 w3 w2 w1 w0
    bltsizeV = value >> 6;
    bltsizeH = value & 0x3F;

    // Overwrite with default values if zero
    if (!bltsizeV) bltsizeV = 0x0400;
    if (!bltsizeH) bltsizeH = 0x0040;
    
    // Warn if the previous Blitter operation is overwritten
    if (EventID id = scheduler.id[SLOT_BLT]) {
        trace(XFILES, "XFILES: Overwriting Blitter event %hhd\n", id);
    }
    
    agnus.scheduleRel<SLOT_BLT>(DMA_CYCLES(1), BLT_STRT1);
}

void
Blitter::pokeBLTSIZV(u16 value)
{
    debug(BLTREG_DEBUG || ECSREG_DEBUG, "pokeBLTSIZV(%X)\n", value);

    // ECS only register
    if (agnus.isOCS()) return;

    agnus.recordRegisterChange(DMA_CYCLES(2), SET_BLTSIZV, value);
}

void
Blitter::setBLTSIZV(u16 value)
{
    if (BLT_GUARD && running) {
        trace(true, "BLTSIZV written while Blitter is running\n");
    }

    // 15  14  13  12  11  10 09 08 07 06 05 04 03 02 01 00
    //  0 h14 h13 h12 h11 h10 h9 h8 h7 h6 h5 h4 h3 h2 h1 h0
    bltsizeV = value & 0x7FFF;
}

void
Blitter::pokeBLTSIZH(u16 value)
{
    debug(BLTREG_DEBUG || ECSREG_DEBUG, "pokeBLTSIZH(%X)\n", value);

    // ECS only register
    if (agnus.isOCS()) return;

    if (BLT_GUARD && running) {
        trace(true, "BLTSIZH written while Blitter is running\n");
    }

    // Execute pending event if the Blitter is still running
    if (running && scheduler.hasEvent<SLOT_BLT>()) serviceEvent();

    // 15  14  13  12  11  10 09 08 07 06 05 04 03 02 01 00
    //  0   0   0   0   0 w10 w9 w8 w7 w6 w5 w4 w3 w2 w1 w0
    bltsizeH = value & 0x07FF;

    // Overwrite with default values if zero
    if (!bltsizeV) bltsizeV = 0x8000;
    if (!bltsizeH) bltsizeH = 0x0800;

    agnus.scheduleRel<SLOT_BLT>(DMA_CYCLES(1), BLT_STRT1);
}

void
Blitter::pokeBLTAMOD(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTAMOD(%X)\n", value);
    
    if (BLT_GUARD && running) {
        trace(true, "BLTAMOD written while Blitter is running\n");
    }

    bltamod = (i16)(value & 0xFFFE);
}
void
Blitter::pokeBLTBMOD(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTBMOD(%X)\n", value);
    
    if (BLT_GUARD && running) {
        trace(true, "BLTBMOD written while Blitter is running\n");
    }
    
    bltbmod = (i16)(value & 0xFFFE);
}

void
Blitter::pokeBLTCMOD(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTCMOD(%X)\n", value);
    
    if (BLT_GUARD && running) {
        trace(true, "BLTCMOD written while Blitter is running\n");
    }

    bltcmod = (i16)(value & 0xFFFE);
}

void
Blitter::pokeBLTDMOD(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTDMOD(%X)\n", value);
    
    if (BLT_GUARD && running) {
        trace(true, "BLTDMOD written while Blitter is running\n");
    }
    
    bltdmod = (i16)(value & 0xFFFE);
}

void
Blitter::pokeBLTADAT(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTADAT(%X)\n", value);
    
    if (BLT_GUARD && running) {
        trace(true, "BLTADAT written while Blitter is running\n");
    }
    
    anew = value;
}

void
Blitter::pokeBLTBDAT(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTBDAT(%X)\n", value);
    
    if (BLT_GUARD && running) {
        trace(true, "BLTBDAT written while Blitter is running\n");
    }
    
    bnew = value;
    
    // Writing BLTBDAT triggers the barrel shifter circuit (unlike BLTADAT)
    if (bltconDESC()) {
        bhold = (u16)(HI_W_LO_W(bnew, bold) >> (16 - bltconBSH()));
    } else {
        bhold = (u16)(HI_W_LO_W(bold, bnew) >> bltconBSH());
    }
    bold = bnew;
}

void
Blitter::pokeBLTCDAT(u16 value)
{
    debug(BLTREG_DEBUG, "pokeBLTCDAT(%X)\n", value);
    
    if (BLT_GUARD && running) {
        trace(true, "BLTCDAT written while Blitter is running\n");
    }
    
    chold = value;
}

void
Blitter::pokeDMACON(u16 oldValue, u16 newValue)
{
    bool oldBltDma = (oldValue & (DMAEN | BLTEN)) == (DMAEN | BLTEN);
    bool newBltDma = (newValue & (DMAEN | BLTEN)) == (DMAEN | BLTEN);
    
    // Check if Blitter DMA got switched on
    if (!oldBltDma && newBltDma) {

        // Perform pending blit operation (if any)
        if (scheduler.hasEvent<SLOT_BLT>(BLT_STRT1)) {
            agnus.scheduleRel<SLOT_BLT>(DMA_CYCLES(0), BLT_STRT1);
        }
    }
    
    if (BLT_GUARD && running && oldBltDma && !newBltDma) {
        trace(true, "Blitter DMA off while Blitter is running\n");
    }
    if (BLT_GUARD && running && agnus.bltpri(oldValue) != agnus.bltpri(newValue)) {
        trace(true, "BLTPRI changed while Blitter is running\n");
    }
}

template void Blitter::pokeBLTSIZE<ACCESSOR_CPU>(u16 value);
template void Blitter::pokeBLTSIZE<ACCESSOR_AGNUS>(u16 value);
