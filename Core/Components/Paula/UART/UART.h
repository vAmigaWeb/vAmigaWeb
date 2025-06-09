// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "UARTTypes.h"
#include "Constants.h"
#include "SubComponent.h"
#include "AgnusTypes.h"

namespace vamiga {

class UART final : public SubComponent, public Inspectable<UARTInfo> {

    Descriptions descriptions = {{

        .type           = Class::UART,
        .name           = "UART",
        .description    = "Universal Asynchronous Receiver Transmitter",
        .shell          = "uart"
    }};

    Options options = {

    };

    friend class SerServer;
    
    // Port period and control register
    u16 serper;

    // Input registers
    u16 receiveBuffer;
    u16 receiveShiftReg;

    // Output registers
    u16 transmitBuffer;
    u16 transmitShiftReg;

    // The bit which is currently seen on the TXD line
    bool outBit;

    // Overrun bit
    bool ovrun;

    // Bit reception counter
    u8 recCnt;

    // Experimental
    string payload;


    //
    // Initializing
    //

public:
    
    using SubComponent::SubComponent;
    
    UART& operator= (const UART& other) {

        CLONE(serper)
        CLONE(receiveBuffer)
        CLONE(receiveShiftReg)
        CLONE(transmitBuffer)
        CLONE(transmitShiftReg)
        CLONE(outBit)
        CLONE(ovrun)
        CLONE(recCnt)

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

        << serper
        << receiveBuffer
        << receiveShiftReg
        << transmitBuffer
        << transmitShiftReg
        << outBit
        << ovrun
        << recCnt;

    } SERIALIZERS(serialize);


    //
    // Methods from CoreComponent
    //

public:

    const Descriptions &getDescriptions() const override { return descriptions; }

private:
    
    void _dump(Category category, std::ostream &os) const override;
    void _didReset(bool hard) override;


    //
    // Methods from Configurable
    //

public:

    const Options &getOptions() const override { return options; }


    //
    // Methods from Inspectable
    //

public:

    void cacheInfo(UARTInfo &result) const override;

    
    //
    // Accessing
    //

public:
    
    // Serial port data and status read
    u16 peekSERDATR();
    u16 spypeekSERDATR() const;

    // Serial port data and stop bits write
    void pokeSERDAT(u16 value);
    void setSERDAT(u16 value);

    // Serial port period and control
    void pokeSERPER(u16 value);
    void setSERPER(u16 value);

    // Returns the pulse width measured in master cylces
    Cycle pulseWidth() const { return DMA_CYCLES((serper & 0x7FFF) + 1); }

    // Returns the baud rate
    isize baudRate() const { return PAL::CLK_FREQUENCY / (isize)pulseWidth(); }
    
private:

    // Returns the length of a received packet (8 or 9 bits)
    isize packetLength() const { return GET_BIT(serper, 15) ? 9 : 8; }

    // Returns true if the shift register is empty
    bool shiftRegEmpty() const { return transmitShiftReg == 0; }

    // Copies the contents of the transmit buffer to the transmit shift register
    void copyToTransmitShiftRegister();

    // Copies the contents of the receive shift register to the receive buffer
    void copyFromReceiveShiftRegister();


    //
    // Operationg this device
    //

public:

    // Updates the value on the UART's TXD line
    void updateTXD();

    // Called when the RXD port pin changes it's value
    void rxdHasChanged(bool value);

    // Feeds text into the UART
    void operator<<(char c);
    void operator<<(const string &s);

private:

    // Called when a byte has been received
    void recordIncomingByte(int byte);

    // Called when a byte has been sent
    void recordOutgoingByte(int byte);


    //
    // Serving events (UARTEvents.cpp)
    //

public:

    // Processes a bit transmission event
    void serviceTxdEvent(EventID id);

    // Process a bit reception event
    void serviceRxdEvent(EventID id);
};

}
