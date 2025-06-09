// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#pragma once

#include "CopperTypes.h"
#include "SubComponent.h"
#include "AgnusTypes.h"
#include "Beam.h"
#include "Checksum.h"
#include "CopperDebugger.h"
#include "Memory.h"

namespace vamiga {

class Copper final : public SubComponent, public Inspectable<CopperInfo>
{
    Descriptions descriptions = {{

        .type           = Class::Copper,
        .name           = "Copper",
        .description    = "Copper",
        .shell          = "copper"
    }};

    Options options = {

    };

    friend class Agnus;
    friend class CopperDebugger;
    
public:

    // The Copper debugger
    CopperDebugger debugger = CopperDebugger(amiga);

private:
    
    // The currently executed Copper list (1 or 2)
    isize copList = 1;

    /* Indicates if the next instruction should be skipped. This flag is
     * usually false. It is set to true by the SKIP instruction if the skip
     * condition holds.
     */
    bool skip = false;

    // The Copper list location pointers
    u32 cop1lc = 0;
    u32 cop2lc = 0;

    // The Copper Danger bit (CDANG)
    bool cdang = false;

    // The Copper instruction registers
    u16 cop1ins = 0;
    u16 cop2ins = 0;

    // The Copper program counter
    u32 coppc = 0;

    // The Copper program counter at the time of the latest FETCH
    u32 coppc0 = 0;
    
    /* Indicates whether the Copper has been active since the last vertical
     * sync. The value of this variable is used to determine if a write to the
     * location registers will be pushed through the Copper's program counter.
     */
    bool activeInThisFrame = false;

public:

    // Indicates if breakpoint or watchpoint checking is needed
    bool checkForBreakpoints = false;
    bool checkForWatchpoints = false;

    // Indicates if Copper is currently servicing an event (for debugging only)
    bool servicing = false;
    

    //
    // Debugging
    //

private:

    u64 checkcnt = 0;
    u32 checksum = util::fnvInit32();


    //
    // Initializing
    //
    
public:
    
    Copper(Amiga& ref);

    Copper& operator= (const Copper& other) {

        CLONE(copList)
        CLONE(skip)
        CLONE(cop1lc)
        CLONE(cop2lc)
        CLONE(cdang)
        CLONE(cop1ins)
        CLONE(cop2ins)
        CLONE(coppc)
        CLONE(coppc0)
        CLONE(activeInThisFrame)

        return *this;
    }


    //
    // Methods from Serializable
    //
    
private:
        
    template <class T>
    void serialize(T& worker)
    {
        worker

        << copList
        << skip
        << cop1lc
        << cop2lc
        << cdang
        << cop1ins
        << cop2ins
        << coppc
        << coppc0
        << activeInThisFrame;
   
    } SERIALIZERS(serialize);

public:

    const Descriptions &getDescriptions() const override { return descriptions; }


    //
    // Methods from CoreComponent
    //

public:

    const Options &getOptions() const override { return options; }

private:

    void _dump(Category category, std::ostream &os) const override;


    //
    // Methods from Inspectable
    //

public:

    void cacheInfo(CopperInfo &result) const override;
    

    //
    // Accessing
    //

public:
    
    u32 getCopPC0() const { return coppc0; }

    void pokeCOPCON(u16 value);
    template <Accessor s> void pokeCOPJMP1();
    template <Accessor s> void pokeCOPJMP2();
    void pokeCOPINS(u16 value);
    void pokeCOP1LCH(u16 value);
    void pokeCOP1LCL(u16 value);
    void pokeCOP2LCH(u16 value);
    void pokeCOP2LCL(u16 value);
    void pokeNOOP(u16 value);

    
    //
    // Running the device
    //
    
private:

    // Sets the program counter to a given address
    void setPC(u32 addr);
    
    // Advances the program counter
    void advancePC();

    // Switches the Copper list
    void switchToCopperList(isize nr);

    /* Searches for the next matching beam position. This function is called
     * when a WAIT statement is processed. It is uses to compute where the
     * Copper wakes up.
     *
     * Return values:
     *
     * true:  The Copper wakes up in the current frame.
     *        The wake-up position is returned in variable 'result'.
     * false: The Copper does not wake up the current frame.
     *        Variable 'result' remains untouched.
     */
    bool findMatchOld(Beam &result) const; // DEPRECATED
    bool findMatch(Beam &result) const;

    // Called by findMatch() to determine the horizontal trigger position
    bool findHorizontalMatchOld(u32 &beam, u32 comp, u32 mask) const; // DEPRECATED
    bool findHorizontalMatch(u32 &beam, u32 comp, u32 mask) const;

    // Emulates the Copper writing a value into one of the custom registers
    void move(u32 addr, u16 value);

    // Runs the comparator circuit (DEPRECATED)
    /*
     bool comparator(Beam beam, u16 waitpos, u16 mask) const;
     bool comparator(Beam beam) const;
     bool comparator() const;
     */
    
    // Runs the comparator circuit
    bool runComparator() const;
    bool runComparator(Beam beam) const;
    bool runComparator(Beam beam, u16 waitpos, u16 mask) const;
    bool runHorizontalComparator(Beam beam, u16 waitpos, u16 mask) const;

    // Emulates a WAIT command
    void scheduleWaitWakeup(bool bfd);


    //
    // Analyzing Copper instructions
    //
    
private:
    
    /*             MOVE              WAIT              SKIP
     * Bit   cop1ins cop2ins   cop1ins cop2ins   cop1ins cop2ins
     *  15      x     DW15       VP7     BFD       VP7     BFD
     *  14      x     DW14       VP6     VM6       VP6     VM6
     *  13      x     DW13       VP5     VM5       VP5     VM5
     *  12      x     DW12       VP4     VM4       VP4     VM4
     *  11      x     DW11       VP3     VM3       VP3     VM3
     *  10      x     DW10       VP2     VM2       VP2     VM2
     *   9      x     DW9        VP1     VM1       VP1     VM1
     *   8     RA8    DW8        VP0     VM0       VP0     VM0
     *   7     RA7    DW7        HP8     HM8       HP8     HM8
     *   6     RA6    DW6        HP7     HM7       HP7     HM7
     *   5     RA5    DW5        HP6     HM6       HP6     HM6
     *   4     RA4    DW4        HP5     HM5       HP5     HM5
     *   3     RA3    DW3        HP4     HM4       HP4     HM4
     *   2     RA2    DW2        HP3     HM3       HP3     HM3
     *   1     RA1    DW1        HP2     HM2       HP2     HM2
     *   0      0     DW0         1       0         1       1
     *
     * Each of the following functions exists in two variants. The first
     * variant analyzes the instruction in the instructions register. The
     * second variant analyzes the instruction at a certain location in memory.
     */

    bool isMoveCmd() const;
    bool isMoveCmd(u32 addr) const;
    
    bool isWaitCmd() const;
    bool isWaitCmd(u32 addr) const;

    bool isSkipCmd() const;
    bool isSkipCmd(u32 addr) const;

    u16 getRA() const;
    u16 getRA(u32 addr) const;

    u16 getDW() const;
    u16 getDW(u32 addr) const;

    bool getBFD() const;
    bool getBFD(u32 addr) const;

    u16 getVPHP() const;
    u16 getVPHP(u32 addr) const;
    u16 getVP() const { return HI_BYTE(getVPHP()); }
    u16 getVP(u32 addr) { return HI_BYTE(getVPHP(addr)); }
    u16 getHP() const { return LO_BYTE(getVPHP()); }
    u16 getHP(u32 addr) { return LO_BYTE(getVPHP(addr)); }
    
    u16 getVMHM() const;
    u16 getVMHM(u32 addr) const;
    u16 getVM() const { return HI_BYTE(getVMHM()); }
    u16 getVM(u32 addr) { return HI_BYTE(getVMHM(addr)); }
    u16 getHM() const { return LO_BYTE(getVMHM()); }
    u16 getHM(u32 addr) { return LO_BYTE(getVMHM(addr)); }
    
public:
    
    // Returns true if the Copper has no access to this custom register
    bool isIllegalAddress(u32 addr) const;
    
    // Returns true if the Copper instruction at addr is illegal
    bool isIllegalInstr(u32 addr) const;
    

    //
    // Managing events
    //
    
public:
    
    // Processes a Copper event
    void serviceEvent();
    void serviceEvent(EventID id);

    // Schedules the next Copper event
    void schedule(EventID next, int delay = 2);

    // Reschedules the current Copper event
    void reschedule(int delay = 1);

private:
    
    // Executed after each frame
    void eofHandler();
    

    //
    // Handling delegation calls
    //

public:

    void blitterDidTerminate();
};

}
