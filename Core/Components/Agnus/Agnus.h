// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#pragma once

#include "AgnusTypes.h"
#include "SubComponent.h"
#include "Beam.h"
#include "Blitter.h"
#include "ChangeRecorder.h"
#include "Copper.h"
#include "DmaDebugger.h"
#include "Sequencer.h"
#include "Memory.h"

namespace vamiga {

/* Bitplane event modifiers
 *
 *                DRAW_ODD : Starts the shift registers of the odd bitplanes
 *                           to generate pixels.
 *               DRAW_EVEN : Starts the shift registers of the even bitplanes
 *                           to generate pixels.
 */
static constexpr usize DRAW_ODD =  0b001;
static constexpr usize DRAW_EVEN = 0b010;
static constexpr usize DRAW_BOTH = 0b011;


class Agnus : public SubComponent, public Inspectable<AgnusInfo, AgnusStats> {

    Descriptions descriptions = {{

        .type           = Class::Agnus,
        .name           = "Agnus",
        .description    = "DMA Controller",
        .shell          = "agnus"
    }};

    Options options = {

        Opt::AGNUS_REVISION,
        Opt::AGNUS_PTR_DROPS
    };

    // Current configuration
    AgnusConfig config = {};


    //
    // Subcomponents
    //
    
public:

    Sequencer sequencer = Sequencer(amiga);
    Copper copper = Copper(amiga);
    Blitter blitter = Blitter(amiga);
    DmaDebugger dmaDebugger = DmaDebugger(amiga);


    //
    // Event scheduler
    //

public:
    
    // Trigger cycle
    Cycle trigger[SLOT_COUNT] = { };

    // The event identifier
    EventID id[SLOT_COUNT] = { };

    // An optional data value
    i64 data[SLOT_COUNT] = { };
    
    // Next trigger cycle
    Cycle nextTrigger = NEVER;
    
    // Pending register changes
    RegChangeRecorder<8> changeRecorder;

    // Optional events to be processed in serviceRegEvent()
    EventFlags syncEvent = 0;
    
    //
    // Counters
    //

    // Agnus has been emulated up to this master clock cycle
    Cycle clock = 0;

    // The current beam position
    Beam pos = { };

    // Latched beam position (recorded when BPLCON0::ERSY is set)
    Beam latchedPos = { };

    
    //
    // Registers
    //

    // Memory mask (determines the width of all DMA memory pointer registers)
    u32 ptrMask = 0;
    
    // A copy of BPLCON0 and BPLCON1 (Denise has its own copies)
    u16 bplcon0 = 0;
    u16 bplcon0Initial = 0;
    u16 bplcon1 = 0;
    u16 bplcon1Initial = 0;

    // The DMA control register
    u16 dmacon = 0;
    u16 dmaconInitial = 0;

    // The AGA bitplane/sprite fetch mode register
    u16 fmode = 0;
    
    // The disk DMA pointer
    u32 dskpt = 0;

    // The audio DMA pointers and pointer latches
    u32 audpt[4] = { };
    u32 audlc[4] = { };

    // The bitplane DMA pointers
    u32 bplpt[8] = { };

    // AGA bus prefetch buffer (up to 4 words per fetch)
    u64 bpldatNext[8] = { };
    u8 bpldatNextValid[8] = { };

    // The bitplane modulo registers for odd bitplanes
    i16 bpl1mod = 0;

    // The bitplane modulo registers for even bitplanes
    i16 bpl2mod = 0;

    // Bitplane guesser: records the bitplane DMA address range per frame
    // All eight planes are tracked, so AGA screens are covered as well.
    static constexpr isize BPL_GUESS_PLANES = 8;
    bool bplGuessEnabled = false;
    u32 bplGuessMinLive[BPL_GUESS_PLANES] = { ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u };  // running min (current frame)
    u32 bplGuessMaxLive[BPL_GUESS_PLANES] = { };                // running max (current frame)
    u32 bplGuessMin[BPL_GUESS_PLANES] = { ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u };      // committed (last finished frame)
    u32 bplGuessMax[BPL_GUESS_PLANES] = { };
    i16 bplGuessMod[BPL_GUESS_PLANES] = { };                    // committed modulo per plane
    u16 bplGuessLineWords[BPL_GUESS_PLANES] = { };              // words in current line
    u16 bplGuessWordsLive[BPL_GUESS_PLANES] = { };              // max words per line (current frame)
    u16 bplGuessWords[BPL_GUESS_PLANES] = { };                  // committed words per line
    u16 bplGuessLinesLive[BPL_GUESS_PLANES] = { };              // scanlines with dma (current frame)
    u16 bplGuessLines[BPL_GUESS_PLANES] = { };                  // committed scanline count
    // Per-scanline first bitplane pointer (start of each line's fetch). This
    // lets the guesser derive the real line-to-line stride/modulo from the
    // measured pointer deltas instead of the BPLxMOD registers, so it stays
    // correct even for demos that reload bplpt every line via the copper/cpu.
    static constexpr isize BPL_GUESS_MAX_LINES = 320;
    bool bplGuessHadFirst[BPL_GUESS_PLANES] = { };             // first fetch seen on current line
    u32 bplGuessFirstLive[BPL_GUESS_MAX_LINES][BPL_GUESS_PLANES] = { }; // first bplpt per line (current frame)
    u32 bplGuessFirst[BPL_GUESS_MAX_LINES][BPL_GUESS_PLANES] = { };     // committed (last finished frame)
    // Per-scanline word count. The frame-wide maximum in bplGuessWords cannot
    // be used to describe an individual area, because a single odd scanline
    // (mode switch, split screen, menu bar) would widen every reported area.
    u16 bplGuessWordsPerLineLive[BPL_GUESS_MAX_LINES][BPL_GUESS_PLANES] = { }; // words per line (current frame)
    u16 bplGuessWordsPerLine[BPL_GUESS_MAX_LINES][BPL_GUESS_PLANES] = { };     // committed (last finished frame)
    /* Interlace flag of the last finished frame. In interlaced modes a frame
     * covers a single field, so the measured line-to-line stride spans two
     * picture rows (both fields are interleaved in one buffer).
     */
    bool bplGuessLace = false;

    // The sprite DMA pointers
    u32 sprpt[8] = { };

    /* AGA fetches up to four words per sprite in a single DMA cycle (see
     * FMODE). The first word is passed to Denise as usual, the remaining words
     * of a 32 or 64 pixel wide sprite are collected here (first fetched word
     * in the upper bits, so that bit 63 stays the leftmost pixel). The value
     * is always zero on OCS and ECS.
     */
    u64 sprdatNext[8] = { };


    //
    // Derived values
    //

    // Bitplane resolution (derived from bplcon0)
    Resolution res = Resolution::LORES;

    // Bitplane offsets (derived from bplcon1)
    i8 scrollOdd = 0;
    i8 scrollEven = 0;
    

    //
    // Data bus
    //

public:
    
    // Recorded bus ownership for all cycles in the current rasterline
    BusOwner busOwner[HPOS_CNT] = { };

    // Recorded address bus for all cycles in the current rasterline
    u32 busAddr[HPOS_CNT] = { };
    
    // Recorded data bus for all cycles in the current rasterline
    u16 busData[HPOS_CNT] = { };

    // Remembers the last write to SPRxCTL (EXPERIMENTAL)
    u8 lastCtlWrite[8] = { };

    //
    // Signals from other components
    //
    
private:

    // DMA requests from Paula
    bool audxDR[4] = { };
    bool audxDSR[4] = { };
    
    /* Blitter slow down. The BLS signal indicates that the CPU's request to
     * access the bus has been denied for three or more consecutive cycles.
     */
    bool bls = false;


    //
    // Sprites
    //

public:
    
    /* The vertical trigger positions of all 8 sprites. Note that Agnus knows
     * nothing about the horizontal trigger positions (only Denise does).
     */
    isize sprVStrt[8] = { };
    isize sprVStop[8] = { };

    // The current DMA states of all 8 sprites
    bool sprDmaEnabled[8] = { };

    /* Scan doubling flags of all 8 sprites (AGA only). In AGA, bit 7 of
     * SPRxPOS requests that every sprite line is displayed twice. The flag
     * only takes effect when SSCAN2 is set in FMODE (see sscan2Skips).
     */
    bool sprSscan2[8] = { };

    
    //
    // Class methods
    //

    static const char *eventName(EventSlot slot, EventID id);


    //
    // Initializing
    //
    
public:
    
    Agnus(Amiga& ref);
    Agnus& operator= (const Agnus& other);
 
    
    //
    // Methods from Serializable
    //

private:

    template <class T>
    void serialize(T& worker)
    {
        worker

        << trigger
        << id
        << data
        << nextTrigger
        << changeRecorder
        << syncEvent

        << pos
        << latchedPos

        << bplcon0
        << bplcon0Initial
        << bplcon1
        << bplcon1Initial
        << dmacon
        << dmaconInitial
        << fmode
        << dskpt
        << audpt
        << audlc
        << bplpt
        << bpldatNext
        << bpldatNextValid
        << bpl1mod
        << bpl2mod
        << sprpt
        << res
        << scrollOdd
        << scrollEven

        << busOwner
        // << busAddr
        << busData
        << lastCtlWrite

        << audxDR
        << audxDSR
        << bls

        << sprVStrt
        << sprVStop
        << sprDmaEnabled
        << sprSscan2;

        if (isSoftResetter(worker)) return;

        worker

        << clock;

        if (isResetter(worker)) return;

        worker

        << config.revision
        << config.ptrDrops
        << ptrMask;
    }

    void operator << (SerResetter &worker) override;
    void operator << (SerChecker &worker) override { serialize(worker); }
    void operator << (SerCounter &worker) override { serialize(worker); }
    void operator << (SerReader &worker) override { serialize(worker); }
    void operator << (SerWriter &worker) override { serialize(worker); }


    //
    // Methods from CoreComponent
    //

public:

    const Descriptions &getDescriptions() const override { return descriptions; }

private:
    
    void _dump(Category category, std::ostream &os) const override;

    
    //
    // Methods from Configurable
    //

public:
    
    const AgnusConfig &getConfig() const { return config; }
    const Options &getOptions() const override { return options; }
    i64 getOption(Opt option) const override;
    void checkOption(Opt opt, i64 value) override;
    void setOption(Opt option, i64 value) override;

    void setVideoFormat(TV newFormat);


    //
    // Deriving chip properties
    //

public:

    // Returns properties about the currently selected VICII revision
    AgnusTraits getTraits() const;

    bool isOCS() const;
    bool isECS() const;
    bool isAGA() const;

    /* Returns true for ECS and AGA. Because AGA is a superset of ECS, this
     * predicate has to be used for all features that were introduced by ECS
     * and are still present in AGA. Note that isECS() is an exact match which
     * yields false on an AGA machine.
     */
    bool isECSorLater() const { return isECS() || isAGA(); }
    bool isPAL() const { return pos.type == TV::PAL; }
    bool isNTSC() const { return !isPAL(); }

    // Returns the chip identification bits of this Agnus (show up in VPOSR)
    u16 idBits() const;
    
    // Returns the maximum amout of Chip Ram in KB this Agnus can handle
    isize chipRamLimit() const;

    // Returns the line in which the VERTB interrupt is triggered
    isize vStrobeLine() const { return config.revision == AgnusRevision::OCS_OLD ? 1 : 0; }
    
    // Returns a bitmask indicating the used bits in DDFSTRT / DDFSTOP
    u16 ddfMask() const { return isOCS() ? 0xFC : 0xFE; }
    
    
    //
    // Analyzing
    //
    
public:
    
    void cacheInfo(AgnusInfo &result) const override;

private:
    
    void updateStats();


    //
    // Examining the current rasterline
    //

public:

    // Indicates if the electron beam is inside the VBLANK area
    bool inVBlankArea(isize posv) const { return posv < (isPAL() ? 26 : 20); }
    bool inVBlankArea() const { return inVBlankArea(pos.v); }

    // Indicates if the current rasterline is the last line in this frame
    bool inLastRasterline(isize posv) const { return posv == pos.vMax(); }
    bool inLastRasterline() const { return inLastRasterline(pos.v); }


    //
    // Querying graphic modes
    //
    
public:

    // Computes the bitmap resolution from a given BPLCON0 value
    Resolution resolution(u16 v);

    // Queries the currently set bitmap resolution
    bool lores() { return res == Resolution::LORES; }
    bool hires() { return res == Resolution::HIRES; }
    bool shres() { return res == Resolution::SHRES; }

    /* Returns the number of words read in a single bitplane DMA cycle. The
     * value is determined by the sequencer when the fetch unit layout is
     * computed, so that DMA and display logic always agree.
     */
    u8 bplFetchWords() const { return sequencer.fetchWords; }

    /* Returns the number of words read in a single sprite DMA cycle. AGA
     * widens sprites to 32 or 64 pixels via FMODE bits 2 and 3. Both bits
     * are zero on OCS and ECS, which yields the classic 16 pixel sprite.
     */
    u8 sprFetchWords() const {

        if (!isAGA()) return 1;

        switch ((fmode >> 2) & 0b11) {

            case 0b00: return 1;
            case 0b11: return 4;
            default:   return 2;
        }
    }

    // Returns the sprite width in pixels (16, 32 or 64)
    isize spriteWidth() const { return 16 * sprFetchWords(); }

    /* Aligns a DMA pointer to the width of a single fetch. A 32 or 64 bit
     * fetch ignores the low address bits, so an unaligned pointer is truncated
     * by the hardware rather than honored. The mask is derived from the number
     * of words per fetch: 1 word leaves the pointer untouched, 2 words align it
     * to 4 bytes, 4 words to 8 bytes.
     */
    static u32 alignPtr(u32 ptr, u8 words) { return ptr & ~u32(2 * words - 1); }

    /* Adds the modulo to a bitplane pointer. AGA clears the low address bits
     * before the addition, so a pointer that was set up unaligned stays
     * truncated for the rest of the frame.
     */
    void addBplMod(isize plane) {

        u32 p = bplpt[plane];
        if (isAGA()) p = alignPtr(p, bplFetchWords());
        U32_INC(p, bplMod(plane));
        bplpt[plane] = p;
    }

    /* Selects the modulo to add to a bitplane pointer. Normally, BPL1MOD
     * belongs to the odd and BPL2MOD to the even bitplanes. The AGA bitplane
     * scan doubling bit changes the meaning of both registers completely:
     * the modulo is then picked by the parity of the rasterline instead, which
     * lets a program display every line twice by cancelling out the advance on
     * every other line (getbplmod() in WinUAE / Amiberry).
     */
    i16 bplMod(isize plane) const {

        if (bscan2()) {
            return ((sequencer.diwstrt >> 8) ^ (pos.v ^ 1)) & 1 ? bpl1mod : bpl2mod;
        }
        return (plane & 1) ? bpl2mod : bpl1mod;
    }

    // Returns the state of the AGA sprite scan doubling bit (FMODE bit 15)
    bool sscan2() const { return isAGA() && GET_BIT(fmode, 15); }

    // Returns the state of the AGA bitplane scan doubling bit (FMODE bit 14)
    bool bscan2() const { return isAGA() && GET_BIT(fmode, 14); }

    // Returns the external synchronization bit from BPLCON0
    static bool ersy(u16 value) { return GET_BIT(value, 1); }
    bool ersy() { return ersy(bplcon0); }


    //
    // Operating the device
    //
    
public:

    // Executes Agnus for a single cycle
    void execute();

    // Executes Agnus for a certain amount of cycles
    void execute(DMACycle cycles);
    
    // Executes Agnus to the beginning of the next E clock cycle
    void syncWithEClock();

    // Executes Agnus until the CPU can acquire the bus
    void executeUntilBusIsFree();
    void executeUntilBusIsFreeForCIA();

    // Schedules a register to change its value
    void recordRegisterChange(Cycle delay, RegChange regChange);
    void recordRegisterChange(Cycle delay, Reg reg, u16 value, Accessor acc = Accessor::CPU);

private:

    // Processes all events up to a given master cycle
    void executeUntil(Cycle cycle);

    // Executes the first sprite DMA cycle
    template <isize nr> void executeFirstSpriteCycle();

    // Executes the second sprite DMA cycle
    template <isize nr> void executeSecondSpriteCycle();

    // Checks whether the sprite DMA cycle is blocked by bitplane DMA
    bool spriteCycleIsBlocked();

    // Checks whether the sprite data fetch is skipped by AGA scan doubling
    bool sscan2Skips(isize nr) const;

    // Updates the sprite DMA status in cycle 0xDF
    void updateSpriteDMA();

    // Finishes up the current line
    void eolHandler();

    // Finishes up the current frame
    void eofHandler();

    // Called at the beginning of the HSYNC area
    void hsyncHandler();

    // Called at the beginning of the VSYNC area
    void vsyncHandler();


    //
    // Controlling DMA (AgnusDma.cpp)
    //

public:
    
    // Returns true if the Blitter has priority over the CPU
    static bool bltpri(u16 value) { return GET_BIT(value, 10); }
    bool bltpri() const { return bltpri(dmacon); }

    // Returns true if a certain DMA channel is enabled
    template <int x> static bool auddma(u16 v);
    template <int x> bool auddma() const { return auddma<x>(dmacon); }

    static bool bpldma(u16 v) { return (v & DMAEN) && (v & BPLEN); }
    static bool copdma(u16 v) { return (v & DMAEN) && (v & COPEN); }
    static bool bltdma(u16 v) { return (v & DMAEN) && (v & BLTEN); }
    static bool sprdma(u16 v) { return (v & DMAEN) && (v & SPREN); }
    static bool dskdma(u16 v) { return (v & DMAEN) && (v & DSKEN); }
    bool bpldma() const { return bpldma(dmacon); }
    bool copdma() const { return copdma(dmacon); }
    bool bltdma() const { return bltdma(dmacon); }
    bool sprdma() const { return sprdma(dmacon); }
    bool dskdma() const { return dskdma(dmacon); }
    

    //
    // Performing DMA (AgnusDma.cpp)
    //

public:

    // Checks if the bus is currently available for the specified resource
    template <BusOwner owner> bool busIsFree();

    // Attempts to allocate the bus for the specified resource
    template <BusOwner owner> bool allocateBus();

    // Performs a DMA read
    u16 doDiskDmaRead();
    template <int channel> u16 doAudioDmaRead();
    template <int channel> u16 doBitplaneDmaRead();
    template <int channel> u16 doSpriteDmaRead();
    u16 doCopperDmaRead(u32 addr);
    u16 doBlitterDmaRead(u32 addr);

    // Performs a DMA write
    void doDiskDmaWrite(u16 value);
    void doCopperDmaWrite(u32 addr, u16 value);
    void doBlitterDmaWrite(u32 addr, u16 value);

    // Transmits a DMA request from Agnus to Paula
    template <int channel> void setAudxDR() { audxDR[channel] = true; }
    template <int channel> void setAudxDSR() { audxDSR[channel] = true; }

    // Getter and setter for the BLS signal (Blitter slow down)
    bool getBLS() { return bls; }
    void setBLS(bool value) { bls = value; }


    //
    // Accessing registers (AgnusRegisters.cpp)
    //
    
public:

    u16 peekDMACONR() const;
    template <Accessor s> void pokeDMACON(u16 value);
    void setDMACON(u16 oldValue, u16 newValue);
    void setBPLEN(bool value);
    void setCOPEN(bool value);
    void setBLTEN(bool value);
    void setSPREN(bool value);
    void setDSKEN(bool value);
    void setAUD0EN(bool value);
    void setAUD1EN(bool value);
    void setAUD2EN(bool value);
    void setAUD3EN(bool value);

    u16 peekVHPOSR() const;
    void pokeVHPOS(u16 value);
    void setVHPOS(u16 value);

    u16 peekVPOSR() const;
    void pokeVPOS(u16 value);
    void setVPOS(u16 value);

    template <Accessor s> void pokeBPLCON0(u16 value);
    void setBPLCON0(u16 oldValue, u16 newValue);

    void pokeBPLCON1(u16 value);
    void setBPLCON1(u16 oldValue, u16 newValue);

    template <Accessor s> void pokeFMODE(u16 value);
    void setFMODE(u16 value);

    template <Accessor s> void pokeDIWSTRT(u16 value);
    template <Accessor s> void pokeDIWSTOP(u16 value);
    template <Accessor s> void pokeDIWHIGH(u16 value);

    void pokeBPL1MOD(u16 value);
    void setBPL1MOD(u16 value);

    void pokeBPL2MOD(u16 value);
    void setBPL2MOD(u16 value);
    
    template <int x, Accessor> void pokeSPRxPOS(u16 value);
    template <int x> void setSPRxPOS(u16 value);

    template <int x, Accessor> void pokeSPRxCTL(u16 value);
    template <int x> void setSPRxCTL(u16 value);

    void pokeBEAMCON0(u16 value);

    
    //
    // Accessing DMA pointer registers (AgnusRegisters.cpp)
    //
    
public:
    
    template <Accessor s> void pokeDSKPTH(u16 value);
    void setDSKPTH(u16 value);

    template <Accessor s> void pokeDSKPTL(u16 value);
    void setDSKPTL(u16 value);

    template <int x, Accessor s> void pokeAUDxLCH(u16 value);
    template <int x, Accessor s> void pokeAUDxLCL(u16 value);
    template <int x> void reloadAUDxPT() { audpt[x] = audlc[x]; }

    template <int x, Accessor s> void pokeBPLxPTH(u16 value);
    template <int x> void setBPLxPTH(u16 value);

    template <int x, Accessor s> void pokeBPLxPTL(u16 value);
    template <int x> void setBPLxPTL(u16 value);

    template <int x, Accessor s> void pokeSPRxPTH(u16 value);
    template <int x> void setSPRxPTH(u16 value);

    template <int x, Accessor s> void pokeSPRxPTL(u16 value);
    template <int x> void setSPRxPTL(u16 value);

private:
    
    // Checks whether a write to a pointer register should be dropped
    bool dropWrite(BusOwner owner);


    //
    // Checking events
    //
    
public:
    
    // Returns true iff the specified slot contains any event
    template<EventSlot s> bool hasEvent() const { return this->id[s] != (EventID)0; }
    
    // Returns true iff the specified slot contains a specific event
    template<EventSlot s> bool hasEvent(EventID id) const { return this->id[s] == id; }
    
    // Returns true iff the specified slot contains a pending event
    template<EventSlot s> bool isPending() const { return this->trigger[s] != NEVER; }
    
    // Returns true iff the specified slot contains a due event
    template<EventSlot s> bool isDue(Cycle cycle) const { return cycle >= this->trigger[s]; }
    
    
    //
    // Scheduling events
    //
    
public:
    
    template<EventSlot s> void scheduleAbs(Cycle cycle, EventID id)
    {
        this->trigger[s] = cycle;
        this->id[s] = id;
        
        if (cycle < nextTrigger) nextTrigger = cycle;
        
        if constexpr (isTertiarySlot(s)) {
            if (cycle < trigger[SLOT_TER]) trigger[SLOT_TER] = cycle;
            if (cycle < trigger[SLOT_SEC]) trigger[SLOT_SEC] = cycle;
        }
        if constexpr (isSecondarySlot(s)) {
            if (cycle < trigger[SLOT_SEC]) trigger[SLOT_SEC] = cycle;
        }
    }
    
    template<EventSlot s> void scheduleAbs(Cycle cycle, EventID id, i64 data)
    {
        scheduleAbs<s>(cycle, id);
        this->data[s] = data;
    }
    
    template<EventSlot s> void scheduleImm(EventID id)
    {
        scheduleAbs<s>(0, id);
    }
    
    template<EventSlot s> void scheduleImm(EventID id, i64 data)
    {
        scheduleAbs<s>(0, id);
        this->data[s] = data;
    }

    template<EventSlot s> void scheduleInc(Cycle cycle, EventID id)
    {
        scheduleAbs<s>(trigger[s] + cycle, id);
    }
    
    template<EventSlot s> void scheduleInc(Cycle cycle, EventID id, i64 data)
    {
        scheduleAbs<s>(trigger[s] + cycle, id);
        this->data[s] = data;
    }

    template<EventSlot s> void rescheduleAbs(Cycle cycle)
    {
        trigger[s] = cycle;
        if (cycle < nextTrigger) nextTrigger = cycle;
        
        if constexpr (isTertiarySlot(s)) {
            if (cycle < trigger[SLOT_TER]) trigger[SLOT_TER] = cycle;
        }
        if constexpr (isSecondarySlot(s) || isTertiarySlot(s)) {
            if (cycle < trigger[SLOT_SEC]) trigger[SLOT_SEC] = cycle;
        }
    }
    
    template<EventSlot s> void rescheduleInc(Cycle cycle)
    {
        rescheduleAbs<s>(trigger[s] + cycle);
    }
    template<EventSlot s> void scheduleRel(Cycle cycle, EventID id) {
        scheduleAbs<s>(clock + cycle, id);
    }

    template<EventSlot s> void scheduleRel(Cycle cycle, EventID id, i64 data) {
        scheduleAbs<s>(clock + cycle, id, data);
    }

    template<EventSlot s> void schedulePos(isize vpos, isize hpos, EventID id) {

        assert(vpos > pos.v || (vpos == pos.v && hpos >= pos.h));
        scheduleRel<s>(DMA_CYCLES(pos.diff(vpos, hpos)), id);
    }

    template<EventSlot s> void schedulePos(isize vpos, isize hpos, EventID id, i64 data) {

        assert(vpos > pos.v || (vpos == pos.v && hpos >= pos.h));
        scheduleRel<s>(DMA_CYCLES(pos.diff(vpos, hpos)), id, data);
    }
    
    template<EventSlot s> void rescheduleRel(Cycle cycle) {
        rescheduleAbs<s>(clock + cycle);
    }

    template<EventSlot s> void reschedulePos(i16 vpos, i16 hpos) {

        assert(vpos > pos.v || (vpos == pos.v && hpos >= pos.h));
        rescheduleRel<s>(DMA_CYCLES(pos.diff(vpos, hpos)));
    }

    template<EventSlot s> void cancel()
    {
        id[s] = (EventID)0;
        data[s] = 0;
        trigger[s] = NEVER;
    }

    
    //
    // Scheduling specific events (AgnusEvents.cpp)
    //

public:

    // Schedules the first BPL event
    void scheduleFirstBplEvent();

    // Schedules the next BPL event relative to a given DMA cycle
    void scheduleNextBplEvent(isize hpos);

    // Schedules the next BPL event relative to the currently emulated DMA cycle
    void scheduleNextBplEvent() { scheduleNextBplEvent(pos.h); }

    // Schedules the earliest BPL event that occurs at or after the given DMA cycle
    void scheduleBplEventForCycle(isize hpos);

    // Updates the scheduled BPL event according to the current event table
    void updateBplEvent() { scheduleBplEventForCycle(pos.h); }

    // Schedules the first BPL event
    void scheduleFirstDasEvent();

    // Schedules the next DAS event relative to a given DMA cycle
    void scheduleNextDasEvent(isize hpos);

    // Schedules the next DAS event relative to the currently emulated DMA cycle
    void scheduleNextDasEvent() { scheduleNextDasEvent(pos.h); }

    // Schedules the earliest DAS event that occurs at or after the given DMA cycle
    void scheduleDasEventForCycle(isize hpos);

    // Updates the scheduled DAS event according to the current event table
    void updateDasEvent() { scheduleDasEventForCycle(pos.h); }

    // Schedules the next register change event
    void scheduleNextREGEvent();

    // Schedules a strobe event in the VBL slot
    void scheduleStrobe0Event();
    void scheduleStrobe1Event();
    void scheduleStrobe2Event();


    //
    // Servicing events (AgnusEvents.cpp)
    //

public:

    // Services a register change event
    void serviceREGEvent(Cycle until);

    // Services a bitplane event
    void serviceBPLEvent(EventID id);
    template <isize nr> void serviceBPLEventLores();
    template <isize nr> void serviceBPLEventHires();
    template <isize nr> void serviceBPLEventShres();

    // Services a vertical blank interrupt
    void serviceVBLEvent(EventID id);

    // Renews the trigger cycle of a pending VBL event
    void rectifyVBLEvent();

    // Services a Disk, Audio, or Sprite event
    void serviceDASEvent(EventID id);
    
    // Services an inspection event
    void serviceINSEvent();
};

}
