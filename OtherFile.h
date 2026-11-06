// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "Images/EXE/EXEFile.h"

namespace vamiga {

class OtherFile : public retro::vault::EXEFile {

public:
    
    static bool isCompatible(const std::filesystem::path &path);
    OtherFile(const char *, const u8 *buf, isize len) : EXEFile(buf, len) { }
};

}
