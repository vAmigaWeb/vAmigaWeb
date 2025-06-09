// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#pragma once

#include "Infrastructure/Reflection.h"

namespace vamiga {

/* The Texel datatype is utilized to access single texture elements. Each
 * texture element represents an Amiga hires pixel. On the emulator side, the
 * Texel types either maps to u32 or u64. In the u32 case, a Texel holds a
 * single RGBA value which means that the resulting texture has hires
 * resolution. To support SuperHires modes, the emulator needs to be compiled
 * with Texel being mapped to u64. Please note that on the emulator side,
 * the texture size remains the same, no matter if Texel maps to u32 or u64.
 * Only the pixel format changes from RGBA (u32) to RGBARGBA (u64). On the
 * GPU side, however, a texel is always 32 bit. Hence, if Texel maps to u64,
 * the GPU codes sees a texture with a doubled horizontal resolution.
 */
static_assert(TPP == 1 || TPP == 2, "Texels Per Pixels (TPP) must be 1 or 2");

#if TPP == 1

typedef u32 Texel;
#define TEXEL(rgba) (rgba)

#else

typedef u64 Texel;
#define TEXEL(rgba) ((u64)rgba << 32 | rgba)

#endif

}
