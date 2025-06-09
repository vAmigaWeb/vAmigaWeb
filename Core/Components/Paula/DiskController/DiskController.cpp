// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "DiskController.h"
#include "Amiga.h"
#include "Agnus.h"
#include "ADFFile.h"
#include "FloppyDrive.h"
#include "IOUtils.h"
#include "MsgQueue.h"
#include "Paula.h"
#include "Thread.h"
#include <algorithm>

namespace vamiga {

void
DiskController::operator << (SerResetter &worker)
{
    serialize(worker);

    prb = 0xFF;
    selected = -1;
    dsksync = 0x4489;
}

i64
DiskController::getOption(Opt option) const
{
    switch (option) {
            
        case Opt::DC_SPEED:          return config.speed;
        case Opt::DC_AUTO_DSKSYNC:   return config.autoDskSync;
        case Opt::DC_LOCK_DSKSYNC:   return config.lockDskSync;

        default:
            fatalError;
    }
}

void
DiskController::checkOption(Opt opt, i64 value)
{
    switch (opt) {

        case Opt::DC_SPEED:

            if (!isValidDriveSpeed((isize)value)) {
                throw AppError(Fault::OPT_INV_ARG, "-1, 1, 2, 4, 8");
            }
            return;

        case Opt::DC_AUTO_DSKSYNC:
        case Opt::DC_LOCK_DSKSYNC:

            return;

        default:
            throw(Fault::OPT_UNSUPPORTED);
    }
}

void
DiskController::setOption(Opt option, i64 value)
{
    switch (option) {
            
        case Opt::DC_SPEED:

            config.speed = (i32)value;
            scheduleFirstDiskEvent();
            return;

        case Opt::DC_AUTO_DSKSYNC:
            
            config.autoDskSync = value;
            return;
            
        case Opt::DC_LOCK_DSKSYNC:
            
            config.lockDskSync = value;
            return;
            
        default:
            fatalError;
    }
}

void 
DiskController::cacheInfo(DiskControllerInfo &result) const
{
    {   SYNCHRONIZED

        info.selectedDrive = selected;
        info.state = state;
        info.fifoCount = fifoCount;
        info.dsklen = dsklen;
        info.dskbytr = computeDSKBYTR();
        info.dsksync = dsksync;
        info.prb = prb;
        
        for (isize i = 0; i < 6; i++) {
            info.fifo[i] = (fifo >> (8 * i)) & 0xFF;
        }
    }
}

void
DiskController::_dump(Category category, std::ostream &os) const
{
    using namespace util;
    
    if (category == Category::Config) {
        
        dumpConfig(os);
    }

    if (category == Category::State) {
        
        os << tab("selected");
        os << dec(selected) << std::endl;
        os << tab("state");
        os << DriveStateEnum::key(state) << std::endl;
        os << tab("syncCycle");
        os << dec(syncCycle) << std::endl;
        os << tab("incoming");
        os << hex(incoming) << std::endl;
        os << tab("dataReg");
        os << hex(dataReg) << " (" << dec(dataRegCount) << ")" << std::endl;
        os << tab("fifo");
        os << hex(fifo) << " (" << dec(fifoCount) << ")" << std::endl;
        os << tab("dsklen");
        os << dec(dsklen) << std::endl;
        os << tab("dsksync");
        os << hex(dsksync) << std::endl;
        os << tab("prb");
        os << hex(prb) << std::endl;
        os << tab("spinning");
        os << bol(spinning()) << std::endl;
    }
}

FloppyDrive *
DiskController::getSelectedDrive()
{
    assert(selected < 4);
    return selected < 0 ? nullptr : df[(int)selected];
}

bool
DiskController::spinning(isize driveNr) const
{
    assert(driveNr < 4);
    return df[driveNr]->getMotor();
}

bool
DiskController::spinning() const
{
    return df0.getMotor() || df1.getMotor() ||df2.getMotor() || df3.getMotor();
}

void
DiskController::setState(DriveDmaState newState)
{
    if (state != newState) setState(state, newState);
}

void
DiskController::setState(DriveDmaState oldState, DriveDmaState newState)
{
    trace(DSK_DEBUG, "%s -> %s\n",
          DriveStateEnum::key(oldState), DriveStateEnum::key(newState));
    
    state = newState;
    
    switch (state) {
            
        case DriveDmaState::OFF:
            
            dsklen = 0;
            break;
            
        case DriveDmaState::WRITE:
            
            msgQueue.put(Msg::DRIVE_WRITE, selected);
            break;
            
        default:
            
            if (oldState == DriveDmaState::WRITE)
                msgQueue.put(Msg::DRIVE_READ, selected);
    }
}

void
DiskController::setWriteProtection(isize nr, bool value)
{
    assert(nr >= 0 && nr <= 3);
    df[nr]->setProtectionFlag(value);
}

void
DiskController::clearFifo()
{
    fifo = 0;
    fifoCount = 0;
}

u8
DiskController::readFifo()
{
    assert(fifoCount >= 1);
    
    // Remove and return the oldest byte
    fifoCount -= 1;
    return (fifo >> (8 * fifoCount)) & 0xFF;
}

u16
DiskController::readFifo16()
{
    assert(fifoCount >= 2);
    
    // Remove and return the oldest word
    fifoCount -= 2;
    return (fifo >> (8 * fifoCount)) & 0xFFFF;
}

void
DiskController::writeFifo(u8 byte)
{
    assert(fifoCount <= 6);
    
    // Remove oldest word if the FIFO is full
    if (fifoCount == 6) fifoCount -= 2;
    
    // Add the new byte
    fifo = (fifo & 0x00FF'FFFF'FFFF'FFFF) << 8 | byte;
    fifoCount++;
}

void
DiskController::transferByte()
{
    switch (state) {
            
        case DriveDmaState::OFF:
        case DriveDmaState::WAIT:
        case DriveDmaState::READ:

            readByte();
            break;

        case DriveDmaState::WRITE:
        case DriveDmaState::FLUSH:

            writeByte();
            break;

        default:
            fatalError;

    }
}

void
DiskController::readByte()
{
    FloppyDrive *drive = getSelectedDrive();

    // Read a byte from the drive
    incoming = drive ? drive->readByteAndRotate() : 0;

    // Set the byte ready flag (shows up in DSKBYT)
    incoming |= 0x8000;

    // Process all bits
    for (isize i = 7; i >= 0; i--) readBit(GET_BIT(incoming, i));
}

void
DiskController::readBit(bool bit)
{
    dataReg = (u16)((u32)dataReg << 1 | (u32)bit);

    // Fill the FIFO if we've received an entire byte
    if (++dataRegCount == 8) {

        writeFifo((u8)dataReg);
        dataRegCount = 0;
    }

    // Check if we've reached a SYNC mark
    if (dataReg == dsksync || (config.autoDskSync && syncCounter++ > 8*20000)) {

        // Save time stamp
        syncCycle = agnus.clock;

        // Trigger a word SYNC interrupt
        trace(DSK_DEBUG, "SYNC IRQ (dsklen = %d)\n", dsklen);
        paula.raiseIrq(IrqSource::DSKSYN);

        // Enable DMA if the controller was waiting for it
        if (state == DriveDmaState::WAIT) {

            dataRegCount = 0;
            clearFifo();
            setState(DriveDmaState::READ);
        }

        // Reset the watchdog counter
        syncCounter = 0;
    }
}

void
DiskController::writeByte()
{
    FloppyDrive *drive = getSelectedDrive();

    if (fifoIsEmpty()) {

        // Switch off DMA if the last byte has been flushed out
        if (state == DriveDmaState::FLUSH) setState(DriveDmaState::OFF);

    } else {

        // Read the outgoing byte from the FIFO buffer
        u8 outgoing = readFifo();

        // Write byte to disk
        if (drive) drive->writeByteAndRotate(outgoing);
    }
}

void
DiskController::performDMA()
{
    FloppyDrive *drive = getSelectedDrive();
    
    // Only proceed if there are remaining bytes to process
    if ((dsklen & 0x3FFF) == 0) return;
    
    // Only proceed if DMA is enabled
    if (state != DriveDmaState::READ && state != DriveDmaState::WRITE) return;
    
    // How many words shall we read in?
    u32 count = drive ? config.speed : 1;
    
    // Perform DMA
    switch (state) {
            
        case DriveDmaState::READ:
            
            performDMARead(drive, count);
            break;
            
        case DriveDmaState::WRITE:
            
            performDMAWrite(drive, count);
            break;
            
        default:
            fatalError;
    }
}

void
DiskController::performDMARead(FloppyDrive *drive, u32 remaining)
{
    // Only proceed if the FIFO contains enough data
    if (!fifoHasWord()) return;
    
    do {
        
        // Read next word from the FIFO buffer
        u16 word = readFifo16();
        
        // Write word into memory
        if (DSK_CHECKSUM) {
            
            checkcnt++;
            check1 = util::fnvIt32(check1, word);
            check2 = util::fnvIt32(check2, agnus.dskpt & agnus.ptrMask);
        }
        agnus.doDiskDmaWrite(word);
        
        // Finish up if this was the last word to transfer
        if ((--dsklen & 0x3FFF) == 0) {
            
            paula.raiseIrq(IrqSource::DSKBLK);
            setState(DriveDmaState::OFF);
            
            debug(DSK_CHECKSUM,
                  "read: cnt = %llu check1 = %x check2 = %x\n", checkcnt, check1, check2);
            
            return;
        }
        
        // If the loop repeats, fill the Fifo with new data
        if (--remaining) {
            
            transferByte();
            transferByte();
        }
        
    }
    while (remaining);
}

void
DiskController::performDMAWrite(FloppyDrive *drive, u32 remaining)
{
    // Only proceed if the FIFO has enough free space
    if (!fifoCanStoreWord()) return;
    
    do {

        // Read next word from memory
        if (DSK_CHECKSUM) {
            checkcnt++;
            check2 = util::fnvIt32(check2, agnus.dskpt & agnus.ptrMask);
        }
        u16 word = agnus.doDiskDmaRead();
        
        if (DSK_CHECKSUM) {
            check1 = util::fnvIt32(check1, word);
        }
        
        // Write word into FIFO buffer
        assert(fifoCount <= 4);
        writeFifo(HI_BYTE(word));
        writeFifo(LO_BYTE(word));
        
        // Finish up if this was the last word to transfer
        if ((--dsklen & 0x3FFF) == 0) {
            
            paula.raiseIrq(IrqSource::DSKBLK);
            
            /* The timing-accurate approach: Set state to DRIVE_DMA_FLUSH.
             * The event handler recognises this state and switched to
             * DRIVE_DMA_OFF once the FIFO has been emptied.
             */
            
            // setState(DRIVE_DMA_FLUSH);
            
            /* I'm unsure of the timing-accurate approach works properly,
             * because the disk IRQ would be triggered before the last byte
             * has been written.
             * Hence, we play safe here and flush the FIFO immediately.
             */
            while (!fifoIsEmpty()) {
                
                u8 value = readFifo();
                if (drive) drive->writeByteAndRotate(value);
            }
            setState(DriveDmaState::OFF);
            
            debug(DSK_CHECKSUM, "write: cnt = %llu ", checkcnt);
            debug(DSK_CHECKSUM, "check1 = %x check2 = %x\n", check1, check2);

            return;
        }
        
        // If the loop repeats, do what the event handler would do in between.
        if (--remaining) {
            
            transferByte();
            transferByte();
            assert(fifoCanStoreWord());
        }
        
    } while (remaining);
}

void
DiskController::performTurboDMA(FloppyDrive *drive)
{
    // Only proceed if there is anything to read or write
    if ((dsklen & 0x3FFF) == 0) return;
    
    // Perform action depending on DMA state
    switch (state) {
            
        case DriveDmaState::WAIT:
            
            drive->findSyncMark();
            [[fallthrough]];
            
        case DriveDmaState::READ:
            
            if (drive) performTurboRead(drive);
            if (drive) paula.raiseIrq(IrqSource::DSKSYN);
            break;
            
        case DriveDmaState::WRITE:
            
            if (drive) performTurboWrite(drive);
            break;
            
        default:
            return;
    }
    
    // Trigger disk interrupt with some delay
    Cycle delay = MIMIC_UAE ? 2 * PAL::HPOS_CNT - agnus.pos.h + 30 : 512;
    paula.scheduleIrqRel(IrqSource::DSKBLK, DMA_CYCLES(delay));
    
    setState(DriveDmaState::OFF);
}

void
DiskController::performTurboRead(FloppyDrive *drive)
{
    for (isize i = 0; i < (dsklen & 0x3FFF); i++) {
        
        // Read word from disk
        u16 word = drive->readWordAndRotate();
        
        // Write word into memory
        if (DSK_CHECKSUM) {
            
            checkcnt++;
            check1 = util::fnvIt32(check1, word);
            check2 = util::fnvIt32(check2, agnus.dskpt & agnus.ptrMask);
        }
        mem.poke16 <Accessor::AGNUS> (agnus.dskpt, word);
        agnus.dskpt += 2;
    }
    
    debug(DSK_CHECKSUM, "Turbo read %s: cyl: %ld side: %ld offset: %ld ",
          drive->objectName(),
          drive->head.cylinder,
          drive->head.head,
          drive->head.offset);
    
    debug(DSK_CHECKSUM, "checkcnt = %llu check1 = %x check2 = %x\n",
          checkcnt, check1, check2);
}

void
DiskController::performTurboWrite(FloppyDrive *drive)
{
    for (isize i = 0; i < (dsklen & 0x3FFF); i++) {
        
        // Read word from memory
        u16 word = mem.peek16 <Accessor::AGNUS> (agnus.dskpt);
        
        if (DSK_CHECKSUM) {
            
            checkcnt++;
            check1 = util::fnvIt32(check1, word);
            check2 = util::fnvIt32(check2, agnus.dskpt & agnus.ptrMask);
        }
        
        agnus.dskpt += 2;
        
        // Write word to disk
        drive->writeWordAndRotate(word);
    }
    
    debug(DSK_CHECKSUM,
          "Turbo write %s: checkcnt = %llu check1 = %x check2 = %x\n",
          drive->objectName(), checkcnt, check1, check2);
}

}
