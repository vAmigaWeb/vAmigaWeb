// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "PixelEngine.h"
#include "Amiga.h"
#include "Colors.h"
#include "Denise.h"
#include "DmaDebugger.h"
#include "Emulator.h"

#include <fstream>

namespace vamiga {

void
PixelEngine::clearAll()
{
    // Wipe out all textures
    for (isize i = 0; i < NUM_TEXTURES; i++) emuTexture[i].clear();
}

void
PixelEngine::_dump(Category category, std::ostream &os) const
{
    using namespace util;

    if (category == Category::Config) {

        dumpConfig(os);
    }
}

void
PixelEngine::_initialize()
{
    // Setup ECS/AGA BRDRBLNK color
    palette[288] = TEXEL(GpuColor(0x00, 0x00, 0x00).rawValue);

    // Setup debug colors
    palette[289] = TEXEL(GpuColor(0xD0, 0x00, 0x00).rawValue);
    palette[290] = TEXEL(GpuColor(0xA0, 0x00, 0x00).rawValue);
    palette[291] = TEXEL(GpuColor(0x90, 0x00, 0x00).rawValue);
}

void
PixelEngine::_didReset(bool hard)
{
    if (hard) {
        
        for (isize i = 0; i < NUM_TEXTURES; i++) {
            
            emuTexture[i].nr = 0;
            emuTexture[i].lof = emuTexture[i].prevlof = true;
        }
    }

    activeBuffer = 0;
    updateRGBA();
}

void
PixelEngine::_didLoad()
{
    clearAll();
    updateRGBA();
}

void
PixelEngine::_powerOn()
{
    clearAll();
}

void
PixelEngine::setColor(isize reg, AmigaColor value)
{
    assert(reg < 256);

    color[reg] = value;

    /* Update the standard palette entry. In EHB mode, the entries 32 to 63 are
     * occupied by the darkened copies of the entries 0 to 31, so a write to
     * one of these registers must not touch the palette.
     */
    if (!ehbMode || reg < 32 || reg >= 64) {
        palette[reg] = toTexel(value);
    }

    // Update the mirrored entry in the upper half of the lower palette
    if (reg < 32) {

        if (ehbMode) {
            palette[reg + 32] = toTexel(value.ehb());
        } else if (!denise.isAGA()) {
            palette[reg + 32] = palette[reg];
        }
    }
}

void
PixelEngine::setColor(isize reg, u16 value)
{
    if (denise.isAGA()) {

        /* In AGA, a write with LOCT cleared stores the given nibble in both
         * halves of each component. A program that never performs the second
         * write with LOCT set therefore ends up with the brightest possible
         * shade of the requested color, which is what the hardware does.
         */
        u8 r = u8((value >> 8) & 0xF);
        u8 g = u8((value >> 4) & 0xF);
        u8 b = u8((value >> 0) & 0xF);

        setColor(reg, AmigaColor(u8(r << 4 | r), u8(g << 4 | g), u8(b << 4 | b)));

    } else {

        // OCS and ECS only know 4 bit components, the lower nibble stays clear
        setColor(reg, AmigaColor(u16(value & 0xFFF)));
    }
}

void
PixelEngine::setColorLoct(isize reg, u16 value)
{
    assert(reg < 256);

    // A LOCT write replaces the lower nibbles and preserves the upper ones
    auto c = color[reg];

    setColor(reg, AmigaColor(u8((c.r & 0xF0) | ((value >> 8) & 0xF)),
                             u8((c.g & 0xF0) | ((value >> 4) & 0xF)),
                             u8((c.b & 0xF0) | ((value >> 0) & 0xF))));
}

u16
PixelEngine::getSpriteColor(isize s, isize nr) const
{
    // In AGA, the color bank of the sprite is selected by BPLCON4
    return getColor(denise.sprBase(s) + nr + 2 * (s & 6));
}

Texel
PixelEngine::toTexel(AmigaColor c) const
{
    if (adjIdentity) return TEXEL(HI_HI_LO_LO(0xFF, c.b, c.g, c.r));

    auto clamp = [](i32 v) {
        return u8(v < 0 ? 0 : v > (255 << 16) ? 255 : v >> 16);
    };

    u8 r = clamp(adjLut[adjIdx(0, 0) + c.r] +
                 adjLut[adjIdx(0, 1) + c.g] +
                 adjLut[adjIdx(0, 2) + c.b]);
    u8 g = clamp(adjLut[adjIdx(1, 0) + c.r] +
                 adjLut[adjIdx(1, 1) + c.g] +
                 adjLut[adjIdx(1, 2) + c.b]);
    u8 b = clamp(adjLut[adjIdx(2, 0) + c.r] +
                 adjLut[adjIdx(2, 1) + c.g] +
                 adjLut[adjIdx(2, 2) + c.b]);

    return TEXEL(HI_HI_LO_LO(0xFF, b, g, r));
}

void
PixelEngine::updateRGBA()
{
    // Recompute the adjustment coefficients
    updateAdjLut();

    // Update all cached RGBA values
    for (isize i = 0; i < 256; i++) setColor(i, color[i]);

    // Update the Extra-Half-Brite entries
    updateEhbPalette();
}

void
PixelEngine::updateEhbPalette()
{
    ehbMode = denise.ehb(ehbCon0, ehbCon2);

    for (isize i = 0; i < 32; i++) {

        if (ehbMode) {

            // Extra-Half-Brite: The entry holds a darkened copy
            palette[i + 32] = toTexel(color[i].ehb());

        } else if (denise.isAGA()) {

            // AGA: The entry holds a color register of its own
            palette[i + 32] = toTexel(color[i + 32]);

        } else {

            /* OCS and ECS: The register does not exist. With KILLEHB set, ECS
             * Denise ignores bitplane 6, which makes the entry an alias of the
             * corresponding register in the lower half.
             */
            palette[i + 32] = toTexel(color[i]);
        }
    }
}

void
PixelEngine::updateAdjLut()
{
    auto palette = monitor.getConfig().palette;

    // The RGB palette does not alter anything
    adjIdentity = palette == Palette::RGB;
    if (adjIdentity) return;

    // Normalize adjustment parameters
    double brightness = (monitor.getConfig().brightness - 50.0);
    double contrast = monitor.getConfig().contrast / 100.0;
    double saturation = monitor.getConfig().saturation / 50.0;

    /* Coefficients of the RGB to YUV conversion. The luminance always depends
     * on the input color, whereas the chrominance is replaced by a constant
     * for the monochrome palettes.
     */
    double y[3] = { 0.299 * contrast, 0.587 * contrast, 0.114 * contrast };
    double u[3] = { 0.0, 0.0, 0.0 };
    double v[3] = { 0.0, 0.0, 0.0 };
    double u0 = 0.0, v0 = 0.0;

    switch (palette) {

        case Palette::BLACK_WHITE:

            break;

        case Palette::PAPER_WHITE:

            u0 = -128.0 + 120.0;
            v0 = -128.0 + 133.0;
            break;

        case Palette::GREEN:

            u0 = -128.0 + 29.0;
            v0 = -128.0 + 64.0;
            break;

        case Palette::AMBER:

            u0 = -128.0 + 24.0;
            v0 = -128.0 + 178.0;
            break;

        case Palette::SEPIA:

            u0 = -128.0 + 97.0;
            v0 = -128.0 + 154.0;
            break;

        default:
        {
            assert(palette == Palette::COLOR);

            double s = saturation * contrast;
            u[0] = -0.147 * s; u[1] = -0.289 * s; u[2] =  0.436 * s;
            v[0] =  0.615 * s; v[1] = -0.515 * s; v[2] = -0.100 * s;
            break;
        }
    }

    // Convert YUV back to RGB, which yields the affine transformation
    double m[3][3], off[3];

    for (isize i = 0; i < 3; i++) {

        m[0][i] = y[i]                + 1.140 * v[i];
        m[1][i] = y[i] - 0.396 * u[i] - 0.581 * v[i];
        m[2][i] = y[i] + 2.029 * u[i];
    }
    off[0] = brightness                + 1.140 * v0;
    off[1] = brightness - 0.396 * u0   - 0.581 * v0;
    off[2] = brightness + 2.029 * u0;

    // Tabulate the transformation in 16.16 fixed point format
    for (isize out = 0; out < 3; out++) {

        for (isize in = 0; in < 3; in++) {

            for (isize val = 0; val < 256; val++) {

                // The constant part is folded into the table of the red input
                double t = m[out][in] * val + (in == 0 ? off[out] : 0.0);
                adjLut[adjIdx(out, in) + val] = i32(std::round(t * 65536.0));
            }
        }
    }
}

const Texture &
PixelEngine::getStableBuffer(isize offset) const
{
    auto nr = activeBuffer + offset - 1;
    return emuTexture[(nr + NUM_TEXTURES) % NUM_TEXTURES];
}

Texture &
PixelEngine::getWorkingBuffer()
{
    return emuTexture[activeBuffer];
}

Texel *
PixelEngine::workingPtr(isize row, isize col)
{
    assert(row >= 0 && row <= VPOS_MAX);
    assert(col >= 0 && col <= HPOS_MAX);

    return getWorkingBuffer().pixels.ptr + row * HPIXELS + col;
}

Texel *
PixelEngine::stablePtr(isize row, isize col)
{
    assert(row >= 0 && row <= VPOS_MAX);
    assert(col >= 0 && col <= HPOS_MAX);

    return getStableBuffer().pixels.ptr + row * HPIXELS + col;
}

void
PixelEngine::swapBuffers()
{
    emulator.lockTexture();

    videoPort.buffersWillSwap();

    isize oldActiveBuffer = activeBuffer;
    isize newActiveBuffer = (activeBuffer + 1) % NUM_TEXTURES;

    emuTexture[newActiveBuffer].nr = agnus.pos.frame;
    emuTexture[newActiveBuffer].lof = agnus.pos.lof;
    emuTexture[newActiveBuffer].prevlof = emuTexture[oldActiveBuffer].lof;

    activeBuffer = newActiveBuffer;

    emulator.unlockTexture();
}

void
PixelEngine::vsyncHandler()
{
    dmaDebugger.vSyncHandler();
}

void
PixelEngine::eofHandler()
{
    dmaDebugger.eofHandler();
}

void
PixelEngine::replayColRegChanges()
{
    // Apply all color register changes that happened in this line
    for (isize i = 0, end = colChanges.end(); i < end; i++) {
        applyRegisterChange(colChanges.elements[i]);
    }
    colChanges.clear();
}

void
PixelEngine::applyRegisterChange(const RegChange &change)
{
    switch (change.reg) {

        case Reg(0):
            
            break;

        case Reg::BPLCON0:

            hamMode = Denise::ham(change.value);
            hamMode8 = denise.ham8(change.value);
            shresMode = Denise::shres(change.value);
            ehbCon0 = change.value;
            updateEhbPalette();
            break;

        case Reg::BPLCON2:

            ehbCon2 = change.value;
            updateEhbPalette();
            break;
            
        default: // It must be a color register then
        {
            /* Register numbers beyond the 256 color registers denote AGA
             * writes with the LOCT bit set (see Denise::recordColorChange).
             */
            auto nr = isize(change.reg) - isize(Reg::COLOR00);
            assert(0 <= nr && nr < 512);

            /* Both cases are applied unconditionally. Comparing against the
             * current register value would be wrong, because a write with
             * LOCT cleared also resets the lower nibbles, even if the upper
             * ones are left at their previous value.
             */
            if (nr >= 256) {
                setColorLoct(nr - 256, change.value);
            } else {
                setColor(nr, change.value);
            }
            break;
        }
    }
}

void
PixelEngine::colorize(isize line)
{
    // Jump to the first pixel in the specified line in the active frame buffer
    auto *dst = workingPtr(line);
    Pixel pixel = 0;

    // Initialize the HAM mode hold register with the current background color
    AmigaColor hold = color[0];

    // Add a dummy register change to ensure we draw until the line end
    colChanges.insert(HPIXELS, RegChange { .reg = Reg(0), .value = 0 } );

    // Iterate over all recorded register changes
    for (isize i = 0, end = colChanges.end(); i < end; i++) {

        Pixel trigger = (Pixel)colChanges.keys[i];
        RegChange &change = colChanges.elements[i];

        // Colorize a chunk of pixels
        if (shresMode) {
            colorizeSHRES(dst, pixel, trigger);
        } else if (hamMode) {
            colorizeHAM(dst, pixel, trigger, hold);
        } else {
            colorize(dst, pixel, trigger);
        }
        pixel = trigger;

        // Perform the register change
        applyRegisterChange(change);
    }

    // Clear the history cache
    colChanges.clear();

    // Wipe out the HBLANK area
    auto start = agnus.pos.pixel(HBLANK_MIN);
    auto stop  = agnus.pos.pixel(HBLANK_MAX);
    for (pixel = start; pixel <= stop; pixel++) dst[pixel] = Texture::hblank;
}

void
PixelEngine::colorize(Texel *dst, Pixel from, Pixel to)
{
    auto *mbuf = denise.mBuffer;
    auto *bbuf = denise.bBuffer;

    /* With BRDSPRT enabled, a sprite pixel takes precedence over the border
     * color. The case is checked outside the loop to keep the common path,
     * where the border always wins, as tight as it was before.
     */
    if (denise.brdsprt()) {

        auto *zbuf = denise.zBuffer;

        for (Pixel i = from; i < to; i++) {

            bool spr = bbuf[i] != 0xFFFF && Denise::isSpritePixel(zbuf[i]);
            dst[i] = palette[bbuf[i] == 0xFFFF || spr ? mbuf[i] : bbuf[i]];
        }
        return;
    }

    for (Pixel i = from; i < to; i++) {
        dst[i] = palette[bbuf[i] == 0xFFFF ? mbuf[i] : bbuf[i]];
    }
}

void
PixelEngine::colorizeSHRES(Texel *dst, Pixel from, Pixel to)
{
    auto *mbuf = denise.mBuffer;
    auto *bbuf = denise.bBuffer;
    auto *zbuf = denise.zBuffer;

    // A sprite may shine through the border in AGA (see BRDSPRT)
    bool brdsprt = denise.brdsprt();

    if constexpr (sizeof(Texel) == 4) {

        /* Output two super-hires pixels as a single texel. Only bitplane
         * pixels carry two packed color indices, one in bit 3 and 2 and one in
         * bit 1 and 0 (see Denise::drawOdd and Denise::drawEven). Border and
         * sprite pixels are stored as ordinary indices and must not be
         * unpacked. Since the texture provides just one texel per hires pixel,
         * the two colors are averaged, which is the correct box filter for
         * halving the horizontal resolution.
         */
        for (Pixel i = from; i < to; i++) {

            if (bbuf[i] != 0xFFFF && !(brdsprt && Denise::isSpritePixel(zbuf[i]))) {

                dst[i] = palette[bbuf[i]];

            } else if (Denise::isSpritePixel(zbuf[i])) {

                dst[i] = palette[mbuf[i]];

            } else {

                auto t0 = u32(palette[(mbuf[i] >> 2) & 3]);
                auto t1 = u32(palette[mbuf[i] & 3]);
                dst[i] = Texel((t0 & t1) + (((t0 ^ t1) >> 1) & 0x7F7F7F7F));
            }
        }

    } else {

        // Output each super-hires pixel as a seperate texel
        for (Pixel i = from; i < to; i++) {

            u32 *p = (u32 *)(dst + i);

            if (bbuf[i] != 0xFFFF && !(brdsprt && Denise::isSpritePixel(zbuf[i]))) {

                p[0] =
                p[1] = u32(palette[bbuf[i]]);

            } else if (Denise::isSpritePixel(zbuf[i])) {

                p[0] =
                p[1] = u32(palette[mbuf[i]]);

            } else {

                p[0] = u32(palette[mbuf[i] >> 2]);
                p[1] = u32(palette[mbuf[i] & 3]);
            }
        }
    }
}

void
PixelEngine::colorizeHAM(Texel *dst, Pixel from, Pixel to, AmigaColor& ham)
{
    auto *dbuf = denise.dBuffer;
    auto *ibuf = denise.iBuffer;
    auto *mbuf = denise.mBuffer;
    auto *bbuf = denise.bBuffer;

    bool brdsprt = denise.brdsprt();

    for (Pixel i = from; i < to; i++) {

        // Check for border pixels
        if (bbuf[i] != 0xFFFF) {

            // A sprite may shine through the border in AGA (see BRDSPRT)
            if (brdsprt && Denise::isSpritePixel(denise.zBuffer[i])) {

                dst[i] = palette[mbuf[i]];
                continue;
            }

            dst[i] = palette[bbuf[i]];

            /* Track the border color in the hold register. The buffer may also
             * carry one of the artificial palette entries beyond the color
             * registers, such as the pure black of the BRDRBLNK bit, which
             * has no counterpart in the register file.
             */
            if (bbuf[i] < 256) ham = color[bbuf[i]];
            continue;
        }

        /* The two data paths of HAM mode have different origins. The control
         * bits are taken from the raw bitplane value, because Denise taps them
         * off the bitplane serializer, ahead of the color index logic. The
         * value that is written into the hold register, on the other hand, is
         * the output of that logic, which is why it is read from the iBuffer.
         *
         * The distinction only matters if HAM is combined with dual playfield
         * mode, where the iBuffer holds a playfield color index instead of the
         * bitplane bits. The vAmigaTS test dualpf1 enables both modes and
         * confirms the split: the picture reacts to the playfield priorities in
         * BPLCON2, so the data path runs through the playfield logic, and it
         * still shows HAM modifications, so the control bits do not.
         */
        u8 pw = dbuf[i];
        u8 pv = ibuf[i];

        if (hamMode8) {

            /* HAM8 uses the lowest two bitplanes as control bits and the
             * remaining six as data. Because a component holds eight bits,
             * the two least significant ones are left untouched.
             */
            u8 val = pv & 0xFC;
            assert(isPaletteIndex(pv >> 2));

            switch (pw & 0b11) {

                case 0b00: ham = color[pv >> 2]; break;
                case 0b01: ham.b = u8((ham.b & 0x03) | val); break;
                case 0b10: ham.r = u8((ham.r & 0x03) | val); break;
                case 0b11: ham.g = u8((ham.g & 0x03) | val); break;
            }

            if (denise.spritePixelIsVisible(i)) {
                dst[i] = palette[mbuf[i]];
            } else {
                dst[i] = toTexel(ham);
            }
            continue;
        }

        /* HAM6 supplies four data bits. AGA replicates them into both nibbles
         * of the component, OCS and ECS only know the upper nibble.
         */
        u8 index = pv & 0xF;
        u8 val = denise.isAGA() ? u8(index << 4 | index) : u8(index << 4);
        assert(isPaletteIndex(index));

        switch ((pw >> 4) & 0b11) {

            case 0b00: // Get color from register

                ham = color[index];
                break;

            case 0b01: // Modify blue

                ham.b = val;
                break;

            case 0b10: // Modify red

                ham.r = val;
                break;

            case 0b11: // Modify green

                ham.g = val;
                break;

            default:
                fatalError;
        }

        // Synthesize pixel
        if (denise.spritePixelIsVisible(i)) {
            dst[i] = palette[mbuf[i]];
        } else {
            dst[i] = toTexel(ham);
        }
    }
}

void
PixelEngine::hide(isize line, u16 layers, u8 alpha)
{
    auto *p = workingPtr(line);

    for (Pixel i = 0; i < HPIXELS; i++) {

        u16 z = denise.zBuffer[i];

        // Check for case 1: A sprite is visible
        if (Denise::isSpritePixel(z)) {

            if (Denise::isSpritePixel<0>(z) && !(layers & 0x01)) continue;
            if (Denise::isSpritePixel<1>(z) && !(layers & 0x02)) continue;
            if (Denise::isSpritePixel<2>(z) && !(layers & 0x04)) continue;
            if (Denise::isSpritePixel<3>(z) && !(layers & 0x08)) continue;
            if (Denise::isSpritePixel<4>(z) && !(layers & 0x10)) continue;
            if (Denise::isSpritePixel<5>(z) && !(layers & 0x20)) continue;
            if (Denise::isSpritePixel<6>(z) && !(layers & 0x40)) continue;
            if (Denise::isSpritePixel<7>(z) && !(layers & 0x80)) continue;

        } else {

            // Check for case 2: Playfield 1 is visible
            if ((Denise::upperPlayfield(z) == 1) && !(layers & 0x100)) continue;

            // Check for case 3: layfield 2 is visible
            if ((Denise::upperPlayfield(z) == 2) && !(layers & 0x200)) continue;
        }
        
        u8 r = p[i] & 0xFF;
        u8 g = (p[i] >> 8) & 0xFF;
        u8 b = (p[i] >> 16) & 0xFF;

        double scale = alpha / 255.0;
        u8 bg = (line / 4) % 2 == (i / 8) % 2 ? 0x22 : 0x44;
        u8 newr = (u8)(r * (1 - scale) + bg * scale);
        u8 newg = (u8)(g * (1 - scale) + bg * scale);
        u8 newb = (u8)(b * (1 - scale) + bg * scale);
        
        p[i] = 0xFF000000 | newb << 16 | newg << 8 | newr;
    }
}

}
