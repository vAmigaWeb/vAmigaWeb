// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#pragma once

#include "Types.h"

#ifdef __cplusplus
namespace vamiga
{
#endif

//
// Frequencies
//

// Clock rate of the master clock in Hz
static const isize CLK_FREQUENCY_PAL =  28375160;               // 28.375160 MHz
static const isize CLK_FREQUENCY_NTSC = 28636360;               // 28.636360 MHz

// Clock rate of the CPU in Hz
static const isize CPU_FREQUENY_PAL =  CLK_FREQUENCY_PAL / 4;   //  7.093790 MHz
static const isize CPU_FREQUENY_NTSC = CLK_FREQUENCY_NTSC / 4;  //  7.159090 MHz

// Clock rate of the DMA bus in Hz
static const isize DMA_FREQUENY_PAL =  CLK_FREQUENCY_PAL / 8;   //  3.546895 MHz
static const isize DMA_FREQUENY_NTSC = CLK_FREQUENCY_NTSC / 8;  //  3.579545 MHz


//
// Screen parameters
//

/* Beam positions
 *
 * Vertical coordinates are measured in scanlines
 * Horizontal coordinates are measured in DMA cycles
 */

static const isize VPOS_CNT_PAL_LF =    313;
static const isize VPOS_CNT_PAL_SF =    312;
static const isize VPOS_MAX_PAL_LF =    312;
static const isize VPOS_MAX_PAL_SF =    311;
static const isize VPOS_CNT_PAL =       VPOS_CNT_PAL_LF;
static const isize VPOS_MAX_PAL =       VPOS_MAX_PAL_LF;

static const isize VPOS_CNT_NTSC_LF =   263;
static const isize VPOS_CNT_NTSC_SF =   262;
static const isize VPOS_MAX_NTSC_LF =   262;
static const isize VPOS_MAX_NTSC_SF =   261;
static const isize VPOS_CNT_NTSC =      VPOS_CNT_NTSC_LF;
static const isize VPOS_MAX_NTSC =      VPOS_MAX_NTSC_LF;

static const isize VPOS_CNT =           VPOS_CNT_PAL;
static const isize VPOS_MAX =           VPOS_MAX_PAL;

static const isize HPOS_CNT_PAL =       227;
static const isize HPOS_MAX_PAL =       226;

static const isize HPOS_CNT_NTSC_LL =   228;
static const isize HPOS_CNT_NTSC_SL =   227;
static const isize HPOS_MAX_NTSC_LL =   227;
static const isize HPOS_MAX_NTSC_SL =   226;
static const isize HPOS_CNT_NTSC =      HPOS_CNT_NTSC_LL;
static const isize HPOS_MAX_NTSC =      HPOS_MAX_NTSC_LL;

static const isize HPOS_MAX =           HPOS_MAX_NTSC_LL;
static const isize HPOS_CNT =           HPOS_CNT_NTSC_LL;


/* Screen buffer dimensions
 *
 * All values are measured in pixels. One DMA cycle corresponds to 4 pixels.
 * Hence, HPIXELS equals 4 * HPOS_CNT.
 */
static const isize VPIXELS =    313;                // VPOS_CNT
static const isize HPIXELS =    912;                // 4 * HPOS_CNT
static const isize PIXELS =     VPIXELS * HPIXELS;


/* Blanking area
 *
 * To understand the horizontal alignment of the Amiga screen, it is important
 * to note that the HBLANK area does *not* start at DMA cycle 0. According to
 * "Amiga Intern", DMA cycle $0F is the first and $35 the last cycles inside
 * the HBLANK area. However, these values seem to be wrong. According to the
 * Agnus schematics, the first HBLANK cycle is $12 and the last cycle is $23.
 * As a result, the early DMA cycles do not appear on the left side of the
 * screen.
 */

static const isize HBLANK_MIN = 0x12;       // First HBLANK cycle
static const isize HBLANK_MAX = 0x23;       // Last HBLANK cycle
static const isize HBLANK_CNT = 0x12;       // HBLANK_MAX - HBLANK_MIN + 1

static const isize VBLANK_MIN_PAL = 0x00;   // First VBLANK line
static const isize VBLANK_MAX_PAL = 0x19;   // Last VBLANK line
static const isize VBLANK_CNT_PAL = 0x1A;   // VBLANK_MAX - VBLANK_MIN + 1

static const isize VBLANK_MIN_NTSC = 0x00;  // First VBLANK line
static const isize VBLANK_MAX_NTSC = 0x13;  // Last VBLANK line
static const isize VBLANK_CNT_NTSC = 0x14;  // VBLANK_MAX - VBLANK_MIN + 1


//
// Hard drive geometry limits
//

static const isize HDR_C_MIN =  16;
static const isize HDR_C_MAX =  16384;
static const isize HDR_H_MIN =  1;
static const isize HDR_H_MAX =  16;
static const isize HDR_S_MIN =  16;
static const isize HDR_S_MAX =  63;


//
// Custom registers
//

// DMACON register bits
static const u16 BBUSY  =   0x4000;
static const u16 BZERO  =   0x2000;
static const u16 BLTPRI =   0x0400;
static const u16 DMAEN  =   0x0200;
static const u16 BPLEN  =   0x0100;
static const u16 COPEN  =   0x0080;
static const u16 BLTEN  =   0x0040;
static const u16 SPREN  =   0x0020;
static const u16 DSKEN  =   0x0010;
static const u16 AUD3EN =   0x0008;
static const u16 AUD2EN =   0x0004;
static const u16 AUD1EN =   0x0002;
static const u16 AUD0EN =   0x0001;

static const u16 AUDEN  =   0x000F;

#ifdef __cplusplus
}
#endif
