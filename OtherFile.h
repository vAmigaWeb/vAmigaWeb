// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "Images/ADF/ADFFile.h"

namespace retro::vault {

class OtherFile : public FloppyDiskImage {

    ADFFile adf;
    string filename;

public:

    static bool isCompatible(const fs::path &path);


    //
    // Initializing
    //

public:

    OtherFile(const char *name, const u8 *buf, isize len) : filename(name) {

        // Remove the .disk meta extension
        if (filename.ends_with(".disk") || filename.ends_with(".DISK")) {
            filename.erase(filename.length() - 5);
        }
        init(buf, len);
    }

    using BinaryImage::init;

    const ADFFile &getADF() const { return adf; }


    //
    // Methods from Hashable
    //

public:

    u64 hash(HashAlgorithm algorithm) const noexcept override {
        return adf.hash(algorithm);
    }


    //
    // Methods from AnyImage
    //

public:

    bool validateURL(const fs::path& path) const noexcept override {
        return isCompatible(path);
    }

    ImageType type() const noexcept override { return ImageType::FLOPPY; }
    ImageFormat format() const noexcept override { return ImageFormat::EXE; }
    std::vector<string> describeImage() const noexcept override { return adf.describeImage(); }

    void didInitialize() override;


    //
    // Methods from DiskImage
    //

public:

    FSFamily fsFamily() const noexcept override { return FSFamily::AMIGA; }


    //
    // Methods from BlockDevice
    //

public:

    isize bsize() const override { return adf.bsize(); }
    isize capacity() const override { return adf.capacity(); }
    void readBlocks(u8 *dst, utl::Range<isize> r) const override { adf.readBlocks(dst, r); }
    void writeBlocks(const u8 *src, utl::Range<isize> r) override { adf.writeBlocks(src, r); };


    //
    // Methods from TrackDevice
    //

public:

    isize numCyls() const override { return adf.numCyls(); }
    isize numHeads() const override { return adf.numHeads(); }
    isize numSectors(isize t) const override { return adf.numSectors(t); }


    //
    // Methods from FloppyDiskImage
    //

public:

    Diameter getDiameter() const noexcept override { return adf.getDiameter(); }
    Density getDensity() const noexcept override { return adf.getDensity(); }

    utl::BitView encode(TrackNr t) const override { return adf.encode(t); }
    void decode(TrackNr t, utl::BitView bits) override { return adf.decode(t, bits); }
};

}
