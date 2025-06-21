// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "OtherFile.h"
//#include "AmigaFile.h"
#include "MutableFileSystem.h"
#include "IOUtils.h"
#include "OSDescriptors.h"
#include "ErrorTypes.h"

namespace vamiga {

/*
string
extractSuffix(const string &s)
{
    auto idx = s.rfind('.');
    auto pos = idx != string::npos ? idx + 1 : 0;
    auto len = string::npos;
    return s.substr(pos, len);
}
bool
OtherFile::isCompatible(string path)
{
    //return true;
    auto suffix = util::uppercased(extractSuffix(path));
    return suffix == "DISK";
}
*/
bool
OtherFile::isCompatible(const std::filesystem::path &path)
{
    auto suffix = util::uppercased(path.extension().string());
    return suffix == ".DISK";
}

bool
OtherFile::isCompatible(const u8 *buf, isize len)
{
    return true;
}

bool
OtherFile::isCompatible(const Buffer<u8> &buf)
{
    return isCompatible(buf.ptr, buf.size);
}





/*bool
OtherFile::isCompatible(std::istream &stream)
{
  //  u8 signature[] = { 0x00, 0x00, 0x03, 0xF3 };

    // Only accept the file if it fits onto a HD disk
    //return util::matchingStreamHeader(stream, signature, sizeof(signature));
    return true;
}*/

void
OtherFile::finalizeRead()
{    
    // Check if this file requires a high-density disk
    bool hd = data.size > 853000;

    // Create a new file system
    MutableFileSystem volume(Diameter::INCH_35, hd ? Density::HD : Density::DD, FSVolumeType::OFS);
    volume.setName(FSName(filename));
    // Make the volume bootable
    volume.makeBootable(BootBlockId::AMIGADOS_13);
    
    // Add the file
        // Start at the root directory
    auto dir = volume.rootDir();

    // Add the executable
    volume.createFile(dir, filename , data);

    // Finalize
    volume.updateChecksums();
    
    // Move to the to root directory
 //   volume.changeDir("/");

    // Print some debug information about the volume
    if (FS_DEBUG) {
        volume.dump(Category::State);
        volume.printDirectory(true);
    }
    
    // Check file system integrity
    FSErrorReport report = volume.check(true);
    if (report.corruptedBlocks > 0) {
        warn("Found %ld corrupted blocks\n", report.corruptedBlocks);
        if (FS_DEBUG) volume.dump(Category::Blocks);
    }

    // Convert the volume into an ADF
    adf.init(volume);
}

}
