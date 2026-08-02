// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#pragma once

#include "SubComponent.h"
#include "ChangeRecorder.h"
#include "Constants.h"
#include "Texture.h"

namespace vamiga {

class PixelEngine final : public SubComponent {

    Descriptions descriptions = {{

        .type           = Class::PixelEngine,
        .name           = "PixelEngine",
        .description    = "Amiga Monitor",
        .shell          = "monitor"
    }};

    Options options = {

        /*
        Opt::MON_PALETTE,
        Opt::MON_BRIGHTNESS,
        Opt::MON_CONTRAST,
        Opt::MON_SATURATION
        */
    };

    friend class Denise;


    //
    // Screen buffers
    //

private:

    static constexpr isize NUM_TEXTURES = 8;
    
    /* The emulator manages textures in a ring buffer to allow access to older
     * frames ("run-behind" feature). At any time, one texture serves as the
     * working buffer, where all drawing functions write, while the other
     * textures are considered stable. Once a frame is completed, the next
     * texture in the ring becomes the new working buffer.
     */
    Texture emuTexture[NUM_TEXTURES];

    // The currently active buffer
    isize activeBuffer = 0;

    // Mutex for synchronizing access to the stable buffer
    util::Mutex bufferMutex;

    
    //
    // Color management
    //

private:

    /* Coefficients of the color adjustment performed by the monitor settings.
     * The adjustment is an affine transformation of the RGB components, which
     * allows it to be tabulated: the contribution of each input component to
     * each output component is looked up and summed (see toTexel). A lookup
     * table over all colors is not an option, because AGA colors span the full
     * 24 bit range.
     *
     * The table is indexed by adjIdx(out, in) + value, where out selects the
     * output component and in the input component. Values are stored in 16.16
     * fixed point format, and the constant part of the transformation is
     * folded into the tables of the red input component.
     */
    i32 adjLut[9 * 256];

    static constexpr isize adjIdx(isize out, isize in) { return (out * 3 + in) * 256; }

    /* Set if the color adjustment leaves all colors untouched. This is the
     * case for the default RGB palette, which is checked for separately to
     * keep the common case as fast as possible.
     */
    bool adjIdentity;

    // Color register colors
    AmigaColor color[256];

    /* Active color palette
     *
     *  0 ..255 : ABGR values of the color registers (32 for OCS/ECS, 256 for AGA)
     * 256 ..287 : Halfbright OCS/ECS palette entries (unused on AGA)
     *      288 : Pure black (used if the ECS BRDRBLNK bit is set)
     * 289 ..291 : Additional debug colors
     */
    static const int paletteCnt = 256 + 32 + 1 + 3;
    Texel palette[paletteCnt];
    
    // Indicates whether HAM mode or SHRES mode is enabled
    bool hamMode;
    bool shresMode;

    /* Indicates whether the AGA variant of HAM mode is enabled. HAM8 takes its
     * control bits from the two lowest bitplanes instead of the two highest
     * ones, and it modifies six bits of a component instead of four.
     */
    bool hamMode8;

    
    //
    // Register change history buffer
    //

public:

    /* Color register history. The capacity has to accommodate every color
     * write that fits into a single rasterline. In AGA, a program supplies the
     * full 8 bit range by writing each register twice (once with LOCT cleared
     * and once with LOCT set), and both writes are recorded here. Overflowing
     * this buffer is not a benign event: the ring buffer would wrap around and
     * report itself as empty, discarding the whole line.
     */
    RegChangeRecorder<1024> colChanges;


    //
    // Initializing
    //
    
public:
    
    using SubComponent::SubComponent;

    // Initializes both frame buffers with a checkerboard pattern
    void clearAll();

    PixelEngine& operator= (const PixelEngine& other) {

        CLONE_ARRAY(adjLut)
        CLONE(adjIdentity)
        CLONE(colChanges)
        CLONE_ARRAY(color)
        CLONE(hamMode)
        CLONE(shresMode)
        CLONE(hamMode8)
        CLONE_ARRAY(palette)

        return *this;
    }


    //
    // Methods from Serializable
    //

private:

    template <class T>
    void serialize(T& worker)
    {
        worker

        << colChanges
        << color
        << hamMode
        << shresMode
        << hamMode8;

    } SERIALIZERS(serialize);


    //
    // Methods from CoreComponent
    //
    
public:

    const Descriptions &getDescriptions() const override { return descriptions; }

private:

    void _dump(Category category, std::ostream &os) const override;
    void _initialize() override;
    void _powerOn() override;
    void _didLoad() override;
    void _didReset(bool hard) override;

    
    //
    // Methods from Configurable
    //

public:
    
    const Options &getOptions() const override { return options; }


    //
    // Accessing color registers
    //

public:

    // Performs a consistency check for debugging
    static bool isPaletteIndex(isize nr) { return nr < paletteCnt; }
    
    /* Changes one of the color registers. The u16 variant performs a regular
     * register write, which sets the upper nibble of each component. The Loct
     * variant performs an AGA write with the LOCT bit set, which replaces the
     * lower nibbles and leaves the upper ones alone.
     */
    void setColor(isize reg, u16 value);
    void setColorLoct(isize reg, u16 value);
    void setColor(isize reg, AmigaColor value);

    // Returns a color value in Amiga format
    u16 getColor(isize nr) const { return color[nr].rawValue(); }

    // Returns a color register with the full AGA precision
    AmigaColor getAmigaColor(isize nr) const { return color[nr]; }

    // Returns sprite color in Amiga format
    u16 getSpriteColor(isize s, isize nr) const;


    //
    // Using the color lookup table
    //

public:

    // Recomputes the color adjustment tables and all cached RGBA values
    void updateRGBA();

    // Converts an Amiga color into a texel, applying the monitor settings
    Texel toTexel(AmigaColor c) const;

private:

    // Recomputes the color adjustment tables from the monitor settings
    void updateAdjLut();


    //
    // Working with frame buffers
    //

public:

    // Returns the working buffer or the stable buffer
    Texture &getWorkingBuffer();
    const Texture &getStableBuffer(isize offset = 0) const;

    // Return a pointer into the pixel storage
    Texel *workingPtr(isize row = 0, isize col = 0);
    Texel *stablePtr(isize row = 0, isize col = 0);
    
    // Swaps the working buffer and the stable buffer
    void swapBuffers();
    
    // Called after each frame to switch the frame buffers
    void vsyncHandler();

    // Called at the end of each frame
    void eofHandler();

    //
    // Working with recorded register changes
    //

public:

    // Applies all recorded color register changes
    void replayColRegChanges();

    // Applies a single register change
    void applyRegisterChange(const RegChange &change);


    //
    // Synthesizing pixels
    //

public:
    
    /* Colorizes a rasterline. This function implements the last stage in the
     * graphics pipelile. It translates a line of color register indices into a
     * line of RGBA values in GPU format.
     */
    void colorize(isize line);
    
private:
    
    void colorize(Texel *dst, Pixel from, Pixel to);
    void colorizeSHRES(Texel *dst, Pixel from, Pixel to);
    void colorizeHAM(Texel *dst, Pixel from, Pixel to, AmigaColor& ham);
    
    /* Hides some graphics layers. This function is an optional stage applied
     * after colorize(). It can be used to hide some layers for debugging.
     */
    
public:
    
    void hide(isize line, u16 layer, u8 alpha);
};

}
