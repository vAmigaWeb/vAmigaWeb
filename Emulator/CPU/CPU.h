// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "CPUTypes.h"
#include "SubComponent.h"
#include "Moira.h"

class CPU : public moira::Moira {

    // The current configuration
    CPUConfig config = {};

    // Result of the latest inspection
    mutable CPUInfo info = {};

    
    //
    // Initializing
    //

public:

    CPU(Amiga& ref);

    
    //
    // Methods from AmigaObject
    //
    
private:
    
    const char *getDescription() const override { return "CPU"; }
    void _dump(dump::Category category, std::ostream& os) const override;

    
    //
    // Methods from AmigaComponent
    //
    
private:
    
    void _reset(bool hard) override;
    void _inspect() const override;
    void _debugOn() override;
    void _debugOff() override;
    void _inspect(u32 dasmStart) const;
    
    template <class T>
    void applyToPersistentItems(T& worker)
    {
        worker

        << config.regResetVal;
    }

    template <class T>
    void applyToResetItems(T& worker, bool hard = true)
    {
        if (hard) {
            
            worker
            
            << flags
            << clock
            
            << reg.pc
            << reg.pc0
            << reg.sr.t
            << reg.sr.s
            << reg.sr.x
            << reg.sr.n
            << reg.sr.z
            << reg.sr.v
            << reg.sr.c
            << reg.sr.ipl
            << reg.r
            << reg.usp
            << reg.ssp
            << reg.ipl
            
            << queue.irc
            << queue.ird
            
            << ipl
            << fcl
            << exception;
        }
    }

    isize _size() override { COMPUTE_SNAPSHOT_SIZE }
    u64 _checksum() override { COMPUTE_SNAPSHOT_CHECKSUM }
    isize _load(const u8 *buffer) override { LOAD_SNAPSHOT_ITEMS }
    isize _save(u8 *buffer) override { SAVE_SNAPSHOT_ITEMS }
    isize didLoadFromBuffer(const u8 *buffer) override;
    
    
    //
    // Configuring
    //
    
public:
    
    static CPUConfig getDefaultConfig();
    const CPUConfig &getConfig() const { return config; }
    void resetConfig() override;
    
    i64 getConfigItem(Option option) const;
    void setConfigItem(Option option, i64 value);
 
    
    //
    // Analyzing
    //
    
public:
    
    CPUInfo getInfo() const { return AmigaComponent::getInfo(info); }
        

    //
    // Working with the clock
    //

public:

    // Returns the clock in CPU cycles
    CPUCycle getCpuClock() const { return getClock(); }

    // Returns the clock in master cycles
    Cycle getMasterClock() const { return CPU_CYCLES(getClock()); }

    // Delays the CPU by a certain amout of master cycles
    void addWaitStates(Cycle cycles) { clock += AS_CPU_CYCLES(cycles); }
    
    
    //
    // Running the disassembler
    //
    
    // Disassembles a recorded instruction from the log buffer
    const char *disassembleRecordedInstr(isize i, isize *len);
    const char *disassembleRecordedWords(isize i, isize len);
    const char *disassembleRecordedFlags(isize i);
    const char *disassembleRecordedPC(isize i);

    // Disassembles the instruction at the specified address
    const char *disassembleInstr(u32 addr, isize *len);
    const char *disassembleWords(u32 addr, isize len);
    const char *disassembleAddr(u32 addr);

    // Disassembles the currently executed instruction
    const char *disassembleInstr(isize *len);
    const char *disassembleWords(isize len);
};
