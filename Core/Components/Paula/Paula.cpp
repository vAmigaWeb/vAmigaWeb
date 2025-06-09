// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Paula.h"
#include "Agnus.h"
#include "CPU.h"
#include "IOUtils.h"

namespace vamiga {

Paula::Paula(Amiga& ref) : SubComponent(ref)
{
    subComponents = std::vector<CoreComponent *> {
        
        &channel0,
        &channel1,
        &channel2,
        &channel3,
        &diskController,
        &uart
    };
}

void
Paula::_dump(Category category, std::ostream &os) const
{
    using namespace util;

    if (category == Category::Registers) {
        
        os << tab("INTENA") << hex(intena) << std::endl;
        os << tab("INTREQ") << hex(intreq) << std::endl;
        os << tab("ADKCON") << hex(adkcon) << std::endl;
        os << tab("POTGO") << hex(potgo) << std::endl;
    }
}

void
Paula::_didReset(bool hard)
{
    for (isize i = 0; i < 16; i++) setIntreq[i] = NEVER;

    // This should not be needed...
    // cpu.setIPL(0);
    assert(cpu.getIPL() == 0);
}

void
Paula::_run()
{

}

void
Paula::_pause()
{

}

void
Paula::_warpOn()
{
    /* Warping has the unavoidable drawback that audio playback gets out of
     * sync. To cope with it, we ramp down the volume when warping is switched
     * on and fade in smoothly when it is switched off.
     */
    // audioPort.rampDown();
}

void
Paula::_warpOff()
{
    // audioPort.rampUp();
    audioPort.clear();
}

void 
Paula::cacheInfo(PaulaInfo &info) const
{
    {   SYNCHRONIZED
        
        info.intreq = intreq;
        info.intena = intena;
        info.adkcon = adkcon;
    }
}

void
Paula::_didLoad()
{
    audioPort.clear();
}

void
Paula::executeUntil(Cycle target)
{
    audioPort.synthesize(audioClock, target);
    audioClock = target;
}

void
Paula::scheduleIrqAbs(IrqSource src, Cycle trigger)
{
    assert_enum(IrqSource, src);
    assert(trigger != 0);
    assert(agnus.id[SLOT_IRQ] == IRQ_CHECK);

    trace(INT_DEBUG, "scheduleIrq(%ld, %lld)\n", long(src), trigger);

    // Record the interrupt request
    if (trigger < setIntreq[isize(src)])
        setIntreq[isize(src)] = trigger;

    // Schedule the interrupt
    if (trigger < agnus.trigger[SLOT_IRQ])
        agnus.scheduleAbs<SLOT_IRQ>(trigger, IRQ_CHECK);
}

void
Paula::scheduleIrqRel(IrqSource src, Cycle trigger)
{
    assert(trigger != 0);
    scheduleIrqAbs(src, agnus.clock + trigger);
}

void
Paula::checkInterrupt()
{
    u8 level = interruptLevel();

    if ((iplPipe & 0xFF) != level) {

        iplPipe = (iplPipe & ~0xFF) | level;
        agnus.scheduleRel<SLOT_IPL>(0, IPL_CHANGE, 5);

        trace(CPU_DEBUG, "iplPipe: %016llx\n", iplPipe);
    }
}

u8
Paula::interruptLevel()
{
    if (intena & 0x4000) {

        u16 mask = intreq & intena;

        if (mask & 0b0110000000000000) return 6;
        if (mask & 0b0001100000000000) return 5;
        if (mask & 0b0000011110000000) return 4;
        if (mask & 0b0000000001110000) return 3;
        if (mask & 0b0000000000001000) return 2;
        if (mask & 0b0000000000000111) return 1;
    }

    return 0;
}

void
Paula::eofHandler()
{
    // Synthesize sound samples
    executeUntil(agnus.clock - 50 * DMA_CYCLES(PAL::HPOS_CNT));
}

}
