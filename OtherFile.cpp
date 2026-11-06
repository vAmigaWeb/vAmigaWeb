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

namespace vamiga {

bool
OtherFile::isCompatible(const std::filesystem::path &path)
{
    auto ext = path.extension().string();
    return ext == ".disk" || ext == ".DISK";
}

}
