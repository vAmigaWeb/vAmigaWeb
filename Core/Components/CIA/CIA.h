// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#pragma once

#include "CIATypes.h"
#include "SubComponent.h"
#include "AgnusTypes.h"
#include "TOD.h"

namespace vamiga {

class CIA : public SubComponent, public Inspectable<CIAInfo, CIAStats> {

    friend class TOD;

    Descriptions descriptions = {
        {
            .type           = Class::CIA,
            .name           = "CIAA",
            .description    = "Complex Interface Adapter A",
            .shell          = "ciaa"
        },
        {
            .type           = Class::CIA,
            .name           = "CIAB",
            .description    = "Complex Interface Adapter B",
            .shell          = "ciab"
        }
    };

    Options options = {

        Opt::CIA_REVISION,
        Opt::CIA_TODBUG,
        Opt::CIA_ECLOCK_SYNCING,
        Opt::CIA_IDLE_SLEEP
    };

protected:

    // Current configuration
    CIAConfig config = {};


    //
    // Action flags
    //

protected:

    // Decrements timer A
    static constexpr u64 CIACountA0 =   (1ULL << 0);
    static constexpr u64 CIACountA1 =   (1ULL << 1);
    static constexpr u64 CIACountA2 =   (1ULL << 2);
    static constexpr u64 CIACountA3 =   (1ULL << 3);

    // Decrements timer B
    static constexpr u64 CIACountB0 =   (1ULL << 4);
    static constexpr u64 CIACountB1 =   (1ULL << 5);
    static constexpr u64 CIACountB2 =   (1ULL << 6);
    static constexpr u64 CIACountB3 =   (1ULL << 7);

    // Loads timer A
    static constexpr u64 CIALoadA0 =    (1ULL << 8);
    static constexpr u64 CIALoadA1 =    (1ULL << 9);
    static constexpr u64 CIALoadA2 =    (1ULL << 10);

    // Loads timer B
    static constexpr u64 CIALoadB0 =    (1ULL << 11);
    static constexpr u64 CIALoadB1 =    (1ULL << 12);
    static constexpr u64 CIALoadB2 =    (1ULL << 13);

    // Sets pin PB6 low
    static constexpr u64 CIAPB6Low0 =   (1ULL << 14);
    static constexpr u64 CIAPB6Low1 =   (1ULL << 15);

    // Sets pin PB7 low
    static constexpr u64 CIAPB7Low0 =   (1ULL << 16);
    static constexpr u64 CIAPB7Low1 =   (1ULL << 17);

    // Triggers an interrupt
    static constexpr u64 CIASetInt0 =   (1ULL << 18);
    static constexpr u64 CIASetInt1 =   (1ULL << 19);

    // Releases the interrupt line
    static constexpr u64 CIAClearInt0 = (1ULL << 20);
    static constexpr u64 CIAOneShotA0 = (1ULL << 21);
    static constexpr u64 CIAOneShotB0 = (1ULL << 22);

    // Indicates that ICR was read recently
    static constexpr u64 CIAReadIcr0 =  (1ULL << 23);
    static constexpr u64 CIAReadIcr1 =  (1ULL << 24);

    // Clears bit 8 in ICR register
    static constexpr u64 CIAClearIcr0 = (1ULL << 25);
    static constexpr u64 CIAClearIcr1 = (1ULL << 26);
    static constexpr u64 CIAClearIcr2 = (1ULL << 27);

    // Clears bit 0 - 7 in ICR register
    static constexpr u64 CIAAckIcr0 =   (1ULL << 28);
    static constexpr u64 CIAAckIcr1 =   (1ULL << 29);

    // Sets bit 8 in ICR register
    static constexpr u64 CIASetIcr0 =   (1ULL << 30);
    static constexpr u64 CIASetIcr1 =   (1ULL << 31);

    // Triggers an IRQ with TOD as source
    static constexpr u64 CIATODInt0 =   (1ULL << 32);

    // Triggers an IRQ with serial reg as source
    static constexpr u64 CIASerInt0 =   (1ULL << 33);
    static constexpr u64 CIASerInt1 =   (1ULL << 34);
    static constexpr u64 CIASerInt2 =   (1ULL << 35);

    // Move serial data reg to serial shift reg
    static constexpr u64 CIASdrToSsr0 = (1ULL << 36);
    static constexpr u64 CIASdrToSsr1 = (1ULL << 37);

    // Move serial shift reg to serial data reg
    static constexpr u64 CIASsrToSdr0 = (1ULL << 38);
    static constexpr u64 CIASsrToSdr1 = (1ULL << 39);
    static constexpr u64 CIASsrToSdr2 = (1ULL << 40);
    static constexpr u64 CIASsrToSdr3 = (1ULL << 41);

    // Clock signal driving the serial register
    static constexpr u64 CIASerClk0 =   (1ULL << 42);
    static constexpr u64 CIASerClk1 =   (1ULL << 43);
    static constexpr u64 CIASerClk2 =   (1ULL << 44);
    static constexpr u64 CIASerClk3 =   (1ULL << 45);

    static constexpr u64 CIALast =      (1ULL << 46);

    static constexpr u64 CIADelayMask = ~CIALast
    & ~CIACountA0 & ~CIACountB0 & ~CIALoadA0 & ~CIALoadB0 & ~CIAPB6Low0
    & ~CIAPB7Low0 & ~CIASetInt0 & ~CIAClearInt0 & ~CIAOneShotA0 & ~CIAOneShotB0
    & ~CIAReadIcr0 & ~CIAClearIcr0 & ~CIAAckIcr0 & ~CIASetIcr0 & ~CIATODInt0
    & ~CIASerInt0 & ~CIASdrToSsr0 & ~CIASsrToSdr0 & ~CIASerClk0;


    //
    // Subcomponents
    //

public:
    
    TOD tod = TOD(*this, amiga);


    //
    // Internals
    //
    
protected:
    
    // The CIA has been executed up to this master clock cycle
    Cycle clock;

    // Total number of skipped cycles (used by the debugger, only)
    Cycle idleCycles;

    // Action flags
    u64 delay;
    u64 feed;

    
    //
    // Timers
    //
    
protected:
    
    // Timer counters
    u16 counterA;
    u16 counterB;

    // Timer latches
    u16 latchA;
    u16 latchB;

    // Timer control registers
    u8 cra;
    u8 crb;

    
    //
    // Interrupts
    //
    
    // Interrupt mask register
    u8 imr;

    // Interrupt control register
    u8 icr;
    
    // ICR bits that need to deleted when CIAAckIcr1 hits
    u8 icrAck;

    
    //
    // Peripheral ports
    //
    
protected:
    
    // Data registers
    u8 pra;
    u8 prb;
    
    // Data directon registers
    u8 ddra;
    u8 ddrb;
    
    // Bit mask for PB outputs (0 = port register, 1 = timer)
    u8 pb67TimerMode;
    
    // PB output bits 6 and 7 in timer mode
    u8 pb67TimerOut;
    
    // PB output bits 6 and 7 in toggle mode
    u8 pb67Toggle;

    
    //
    // Port values (chip pins)
    //
    
    // Peripheral port pins
    u8 pa;
    u8 pb;

    // Serial port pins
    bool sp;
    bool cnt;
    
    // Interrupt request pin
    bool irq;
    
    
    //
    // Shift register
    //
    
protected:
    
    /* Serial data register
     * http://unusedino.de/ec64/technical/misc/cia6526/serial.html
     * "The serial port is a buffered, 8-bit synchronous shift register system.
     *  A control bit selects input or output mode. In input mode, data on the
     *  SP pin is shifted into the shift register on the rising edge of the
     *  signal applied to the CNT pin. After 8 CNT pulses, the data in the shift
     *  register is dumped into the Serial Data Register and an interrupt is
     *  generated. In the output mode, TIMER A is used for the baud rate
     *  generator. Data is shifted out on the SP pin at 1/2 the underflow rate
     *  of TIMER A. [...] Transmission will start following a write to the
     *  Serial Data Register (provided TIMER A is running and in continuous
     *  mode). The clock signal derived from TIMER A appears as an output on the
     *  CNT pin. The data in the Serial Data Register will be loaded into the
     *  shift register then shift out to the SP pin when a CNT pulse occurs.
     *  Data shifted out becomes valid on the falling edge of CNT and remains
     *  valid until the next falling edge. After 8 CNT pulses, an interrupt is
     *  generated to indicate more data can be sent. If the Serial Data Register
     *  was loaded with new information prior to this interrupt, the new data
     *  will automatically be loaded into the shift register and transmission
     *  will continue. If the microprocessor stays one byte ahead of the shift
     *  register, transmission will be continuous. If no further data is to be
     *  transmitted, after the 8th CNT pulse, CNT will return high and SP will
     *  remain at the level of the last data bit transmitted. SDR data is
     *  shifted out MSB first and serial input data should also appear in this
     *  format.
     */
    u8 sdr;

    // Serial shift register
    u8 ssr;
    
    /* Shift register counter
     * The counter is set to 8 when the shift register is loaded and decremented
     * when a bit is shifted out.
     */
    i8 serCounter;
    
    
    //
    // Sleep logic
    //

public:
    
    // Indicates if the CIA is currently idle
    bool sleeping;
    
    /* The last executed cycle before the chip went idle.
     * The variable is set in sleep()
     */
    Cycle sleepCycle;
    
    /* The first cycle to be executed after the chip went idle.
     * The variable is set in sleep()
     */
    Cycle wakeUpCycle;

protected:
    
    /* Idle counter. When the CIA's state does not change during execution,
     * this variable is increased by one. If it exceeds a certain threshhold,
     * the chip is put into idle state via sleep().
     */
    u8 tiredness;
    
    
    //
    // Initializing
    //

public:
    
    CIA(Amiga& ref, isize objid);

    bool isCIAA() const { return objid == 0; }
    bool isCIAB() const { return objid == 1; }

    CIA& operator= (const CIA& other) {

        CLONE(tod)

        CLONE(delay)
        CLONE(feed)
        CLONE(counterA)
        CLONE(counterB)
        CLONE(latchA)
        CLONE(latchB)
        CLONE(cra)
        CLONE(crb)
        CLONE(imr)
        CLONE(icr)
        CLONE(icrAck)
        CLONE(pra)
        CLONE(prb)
        CLONE(ddra)
        CLONE(ddrb)
        CLONE(pb67TimerMode)
        CLONE(pb67TimerOut)
        CLONE(pb67Toggle)
        CLONE(pa)
        CLONE(pb)
        CLONE(sp)
        CLONE(cnt)
        CLONE(irq)
        CLONE(sdr)
        CLONE(ssr)
        CLONE(serCounter)

        CLONE(clock)
        CLONE(idleCycles)
        CLONE(tiredness)
        CLONE(sleeping)
        CLONE(sleepCycle)
        CLONE(wakeUpCycle)

        CLONE(config)

        return *this;
    }


    //
    // Methods from Serializable
    //

public:

    template <class T>
    void serialize(T& worker)
    {
        worker

        << delay
        << feed
        << counterA
        << counterB
        << latchA
        << latchB
        << cra
        << crb
        << imr
        << icr
        << icrAck
        << pra
        << prb
        << ddra
        << ddrb
        << pb67TimerMode
        << pb67TimerOut
        << pb67Toggle
        << pa
        << pb
        << sp
        << cnt
        << irq
        << sdr
        << ssr
        << serCounter;

        if (isSoftResetter(worker)) return;

        worker

        << clock
        << idleCycles
        << tiredness
        << sleeping
        << sleepCycle
        << wakeUpCycle;

        if (isResetter(worker)) return;

        worker

        << config.revision
        << config.todBug
        << config.eClockSyncing;

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
    void _initialize() override;
    void _willReset(bool hard) override;
    void _didReset(bool hard) override;



    //
    // Methods from Inspectable
    //

public:

    void cacheInfo(CIAInfo &result) const override;
    void cacheStats(CIAStats &result) const override;


    //
    // Methods from Configurable
    //

public:

    const CIAConfig &getConfig() const { return config; }
    const Options &getOptions() const override { return options; }
    i64 getOption(Opt option) const override;
    void checkOption(Opt opt, i64 value) override;
    void setOption(Opt option, i64 value) override;

    
    //
    // Analyzing
    //
    
public:
    
    Cycle getClock() const { return clock; }

    
    //
    // Accessing registers
    //
    
public:
    
    // Reads a value from a CIA register
    u8 peek(u16 addr);
    
    // Reads a value from a CIA register without causing side effects
    u8 spypeek(u16 addr) const;

    // Writes a value into a CIA register
    void poke(u16 addr, u8 value);
    
    
    //
    // Accessing the data ports
    //
    
public:
    
    // Returns the data registers (call updatePA() or updatePB() first)
    u8 getPA() const { return pa; }
    u8 getPB() const { return pb; }

private:

    // Returns the data direction register
    u8 getDDRA() const { return ddra; }
    u8 getDDRB() const { return ddrb; }

    // Updates variable pa with the value we currently see at port A
    virtual void updatePA() = 0;
    virtual u8 computePA() const = 0;

    // Returns the value driving port A from inside the chip
    virtual u8 portAinternal() const = 0;
    
    // Returns the value driving port A from outside the chip
    virtual u8 portAexternal() const = 0;
    
    // Updates variable pa with the value we currently see at port B
    virtual void updatePB() = 0;
    virtual u8 computePB() const = 0;
    
    // Values driving port B from inside the chip
    virtual u8 portBinternal() const = 0;
    
    // Values driving port B from outside the chip
    virtual u8 portBexternal() const = 0;
    
protected:
    
    // Action method for poking the PA or PB register
    virtual void pokePA(u8 value) { pra = value; updatePA(); }
    virtual void pokePB(u8 value) { prb = value; updatePB(); }

    // Action method for poking the DDRA or DDRB register
    virtual void pokeDDRA(u8 value) { ddra = value; updatePA(); }
    virtual void pokeDDRB(u8 value) { ddrb = value; updatePB(); }

    
    //
    // Accessing port pins
    //
    
public:
    
    // Getter for the interrupt line
    bool getIrq() const { return irq; }

    // Simulates an edge edge on the flag pin
    void emulateRisingEdgeOnFlagPin();
    void emulateFallingEdgeOnFlagPin();

    // Simulates an edge on the CNT pin
    void emulateRisingEdgeOnCntPin();
    void emulateFallingEdgeOnCntPin();

    // Sets the serial port pin
    void setSP(bool value) { sp = value; }
    
    
    //
    // Handling interrupts
    //

public:
    
    // Handles an interrupt request from TOD
    void todInterrupt();

private:

    // Requests the CPU to interrupt
    virtual void pullDownInterruptLine() = 0;
    
    // Removes the interrupt requests
    virtual void releaseInterruptLine() = 0;
    
    // Loads a latched value into timer
    void reloadTimerA(u64 *delay);
    void reloadTimerB(u64 *delay);
    
    // Triggers an interrupt (invoked inside executeOneCycle())
    void triggerTimerIrq(u64 *delay);
    void triggerTodIrq(u64 *delay);
    void triggerFlagPinIrq(u64 *delay);
    void triggerSerialIrq(u64 *delay);
    
    
    //
    // Handling events
    //
    
public:
    
    // Services an event in the CIA slot
    void serviceEvent(EventID id);
    
    // Schedules the next execution event
    void scheduleNextExecution();
    
    // Schedules the next wakeup event
    void scheduleWakeUp();

    
    //
    // Executing
    //
    
public:

    // Executes the CIA for one CIA cycle
    void executeOneCycle();
    
    // Called by Agnus at the end of each frame
    void eofHandler();
    
    
    //
    // Speeding up (sleep logic)
    //
    
private:
    
    // Puts the CIA into idle state
    void sleep();
    
public:
    
    // Emulates all previously skipped cycles
    void wakeUp();
    void wakeUp(Cycle targetCycle);
    
    // Returns true if the CIA is in idle state or not
    bool isSleeping() const { return sleeping; }
    bool isAwake() const { return !sleeping; }

    // Returns the number of cycles the CIA is idle since
    CIACycle idleSince() const;
    
    // Retruns the total number of cycles the CIA was idle
    CIACycle idleTotal() const { return AS_CIA_CYCLES(idleCycles); }
};


//
// CIAA
//

class CIAA final : public CIA {

public:
    
    CIAA(Amiga& ref) : CIA(ref, 0) { };

private:
    
    void _powerOn() override;
    void _powerOff() override;
    
    void pullDownInterruptLine() override;
    void releaseInterruptLine() override;
    
    u8 portAinternal() const override;
    u8 portAexternal() const override;
    void updatePA() override;
    u8 computePA() const override;

    u8 portBinternal() const override;
    u8 portBexternal() const override;
    void updatePB() override;
    u8 computePB() const override;
    
public:

    // Indicates if the power LED is currently on or off
    bool powerLED() const { return (pa & 0x2) == 0; }

    // Emulates the reception of a keycode from the keyboard
    void setKeyCode(u8 keyCode);
};


//
// CIAB
//

class CIAB final : public CIA {
    
public:
    
    CIAB(Amiga& ref) : CIA(ref, 1) { };

private:

    void pullDownInterruptLine() override;
    void releaseInterruptLine() override;
    
    u8 portAinternal() const override;
    u8 portAexternal() const override;
    void updatePA() override;
    u8 computePA() const override;

    u8 portBinternal() const override;
    u8 portBexternal() const override;
    void updatePB() override;
    u8 computePB() const override;
};

}
