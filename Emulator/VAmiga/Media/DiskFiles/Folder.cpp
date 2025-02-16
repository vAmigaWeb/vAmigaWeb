// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "VAmigaConfig.h"
#include "Folder.h"
#include "MutableFileSystem.h"
#include "IOUtils.h"

namespace vamiga {

bool
Folder::isCompatible(const std::filesystem::path &path)
{
    if (!util::isDirectory(path)) return false;
    
    auto suffix = util::uppercased(path.extension().string());
    return suffix != ".VAMIGA";
}

bool
Folder::isCompatible(const u8 *buf, isize len)
{
    return false;
}

bool
Folder::isCompatible(const Buffer<u8> &buf)
{
    return isCompatible(buf.ptr, buf.size);
}

void
Folder::init(const std::filesystem::path &path)
{
    debug(FS_DEBUG, "make(%s)\n", path.string().c_str());

    // Only proceed if the provided filename points to a directory
    if (!isCompatiblePath(path)) throw CoreError(Fault::FILE_TYPE_MISMATCH);

    // Create a file system and import the directory
    MutableFileSystem volume(FSVolumeType::OFS, path.c_str());
    
    // Make the volume bootable
    volume.makeBootable(BootBlockId::AMIGADOS_13);
    
    // Check the file system for errors
    volume.dump(Category::State);
    volume.printDirectory(true);

    // Check the file system for consistency
    FSErrorReport report = volume.check(true);
    if (report.corruptedBlocks > 0) {
        warn("Found %ld corrupted blocks\n", report.corruptedBlocks);
        if (FS_DEBUG) volume.dump(Category::Blocks);
    }

    // Convert the file system into an ADF
    adf = new ADFFile(volume);
}

}
