// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "ADFFile.h"

namespace vamiga {

class Folder : public FloppyFile {

public:

    ADFFile *adf = nullptr;

    static bool isCompatible(const fs::path &path);
    static bool isCompatible(const u8 *buf, isize len);
    static bool isCompatible(const Buffer<u8> &buffer);
    static bool isCompatible(std::istream &stream) { return false; }

    
    //
    // Initializing
    //
    
    Folder(const fs::path &path) throws { init(path); }
    
private:
    
    void init(const fs::path &path) throws;
    
    
    //
    // Methods from CoreObject
    //
    
public:
    
    const char *objectName() const override { return "Folder"; }

    
    //
    // Methods from AnyFile
    //
    
    bool isCompatiblePath(const fs::path &path) const override { return isCompatible(path); }
    bool isCompatibleBuffer(const u8 *buf, isize len) override { return isCompatible(buf, len); }
    FileType type() const override { return FileType::DIR; }
    u64 fnv64() const override { return adf->fnv64(); }
    
    
    //
    // Methods from DiskFile
    //

    isize numCyls() const override { return adf->numCyls(); }
    isize numHeads() const override { return adf->numHeads(); }
    isize numSectors() const override { return adf->numSectors(); }

    
    //
    // Methods from FloppyFile
    //
    
public:
    
    FSVolumeType getDos() const override { return adf->getDos(); }
    void setDos(FSVolumeType dos) override { adf->setDos(dos); }
    Diameter getDiameter() const override { return adf->getDiameter(); }
    Density getDensity() const override { return adf->getDensity(); }
    BootBlockType bootBlockType() const override { return adf->bootBlockType(); }
    const char *bootBlockName() const override { return adf->bootBlockName(); }
    void killVirus() override { adf->killVirus(); }
    void readSector(u8 *target, isize s) const override { return adf->readSector(target, s); }
    void readSector(u8 *target, isize t, isize s) const override { return adf->readSector(target, t, s); }
    void encodeDisk(class FloppyDisk &disk) const throws override { adf->encodeDisk(disk); }
};

}
