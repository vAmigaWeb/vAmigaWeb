// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "StateMachine.h"
#include "Paula.h"
#include "IOUtils.h"
#include "Amiga.h"

namespace vamiga {

template <isize nr>
StateMachine<nr>::StateMachine(Amiga& ref) : SubComponent(ref)
{
    
}

template <isize nr> void
StateMachine<nr>::_dump(Category category, std::ostream &os) const
{
    using namespace util;
    
    if (category == Category::State) {

        os << tab("State machine") << dec(nr) << std::endl;
        os << tab("State") << dec(state) << std::endl;
        os << tab("AUDxIP") << bol(AUDxIP()) << std::endl;
        os << tab("AUDxON") << bol(AUDxON()) << std::endl;
    }
}

template <isize nr> void
StateMachine<nr>::cacheInfo(StateMachineInfo &info) const
{
    {   SYNCHRONIZED
        
        info.state = state;
        info.dma = AUDxON();
        info.audlenLatch = audlenLatch;
        info.audlen = audlen;
        info.audperLatch = audperLatch;
        info.audper = audper;
        info.audvolLatch = audvolLatch;
        info.audvol = audvol;
        info.auddat = auddat;
    }
}

template <isize nr> void
StateMachine<nr>::enableDMA()
{
    trace(AUD_DEBUG, "Enable DMA\n");

    switch (state) {

        case 0b000:

            move_000_001();
            break;
    }
}

template <isize nr> void
StateMachine<nr>::disableDMA()
{
    trace(AUD_DEBUG, "Disable DMA\n");

    switch (state) {

        case 0b001:

            move_001_000();
            break;
            
        case 0b010:
            
            move_010_000();
            break;
            
        case 0b011:
            
            move_011_000();
            break;

        case 0b101:

            move_101_000();
            break;
    }
}

template <isize nr> bool
StateMachine<nr>::AUDxIP() const 
{
    return GET_BIT(paula.intreq, 7 + nr);
}

template <isize nr> void
StateMachine<nr>::AUDxIR() const
{
    if (DISABLE_AUDIRQ) return;
    
    if constexpr (nr == 0) { paula.scheduleIrqRel(IrqSource::AUD0, DMA_CYCLES(1)); }
    if constexpr (nr == 1) { paula.scheduleIrqRel(IrqSource::AUD1, DMA_CYCLES(1)); }
    if constexpr (nr == 2) { paula.scheduleIrqRel(IrqSource::AUD2, DMA_CYCLES(1)); }
    if constexpr (nr == 3) { paula.scheduleIrqRel(IrqSource::AUD3, DMA_CYCLES(1)); }
}

template <isize nr> void
StateMachine<nr>::percntrld()
{
    u64 delay = DMA_CYCLES(audperLatch == 0 ? 0x10000 : audperLatch);

    if constexpr (nr == 0) agnus.scheduleRel <SLOT_CH0> (delay, CHX_PERFIN);
    if constexpr (nr == 1) agnus.scheduleRel <SLOT_CH1> (delay, CHX_PERFIN);
    if constexpr (nr == 2) agnus.scheduleRel <SLOT_CH2> (delay, CHX_PERFIN);
    if constexpr (nr == 3) agnus.scheduleRel <SLOT_CH3> (delay, CHX_PERFIN);
}

template <isize nr> void
StateMachine<nr>::pbufld1()
{
    if (!AUDxAV()) { buffer = auddat; return; }
    
    if constexpr (nr == 0) paula.channel1.pokeAUDxVOL(auddat);
    if constexpr (nr == 1) paula.channel2.pokeAUDxVOL(auddat);
    if constexpr (nr == 2) paula.channel3.pokeAUDxVOL(auddat);
}

template <isize nr> void
StateMachine<nr>::pbufld2()
{
    assert(AUDxAP());
    
    if constexpr (nr == 0) paula.channel1.pokeAUDxPER(auddat);
    if constexpr (nr == 1) paula.channel2.pokeAUDxPER(auddat);
    if constexpr (nr == 2) paula.channel3.pokeAUDxPER(auddat);
}

template <isize nr> bool
StateMachine<nr>::AUDxAV() const
{
    return (paula.adkcon >> nr) & 0x01;
}

template <isize nr> bool
StateMachine<nr>::AUDxAP() const
{
    return (paula.adkcon >> nr) & 0x10;
}

template <isize nr> void
StateMachine<nr>::penhi()
{
    // Only proceed if this is not the run-ahead instance
    if (amiga.objid != 0) return;

    if (!enablePenhi) return;

    Sampler &sampler = audioPort.sampler[nr];

    i8 sample = (i8)HI_BYTE(buffer);
    i16 scaled = (i16)(sample * audvol);
    
    trace(AUD_DEBUG, "penhi: %d %d\n", sample, scaled);

    if (!sampler.isFull()) {
        sampler.append(agnus.clock, scaled);
    } else {
        trace(AUD_DEBUG, "penhi: Sample buffer is full\n");
    }
    
    enablePenhi = false;
}

template <isize nr> void
StateMachine<nr>::penlo()
{
    // Only proceed if this is not the run-ahead instance
    if (amiga.objid != 0) return;

    if (!enablePenlo) return;

    Sampler &sampler = audioPort.sampler[nr];
    
    i8 sample = (i8)LO_BYTE(buffer);
    i16 scaled = (i16)(sample * audvol);

    trace(AUD_DEBUG, "penlo: %d %d\n", sample, scaled);

    if (!sampler.isFull()) {
        sampler.append(agnus.clock, scaled);
    } else {
        trace(AUD_DEBUG, "penlo: Sample buffer is full\n");
    }
    
    enablePenlo = false;
}

template <isize nr> void
StateMachine<nr>::move_000_010() {

    trace(AUD_DEBUG, "move_000_010\n");

    // This transition is taken in IRQ mode only
    assert(!AUDxON());
    assert(!AUDxIP());

    volcntrld();
    percntrld();
    pbufld1();
    AUDxIR();

    state = 0b010;
    penhi();
}

template <isize nr> void
StateMachine<nr>::move_000_001() {

    trace(AUD_DEBUG, "move_000_001\n");

    // This transition is taken in DMA mode only
    assert(AUDxON());

    lencntrld();
    AUDxDR();

    state = 0b001;
}

template <isize nr> void
StateMachine<nr>::move_001_000() {

    trace(AUD_DEBUG, "move_001_000\n");

    // This transition is taken in IRQ mode only
    assert(!AUDxON());

    state = 0b000;
}

template <isize nr> void
StateMachine<nr>::move_001_101() {

    trace(AUD_DEBUG, "move_001_101\n");

    // This transition is taken in DMA mode only
    assert(AUDxON());

    AUDxIR();
    AUDxDR();
    AUDxDSR();
    if (!lenfin()) lencount();

    state = 0b101;
}

template <isize nr> void
StateMachine<nr>::move_101_000() {

    trace(AUD_DEBUG, "move_101_000\n");

    // This transition is taken in IRQ mode only
    assert(!AUDxON());

    state = 0b000;
}

template <isize nr> void
StateMachine<nr>::move_101_010() {

    trace(AUD_DEBUG, "move_101_010\n");

    // This transition is taken in DMA mode only
    assert(AUDxON());

    percntrld();
    volcntrld();
    pbufld1();
    if (napnav()) AUDxDR();

    state = 0b010;
    penhi();
}

template <isize nr> void
StateMachine<nr>::move_010_011() {

    trace(AUD_DEBUG, "move_010_011\n");
    
    percntrld();
    
    // Check for attach period mode
    if (AUDxAP()) {
        
        pbufld2();
        
        if (AUDxON()) {
            
            // Additional DMA mode action
            AUDxDR();
            if (intreq2) { AUDxIR(); intreq2 = false; }
            
        } else {
            
            // Additional IRQ mode action
            AUDxIR();
        }
    }

    state = 0b011;
    penlo();
}

template <isize nr> void
StateMachine<nr>::move_010_000() {

    trace(AUD_DEBUG, "move_010_000\n");

    constexpr EventSlot slot = (EventSlot)(SLOT_CH0 + nr);
    agnus.cancel<slot>();

    intreq2 = false;
    state = 0b000;
}

template <isize nr> void
StateMachine<nr>::move_011_000() {

    trace(AUD_DEBUG, "move_011_000\n");

    constexpr EventSlot slot = (EventSlot)(SLOT_CH0 + nr);
    agnus.cancel<slot>();

    intreq2 = false;
    state = 0b000;
}

template <isize nr> void
StateMachine<nr>::move_011_010()
{
    trace(AUD_DEBUG, "move_011_010\n");

    percntrld();
    pbufld1();
    volcntrld();
    
    if (napnav()) {

        if (AUDxON()) {

            // Additional DMA mode action
            AUDxDR();
            if (intreq2) { AUDxIR(); intreq2 = false; }

        } else {

            // Additional IRQ mode action
            AUDxIR();
        }
        
        // intreq2 = false;
    }

    state = 0b010;
    penhi();
}

template StateMachine<0>::StateMachine(Amiga &ref);
template StateMachine<1>::StateMachine(Amiga &ref);
template StateMachine<2>::StateMachine(Amiga &ref);
template StateMachine<3>::StateMachine(Amiga &ref);

template void StateMachine<0>::cacheInfo(StateMachineInfo &result) const;
template void StateMachine<1>::cacheInfo(StateMachineInfo &result) const;
template void StateMachine<2>::cacheInfo(StateMachineInfo &result) const;
template void StateMachine<3>::cacheInfo(StateMachineInfo &result) const;

template void StateMachine<0>::enableDMA();
template void StateMachine<1>::enableDMA();
template void StateMachine<2>::enableDMA();
template void StateMachine<3>::enableDMA();

template void StateMachine<0>::disableDMA();
template void StateMachine<1>::disableDMA();
template void StateMachine<2>::disableDMA();
template void StateMachine<3>::disableDMA();

template bool StateMachine<0>::AUDxIP() const;
template bool StateMachine<1>::AUDxIP() const;
template bool StateMachine<2>::AUDxIP() const;
template bool StateMachine<3>::AUDxIP() const;

template bool StateMachine<0>::AUDxON() const;
template bool StateMachine<1>::AUDxON() const;
template bool StateMachine<2>::AUDxON() const;
template bool StateMachine<3>::AUDxON() const;

template void StateMachine<0>::move_000_010();
template void StateMachine<1>::move_000_010();
template void StateMachine<2>::move_000_010();
template void StateMachine<3>::move_000_010();

template void StateMachine<0>::move_000_001();
template void StateMachine<1>::move_000_001();
template void StateMachine<2>::move_000_001();
template void StateMachine<3>::move_000_001();

template void StateMachine<0>::move_001_101();
template void StateMachine<1>::move_001_101();
template void StateMachine<2>::move_001_101();
template void StateMachine<3>::move_001_101();

template void StateMachine<0>::move_010_011();
template void StateMachine<1>::move_010_011();
template void StateMachine<2>::move_010_011();
template void StateMachine<3>::move_010_011();

template void StateMachine<0>::move_101_010();
template void StateMachine<1>::move_101_010();
template void StateMachine<2>::move_101_010();
template void StateMachine<3>::move_101_010();

template void StateMachine<0>::move_011_000();
template void StateMachine<1>::move_011_000();
template void StateMachine<2>::move_011_000();
template void StateMachine<3>::move_011_000();

template void StateMachine<0>::move_011_010();
template void StateMachine<1>::move_011_010();
template void StateMachine<2>::move_011_010();
template void StateMachine<3>::move_011_010();

}
