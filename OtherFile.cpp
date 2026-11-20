// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "vaconfig.h"
#include "OtherFile.h"
#include "FileSystems/Amiga/FileSystem.h"
#include "utl/support/Strings.h"

namespace retro::vault {

using retro::vault::amiga::FileSystem;
using retro::vault::amiga::FSFormat;
using retro::vault::amiga::FSName;
using retro::vault::amiga::BootBlockId;

bool
OtherFile::isCompatible(const fs::path &path)
{
    return utl::uppercased(path.extension().string()) == ".DISK";
}

void
OtherFile::didInitialize()
{
    // Check if this file requires a high-density disk
    bool hd = getSize() > 853000;

    // Create a suitable ADF
    adf.init(Diameter::INCH_35, hd ? Density::HD : Density::DD);

    // Mount a file system on top of it
    auto vol = Volume(adf);
    auto fs = FileSystem(vol);

    // Format the file system, name it after the file, and make it bootable
    fs.format(FSFormat::OFS);
    fs.setName(FSName(filename));
    fs.makeBootable(BootBlockId::AMIGADOS_13);

    // Add the file under its original name 
    auto data = byteView(0, getSize());
    fs.createFile(fs.root(), FSName(filename), data.data(), data.size());

    // Finalize
    fs.flush();
}

}
