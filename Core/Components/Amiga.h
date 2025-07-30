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
#include "MsgQueue.h"
#include "Thread.h"

// Components
#include "Agnus.h"
#include "CIA.h"
#include "CPU.h"
#include "Defaults.h"
#include "Denise.h"
#include "Memory.h"
#include "Paula.h"
#include "RTC.h"

// Ports
#include "AudioPort.h"
#include "ControlPort.h"
#include "VideoPort.h"
#include "ZorroManager.h"

// Peripherals
#include "FloppyDrive.h"
#include "HardDrive.h"
#include "Keyboard.h"
#include "Monitor.h"

// Misc
#include "GdbServer.h"
#include "Host.h"
#include "LogicAnalyzer.h"
#include "OSDebugger.h"
#include "RegressionTester.h"
#include "RemoteManager.h"
#include "RetroShell.h"
#include "RshServer.h"
#include "SerialPort.h"

namespace vamiga {

class Amiga final : public CoreComponent, public Inspectable<AmigaInfo> {

    friend class Emulator;

    Descriptions descriptions = {
        {
            .type           = Class::Amiga,
            .name           = "Amiga",
            .description    = "Commodore Amiga",
            .shell          = "amiga"
        },
        {
            .type           = Class::Amiga,
            .name           = "Amiga",
            .description    = "Commodore Amiga",
            .shell          = ""
        }
    };

    Options options = {

        Opt::AMIGA_VIDEO_FORMAT,
        Opt::AMIGA_WARP_BOOT,
        Opt::AMIGA_WARP_MODE,
        Opt::AMIGA_VSYNC,
        Opt::AMIGA_SPEED_BOOST,
        Opt::AMIGA_RUN_AHEAD,
        Opt::AMIGA_SNAP_AUTO,
        Opt::AMIGA_SNAP_DELAY,
        Opt::AMIGA_SNAP_COMPRESSOR,
        Opt::AMIGA_WS_COMPRESSION,
    };
    
    // The current configuration
    AmigaConfig config = {};


    //
    // Subcomponents
    //

public:

    // Host system information
    Host host = Host(*this);

    // Components
    CPU cpu = CPU(*this);
    CIAA ciaA = CIAA(*this);
    CIAB ciaB = CIAB(*this);
    Memory mem = Memory(*this);
    Monitor monitor = Monitor(*this);
    Agnus agnus = Agnus(*this);
    Denise denise = Denise(*this);
    Paula paula = Paula(*this);
    RTC rtc = RTC(*this);

    // Ports
    AudioPort audioPort = AudioPort(*this);
    VideoPort videoPort = VideoPort(*this);
    ControlPort controlPort1 = ControlPort(*this, 0);
    ControlPort controlPort2 = ControlPort(*this, 1);
    SerialPort serialPort = SerialPort(*this);
    ZorroManager zorro = ZorroManager(*this);

    // Floppy drives
    FloppyDrive df0 = FloppyDrive(*this, 0);
    FloppyDrive df1 = FloppyDrive(*this, 1);
    FloppyDrive df2 = FloppyDrive(*this, 2);
    FloppyDrive df3 = FloppyDrive(*this, 3);

    // Hard drives
    HardDrive hd0 = HardDrive(*this, 0);
    HardDrive hd1 = HardDrive(*this, 1);
    HardDrive hd2 = HardDrive(*this, 2);
    HardDrive hd3 = HardDrive(*this, 3);

    // Zorro boards
    HdController hd0con = HdController(*this, hd0);
    HdController hd1con = HdController(*this, hd1);
    HdController hd2con = HdController(*this, hd2);
    HdController hd3con = HdController(*this, hd3);
    RamExpansion ramExpansion = RamExpansion(*this);
    DiagBoard diagBoard= DiagBoard(*this);

    // Other Peripherals
    Keyboard keyboard = Keyboard(*this);

    // Gateway to the GUI
    MsgQueue msgQueue = MsgQueue();

    // Misc
    LogicAnalyzer logicAnalyzer = LogicAnalyzer(*this);
    RetroShell retroShell = RetroShell(*this);
    RemoteManager remoteManager = RemoteManager(*this);
    OSDebugger osDebugger = OSDebugger(*this);
    RegressionTester regressionTester = RegressionTester(*this);

    // Shortcuts
    FloppyDrive *df[4] = { &df0, &df1, &df2, &df3 };
    HardDrive *hd[4] = { &hd0, &hd1, &hd2, &hd3 };
    HdController *hdcon[4] = { &hd0con, &hd1con, &hd2con, &hd3con };


    //
    // Emulator thread
    //

private:

    /* Run loop flags. This variable is checked at the end of each runloop
     * iteration. Most of the time, the variable is 0 which causes the runloop
     * to repeat. A value greater than 0 means that one or more runloop control
     * flags are set. These flags are flags processed and the loop either
     * repeats or terminates depending on the provided flags.
     */
    RunLoopFlags flags = 0;


    //
    // Storage
    //

private:

    typedef struct { Cycle trigger; i64 payload; } Alarm;
    std::vector<Alarm> alarms;


    //
    // Static methods
    //

public:

    // Returns a version string for this release
    static string version();

    // Returns a build number string for this release
    static string build();

    // Converts a time span to the (approximate) number of master cycles
    static Cycle usec(i64 delay) { return Cycle(delay * 28LL); }
    static Cycle msec(i64 delay) { return Cycle(delay * 28000LL); }
    static Cycle sec(double delay) { return Cycle(delay * 28000000LL); }

    // Converts a number of master cycles to the (approximate) time span
    static i64 asUsec(Cycle count) { return count / 28LL; }
    static i64 asMsec(Cycle count) { return count / 28000LL; }
    static double asSec(Cycle count) { return count / 28000000.0; }


    //
    // Initializing
    //

public:

    Amiga(class Emulator& ref, isize id);
    ~Amiga();

    bool isRunAheadInstance() const { return objid == 1; }

    
    //
    // Operators
    //

public:

    Amiga& operator= (const Amiga& other) {

        CLONE(host)
        CLONE(agnus)
        CLONE(audioPort)
        CLONE(videoPort)
        CLONE(rtc)
        CLONE(denise)
        CLONE(paula)
        CLONE(zorro)
        CLONE(controlPort1)
        CLONE(controlPort2)
        CLONE(serialPort)
        CLONE(keyboard)
        CLONE(df0)
        CLONE(df1)
        CLONE(df2)
        CLONE(df3)
        CLONE(hd0)
        CLONE(hd1)
        CLONE(hd2)
        CLONE(hd3)
        CLONE(hd0con)
        CLONE(hd1con)
        CLONE(hd2con)
        CLONE(hd3con)
        CLONE(ramExpansion)
        CLONE(diagBoard)
        CLONE(ciaA)
        CLONE(ciaB)
        CLONE(mem)
        CLONE(cpu)
        CLONE(remoteManager)
        CLONE(retroShell)
        CLONE(osDebugger)
        CLONE(regressionTester)

        CLONE(flags)
        CLONE(config)

        return *this;
    }


    //
    // Methods from Serializable
    //

    template <class T>
    void serialize(T& worker)
    {
        if (isResetter(worker)) return;

        worker

        << config.type
        << config.warpMode
        << config.warpBoot
        << config.vsync
        << config.speedBoost;

    } SERIALIZERS(serialize);


    //
    // Methods from CoreComponent
    //

public:

    const Descriptions &getDescriptions() const override { return descriptions; }
    void prefix(isize level, const char *component, isize line) const override;

private:

    void _dump(Category category, std::ostream &os) const override;

    void _willReset(bool hard) override;
    void _didReset(bool hard) override;
    void _powerOn() override;
    void _powerOff() override;
    void _run() override;
    void _pause() override;
    void _halt() override;
    void _warpOn() override;
    void _warpOff() override;
    void _trackOn() override;
    void _trackOff() override;


    //
    // Methods from Inspectable
    //

public:

    void cacheInfo(AmigaInfo &result) const override;

    u64 getAutoInspectionMask() const;
    void setAutoInspectionMask(u64 mask);


    //
    // Methods from Configurable
    //

public:

    const AmigaConfig &getConfig() const { return config; }
    const Options &getOptions() const override { return options; }

    i64 getOption(Opt option) const override;
    void checkOption(Opt opt, i64 value) override;
    void setOption(Opt option, i64 value) override;
    
    // Reverts to factory settings
    void revertToFactorySettings();


    //
    // Main API for configuring the emulator
    //

public:

    // Queries an option
    i64 get(Opt opt, isize id = 0) const throws;

    // Checks an option
    void check(Opt opt, i64 value, const std::vector<isize> objids = { }) throws;

    // Sets an option
    void set(Opt opt, i64 value, const std::vector<isize> objids = { }) throws;

    // Convenience wrappers
    void set(Opt opt, const string &value, const std::vector<isize> objids = { }) throws;
    void set(const string &opt, const string &value, const std::vector<isize> objids = { }) throws;

    // Configures the emulator to match a specific Amiga model
    void set(ConfigScheme model);

public:

    // Returns the target component for an option
    Configurable *routeOption(Opt opt, isize objid);
    const Configurable *routeOption(Opt opt, isize objid) const;


    //
    // Analyzing
    //

public:

    // Returns the native refresh rate of the emulated Amiga (50Hz or 60Hz)
    double nativeRefreshRate() const;

    // Returns the native master clock frequency
    i64 nativeMasterClockFrequency() const;

    // Returns the emulated refresh rate
    double refreshRate() const;

    // Returns the master clock frequency based on the emulated refresh rate
    i64 masterClockFrequency() const;


    //
    // Emulating
    //

public:

    // Called by the Emulator class in it's own update function
    void update(CmdQueue &queue);

    // Emulates a frame
    void computeFrame();

    // Fast-forward the run-ahead instance
    void fastForward(isize frames);


    //
    // Controlling the run loop
    //

public:

    /* Sets or clears a flag for controlling the run loop. The functions are
     * thread-safe and can be called safely from outside the emulator thread.
     */
    void setFlag(u32 flags);
    void clearFlag(u32 flags);

    // Convenience wrappers
    void signalStop() { setFlag(RL::STOP); }
 
    
    //
    // Handling workspaces
    //
    
public:
    
    // Loads a workspace from a file
    void loadWorkspace(const fs::path &path) throws;

    // Saves the current workspace to a file
    void saveWorkspace(const fs::path &path);
    
    // Called by (hidden) RetroShell 'workspace' commands
    void initWorkspace();
    void activateWorkspace();
    
    
    //
    // Handling snapshots
    //

public:

    // Takes a snapshot
    MediaFile *takeSnapshot();

    // Loads a snapshot from a file
    void loadSnapshot(const fs::path &path) throws;
    void loadSnapshot(const MediaFile &file) throws;
    void loadSnapshot(const class Snapshot &snapshot) throws;

    // Saves a snapshot to a file
    void saveSnapshot(const fs::path &path) throws;

    // Services a snapshot event
    void serviceSnpEvent(EventID id);

private:

    // Schedules the next snapshot event
    void scheduleNextSnpEvent();


    //
    // Managing commands and events
    //

public:

    // Processes a command from the command queue
    void processCommand(const Command &cmd);

    // End-of-line handler
    void eolHandler();

    /* Alarms are scheduled notifications set by the client (GUI). Once the
     * trigger cycle of an alarm has been reached, the emulator sends a
     * Msg::ALARM to the client.
     */
    void setAlarmAbs(Cycle trigger, i64 payload);
    void setAlarmRel(Cycle trigger, i64 payload);

    // Services an alarm event
    void serviceAlarmEvent();

private:

    // Schedules the next alarm event
    void scheduleNextAlarm();


    //
    // Miscellaneous
    //

public:

    // Translates the current clock cycle into pseudo-random number
    u32 random();

    // Translates seed into a pseudo-random number
    u32 random(u32 seed);
};

}
