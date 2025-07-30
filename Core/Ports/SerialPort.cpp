// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "SerialPort.h"
#include "IOUtils.h"
#include "Amiga.h"

namespace vamiga {

i64
SerialPort::getOption(Opt option) const
{
    switch (option) {
            
        case Opt::SER_DEVICE:    return (i64)config.device;
        case Opt::SER_VERBOSE:   return (i64)config.verbose;

        default:
            fatalError;
    }
}

void
SerialPort::checkOption(Opt opt, i64 value)
{
    switch (opt) {

        case Opt::SER_DEVICE:

            if (!SerialPortDeviceEnum::isValid(value)) {
                throw AppError(Fault::OPT_INV_ARG, SerialPortDeviceEnum::keyList());
            }
            return;

        case Opt::SER_VERBOSE:

            return;

        default:
            throw(Fault::OPT_UNSUPPORTED);
    }
}

void
SerialPort::setOption(Opt option, i64 value)
{
    switch (option) {
            
        case Opt::SER_DEVICE:
            
            if (!SerialPortDeviceEnum::isValid(value)) {
                throw AppError(Fault::OPT_INV_ARG, SerialPortDeviceEnum::keyList());
            }
            
            config.device = (SerialPortDevice)value;
            return;

        case Opt::SER_VERBOSE:

            config.verbose = bool(value);
            return;

        default:
            fatalError;
    }
}

void
SerialPort::cacheInfo(SerialPortInfo &info) const
{
    {   SYNCHRONIZED
        
        info.port = port;
        info.txd = getTXD();
        info.rxd = getRXD();
        info.rts = getRTS();
        info.cts = getCTS();
        info.dsr = getDSR();
        info.cd = getCD();
        info.dtr = getDTR();
    }
}

void
SerialPort::_dump(Category category, std::ostream &os) const
{
    using namespace util;
    
    if (category == Category::Config) {
        
        dumpConfig(os);
    }
    
    if (category == Category::State) {
        
        os << tab("Port pins");
        os << hex(port) << std::endl;
        os << tab("TXD");
        os << dec(getTXD()) << std::endl;
        os << tab("RXD");
        os << dec(getRXD()) << std::endl;
        os << tab("RTS");
        os << dec(getRTS()) << std::endl;
        os << tab("CTS");
        os << dec(getCTS()) << std::endl;
        os << tab("DSR");
        os << dec(getDSR()) << std::endl;
        os << tab("CD");
        os << dec(getCD()) << std::endl;
        os << tab("DTR");
        os << dec(getDTR()) << std::endl;
        os << tab("RI");
        os << dec(getRI()) << std::endl;
    }
}

void
SerialPort::_didReset(bool hard)
{
    incoming.clear();
    outgoing.clear();
}

bool
SerialPort::getPin(isize nr) const
{
    assert(nr >= 1 && nr <= 25);

    bool result = GET_BIT(port, nr);

    // debug(SER_DEBUG, "getPin(%d) = %d port = %X\n", nr, result, port);
    return result;
}

void
SerialPort::setPin(isize nr, bool value)
{
    // debug(SER_DEBUG, "setPin(%d,%d)\n", nr, value);
    assert(nr >= 1 && nr <= 25);

    setPort(1 << nr, value);
}

void
SerialPort::setPort(u32 mask, bool value)
{
    u32 oldPort = port;

    /* Emulate the loopback cable (if connected)
     *
     *     Connected pins: A: 2 - 3       (TXD - RXD)
     *                     B: 4 - 5 - 6   (RTS - CTS - DSR)
     *                     C: 8 - 20 - 22 (CD - DTR - RI)
     */
    if (config.device == SerialPortDevice::LOOPBACK) {

        u32 maskA = TXD_MASK | RXD_MASK;
        u32 maskB = RTS_MASK | CTS_MASK | DSR_MASK;
        u32 maskC = CD_MASK | DTR_MASK | RI_MASK;

        if (mask & maskA) mask |= maskA;
        if (mask & maskB) mask |= maskB;
        if (mask & maskC) mask |= maskC;
    }

    // Change the port pins
    if (value) port |= mask; else port &= ~mask;

    // Inform the UART if RXD has changed
    if ((oldPort ^ port) & RXD_MASK) uart.rxdHasChanged(value);
}

std::u16string
SerialPort::readIncoming()
{
    {   SYNCHRONIZED

        auto result = incoming;
        incoming.clear();
        return result;
    }
}

std::u16string
SerialPort::readOutgoing()
{
    {   SYNCHRONIZED

        auto result = outgoing;
        outgoing.clear();
        return result;
    }
}

int
SerialPort::readIncomingByte()
{
    {   SYNCHRONIZED

        if (incoming.empty()) return -1;

        int result = incoming[0];
        incoming.erase(incoming.begin());
        return result;
    }
}

int
SerialPort::readOutgoingByte()
{
    {   SYNCHRONIZED

        if (outgoing.empty()) return -1;

        int result = outgoing[0];
        outgoing.erase(outgoing.begin());
        return result;
    }
}

int
SerialPort::readIncomingPrintableByte()
{
    {   SYNCHRONIZED

        while (1) {

            auto byte = readIncomingByte();
            if (byte == -1 || isprint(byte) || byte == '\n') return byte;
        }
    }
}

int
SerialPort::readOutgoingPrintableByte()
{
    {   SYNCHRONIZED

        while (1) {

            auto byte = readOutgoingByte();
            if (byte == -1 || isprint(byte) || byte == '\n') return byte;
        }
    }
}

void 
SerialPort::operator<<(char c)
{
    uart << c;
}

void
SerialPort::operator<<(const string &text)
{
    uart << text;
}

void
SerialPort::recordIncomingByte(int byte)
{
    {   SYNCHRONIZED

        trace(SER_DEBUG, "Incoming: %02X ('%c')\n", byte, isprint(byte) ? char(byte) : '?');

        // Record the incoming byte
        incoming += char(byte);

        // Inform the GUI if the record buffer had been empty
        if (incoming.length() == 1) msgQueue.put(Msg::SER_IN);

        // Inform RetroShell
        if (config.verbose) dumpByte(byte);
    }
}

void
SerialPort::recordOutgoingByte(int byte)
{
    {   SYNCHRONIZED

        trace(SER_DEBUG, "Outgoing: %02X ('%c')\n", byte, isprint(byte) ? char(byte) : '?');

        // Record the incoming byte
        outgoing += char(byte);

        // Inform the GUI if the record buffer had been empty
        if (outgoing.length() == 1) msgQueue.put(Msg::SER_OUT);

        // Inform RetroShell
        if (config.device == SerialPortDevice::RETROSHELL || config.device == SerialPortDevice::COMMANDER) dumpByte(byte);
    }
}

void
SerialPort::dumpByte(int byte)
{
    char c = char(byte);

    if (config.device == SerialPortDevice::RETROSHELL) {

        if (isprint(c) || c == '\n') {
            retroShell << c;
        }
    }

    if (config.device == SerialPortDevice::COMMANDER) {

        switch (c) {

            case '\n':

                retroShell.press(RSKey::RETURN);
                break;

            default:

                if (isprint(c)) retroShell.press(c);
                break;
        }
    }
}

}
