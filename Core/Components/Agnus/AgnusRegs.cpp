// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Agnus.h"
#include "Denise.h"
#include "Paula.h"
#include "Amiga.h"

namespace vamiga {

u16
Agnus::peekDMACONR() const
{
    u16 result = dmacon;
    
    assert((result & ((1 << 14) | (1 << 13))) == 0);
    
    if (blitter.isBusy()) result |= (1 << 14);
    if (blitter.isZero()) result |= (1 << 13);
    
    return result;
}

template <Accessor s> void
Agnus::pokeDMACON(u16 value)
{
    trace(DMA_DEBUG, "pokeDMACON(%04x)\n", value);

    // Schedule the write cycle
    recordRegisterChange(DMA_CYCLES(2), Reg::DMACON, value);
}

void
Agnus::setDMACON(u16 oldValue, u16 value)
{
    trace(DMA_DEBUG, "setDMACON(%x, %x)\n", oldValue, value);
    
    u16 newValue;
    
    if (value & 0x8000) {
        newValue = (dmacon | value) & 0x07FF;
    } else {
        newValue = (dmacon & ~value) & 0x07FF;
    }
    if (oldValue == newValue) {
        
        trace(SEQ_DEBUG, "setDMACON: Skipping (value does not change)\n");
        return;
    }

    dmacon = newValue;
    
    u16 oldDma = (oldValue & DMAEN) ? oldValue : 0;
    u16 newDma = (newValue & DMAEN) ? newValue : 0;
    u16 diff = oldDma ^ newDma;

    // Inform the delegates
    blitter.pokeDMACON(oldValue, newValue);
    
    // Bitplane DMA
    if (diff & BPLEN) setBPLEN(newDma & BPLEN);

    // Disk DMA and sprite DMA
    if (diff & (DSKEN | SPREN)) {

        if (diff & SPREN) setSPREN(newDma & SPREN);
        if (diff & DSKEN) setDSKEN(newDma & DSKEN);
        
        u16 newDAS = (newValue & DMAEN) ? (newValue & 0x3F) : 0;
        
        // Schedule the DAS DMA table to be rebuild
        sequencer.hsyncActions |= UPDATE_DAS_TABLE;
        
        // Make the effect visible in the current rasterline as well
        sequencer.updateDasEvents(newDAS, pos.h + 2);

        // Rectify the currently scheduled DAS event
        scheduleDasEventForCycle(pos.h);
    }
    
    // Copper DMA
    if (diff & COPEN) setCOPEN(newDma & COPEN);
    
    // Blitter DMA
    if (diff & BLTEN) setBLTEN(newDma & BLTEN);
    
    // Audio DMA
    if (diff & (AUD0EN | AUD1EN | AUD2EN | AUD3EN)) {
        
        if (diff & AUD0EN) setAUD0EN(newDma & AUD0EN);
        if (diff & AUD1EN) setAUD1EN(newDma & AUD1EN);
        if (diff & AUD2EN) setAUD2EN(newDma & AUD2EN);
        if (diff & AUD3EN) setAUD3EN(newDma & AUD3EN);
    }
}

void
Agnus::setBPLEN(bool value)
{
    trace(SEQ_DEBUG, "setBPLEN(%d)\n", value);
    
    // Update the bitplane event table
    if (value) {
        sequencer.sigRecorder.insert(pos.h + 3, SIG_BMAPEN_SET);
    } else {
        sequencer.sigRecorder.insert(pos.h + 3, SIG_BMAPEN_CLR);
    }
    sequencer.computeBplEventTable(sequencer.sigRecorder);
}

void
Agnus::setCOPEN(bool value)
{
    trace(DMA_DEBUG, "Copper DMA %s\n", value ? "on" : "off");
    
    if (value) copper.activeInThisFrame = true;
}

void
Agnus::setBLTEN(bool value)
{
    trace(DMA_DEBUG, "Blitter DMA %s\n", value ? "on" : "off");
}

void
Agnus::setSPREN(bool value)
{
    trace(DMA_DEBUG, "Sprite DMA %s\n", value ? "on" : "off");
}

void
Agnus::setDSKEN(bool value)
{
    trace(DMA_DEBUG, "Disk DMA %s\n", value ? "on" : "off");
}

void
Agnus::setAUD0EN(bool value)
{
    value ? paula.channel0.enableDMA() : paula.channel0.disableDMA();
}

void
Agnus::setAUD1EN(bool value)
{
    value ? paula.channel1.enableDMA() : paula.channel1.disableDMA();
}

void
Agnus::setAUD2EN(bool value)
{
    value ? paula.channel2.enableDMA() : paula.channel2.disableDMA();
}

void
Agnus::setAUD3EN(bool value)
{
    value ? paula.channel3.enableDMA() : paula.channel3.disableDMA();
}

u16
Agnus::peekVHPOSR() const
{
    // 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    // V7 V6 V5 V4 V3 V2 V1 V0 H8 H7 H6 H5 H4 H3 H2 H1
    u16 result;

    if (ersy(bplcon0Initial)) {

        // Return the latched position if external synchronization is enabled
        result = HI_LO(latchedPos.v & 0xFF, 0);

    } else {

        // The returned position is five cycles ahead
        auto pos = agnus.pos + 5;

        // Rectify the vertical position if it has wrapped over
        if (pos.v > pos.vMax()) pos.v = 0;
        
        // In cycle 0 and 1, we need to return the old value of posv
        if (pos.h <= 1) {
            result = HI_LO(agnus.pos.v & 0xFF, pos.h);
        } else {
            result = HI_LO(pos.v & 0xFF, pos.h);
        }
    }
    
    trace(POSREG_DEBUG, "peekVHPOSR() = %04x\n", result);
    return result;
}

void
Agnus::pokeVHPOS(u16 value)
{
    trace(POSREG_DEBUG, "pokeVHPOS(%04x)\n", value);
    
    setVHPOS(value);
}

void
Agnus::setVHPOS(u16 value)
{
    [[maybe_unused]] int v7v0 = HI_BYTE(value);
    [[maybe_unused]] int h8h1 = LO_BYTE(value);
    
    xfiles("setVHPOS(%04x) (%d,%d)\n", value, v7v0, h8h1);

    // Don't know what to do here ...
}

u16
Agnus::peekVPOSR() const
{
    // 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    // LF I6 I5 I4 I3 I2 I1 I0 LL -- -- -- -- -- -- V8

    // I5 I4 I3 I2 I1 I0 (Chip Identification)
    u16 result = idBits();

    // LF LL (Long Frame bit, Long Line bit)
    if (pos.lof) result |= 0x8000;
    if (pos.lol) result |= 0x0080;

    if (ersy(bplcon0Initial)) {

        // Return the latched position if external synchronization is enabled
        result |= latchedPos.v >> 8;

    } else {

        // The returned position is five cycles ahead
        auto pos = agnus.pos + 5;

        // Rectify the vertical position if it has wrapped over
        if (pos.v > pos.vMax()) pos.v = 0;
        
        // In cycle 0 and 1, we need to return the old value of posv
        if (pos.h <= 1) {
            result |= agnus.pos.v >> 8;
        } else {
            result |= pos.v >> 8;
        }
    }
    
    trace(POSREG_DEBUG, "peekVPOSR() = %04x\n", result);
    return result;
}

void
Agnus::pokeVPOS(u16 value)
{
    trace(POSREG_DEBUG, "pokeVPOS(%04x)\n", value);
    
    setVPOS(value);
}

void
Agnus::setVPOS(u16 value)
{
    if (!!GET_BIT(value, 0) != !!GET_BIT(pos.v, 8)) {

        xfiles("VPOS: Toggling V8 is not supported\n");
    }

    // Writing to this register clears the LOL bit
    if (pos.lol) {

        trace(NTSC_DEBUG, "Clearing the LOL bit\n");
        pos.lol = false;
        rectifyVBLEvent();
    }

    // Check the LOF bit
    bool newlof = value & 0x8000;
    if (pos.lof != newlof) {
        
        /* If a long frame gets changed to a short frame, we only proceed if
         * Agnus is not in the last rasterline. Otherwise, we would corrupt the
         * emulators internal state (we would be in a line that is unreachable).
         */
        if (!newlof && inLastRasterline()) {

            xfiles("VPOS: LOF bit changed in last scanline\n");
            return;
        }

        xfiles("VPOS: Making a %s frame\n", newlof ? "long" : "short");
        pos.lof = newlof;
        
        /* Reschedule a pending VBL event with a trigger cycle that is consistent
         * with the new value of the LOF bit.
         */
        rectifyVBLEvent();
    }
}

template <Accessor s> void
Agnus::pokeBPLCON0(u16 value)
{
    trace(DMA_DEBUG, "pokeBPLCON0(%04x)\n", value);

    if (bplcon0 != value) {
        recordRegisterChange(DMA_CYCLES(4), Reg::BPLCON0, value, Accessor::AGNUS);
    }
}

void
Agnus::setBPLCON0(u16 oldValue, u16 newValue)
{
    trace(DMA_DEBUG | SEQ_DEBUG, "setBPLCON0(%04x,%04x)\n", oldValue, newValue);

    // Determine the new bitmap resolution
    res = resolution(newValue);

    // Check if one of the resolution bits or the BPU bits have been modified
    if ((oldValue ^ newValue) & 0xF040) {

        // Record the change
        sequencer.sigRecorder.insert(pos.h, HI_W_LO_W(newValue, SIG_CON));
        
        if (bpldma()) {

            trace(SEQ_DEBUG, "setBPLCON0: Recomputing BPL event table\n");

            // Recompute the bitplane event table
            sequencer.computeBplEventTable(sequencer.sigRecorder);

            // Since the table has changed, we need to update the event slot
            scheduleBplEventForCycle(pos.h);

        } else {

            // Speed optimization: Recomputation will happen in the next line
            trace(SEQ_DEBUG, "setBPLCON0: Postponing recomputation\n");
        }
    }
    
    // Latch the position counters if the ERSY bit is set
    if ((newValue & 0b10) && !(oldValue & 0b10)) latchedPos = pos;

    // Check the LACE bit
    pos.lofToggle = newValue & 0b100;

    bplcon0 = newValue;
}

void
Agnus::pokeBPLCON1(u16 value)
{
    trace(DMA_DEBUG, "pokeBPLCON1(%04x)\n", value);
    
    if (bplcon1 != value) {
        recordRegisterChange(DMA_CYCLES(1), Reg::BPLCON1, value, Accessor::AGNUS);
    }
}

void
Agnus::setBPLCON1(u16 oldValue, u16 newValue)
{
    assert(oldValue != newValue);
    trace(DMA_DEBUG | SEQ_DEBUG, "setBPLCON1(%04x,%04x)\n", oldValue, newValue);

    bplcon1 = newValue & 0xFF;
    
    // Compute comparision values for the hpos counter
    scrollOdd  = (bplcon1 & 0b00001110) >> 1;
    scrollEven = (bplcon1 & 0b11100000) >> 5;
    
    // Update the bitplane event table
    sequencer.computeBplEventTable(sequencer.sigRecorder);
    
    // Update the scheduled bitplane event according to the new table
    scheduleBplEventForCycle(pos.h);
}

template <Accessor s> void
Agnus::pokeDIWSTRT(u16 value)
{
    trace(DIW_DEBUG, "pokeDIWSTRT<%s>(%04x)\n", AccessorEnum::key(s), value);

    recordRegisterChange(DMA_CYCLES(4), Reg::DIWSTRT, value, Accessor::AGNUS);
    recordRegisterChange(DMA_CYCLES(1), Reg::DIWSTRT, value, Accessor::DENISE);
}

template <Accessor s> void
Agnus::pokeDIWSTOP(u16 value)
{
    trace(DIW_DEBUG, "pokeDIWSTOP<%s>(%04x)\n", AccessorEnum::key(s), value);

    recordRegisterChange(DMA_CYCLES(4), Reg::DIWSTOP, value, Accessor::AGNUS);
    recordRegisterChange(DMA_CYCLES(1), Reg::DIWSTOP, value, Accessor::DENISE);
}

template <Accessor s> void
Agnus::pokeDIWHIGH(u16 value)
{
    trace(DIW_DEBUG, "pokeDIWHIGH<%s>(%04x)\n", AccessorEnum::key(s), value);

    value &= 0x2727;

    recordRegisterChange(DMA_CYCLES(4), Reg::DIWHIGH, value, Accessor::AGNUS);
    recordRegisterChange(DMA_CYCLES(1), Reg::DIWHIGH, value, Accessor::DENISE);
}

void
Agnus::pokeBPL1MOD(u16 value)
{
    trace(BPLMOD_DEBUG, "pokeBPL1MOD(%04x)\n", value);
    recordRegisterChange(DMA_CYCLES(2), Reg::BPL1MOD, value);
}

void
Agnus::setBPL1MOD(u16 value)
{
    trace(BPLMOD_DEBUG, "setBPL1MOD(%04x)\n", value);
    bpl1mod = (i16)(value & 0xFFFE);
}

void
Agnus::pokeBPL2MOD(u16 value)
{
    trace(BPLMOD_DEBUG, "pokeBPL2MOD(%04x)\n", value);
    recordRegisterChange(DMA_CYCLES(2), Reg::BPL2MOD, value);
}

void
Agnus::setBPL2MOD(u16 value)
{
    trace(BPLMOD_DEBUG, "setBPL2MOD(%04x)\n", value);
    bpl2mod = (i16)(value & 0xFFFE);
}

template <int x, Accessor s> void
Agnus::pokeSPRxPOS(u16 value)
{
    trace(SPRREG_DEBUG, "pokeSPR%dPOS<%s>(%04x)\n", x, AccessorEnum::key(s), value);

    // setSPRxPOS<x>(value);
    // return;

    // Hypothesis (test cases Sprites/sprdma/intefere):
    // DMA cycle is dropped when the register was written one cycle earlier.
    if (lastCtlWrite[x] + 1 == pos.h && (pos.h % 2) == 1) {

        xfiles("pokeSPR%dPOS(%04x) dropped\n", x, value);
        return;
    }

    auto reg = Reg(int(Reg::SPR0POS) + 4 * x);
    recordRegisterChange(DMA_CYCLES(2), reg, value);
}

template <int x> void
Agnus::setSPRxPOS(u16 value)
{
    trace(SPRREG_DEBUG, "setSPR%dPOS(%04x)\n", x, value);

    // Compute the value of the vertical counter that is seen here
    // i16 v = (i16)(pos.h < 0xDF ? pos.v : (pos.v + 1));
    i16 v = (i16)(pos.h < 0xE1 ? pos.v : (pos.v + 1));

    // Compute the new vertical start position
    sprVStrt[x] = ((value & 0xFF00) >> 8) | (sprVStrt[x] & 0x0100);

    // Update sprite DMA status
    if (sprVStrt[x] == v) sprDmaEnabled[x] = true;
    if (sprVStop[x] == v) sprDmaEnabled[x] = false;
}

template <int x, Accessor s> void
Agnus::pokeSPRxCTL(u16 value)
{
    trace(SPRREG_DEBUG, "pokeSPR%dCTL(%04x)\n", x, value);

    // setSPRxCTL<x>(value);
    // return;

    // Hypothesis (test cases Sprites/sprdma/intefere):
    // DMA cycle is dropped when the register was written one cycle earlier.
    if (lastCtlWrite[x] + 1 == pos.h && (pos.h % 2) == 1) {

        xfiles("pokeSPR%dCTL(%04x) dropped\n", x, value);
        return;
    }

    auto reg = Reg(int(Reg::SPR0CTL) + 4 * x);
    recordRegisterChange(DMA_CYCLES(2), reg, value);
}

template <int x> void
Agnus::setSPRxCTL(u16 value)
{
    trace(SPRREG_DEBUG, "setSPR%dCTL(%04x)\n", x, value);

    // Remember the write cycle (checked in pokeSPRxCTL)
    lastCtlWrite[x] = u8(pos.h);

    // Compute the value of the vertical counter that is seen here
    i16 v = (i16)(pos.h < 0xE1 ? pos.v : (pos.v + 1));

    // Compute the new vertical start and stop position
    sprVStrt[x] = (i16)((value & 0b100) << 6 | (sprVStrt[x] & 0x00FF));
    sprVStop[x] = (i16)((value & 0b010) << 7 | (value >> 8));

    // ECS Agnus supports additional position bits (encoded in 'unused' area)
    if (GET_BIT(value, 6)) {

        xfiles("setSPR%dCTL: Extended VSTRT bit set\n", x);
        if (isECS()) sprVStrt[x] |= 0x0200;
    }
    if (GET_BIT(value, 5)) {

        xfiles("setSPR%dCTL: Extended VSTOP bit set\n", x);
        if (isECS()) sprVStop[x] |= 0x0200;
    }

    // Update sprite DMA status
    if (sprVStrt[x] == v) sprDmaEnabled[x] = true;
    if (sprVStop[x] == v) sprDmaEnabled[x] = false;
}

void
Agnus::pokeBEAMCON0(u16 value)
{
    xfiles("pokeBEAMCON0(%04x)\n", value);

    // ECS only register
    if (agnus.isOCS()) return;

    // 15: unused       11: LOLDIS      7: VARBEAMEN    3: unused
    // 14: HARDDIS      10: CSCBEN      6: DUAL         2: CSYTRUE
    // 13: LPENDIS       9: VARVSYEN    5: PAL          1: VSYTRUE
    // 12: VARVBEN       8: VARHSYEN    4: VARCSYEN     0: HSYTRUE

    // PAL
    auto format = GET_BIT(value, 5) ? TV::PAL : TV::NTSC;
    amiga.setOption(Opt::AMIGA_VIDEO_FORMAT, (i64)format);

    // LOLDIS
    bool loldis = GET_BIT(value, 11);
    if (pos.type == TV::NTSC) pos.lolToggle = !loldis;
}

template <Accessor s> void
Agnus::pokeDSKPTH(u16 value)
{
    trace(DSKREG_DEBUG, "pokeDSKPTH(%04x) [%s]\n", value, AccessorEnum::key(s));

    // Schedule the write cycle
    recordRegisterChange(DMA_CYCLES(2), Reg::DSKPTH, value, s);
}

void
Agnus::setDSKPTH(u16 value)
{
    trace(DSKREG_DEBUG, "setDSKPTH(%04x)\n", value);

    // Check if the register is blocked due to ongoing DMA
    if (dropWrite(BusOwner::DISK)) return;
    
    // Perform the write
    dskpt = REPLACE_HI_WORD(dskpt, value);
    
    if (dskpt & ~agnus.ptrMask) {
        xfiles("DSKPT %08x out of range\n", dskpt);
    }
}

template <Accessor s>
void Agnus::pokeDSKPTL(u16 value)
{
    trace(DSKREG_DEBUG, "pokeDSKPTL(%04x) [%s]\n", value, AccessorEnum::key(s));

    // Schedule the write cycle
    recordRegisterChange(DMA_CYCLES(2), Reg::DSKPTL, value, s);
}

void
Agnus::setDSKPTL(u16 value)
{
    trace(DSKREG_DEBUG, "setDSKPTL(%04x)\n", value);

    // Check if the register is blocked due to ongoing DMA
    if (dropWrite(BusOwner::DISK)) return;
    
    // Perform the write
    dskpt = REPLACE_LO_WORD(dskpt, value & 0xFFFE);
}

template <int x, Accessor s> void
Agnus::pokeAUDxLCH(u16 value)
{
    debug(AUDREG_DEBUG, "pokeAUD%dLCH(%X)\n", x, value);

    audlc[x] = REPLACE_HI_WORD(audlc[x], value);
}

template <int x, Accessor s> void
Agnus::pokeAUDxLCL(u16 value)
{
    trace(AUDREG_DEBUG, "pokeAUD%dLCL(%X)\n", x, value);

    audlc[x] = REPLACE_LO_WORD(audlc[x], value & 0xFFFE);
}

template <int x, Accessor s> void
Agnus::pokeBPLxPTH(u16 value)
{
    trace(BPLREG_DEBUG, "pokeBPL%dPTH(%04x) [%s]\n", x, value, AccessorEnum::key(s));
    
    // Schedule the write cycle
    auto reg = Reg(int(Reg::BPL1PTH) + 2 * (x - 1));
    recordRegisterChange(DMA_CYCLES(2), reg, value, s);
}

template <int x> void
Agnus::setBPLxPTH(u16 value)
{
    trace(BPLREG_DEBUG, "setBPL%dPTH(%X)\n", x, value);

    // Check if the register is blocked due to ongoing DMA
    if (dropWrite(BusOwner(BUS_BPL1 + x - 1))) return;
    
    // Perform the write
    bplpt[x - 1] = REPLACE_HI_WORD(bplpt[x - 1], value);
    
    if (bplpt[x - 1] & ~agnus.ptrMask) {
        xfiles("BPL%dPT %08x out of range\n", x, bplpt[x - 1]);
    }
}

template <int x, Accessor s> void
Agnus::pokeBPLxPTL(u16 value)
{
    trace(BPLREG_DEBUG, "pokeBPL%dPTL(%04x) [%s]\n", x, value, AccessorEnum::key(s));

    // Schedule the write cycle
    auto reg = Reg(int(Reg::BPL1PTL) + 2 * (x - 1));
    recordRegisterChange(DMA_CYCLES(2), reg, value, s);
}

template <int x> void
Agnus::setBPLxPTL(u16 value)
{
    trace(BPLREG_DEBUG, "setBPL%dPTL(%X)\n", x, value);

    // Check if the register is blocked due to ongoing DMA
    if (dropWrite(BusOwner(BUS_BPL1 + x - 1))) return;
    
    // Perform the write
    bplpt[x - 1] = REPLACE_LO_WORD(bplpt[x - 1], value & 0xFFFE);
}

template <int x, Accessor s> void
Agnus::pokeSPRxPTH(u16 value)
{
    trace(SPRREG_DEBUG, "pokeSPR%dPTH(%04x) [%s]\n", x, value, AccessorEnum::key(s));

    // Schedule the write cycle
    auto reg = Reg(int(Reg::SPR0PTH) + 2 * x);
    recordRegisterChange(DMA_CYCLES(2), reg, value, s);
}

template <int x> void
Agnus::setSPRxPTH(u16 value)
{
    trace(SPRREG_DEBUG, "setSPR%dPTH(%04x)\n", x, value);
    
    // Check if the register is blocked due to ongoing DMA
    if (dropWrite(BusOwner(BUS_SPRITE0 + x))) return;
    
    // Perform the write
    sprpt[x] = REPLACE_HI_WORD(sprpt[x], value);
    
    if (sprpt[x] & ~agnus.ptrMask) {
        xfiles("SPR%dPT %08x out of range\n", x, sprpt[x]);
    }
}

template <int x, Accessor s> void
Agnus::pokeSPRxPTL(u16 value)
{
    trace(SPRREG_DEBUG, "pokeSPR%dPTL(%04x) [%s]\n", x, value, AccessorEnum::key(s));

    // Schedule the write cycle
    auto reg = Reg(int(Reg::SPR0PTL) + 2 * x);
    recordRegisterChange(DMA_CYCLES(2), reg, value, s);
}

template <int x> void
Agnus::setSPRxPTL(u16 value)
{
    trace(SPRREG_DEBUG, "setSPR%dPTH(%04x)\n", x, value);
    
    // Check if the register is blocked due to ongoing DMA
    if (dropWrite(BusOwner(BUS_SPRITE0 + x))) return;
    
    // Perform the write
    sprpt[x] = REPLACE_LO_WORD(sprpt[x], value & 0xFFFE);
}

bool
Agnus::dropWrite(BusOwner owner)
{
    /* A write to a pointer register is dropped if the pointer was used one
     * cycle before the update would happen.
     */
    if (config.ptrDrops && pos.h >= 1 && busOwner[pos.h - 1] == owner) {
        
        xfiles("Dropping pointer register write (%s)\n", BusOwnerEnum::key(owner));
        return true;
    }
    
    return false;
}

//
// Instantiate template functions
//

#define DECLARE(x) \
DECLAREA(x,Accessor::CPU) \
DECLAREA(x,Accessor::AGNUS)

#define DECLAREA(x,y) \
template void Agnus::x<y>(u16 value);

DECLARE(pokeDSKPTH)
DECLARE(pokeDSKPTL)
DECLARE(pokeBPLCON0)
DECLARE(pokeDMACON)
DECLARE(pokeDIWSTRT)
DECLARE(pokeDIWSTOP)
DECLARE(pokeDIWHIGH)

#undef DECLAREA

#define DECLAREA(x,y) \
template void Agnus::x<0,y>(u16 value); \
template void Agnus::x<1,y>(u16 value); \
template void Agnus::x<2,y>(u16 value); \
template void Agnus::x<3,y>(u16 value);

DECLARE(pokeAUDxLCH);
DECLARE(pokeAUDxLCL);

#undef DECLAREA

#define DECLAREA(x,y) \
template void Agnus::x<1,y>(u16 value); \
template void Agnus::x<2,y>(u16 value); \
template void Agnus::x<3,y>(u16 value); \
template void Agnus::x<4,y>(u16 value); \
template void Agnus::x<5,y>(u16 value); \
template void Agnus::x<6,y>(u16 value);

DECLARE(pokeBPLxPTH)
DECLARE(pokeBPLxPTL)

#undef DECLAREA

#define DECLAREA(x,y) \
template void Agnus::x<0,y>(u16 value); \
template void Agnus::x<1,y>(u16 value); \
template void Agnus::x<2,y>(u16 value); \
template void Agnus::x<3,y>(u16 value); \
template void Agnus::x<4,y>(u16 value); \
template void Agnus::x<5,y>(u16 value); \
template void Agnus::x<6,y>(u16 value); \
template void Agnus::x<7,y>(u16 value);

DECLARE(pokeSPRxPOS)
DECLARE(pokeSPRxCTL)
DECLARE(pokeSPRxPTH)
DECLARE(pokeSPRxPTL)

#undef DECLAREA
#undef DECLARE

#define DECLARE(x) \
template void Agnus::x<1>(u16 value); \
template void Agnus::x<2>(u16 value); \
template void Agnus::x<3>(u16 value); \
template void Agnus::x<4>(u16 value); \
template void Agnus::x<5>(u16 value); \
template void Agnus::x<6>(u16 value);

DECLARE(setBPLxPTH)
DECLARE(setBPLxPTL)

#undef DECLARE

#define DECLARE(x) \
template void Agnus::x<0>(u16 value); \
template void Agnus::x<1>(u16 value); \
template void Agnus::x<2>(u16 value); \
template void Agnus::x<3>(u16 value); \
template void Agnus::x<4>(u16 value); \
template void Agnus::x<5>(u16 value); \
template void Agnus::x<6>(u16 value); \
template void Agnus::x<7>(u16 value);

DECLARE(setSPRxPTH)
DECLARE(setSPRxPTL)
DECLARE(setSPRxPOS)
DECLARE(setSPRxCTL)

#undef DECLARE
}
