// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#pragma once

#include "AmigaTypes.h"
#include "BeamTypes.h"
#include "BlitterTypes.h"
#include "BusTypes.h"
#include "CopperTypes.h"
#include "DmaDebuggerTypes.h"
#include "SequencerTypes.h"

namespace vamiga {

//
// Constants
//

// Time stamp used for messages that never trigger
static constexpr i64 NEVER = INT64_MAX;

// Inspection interval in seconds (interval between INS_xxx events)
static constexpr double inspectionInterval = 0.1;


//
// Enumerations
//

enum class AgnusRevision : long
{
    OCS_OLD,          // Revision 8367 (A1000, A2000A)
    OCS,              // Revision 8371 (A500, A2000B)
    ECS_1MB,          // Revision 8372 (A500, A2000B)
    ECS_2MB           // Revision 8375 (A500+, A600)
};

struct AgnusRevisionEnum : Reflection<AgnusRevisionEnum, AgnusRevision>
{
    static constexpr long minVal = 0;
    static constexpr long maxVal = long(AgnusRevision::ECS_2MB);
    
    static const char *_key(AgnusRevision value)
    {
        switch (value) {
                
            case AgnusRevision::OCS_OLD:  return "OCS_OLD";
            case AgnusRevision::OCS:      return "OCS";
            case AgnusRevision::ECS_1MB:  return "ECS_1MB";
            case AgnusRevision::ECS_2MB:  return "ECS_2MB";
        }
        return "???";
    }
    
    static const char *help(AgnusRevision value)
    {
        switch (value) {
                
            case AgnusRevision::OCS_OLD:  return "Amiga 1000";
            case AgnusRevision::OCS:      return "Amiga 500, Amiga 2000";
            case AgnusRevision::ECS_1MB:  return "Fat Agnus";
            case AgnusRevision::ECS_2MB:  return "Fatter Agnus";
        }
        return "???";
    }
};

enum EventSlot : long
{
    // Primary slots
    SLOT_REG,                       // Register changes
    SLOT_CIAA,                      // CIA A execution
    SLOT_CIAB,                      // CIA B execution
    SLOT_BPL,                       // Bitplane DMA
    SLOT_DAS,                       // Disk, Audio, and Sprite DMA
    SLOT_COP,                       // Copper
    SLOT_BLT,                       // Blitter
    SLOT_SEC,                       // Enables secondary slots
    
    // Secondary slots
    SLOT_CH0,                       // Audio channel 0
    SLOT_CH1,                       // Audio channel 1
    SLOT_CH2,                       // Audio channel 2
    SLOT_CH3,                       // Audio channel 3
    SLOT_DSK,                       // Disk controller
    SLOT_VBL,                       // Vertical blank
    SLOT_IRQ,                       // Interrupts
    SLOT_IPL,                       // CPU Interrupt Priority Lines
    SLOT_KBD,                       // Keyboard
    SLOT_TXD,                       // Serial data out (UART)
    SLOT_RXD,                       // Serial data in (UART)
    SLOT_POT,                       // Potentiometer
    SLOT_TER,                       // Enables tertiary slots
    
    // Tertiary slots
    SLOT_DC0,                       // Disk change (Df0)
    SLOT_DC1,                       // Disk change (Df1)
    SLOT_DC2,                       // Disk change (Df2)
    SLOT_DC3,                       // Disk change (Df3)
    SLOT_HD0,                       // Hard drive Hd0
    SLOT_HD1,                       // Hard drive Hd1
    SLOT_HD2,                       // Hard drive Hd2
    SLOT_HD3,                       // Hard drive Hd3
    SLOT_MSE1,                      // Port 1 mouse
    SLOT_MSE2,                      // Port 2 mouse
    SLOT_SNP,                       // Snapshots
    SLOT_RSH,                       // Retro Shell
    SLOT_KEY,                       // Auto-typing
    SLOT_SRV,                       // Remote server manager
    SLOT_SER,                       // Serial remote server
    SLOT_BTR,                       // Beam traps
    SLOT_ALA,                       // Alarms (set by the GUI)
    SLOT_INS,                       // Handles periodic calls to inspect()
    
    SLOT_COUNT
};

constexpr bool isPrimarySlot(long s) { return s <= SLOT_SEC; }
constexpr bool isSecondarySlot(long s) { return s > SLOT_SEC && s <= SLOT_TER; }
constexpr bool isTertiarySlot(long s) { return s > SLOT_TER; }

struct EventSlotEnum : Reflection<EventSlotEnum, EventSlot>
{
    static constexpr long minVal = 0;
    static constexpr long maxVal = SLOT_COUNT - 1;
    
    static long count() { return maxVal; }
    
    static const char *_key(long value)
    {
        switch (value) {
                
            case SLOT_REG:   return "REG";
            case SLOT_CIAA:  return "CIAA";
            case SLOT_CIAB:  return "CIAB";
            case SLOT_BPL:   return "BPL";
            case SLOT_DAS:   return "DAS";
            case SLOT_COP:   return "COP";
            case SLOT_BLT:   return "BLT";
            case SLOT_SEC:   return "SEC";
                
            case SLOT_CH0:   return "CH0";
            case SLOT_CH1:   return "CH1";
            case SLOT_CH2:   return "CH2";
            case SLOT_CH3:   return "CH3";
            case SLOT_DSK:   return "DSK";
            case SLOT_VBL:   return "VBL";
            case SLOT_IRQ:   return "IRQ";
            case SLOT_IPL:   return "IPL";
            case SLOT_KBD:   return "KBD";
            case SLOT_TXD:   return "TXD";
            case SLOT_RXD:   return "RXD";
            case SLOT_POT:   return "POT";
            case SLOT_TER:   return "TER";
                
            case SLOT_DC0:   return "DC0";
            case SLOT_DC1:   return "DC1";
            case SLOT_DC2:   return "DC2";
            case SLOT_DC3:   return "DC3";
            case SLOT_HD0:   return "HD0";
            case SLOT_HD1:   return "HD1";
            case SLOT_HD2:   return "HD2";
            case SLOT_HD3:   return "HD3";
            case SLOT_MSE1:  return "MSE1";
            case SLOT_MSE2:  return "MSE2";
            case SLOT_SNP:   return "SNP";
            case SLOT_RSH:   return "RSH";
            case SLOT_KEY:   return "KEY";
            case SLOT_SRV:   return "SRV";
            case SLOT_SER:   return "SER";
            case SLOT_BTR:   return "BTR";
            case SLOT_ALA:   return "ALA";
            case SLOT_INS:   return "INS";
            case SLOT_COUNT: return "???";
        }
        return "???";
    }
    static const char *help(long value)
    {
        switch (value) {
                
            case SLOT_REG:   return "Registers";
            case SLOT_CIAA:  return "CIA A";
            case SLOT_CIAB:  return "CIA B";
            case SLOT_BPL:   return "Bitplane DMA";
            case SLOT_DAS:   return "Other DMA";
            case SLOT_COP:   return "Copper";
            case SLOT_BLT:   return "Blitter";
            case SLOT_SEC:   return "Next Secondary Event";
                
            case SLOT_CH0:   return "Audio Channel 0";
            case SLOT_CH1:   return "Audio Channel 1";
            case SLOT_CH2:   return "Audio Channel 2";
            case SLOT_CH3:   return "Audio Channel 3";
            case SLOT_DSK:   return "Disk Controller";
            case SLOT_VBL:   return "Vertical Blank";
            case SLOT_IRQ:   return "Interrupts";
            case SLOT_IPL:   return "IPL";
            case SLOT_KBD:   return "Keyboard";
            case SLOT_TXD:   return "UART Out";
            case SLOT_RXD:   return "UART In";
            case SLOT_POT:   return "Potentiometer";
            case SLOT_TER:   return "Next Tertiary Event";
                
            case SLOT_DC0:   return "Disk Change Df0";
            case SLOT_DC1:   return "Disk Change Df1";
            case SLOT_DC2:   return "Disk Change Df2";
            case SLOT_DC3:   return "Disk Change Df3";
            case SLOT_HD0:   return "Hard Drive Hd0";
            case SLOT_HD1:   return "Hard Drive Hd1";
            case SLOT_HD2:   return "Hard Drive Hd2";
            case SLOT_HD3:   return "Hard Drive Hd3";
            case SLOT_MSE1:  return "Port 1 Mouse";
            case SLOT_MSE2:  return "Port 2 Mouse";
            case SLOT_SNP:   return "Snapshots";
            case SLOT_RSH:   return "Retro Shell";
            case SLOT_KEY:   return "Auto Typing";
            case SLOT_SRV:   return "Server Daemon";
            case SLOT_SER:   return "Null Modem Cable";
            case SLOT_BTR:   return "Beam Traps";
            case SLOT_ALA:   return "Alarms";
            case SLOT_INS:   return "Inspector";
            case SLOT_COUNT: return "";
        }
        return "???";
    }
};

enum EventID : i8
{
    EVENT_NONE          = 0,
    
    //
    // Events in the primary event table
    //
    
    // REG slot
    REG_CHANGE          = 1,
    REG_EVENT_COUNT,
    
    // CIA slots
    CIA_EXECUTE         = 1,
    CIA_WAKEUP,
    CIA_EVENT_COUNT,
    
    // BPL slot
    BPL_L1              = 0x04,
    BPL_L1_MOD          = 0x08,
    BPL_L2              = 0x0C,
    BPL_L2_MOD          = 0x10,
    BPL_L3              = 0x14,
    BPL_L3_MOD          = 0x18,
    BPL_L4              = 0x1C,
    BPL_L4_MOD          = 0x20,
    BPL_L5              = 0x24,
    BPL_L5_MOD          = 0x28,
    BPL_L6              = 0x2C,
    BPL_L6_MOD          = 0x30,
    BPL_H1              = 0x34,
    BPL_H1_MOD          = 0x38,
    BPL_H2              = 0x3C,
    BPL_H2_MOD          = 0x40,
    BPL_H3              = 0x44,
    BPL_H3_MOD          = 0x48,
    BPL_H4              = 0x4C,
    BPL_H4_MOD          = 0x50,
    BPL_S1              = 0x54,
    BPL_S1_MOD          = 0x58,
    BPL_S2              = 0x5C,
    BPL_S2_MOD          = 0x60,
    BPL_EVENT_COUNT     = 0x64,
    
    // DAS slot
    DAS_REFRESH         = 1,
    DAS_D0,
    DAS_D1,
    DAS_D2,
    DAS_A0,
    DAS_A1,
    DAS_A2,
    DAS_A3,
    DAS_S0_1,
    DAS_S0_2,
    DAS_S1_1,
    DAS_S1_2,
    DAS_S2_1,
    DAS_S2_2,
    DAS_S3_1,
    DAS_S3_2,
    DAS_S4_1,
    DAS_S4_2,
    DAS_S5_1,
    DAS_S5_2,
    DAS_S6_1,
    DAS_S6_2,
    DAS_S7_1,
    DAS_S7_2,
    DAS_SDMA,
    DAS_TICK,
    DAS_EOL,
    DAS_EVENT_COUNT,
    
    DAS_HSYNC = DAS_A2, // Same cycle as A2
    
    // Copper slot
    COP_REQ_DMA         = 1,
    COP_WAKEUP,
    COP_WAKEUP_BLIT,
    COP_FETCH,
    COP_MOVE,
    COP_WAIT_OR_SKIP,
    COP_WAIT1,
    COP_WAIT2,
    COP_WAIT_BLIT,
    COP_SKIP1,
    COP_SKIP2,
    COP_JMP1,
    COP_JMP2,
    COP_VBLANK,
    COP_EVENT_COUNT,
    
    // Blitter slot
    BLT_STRT1           = 1,
    BLT_STRT2,
    BLT_COPY_SLOW,
    BLT_COPY_FAKE,
    BLT_LINE_SLOW,
    BLT_LINE_FAKE,
    BLT_EVENT_COUNT,
    
    // SEC slot
    SEC_TRIGGER         = 1,
    SEC_EVENT_COUNT,
    
    //
    // Events in secondary event table
    //
    
    // Audio channels
    CHX_PERFIN          = 1,
    CHX_EVENT_COUNT,
    
    // Disk controller slot
    DSK_ROTATE          = 1,
    DSK_EVENT_COUNT,
    
    // Strobe slot
    VBL_STROBE0         = 1,
    VBL_STROBE1,
    VBL_STROBE2,
    VBL_EVENT_COUNT,
    
    // IRQ slot
    IRQ_CHECK           = 1,
    IRQ_EVENT_COUNT,
    
    // IPL slot
    IPL_CHANGE          = 1,
    IPL_EVENT_COUNT,
    
    // Keyboard
    KBD_TIMEOUT         = 1,
    KBD_DAT,
    KBD_CLK0,
    KBD_CLK1,
    KBD_SYNC_DAT0,
    KBD_SYNC_CLK0,
    KBD_SYNC_DAT1,
    KBD_SYNC_CLK1,
    KBD_EVENT_COUNT,
    
    // Serial data out (UART)
    TXD_BIT             = 1,
    TXD_EVENT_COUNT,
    
    // Serial data in (UART)
    RXD_BIT             = 1,
    RXD_EVENT_COUT,
    
    // Potentiometer
    POT_DISCHARGE       = 1,
    POT_CHARGE,
    POT_EVENT_COUNT,
    
    // Screenshots
    SCR_TAKE            = 1,
    SCR_EVENT_COUNT,
    
    // SEC slot
    TER_TRIGGER         = 1,
    TER_EVENT_COUNT,
    
    //
    // Events in tertiary event table
    //
    
    // Disk change slot
    DCH_INSERT          = 1,
    DCH_EJECT,
    DCH_EVENT_COUNT,
    
    // Hard drive slot
    HDR_IDLE            = 1,
    HDR_EVENT_COUNT,
    
    // Mouse
    MSE_PUSH_LEFT       = 1,
    MSE_RELEASE_LEFT,
    MSE_PUSH_MIDDLE,
    MSE_RELEASE_MIDDLE,
    MSE_PUSH_RIGHT,
    MSE_RELEASE_RIGHT,
    MSE_EVENT_COUNT,
    
    // Snapshots
    SNP_TAKE            = 1,
    SNP_EVENT_COUNT,
    
    // Retro shell
    RSH_WAKEUP          = 1,
    RSH_EVENT_COUNT,
    
    // Auto typing
    KEY_AUTO_TYPE       = 1,
    KEY_EVENT_COUNT,
    
    // Remote server manager
    SRV_LAUNCH_DAEMON   = 1,
    SRV_EVENT_COUNT,
    
    // Serial remote server
    SER_RECEIVE         = 1,
    SER_EVENT_COUNT,
    
    // Beamtrap event slot
    BTR_TRIGGER         = 1,
    BTR_EVENT_COUNT,
    
    // Alarm event slot
    ALA_TRIGGER         = 1,
    ALA_EVENT_COUNT,
    
    // Inspector slot
    INS_RECORD          = 1,
    INS_EVENT_COUNT
};

static inline bool isBplxEvent(EventID id, int x)
{
    switch(id & ~0b11) {
            
        case BPL_L1: case BPL_H1: return x == 1;
        case BPL_L2: case BPL_H2: return x == 2;
        case BPL_L3: case BPL_H3: return x == 3;
        case BPL_L4: case BPL_H4: return x == 4;
        case BPL_L5:              return x == 5;
        case BPL_L6:              return x == 6;
            
        default:
            return false;
    }
}


//
// Structures
//

typedef struct
{
    bool isOCS;
    bool isECS;
    bool isPAL;
    bool isNTSC;
    
    u16 idBits;
    isize chipRamLimit;
    isize vStrobeLine;
    u16 ddfMask;
}
AgnusTraits;

typedef struct
{
    AgnusRevision revision;
    bool ptrDrops;
}
AgnusConfig;

typedef struct
{
    EventSlot slot;
    EventID eventId;
    const char *eventName;
    
    // Trigger cycle of the event
    Cycle trigger;
    Cycle triggerRel;
    
    // Trigger frame relative to the current frame
    long frameRel;
    
    // The trigger cycle translated to a beam position.
    long vpos;
    long hpos;
}
EventSlotInfo;

typedef struct
{
    Cycle cpuClock;
    Cycle cpuCycles;
    Cycle dmaClock;
    Cycle ciaAClock;
    Cycle ciaBClock;
    i64 frame;
    long vpos;
    long hpos;
}
EventInfo;

typedef struct
{
    isize vpos;
    isize hpos;
    i64 frame;
    
    u16 dmacon;
    u16 bplcon0;
    u16 ddfstrt;
    u16 ddfstop;
    u16 diwstrt;
    u16 diwstop;
    
    u16 bpl1mod;
    u16 bpl2mod;
    u16 bltamod;
    u16 bltbmod;
    u16 bltcmod;
    u16 bltdmod;
    u16 bltcon0;
    
    u32 coppc0;
    u32 dskpt;
    u32 bplpt[6];
    u32 audpt[4];
    u32 audlc[4];
    u32 bltpt[4];
    u32 sprpt[8];
    
    bool bls;
    
    EventInfo eventInfo;
    EventSlotInfo slotInfo[SLOT_COUNT];
}
AgnusInfo;

typedef struct
{
    isize usage[BUS_COUNT];
    
    double copperActivity;
    double blitterActivity;
    double diskActivity;
    double audioActivity;
    double spriteActivity;
    double bitplaneActivity;
}
AgnusStats;


//
// Private data types
//

/* Event flags
 *
 * The event flags are utilized to trigger special actions inside the
 * serviceRegEvent()
 */

typedef u32 EventFlags;

namespace EVFL
{
constexpr u32 EOL                = (1 << 0);
constexpr u32 HSYNC              = (1 << 1);
constexpr u32 PROBE              = (1 << 2);
};

}
