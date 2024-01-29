// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#pragma once

#include "Aliases.h"
#include "Reflection.h"

//
// Enumerations
//

enum_long(SPD)
{
    SPD_NONE,
    SPD_NULLMODEM,
    SPD_LOOPBACK
};
typedef SPD SerialPortDevice;

#ifdef __cplusplus
struct SerialPortDeviceEnum : util::Reflection<SerialPortDeviceEnum, SerialPortDevice>
{
    static constexpr long minVal = 0;
    static constexpr long maxVal = SPD_LOOPBACK;
    static bool isValid(auto val) { return val >= minVal && val <= maxVal; }
    
    static const char *prefix() { return "SPD"; }
    static const char *key(SerialPortDevice value)
    {
        switch (value) {

            case SPD_NONE:      return "NONE";
            case SPD_NULLMODEM: return "NULLMODEM";
            case SPD_LOOPBACK:  return "LOOPBACK";
        }
        return "???";
    }
};
#endif

//
// Structures
//

typedef struct
{
    SerialPortDevice device;
    bool verbose;
}
SerialPortConfig;

typedef struct
{
    u32 port;

    bool txd;
    bool rxd;
    bool rts;
    bool cts;
    bool dsr;
    bool cd;
    bool dtr;
}
SerialPortInfo;
