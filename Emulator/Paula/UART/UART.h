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
#include "SubComponent.h"
#include "SchedulerTypes.h"

class UART : public SubComponent {
    
    // Result of the latest inspection
    mutable UARTInfo info = {};

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


    //
    // Initializing
    //

public:
    
    using SubComponent::SubComponent;
    
    
    //
    // Methods from AmigaObject
    //
    
private:
    
    const char *getDescription() const override { return "UART"; }
    void _dump(dump::Category category, std::ostream& os) const override;

    
    //
    // Methods from AmigaComponent
    //
    
private:
    
    void _reset(bool hard) override;
    void _inspect() const override;
    
    template <class T>
    void applyToPersistentItems(T& worker)
    {
        
    }
    
    template <class T>
    void applyToResetItems(T& worker, bool hard = true)
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
    }

    isize _size() override { COMPUTE_SNAPSHOT_SIZE }
    u64 _checksum() override { COMPUTE_SNAPSHOT_CHECKSUM }
    isize _load(const u8 *buffer) override { LOAD_SNAPSHOT_ITEMS }
    isize _save(u8 *buffer) override { SAVE_SNAPSHOT_ITEMS }
    
    
    //
    // Analyzing
    //

public:

    UARTInfo getInfo() const { return AmigaComponent::getInfo(info); }

 
    //
    // Accessing
    //

public:
    
    // Serial port data and status read
    u16 peekSERDATR();
    
    // Serial port data and stop bits write
    void pokeSERDAT(u16 value);

    // Serial port period and control
    void pokeSERPER(u16 value);

    // Returns the baud rate (converted to DMA cycles)
    int rate() const { return DMA_CYCLES((serper & 0x7FFF) + 1); }

private:

    // Returns the length of a received packet (8 or 9 bits)
    int packetLength() const { return GET_BIT(serper, 15) ? 9 : 8; }

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


    //
    // Serving events
    //

public:

    // Processes a bit transmission event
    void serviceTxdEvent(EventID id);

    // Process a bit reception event
    void serviceRxdEvent(EventID id);

};
