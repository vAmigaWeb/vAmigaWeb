// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#pragma once

#include "OSDebuggerTypes.h"
#include "SubComponent.h"
#include "Constants.h"

namespace vamiga {

class OSDebugger final : public SubComponent {
    
    Descriptions descriptions = {{

        .type           = Class::OSDebugger,
        .name           = "OSDebugger",
        .description    = "Operating System Debugger",
        .shell          = "os"
    }};

    Options options = {

    };

private:
    
    //
    // Constructing
    //
    
public:
    
    using SubComponent::SubComponent;
    
    OSDebugger& operator= (const OSDebugger& other) {

        return *this;
    }


    //
    // Methods from CoreObject
    //
    
private:
    
    void _dump(Category category, std::ostream &os) const override { }

    
    //
    // Methods from CoreComponent
    //
    
public:

    const Descriptions &getDescriptions() const override { return descriptions; }


    //
    // Methods from Configurable
    //

public:

    const Options &getOptions() const override { return options; }


    //
    // Serializing
    //
    
    template <class T> void serialize(T& worker) { } SERIALIZERS(serialize);
        

    //
    // Providing textual representations
    //

public:
    
    static string dosTypeStr(u32 type);
    static string dosVersionStr(u32 version);

private:
    
    string toString(os::LnType value) const;
    string toString(os::TState value) const;
    string toString(os::SigFlags value) const;
    string toString(os::TFlags value) const;
    string toString(os::AttnFlags value) const;
    string toString(os::LibFlags value) const;
    string toString(os::PrFlags value) const;

    void append(string &str, const char *cstr) const;
    
    
    //
    // Managing pointers
    //
    
public:
    
    bool isRamPtr(u32 addr) const;
    bool isRamOrRomPtr(u32 addr) const;
    bool isValidPtr(u32 addr) const;

    
    //
    // Extracting elementary data types from Amiga memory
    //
    
public:

    void read(u32 addr, u8 *result) const;
    void read(u32 addr, u16 *result) const;
    void read(u32 addr, u32 *result) const;
    void read(u32 addr, i8 *result) const { read(addr, (u8 *)result); }
    void read(u32 addr, i16 *result) const { read(addr, (u16 *)result); }
    void read(u32 addr, i32 *result) const { read(addr, (u32 *)result); }
    void read(u32 addr, string &result) const;
    void read(u32 addr, string &result, isize limit) const;

    
    //
    // Extracting basic structures from Amiga memory
    //

public:

    os::ExecBase getExecBase() const throws;
    
    void read(u32 addr, os::CommandLineInterface *result) const;
    void read(u32 addr, os::ExecBase *result) const;
    void read(u32 addr, os::FileSysResource *result) const;
    void read(u32 addr, os::FileSysEntry *result) const;
    void read(u32 addr, os::Interrupt *result) const;
    void read(u32 addr, os::IntVector *result) const;
    void read(u32 addr, os::IOStdReq *result) const;
    void read(u32 addr, os::Library *result) const;
    void read(u32 addr, os::List *result) const;
    void read(u32 addr, os::MinList *result) const;
    void read(u32 addr, os::Message *result) const;
    void read(u32 addr, os::MsgPort *result) const;
    void read(u32 addr, os::Node *result) const;
    void read(u32 addr, os::Process *result) const;
    void read(u32 addr, os::SoftIntList *result) const;
    void read(u32 addr, os::Task *result) const;
    
    
    //
    // Extracting nested structures from Amiga memory
    //

public:
    
    void read(u32 addr, std::vector <os::FileSysEntry> &result) const;
    void read(std::vector <os::Task> &result) const;
    void read(std::vector <os::Process> &result) const;
    void read(u32 addr, std::vector <os::Task> &result) const;
    void read(u32 addr, std::vector <os::Library> &result) const;
    void read(const string &prName, os::SegList &result) const;
    void read(const os::Process &pr, os::SegList &result) const;
    void read(u32 addr, os::SegList &result) const;

    
    //
    // Searches a structure by value (address or index), or name
    //
    
    bool searchLibrary(u32 addr, os::Library &result) const;
    bool searchLibrary(const string &name, os::Library &result) const;

    bool searchDevice(u32 addr, os::Library &result) const;
    bool searchDevice(const string &name, os::Library &result) const;

    bool searchResource(u32 addr, os::Library &result) const;
    bool searchResource(const string &name, os::Library &result) const;

    bool searchTask(u32 addr, os::Task &result) const;
    bool searchTask(const string &name, os::Task &result) const;

    bool searchProcess(u32 addr, os::Process &result) const;
    bool searchProcess(const string &name, os::Process &result) const;

    
    //
    // Performing sanity checks
    //
    
    void checkExecBase(const os::ExecBase &execBase) const throws;
    

    //
    // Printing system information
    //

public:
    
    void dumpInfo(std::ostream &s);
    void dumpExecBase(std::ostream &s);
    
    void dumpIntVectors(std::ostream &s);
    void dumpIntVector(std::ostream &s, const os::IntVector &intVector);

    void dumpLibraries(std::ostream &s);
    void dumpLibrary(std::ostream &s, u32 addr);
    void dumpLibrary(std::ostream &s, const string &name);
    void dumpLibrary(std::ostream &s, const os::Library &lib, bool verbose);

    void dumpDevices(std::ostream &s);
    void dumpDevice(std::ostream &s, u32 addr);
    void dumpDevice(std::ostream &s, const string &name);
    void dumpDevice(std::ostream &s, const os::Library &lib, bool verbose);

    void dumpResources(std::ostream &s);
    void dumpResource(std::ostream &s, u32 addr);
    void dumpResource(std::ostream &s, const string &name);
    void dumpResource(std::ostream &s, const os::Library &lib, bool verbose);

    void dumpTasks(std::ostream &s);
    void dumpTask(std::ostream &s, u32 addr);
    void dumpTask(std::ostream &s, const string &name);
    void dumpTask(std::ostream &s, const os::Task &task, bool verbose);
    
    void dumpProcesses(std::ostream &s);
    void dumpProcess(std::ostream &s, u32 addr);
    void dumpProcess(std::ostream &s, const string &name);
    void dumpProcess(std::ostream &s, const os::Process &process, bool verbose);
};

}
