// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Sequencer.h"
#include "Agnus.h"

namespace vamiga {

void
Sequencer::initBplEvents()
{
    for (isize i = 0; i < HPOS_CNT; i++) bplEvent[i] = EVENT_NONE;
    for (isize i = 0; i < HPOS_CNT; i++) nextBplEvent[i] = HPOS_MAX;
}

void
Sequencer::initSigRecorder()
{
    sigRecorder.clear();

    sigRecorder.insert(0x18, SIG_SHW);
    sigRecorder.insert(ddfstrt, SIG_BPHSTART);
    sigRecorder.insert(ddfstop, SIG_BPHSTOP);
    sigRecorder.insert(0xD8, SIG_RHW);
    sigRecorder.insert(NTSC::HPOS_CNT, SIG_DONE);

    sigRecorder.modified = false;
}

void
Sequencer::computeBplEventTable(const SigRecorder &sr)
{
    /* AGA has to take the ECS path, because AGA is a superset of ECS. */
    agnus.isECSorLater() ?
    computeBplEventTable <true> (sr) : computeBplEventTable <false> (sr);
}

template <bool ecs> void
Sequencer::computeBplEventTable(const SigRecorder &sr)
{
    trace(SEQ_DEBUG, "computeBplEvents\n");

    auto state = ddfInitial;

    // Update the DMA and BMCTL bits
    state.bmapen = agnus.bpldma(agnus.dmaconInitial);
    state.bplcon0 = agnus.bplcon0Initial;
    computeFetchUnit(state.bplcon0);
    
    // Evaluate the current state of the vertical DIW flipflop
    if (!state.bpv) { state.bprun = false; state.cnt = 0; }
    
    // Fill the event table
    if (sr.modified || (state.bpv && state.bmapen) || SEQ_ON_STEROIDS) {
        computeBplEventsSlow <ecs> (sr, state);
    } else {
        computeBplEventsFast <ecs> (sr, state);
    }
    
    // Update the jump table
    updateBplJumpTable();

    // Rectify the scheduled event
    agnus.scheduleBplEventForCycle(agnus.pos.h);
    
    // Write back the new ddf state
    ddf = state;

    // Check if we need to recompute all events in the next scanline
    if (state != ddfInitial) {

        trace(SEQ_DEBUG, "Recompute table in next line\n");
        hsyncActions |= UPDATE_BPL_TABLE;
    }
}

template <bool ecs> void
Sequencer::computeBplEventsFast(const SigRecorder &sr, DDFState &state)
{
    // This path can be taken only if bitplane DMA if off in the entire line
    assert(!sr.modified);
    assert(!state.bpv || !state.bmapen);

    trace(SEQ_DEBUG, "Fast path (no bitplane DMA in this line)\n");

    // Erase all events
    for (isize i = 0; i < HPOS_CNT; i++) bplEvent[i] = EVENT_NONE;
    bprunUp = LONG_MAX;

    // Add drawing flags
    auto odd = agnus.scrollOdd;
    auto even = agnus.scrollEven;
    switch (agnus.resolution(state.bplcon0)) {

        case Resolution::LORES:

            odd &= 0b111;
            even &= 0b111;

            if (odd == even) {
                for (isize i = odd; i < HPOS_CNT; i += 8) bplEvent[i] = (EventID)3;
            } else {
                for (isize i = odd; i < HPOS_CNT; i += 8) bplEvent[i] = (EventID)1;
                for (isize i = even; i < HPOS_CNT; i += 8) bplEvent[i] = (EventID)2;
            }
            break;

        case Resolution::HIRES:

            odd &= 0b11;
            even &= 0b11;

            if (odd == even) {
                for (isize i = odd; i < HPOS_CNT; i += 4) bplEvent[i] = (EventID)3;
            } else {
                for (isize i = odd; i < HPOS_CNT; i += 4) bplEvent[i] = (EventID)1;
                for (isize i = even; i < HPOS_CNT; i += 4) bplEvent[i] = (EventID)2;
            }
            break;

        case Resolution::SHRES:

            odd &= 0b11;
            even &= 0b11;

            if (odd == even) {
                for (isize i = odd; i < HPOS_CNT; i += 2) bplEvent[i] = (EventID)3;
            } else {
                for (isize i = odd; i < HPOS_CNT; i += 2) bplEvent[i] = (EventID)1;
                for (isize i = even; i < HPOS_CNT; i += 2) bplEvent[i] = (EventID)2;
            }
            break;

        default:
            fatalError;
    }

    // Emulate all signal events
    for (isize i = 0;; i++) {
        
        auto signal = sigRecorder.elements[i];
        processSignal <ecs> (signal, state);
        
        if (signal & SIG_DONE) break;
    }
}

template <bool ecs> void
Sequencer::computeBplEventsSlow(const SigRecorder &sr, DDFState &state)
{
    trace(SEQ_DEBUG, "Slow path\n");

    bprunUp = LONG_MAX;

    // Iterate over all recorder signals
    for (isize i = 0, cycle = 0;; i++) {
        
        assert(i < sigRecorder.count());

        auto signal = sigRecorder.elements[i];
        isize trigger = (isize)sigRecorder.keys[i];
        
        assert(trigger <= HPOS_CNT);
        
        // Emulate the display logic up to the next signal change
        computeBplEvents <ecs> (cycle, trigger, state);
        
        // Emulate the signal change
        processSignal <ecs> (signal, state);

        // Exit if the DONE signal has been processed
        if (signal & SIG_DONE) break;

        cycle = trigger;
    }
}

template <bool ecs> void
Sequencer::computeBplEvents(isize strt, isize stop, DDFState &state)
{
    /* The drawing flags repeat every 16 lores pixels, because Denise shifts
     * out 16 pixels per drawing cycle. This holds in AGA as well: the wider
     * fetch modes do not stretch the period, they queue up additional words
     * which are fed into the pipeline one by one (see Denise::feedPipeOdd).
     */
    isize mask;

    switch (agnus.resolution(state.bplcon0)) {

        case Resolution::LORES: mask = 0b111; break;
        case Resolution::HIRES: mask = 0b011; break;
        case Resolution::SHRES: mask = 0b001; break;

        default:
            fatalError;
    }

    for (isize j = strt; j < stop; j++) {

        assert(j >= 0 && j <= HPOS_MAX);

        EventID id;

        auto counter = state.cnt << 1 | (j & 1);

        /*
         if (agnus.pos.v == 115) trace(true, "%d: %d %d %d %d %d %d %d %d %d %d\n", j, state.bpv, state.bmapen, state.shw, state.rhw, state.bphstart, state.bphstop, state.bprun, state.lastFu, state.bmctl, counter);
         */

        if (counter == 0) {

            if (state.lastFu) {

                // trace(1, "%d: STOP\n", j);
                state.bprun = false;
                state.lastFu = false;
                state.bphstop = false;
                if (!ecs) state.shw = false;
            }

            if (state.stopreq) {

                // trace(1, "%d: LASTFU\n", j);
                state.stopreq = false;
                state.lastFu = true;
            }
        }

        if (state.bprun) {

            id = fetch[state.lastFu ? 1 : 0][counter];
            if (IS_ODD(j)) state.cnt = (state.cnt + 1) & cntMask;

        } else {

            id = EVENT_NONE;
            state.cnt = 0;
        }

        // Superimpose drawing flags
        isize jj = j >= 1 ? j : PAL::HPOS_CNT + j;

        if ((jj & mask) == (agnus.scrollOdd & mask))  id = (EventID)(id | 1);
        if ((jj & mask) == (agnus.scrollEven & mask)) id = (EventID)(id | 2);

        bplEvent[j] = id;

        // Remember the first cycle where BPRUN went up
        if (state.bprun) bprunUp = std::min(bprunUp, j);
    }
}

template <> void
Sequencer::processSignal <false> (u32 signal, DDFState &state)
{
    //
    // OCS logic
    //

    if (signal & SIG_CON) {
        
        state.bplcon0 = HI_WORD(signal);
        computeFetchUnit(state.bplcon0);
    }
    switch (signal & (SIG_BMAPEN_CLR | SIG_BMAPEN_SET)) {
            
        case SIG_BMAPEN_CLR:

            state.bmapen = false;
            state.bprun = false;
            state.cnt = 0;
            break;
            
        case SIG_BMAPEN_SET:

            state.bmapen = true;
            break;
    }
    switch (signal & (SIG_VFLOP_SET | SIG_VFLOP_CLR)) {
            
        case SIG_VFLOP_SET:

            state.bpv = true;
            break;
            
        case SIG_VFLOP_CLR:

            state.bpv = false;
            state.bprun = false;
            state.cnt = 0;
            break;
    }
    switch (signal & (SIG_SHW | SIG_RHW)) {
            
        case SIG_SHW:
            
            state.shw = true;
            break;
            
        case SIG_RHW:

            state.rhw |= state.bprun;
            state.stopreq |= state.bprun;
            break;
    }
    switch (signal & (SIG_BPHSTART | SIG_BPHSTOP)) {
            
        case SIG_BPHSTART | SIG_BPHSTOP:
            
            if (state.bprun) {
                
                state.bphstart &= !state.bprun;
                state.bphstop |= state.bprun;
                state.stopreq |= state.bprun;

            } else {
                
                state.bphstart = state.bphstart || state.shw;
                state.bprun = (state.bprun || state.shw) && state.bpv && state.bmapen;
            }
            break;

        case SIG_BPHSTART:

            state.bphstart |= state.shw && state.bmapen;
            state.bprun = (state.bprun || state.shw) && state.bpv && state.bmapen;
            break;
            
        case SIG_BPHSTOP:

            state.bphstart &= !state.bprun;
            state.bphstop |= state.bprun;
            state.stopreq |= state.bprun;
            break;
    }
    if (signal & SIG_DONE) {
        
        state.rhw = false;
        state.stopreq = false;
    }
}

template <> void
Sequencer::processSignal <true> (u32 signal, DDFState &state)
{
    //
    // ECS logic
    //
    
    if (signal & SIG_CON) {
        
        state.bplcon0 = HI_WORD(signal);
        computeFetchUnit(state.bplcon0);
    }
    switch (signal & (SIG_VFLOP_SET | SIG_VFLOP_CLR)) {
            
        case SIG_VFLOP_SET:

            state.bpv = true;
            break;
            
        case SIG_VFLOP_CLR:

            state.bpv = false;
            state.bprun = false;
            state.cnt = 0;
            break;
    }
    switch (signal & (SIG_SHW | SIG_RHW)) {
            
        case SIG_SHW:
            
            state.shw = true;
            state.bprun |= state.bphstart && !(signal & SIG_BPHSTOP);
            break;
            
        case SIG_RHW:

            state.rhw = true;
            state.stopreq |= state.bprun;
            break;
    }
    switch (signal & (SIG_BPHSTART | SIG_BPHSTOP | SIG_SHW | SIG_RHW)) {
            
        case SIG_BPHSTART | SIG_BPHSTOP | SIG_SHW:
            
            state.bphstart = true;
            state.bprun = (state.bprun || state.shw) && state.bpv && state.bmapen;
            break;
            
        case SIG_BPHSTART | SIG_BPHSTOP | SIG_RHW:
            
            state.bphstop |= state.bprun;
            state.stopreq |= state.bprun;
            state.bphstart = true;
            break;
            
        case SIG_BPHSTART | SIG_BPHSTOP:
            
            state.bphstop |= state.bprun;
            state.stopreq |= state.bprun;
            // state.bphstart = true;
            state.bphstart = state.bpv; // Likely fix for test case arosddf2 and arosddf4
            state.bprun = (state.bprun || state.shw) && state.bpv && state.bmapen;
            break;

        case SIG_BPHSTART:
        case SIG_BPHSTART | SIG_SHW:
        case SIG_BPHSTART | SIG_RHW:

            state.bphstart = true;
            state.bprun = (state.bprun || state.shw) && state.bpv && state.bmapen;
            break;
            
        case SIG_BPHSTOP:
        case SIG_BPHSTOP | SIG_SHW:
        case SIG_BPHSTOP | SIG_RHW:

            state.bphstart = false;
            state.bphstop |= state.bprun;
            state.stopreq |= state.bprun;
            break;
    }
    switch (signal & (SIG_BMAPEN_CLR | SIG_BMAPEN_SET)) {

        case SIG_BMAPEN_CLR:

            state.bmapen = false;
            state.bprun = false;
            state.cnt = 0;
            break;

        case SIG_BMAPEN_SET:

            state.bmapen = true;
            state.bprun = (state.bprun || state.shw) && state.bpv && state.bphstart;
            break;
    }
    if (signal & SIG_DONE) {
        
        state.rhw = false;
        state.shw = false;
        state.bphstop = false;

        /* Emulate the hard stop at the end of a rasterline.
         *
         * BPRUN is normally pulled down by the shutdown sequence in
         * computeBplEvents(): The stop request (raised at DDFSTOP or at the
         * hard stop position 0xD8 at the latest) turns into LASTFU at the next
         * fetch unit boundary, and BPRUN goes down at the boundary after that.
         * For a classic 8 cycle fetch unit the remaining cycles of the line
         * always suffice for both steps. AGA fetch units, however, are 16 or 32
         * cycles long when FMODE selects a wider bitplane bus. In that case the
         * second boundary can fall behind the end of the line, leaving BPRUN up
         * when the state is carried over into the next line. BPRUN would then
         * be up from cycle 0 on, which blocks every sprite DMA slot (see
         * Agnus::spriteCycleIsBlocked) and makes all sprites disappear.
         *
         * Real hardware terminates the fetch unconditionally at the end of a
         * line, so the fetch unit state is reset here. The condition keeps OCS
         * and ECS timing bit-exact, as their fetch unit is always 8 cycles.
         */
        if (fetchUnit > 8) {

            state.bprun = false;
            state.lastFu = false;
            state.stopreq = false;
            state.cnt = 0;
        }
    }
}

void
Sequencer::updateBplJumpTable(i16 end)
{
    assert(end <= HPOS_MAX);
    assert(nextBplEvent[HPOS_MAX] == HPOS_MAX);

    u8 next = nextBplEvent[end];
    
    for (isize i = end; i >= 0; i--) {
        
        nextBplEvent[i] = next;
        if (bplEvent[i]) next = (i8)i;
    }
}

void
Sequencer::computeFetchUnit(u16 bplcon0)
{
    auto bpu = bplcon0 >> 12 & 0b111;

    /* In AGA, BPLCON0 bit 4 acts as the fourth BPU bit (BPU3). Setting it in
     * combination with any of the traditional BPU bits would select more than
     * eight bitplanes which disables the bitplanes (same logic as GET_PLANES
     * in WinUAE / Amiberry).
     */
    if (agnus.isAGA() && (bplcon0 & 0x0010)) {
        bpu = (bplcon0 & 0x7000) ? 0 : 8;
    }

    auto resol = agnus.resolution(bplcon0);
    isize res = resol == Resolution::LORES ? 0 : resol == Resolution::HIRES ? 1 : 2;

    /* Determine the width of the bitplane bus. In AGA, the FMODE register
     * selects a 16, 32, or 64 bit wide bus (fetchmode 0, 1, and 2).
     */
    isize fm = 0;
    if (agnus.isAGA()) {
        switch (agnus.fmode & 3) {
            case 1: case 2: fm = 1; break;
            case 3:         fm = 2; break;
        }
    }

    /* Fetch unit length, repetition period, and maximum number of bitplanes,
     * indexed by fetchmode and resolution. These tables correspond to
     * fetchunits[], fetchstarts[], and fm_maxplanes[] in WinUAE / Amiberry.
     *
     * A wider bus stretches the fetch unit, because a single fetch provides
     * data for more pixels. At the same time, fewer fetches per plane are
     * needed, which is why more bitplanes become possible.
     */
    static constexpr u8 fetchUnits[3][3] = { { 8,8,8 }, { 16,8,8 }, { 32,16,8 } };
    static constexpr u8 fetchStarts[3][3] = { { 8,4,2 }, { 16,8,4 }, { 32,16,8 } };
    static constexpr u8 maxPlanes[3][3] = { { 8,4,2 }, { 8,8,4 }, { 8,8,8 } };

    // Cycle sequences for 2, 4, and 8 bitplanes (cycle_sequences[] in Amiberry)
    static constexpr u8 sequences[3][8] = {

        { 2,1,2,1,2,1,2,1 },
        { 4,2,3,1,4,2,3,1 },
        { 8,4,6,2,7,3,5,1 }
    };

    // Bitplane DMA events, indexed by resolution and bitplane number
    static constexpr EventID ids[3][8] = {

        { BPL_L1, BPL_L2, BPL_L3, BPL_L4, BPL_L5, BPL_L6, BPL_L7, BPL_L8 },
        { BPL_H1, BPL_H2, BPL_H3, BPL_H4, BPL_H5, BPL_H6, BPL_H7, BPL_H8 },
        { BPL_S1, BPL_S2, EVENT_NONE, EVENT_NONE,
          EVENT_NONE, EVENT_NONE, EVENT_NONE, EVENT_NONE }
    };
    static constexpr EventID modIds[3][8] = {

        { BPL_L1_MOD, BPL_L2_MOD, BPL_L3_MOD, BPL_L4_MOD,
          BPL_L5_MOD, BPL_L6_MOD, BPL_L7_MOD, BPL_L8_MOD },
        { BPL_H1_MOD, BPL_H2_MOD, BPL_H3_MOD, BPL_H4_MOD,
          BPL_H5_MOD, BPL_H6_MOD, BPL_H7_MOD, BPL_H8_MOD },
        { BPL_S1_MOD, BPL_S2_MOD, EVENT_NONE, EVENT_NONE,
          EVENT_NONE, EVENT_NONE, EVENT_NONE, EVENT_NONE }
    };

    fetchUnit = fetchUnits[fm][res];
    cntMask = u8(fetchUnit / 2 - 1);

    auto start = isize(fetchStarts[fm][res]);
    auto planes = isize(maxPlanes[fm][res]);

    /* Number of words provided by a single fetch. Since the fetch unit grows
     * with the bus width, each plane is fetched exactly (fetchUnit / start)
     * times per fetch unit, no matter which mode is active.
     */
    fetchWords = u8(1 << fm);

    // Super Hires is limited to two bitplanes, because BPL_S3 - BPL_S8 are missing
    if (res == 2) planes = std::min(planes, isize(2));

    // Disable the bitplanes if the current mode cannot feed them all
    if (bpu > planes) bpu = 0;

    // Seven bitplanes is an invalid setting; the hardware treats it as 4
    //if (!agnus.isAGA() && bpu == 7) bpu = 4;

  if (!agnus.isAGA())
  {
    switch(resol)
    {
        case Resolution::LORES:
            if(bpu == 7) bpu = 4;
            break;
        case Resolution::HIRES:
            if(bpu > 4) bpu = 0;
            break;
        case Resolution::SHRES:
            if(bpu > 2) bpu = 0;
            break;        
    }   
  }

    const u8 *seq = sequences[planes == 2 ? 0 : planes == 4 ? 1 : 2];

    for (isize i = 0; i < 32; i++) {
        fetch[0][i] = fetch[1][i] = EVENT_NONE;
    }

    for (isize i = 0; i < fetchUnit; i++) {

        // Determine the position inside the current repetition
        isize cycle = i % start;

        // Cycles beyond the plane count of the current mode stay free
        if (cycle >= planes) continue;

        // Look up which bitplane is fetched in this cycle
        isize plane = seq[cycle & 7];
        if (bpu < plane) continue;

        /* The modulo is added by the last repetition of the fetch unit, so
         * that it is applied exactly once per bitplane and rasterline.
         */
        bool last = i >= fetchUnit - start;

        fetch[0][i] = ids[res][plane - 1];
        fetch[1][i] = last ? modIds[res][plane - 1] : ids[res][plane - 1];
    }
}

}
