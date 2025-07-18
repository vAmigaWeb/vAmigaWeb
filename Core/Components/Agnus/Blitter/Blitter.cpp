// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the Mozilla Public License v2
//
// See https://mozilla.org/MPL/2.0 for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Blitter.h"
#include "Amiga.h"
#include "Checksum.h"
#include "IOUtils.h"
#include "Thread.h"

namespace vamiga {

Blitter::Blitter(Amiga& ref) : SubComponent(ref)
{

}

void
Blitter::_initialize()
{    
    // Initialize the fill pattern tables
    for (isize carryIn = 0; carryIn < 2; carryIn++) {

        for (isize byte = 0; byte < 256; byte++) {

            u8 carry = (u8)carryIn;
            u8 inclPattern = (u8)byte;
            u8 exclPattern = (u8)byte;

            for (isize bit = 0; bit < 8; bit++) {

                inclPattern |= carry << bit; // inclusive fill
                exclPattern ^= carry << bit; // exclusive fill

                if (byte & (1 << bit)) carry = !carry;
            }
            fillPattern[0][carryIn][byte] = inclPattern;
            fillPattern[1][carryIn][byte] = exclPattern;
            nextCarryIn[carryIn][byte] = carry;
        }
    }

    initFastBlitter();
    initSlowBlitter();
}

void
Blitter::_didReset(bool hard)
{
    if (hard) {
        
        blitcount = 1;
        copycount = 0;
        linecount = 0;
    }
}

void
Blitter::_run()
{
    if (BLT_MEM_GUARD) {

        memguard.resize(mem.getConfig().chipSize);
        memguard.clear();
    }
}

i64
Blitter::getOption(Opt option) const
{
    switch (option) {
            
        case Opt::BLITTER_ACCURACY: return config.accuracy;

        default:
            fatalError;
    }
}

void
Blitter::checkOption(Opt opt, i64 value)
{
    switch (opt) {

        case Opt::BLITTER_ACCURACY:

            if (value < 0 || value > 2) {
                throw AppError(Fault::OPT_INV_ARG, "0, 1, 2");
            }
            return;

        default:
            throw(Fault::OPT_UNSUPPORTED);
    }
}

void
Blitter::setOption(Opt option, i64 value)
{
    switch (option) {
            
        case Opt::BLITTER_ACCURACY:

            config.accuracy = (isize)value;
            return;

        default:
            fatalError;
    }
}

u16
Blitter::barrelShifter(u16 anew, u16 aold, u16 shift, bool desc) const
{
    if (desc) {
        return (u16)(HI_W_LO_W(anew, aold) >> (16 - shift));
    } else {
        return (u16)(HI_W_LO_W(aold, anew) >> shift);
    }
}

u16
Blitter::doMintermLogic(u16 a, u16 b, u16 c, u8 minterm) const
{
    u16 result = doMintermLogicQuick(a, b, c, minterm);

    if (BLT_DEBUG) {
        
        u16 result2 = 0;
        
        if (minterm & 0b10000000) result2 |=  a &  b &  c;
        if (minterm & 0b01000000) result2 |=  a &  b & ~c;
        if (minterm & 0b00100000) result2 |=  a & ~b &  c;
        if (minterm & 0b00010000) result2 |=  a & ~b & ~c;
        if (minterm & 0b00001000) result2 |= ~a &  b &  c;
        if (minterm & 0b00000100) result2 |= ~a &  b & ~c;
        if (minterm & 0b00000010) result2 |= ~a & ~b &  c;
        if (minterm & 0b00000001) result2 |= ~a & ~b & ~c;

        if (result != result2) fatal("Blitter minterm error\n");
    }
    
    return result;
}

u16
Blitter::doMintermLogicQuick(u16 a, u16 b, u16 c, u8 minterm) const
{
    switch (minterm) {
        case 0: return 0;
        case 1: return (~c & ~b & ~a);
        case 2: return (c & ~b & ~a);
        case 3: return (~b & ~a);
        case 4: return (~c & b & ~a);
        case 5: return (~c & ~a);
        case 6: return (c & ~b & ~a) | (~c & b & ~a);
        case 7: return (~b & ~a) | (~c & ~a);
        case 8: return (c & b & ~a);
        case 9: return (~c & ~b & ~a) | (c & b & ~a);
        case 10: return (c & ~a);
        case 11: return (~b & ~a) | (c & ~a);
        case 12: return (b & ~a);
        case 13: return (~c & ~a) | (b & ~a);
        case 14: return (c & ~a) | (b & ~a);
        case 15: return (~a);
        case 16: return (~c & ~b & a);
        case 17: return (~c & ~b);
        case 18: return (c & ~b & ~a) | (~c & ~b & a);
        case 19: return (~b & ~a) | (~c & ~b);
        case 20: return (~c & b & ~a) | (~c & ~b & a);
        case 21: return (~c & ~a) | (~c & ~b);
        case 22: return (c & ~b & ~a) | (~c & b & ~a) | (~c & ~b & a);
        case 23: return (~b & ~a) | (~c & ~a) | (~c & ~b);
        case 24: return (c & b & ~a) | (~c & ~b & a);
        case 25: return (~c & ~b) | (c & b & ~a);
        case 26: return (c & ~a) | (~c & ~b & a);
        case 27: return (~b & ~a) | (c & ~a) | (~c & ~b);
        case 28: return (b & ~a) | (~c & ~b & a);
        case 29: return (~c & ~a) | (b & ~a) | (~c & ~b);
        case 30: return (c & ~a) | (b & ~a) | (~c & ~b & a);
        case 31: return (~a) | (~c & ~b);
        case 32: return (c & ~b & a);
        case 33: return (~c & ~b & ~a) | (c & ~b & a);
        case 34: return (c & ~b);
        case 35: return (~b & ~a) | (c & ~b);
        case 36: return (~c & b & ~a) | (c & ~b & a);
        case 37: return (~c & ~a) | (c & ~b & a);
        case 38: return (c & ~b) | (~c & b & ~a);
        case 39: return (~b & ~a) | (~c & ~a) | (c & ~b);
        case 40: return (c & b & ~a) | (c & ~b & a);
        case 41: return (~c & ~b & ~a) | (c & b & ~a) | (c & ~b & a);
        case 42: return (c & ~a) | (c & ~b);
        case 43: return (~b & ~a) | (c & ~a) | (c & ~b);
        case 44: return (b & ~a) | (c & ~b & a);
        case 45: return (~c & ~a) | (b & ~a) | (c & ~b & a);
        case 46: return (c & ~a) | (b & ~a) | (c & ~b);
        case 47: return (~a) | (c & ~b);
        case 48: return (~b & a);
        case 49: return (~c & ~b) | (~b & a);
        case 50: return (c & ~b) | (~b & a);
        case 51: return (~b);
        case 52: return (~c & b & ~a) | (~b & a);
        case 53: return (~c & ~a) | (~b & a);
        case 54: return (c & ~b) | (~c & b & ~a) | (~b & a);
        case 55: return (~b) | (~c & ~a);
        case 56: return (c & b & ~a) | (~b & a);
        case 57: return (~c & ~b) | (c & b & ~a) | (~b & a);
        case 58: return (c & ~a) | (~b & a);
        case 59: return (~b) | (c & ~a);
        case 60: return (b & ~a) | (~b & a);
        case 61: return (~c & ~a) | (b & ~a) | (~b & a);
        case 62: return (c & ~a) | (b & ~a) | (~b & a);
        case 63: return (~a) | (~b);
        case 64: return (~c & b & a);
        case 65: return (~c & ~b & ~a) | (~c & b & a);
        case 66: return (c & ~b & ~a) | (~c & b & a);
        case 67: return (~b & ~a) | (~c & b & a);
        case 68: return (~c & b);
        case 69: return (~c & ~a) | (~c & b);
        case 70: return (c & ~b & ~a) | (~c & b);
        case 71: return (~b & ~a) | (~c & ~a) | (~c & b);
        case 72: return (c & b & ~a) | (~c & b & a);
        case 73: return (~c & ~b & ~a) | (c & b & ~a) | (~c & b & a);
        case 74: return (c & ~a) | (~c & b & a);
        case 75: return (~b & ~a) | (c & ~a) | (~c & b & a);
        case 76: return (b & ~a) | (~c & b);
        case 77: return (~c & ~a) | (b & ~a) | (~c & b);
        case 78: return (c & ~a) | (b & ~a) | (~c & b);
        case 79: return (~a) | (~c & b);
        case 80: return (~c & a);
        case 81: return (~c & ~b) | (~c & a);
        case 82: return (c & ~b & ~a) | (~c & a);
        case 83: return (~b & ~a) | (~c & a);
        case 84: return (~c & b) | (~c & a);
        case 85: return (~c);
        case 86: return (c & ~b & ~a) | (~c & b) | (~c & a);
        case 87: return (~b & ~a) | (~c);
        case 88: return (c & b & ~a) | (~c & a);
        case 89: return (~c & ~b) | (c & b & ~a) | (~c & a);
        case 90: return (c & ~a) | (~c & a);
        case 91: return (~b & ~a) | (c & ~a) | (~c & a);
        case 92: return (b & ~a) | (~c & a);
        case 93: return (~c) | (b & ~a);
        case 94: return (c & ~a) | (b & ~a) | (~c & a);
        case 95: return (~a) | (~c);
        case 96: return (c & ~b & a) | (~c & b & a);
        case 97: return (~c & ~b & ~a) | (c & ~b & a) | (~c & b & a);
        case 98: return (c & ~b) | (~c & b & a);
        case 99: return (~b & ~a) | (c & ~b) | (~c & b & a);
        case 100: return (~c & b) | (c & ~b & a);
        case 101: return (~c & ~a) | (c & ~b & a) | (~c & b);
        case 102: return (c & ~b) | (~c & b);
        case 103: return (~b & ~a) | (~c & ~a) | (c & ~b) | (~c & b);
        case 104: return (c & b & ~a) | (c & ~b & a) | (~c & b & a);
        case 105: return (~c & ~b & ~a) | (c & b & ~a) | (c & ~b & a) | (~c & b & a);
        case 106: return (c & ~a) | (c & ~b) | (~c & b & a);
        case 107: return (~b & ~a) | (c & ~a) | (c & ~b) | (~c & b & a);
        case 108: return (b & ~a) | (c & ~b & a) | (~c & b);
        case 109: return (~c & ~a) | (b & ~a) | (c & ~b & a) | (~c & b);
        case 110: return (c & ~a) | (b & ~a) | (c & ~b) | (~c & b);
        case 111: return (~a) | (c & ~b) | (~c & b);
        case 112: return (~b & a) | (~c & a);
        case 113: return (~c & ~b) | (~b & a) | (~c & a);
        case 114: return (c & ~b) | (~b & a) | (~c & a);
        case 115: return (~b) | (~c & a);
        case 116: return (~c & b) | (~b & a);
        case 117: return (~c) | (~b & a);
        case 118: return (c & ~b) | (~c & b) | (~b & a);
        case 119: return (~b) | (~c);
        case 120: return (c & b & ~a) | (~b & a) | (~c & a);
        case 121: return (~c & ~b) | (c & b & ~a) | (~b & a) | (~c & a);
        case 122: return (c & ~a) | (~b & a) | (~c & a);
        case 123: return (~b) | (c & ~a) | (~c & a);
        case 124: return (b & ~a) | (~b & a) | (~c & a);
        case 125: return (~c) | (b & ~a) | (~b & a);
        case 126: return (c & ~a) | (b & ~a) | (~b & a) | (~c & a);
        case 127: return (~a) | (~b) | (~c);
        case 128: return (c & b & a);
        case 129: return (~c & ~b & ~a) | (c & b & a);
        case 130: return (c & ~b & ~a) | (c & b & a);
        case 131: return (~b & ~a) | (c & b & a);
        case 132: return (~c & b & ~a) | (c & b & a);
        case 133: return (~c & ~a) | (c & b & a);
        case 134: return (c & ~b & ~a) | (~c & b & ~a) | (c & b & a);
        case 135: return (~b & ~a) | (~c & ~a) | (c & b & a);
        case 136: return (c & b);
        case 137: return (~c & ~b & ~a) | (c & b);
        case 138: return (c & ~a) | (c & b);
        case 139: return (~b & ~a) | (c & ~a) | (c & b);
        case 140: return (b & ~a) | (c & b);
        case 141: return (~c & ~a) | (b & ~a) | (c & b);
        case 142: return (c & ~a) | (b & ~a) | (c & b);
        case 143: return (~a) | (c & b);
        case 144: return (~c & ~b & a) | (c & b & a);
        case 145: return (~c & ~b) | (c & b & a);
        case 146: return (c & ~b & ~a) | (~c & ~b & a) | (c & b & a);
        case 147: return (~b & ~a) | (~c & ~b) | (c & b & a);
        case 148: return (~c & b & ~a) | (~c & ~b & a) | (c & b & a);
        case 149: return (~c & ~a) | (~c & ~b) | (c & b & a);
        case 150: return (c & ~b & ~a) | (~c & b & ~a) | (~c & ~b & a) | (c & b & a);
        case 151: return (~b & ~a) | (~c & ~a) | (~c & ~b) | (c & b & a);
        case 152: return (c & b) | (~c & ~b & a);
        case 153: return (~c & ~b) | (c & b);
        case 154: return (c & ~a) | (~c & ~b & a) | (c & b);
        case 155: return (~b & ~a) | (c & ~a) | (~c & ~b) | (c & b);
        case 156: return (b & ~a) | (~c & ~b & a) | (c & b);
        case 157: return (~c & ~a) | (b & ~a) | (~c & ~b) | (c & b);
        case 158: return (c & ~a) | (b & ~a) | (~c & ~b & a) | (c & b);
        case 159: return (~a) | (~c & ~b) | (c & b);
        case 160: return (c & a);
        case 161: return (~c & ~b & ~a) | (c & a);
        case 162: return (c & ~b) | (c & a);
        case 163: return (~b & ~a) | (c & a);
        case 164: return (~c & b & ~a) | (c & a);
        case 165: return (~c & ~a) | (c & a);
        case 166: return (c & ~b) | (~c & b & ~a) | (c & a);
        case 167: return (~b & ~a) | (~c & ~a) | (c & a);
        case 168: return (c & b) | (c & a);
        case 169: return (~c & ~b & ~a) | (c & b) | (c & a);
        case 170: return (c);
        case 171: return (~b & ~a) | (c);
        case 172: return (b & ~a) | (c & a);
        case 173: return (~c & ~a) | (b & ~a) | (c & a);
        case 174: return (c) | (b & ~a);
        case 175: return (~a) | (c);
        case 176: return (~b & a) | (c & a);
        case 177: return (~c & ~b) | (~b & a) | (c & a);
        case 178: return (c & ~b) | (~b & a) | (c & a);
        case 179: return (~b) | (c & a);
        case 180: return (~c & b & ~a) | (~b & a) | (c & a);
        case 181: return (~c & ~a) | (~b & a) | (c & a);
        case 182: return (c & ~b) | (~c & b & ~a) | (~b & a) | (c & a);
        case 183: return (~b) | (~c & ~a) | (c & a);
        case 184: return (c & b) | (~b & a);
        case 185: return (~c & ~b) | (c & b) | (~b & a);
        case 186: return (c) | (~b & a);
        case 187: return (~b) | (c);
        case 188: return (b & ~a) | (~b & a) | (c & a);
        case 189: return (~c & ~a) | (b & ~a) | (~b & a) | (c & a);
        case 190: return (c) | (b & ~a) | (~b & a);
        case 191: return (~a) | (~b) | (c);
        case 192: return (b & a);
        case 193: return (~c & ~b & ~a) | (b & a);
        case 194: return (c & ~b & ~a) | (b & a);
        case 195: return (~b & ~a) | (b & a);
        case 196: return (~c & b) | (b & a);
        case 197: return (~c & ~a) | (b & a);
        case 198: return (c & ~b & ~a) | (~c & b) | (b & a);
        case 199: return (~b & ~a) | (~c & ~a) | (b & a);
        case 200: return (c & b) | (b & a);
        case 201: return (~c & ~b & ~a) | (c & b) | (b & a);
        case 202: return (c & ~a) | (b & a);
        case 203: return (~b & ~a) | (c & ~a) | (b & a);
        case 204: return (b);
        case 205: return (~c & ~a) | (b);
        case 206: return (c & ~a) | (b);
        case 207: return (~a) | (b);
        case 208: return (~c & a) | (b & a);
        case 209: return (~c & ~b) | (b & a);
        case 210: return (c & ~b & ~a) | (~c & a) | (b & a);
        case 211: return (~b & ~a) | (~c & a) | (b & a);
        case 212: return (~c & b) | (~c & a) | (b & a);
        case 213: return (~c) | (b & a);
        case 214: return (c & ~b & ~a) | (~c & b) | (~c & a) | (b & a);
        case 215: return (~b & ~a) | (~c) | (b & a);
        case 216: return (c & b) | (~c & a);
        case 217: return (~c & ~b) | (c & b) | (b & a);
        case 218: return (c & ~a) | (~c & a) | (b & a);
        case 219: return (~b & ~a) | (c & ~a) | (~c & a) | (b & a);
        case 220: return (b) | (~c & a);
        case 221: return (~c) | (b);
        case 222: return (c & ~a) | (b) | (~c & a);
        case 223: return (~a) | (~c) | (b);
        case 224: return (c & a) | (b & a);
        case 225: return (~c & ~b & ~a) | (c & a) | (b & a);
        case 226: return (c & ~b) | (b & a);
        case 227: return (~b & ~a) | (c & a) | (b & a);
        case 228: return (~c & b) | (c & a);
        case 229: return (~c & ~a) | (c & a) | (b & a);
        case 230: return (c & ~b) | (~c & b) | (b & a);
        case 231: return (~b & ~a) | (~c & ~a) | (c & a) | (b & a);
        case 232: return (c & b) | (c & a) | (b & a);
        case 233: return (~c & ~b & ~a) | (c & b) | (c & a) | (b & a);
        case 234: return (c) | (b & a);
        case 235: return (~b & ~a) | (c) | (b & a);
        case 236: return (b) | (c & a);
        case 237: return (~c & ~a) | (b) | (c & a);
        case 238: return (c) | (b);
        case 239: return (~a) | (c) | (b);
        case 240: return (a);
        case 241: return (~c & ~b) | (a);
        case 242: return (c & ~b) | (a);
        case 243: return (~b) | (a);
        case 244: return (~c & b) | (a);
        case 245: return (~c) | (a);
        case 246: return (c & ~b) | (~c & b) | (a);
        case 247: return (~b) | (~c) | (a);
        case 248: return (c & b) | (a);
        case 249: return (~c & ~b) | (c & b) | (a);
        case 250: return (c) | (a);
        case 251: return (~b) | (c) | (a);
        case 252: return (b) | (a);
        case 253: return (~c) | (b) | (a);
        case 254: return (c) | (b) | (a);
        default:  return 0xFFFF;
    }
}

void
Blitter::doFill(u16 &data, bool &carry) const
{
    assert(carry == 0 || carry == 1);

    trace(BLT_DEBUG, "data = %X carry = %X\n", data, carry);
    
    u8 dataHi = HI_BYTE(data);
    u8 dataLo = LO_BYTE(data);
    u8 exclusive = !!bltconEFE();
    
    // Remember: A fill operation is carried out from right to left
    u8 resultLo = fillPattern[exclusive][carry][dataLo];
    carry = nextCarryIn[carry][dataLo];
    u8 resultHi = fillPattern[exclusive][carry][dataHi];
    carry = nextCarryIn[carry][dataHi];
    
    data = HI_LO(resultHi, resultLo);
}

void
Blitter::doLine()
{
    auto incx = [&]() { if (incASH()) U32_INC(bltcpt, 2); };
    auto decx = [&]() { if (decASH()) U32_INC(bltcpt, -2); };
    auto incy = [&]() { U32_INC(bltcpt, bltcmod); fillCarry = true; };
    auto decy = [&]() { U32_INC(bltcpt, -bltcmod); fillCarry = true; };

    bool sign = bltcon1 & BLTCON1_SIGN;
    fillCarry = false;

    if (bltcon1 & BLTCON1_SUD) {
        
        if (bltcon1 & BLTCON1_AUL) {
            decx();
        } else {
            incx();
        }
        if (bltcon1 & BLTCON1_SUL) {
            if (!sign) decy();
        } else {
            if (!sign) incy();
        }
        
    } else {
        
        if (bltcon1 & BLTCON1_AUL) {
            decy();
        } else {
            incy();
        }
        if (bltcon1 & BLTCON1_SUL) {
            if (!sign) decx();
        } else {
            if (!sign) incx();
        }
    }
    
    if (bltcon0 & BLTCON0_USEA) {
        if (sign)
            U32_INC(bltapt, bltbmod);
        else
            U32_INC(bltapt, bltamod);
    }

    // Update the SIGN bit in BPLCON1
    REPLACE_BIT(bltcon1, 6, (i16)bltapt < 0);
}

void
Blitter::prepareBlit()
{
    remaining = bltsizeH * bltsizeV;
    cntA = cntB = cntC = cntD = bltsizeH;

    running = true;
    bzero = true;
    bbusy = true;
    birq = false;

    bltpc = 0;
    iteration = 0;
}

void
Blitter::beginBlit()
{
    auto level = config.accuracy;

    if (bltconLINE()) {

        if (BLT_CHECKSUM) {
            
            linecount++;
            check1 = check2 = util::fnvInit32();
            msg("Line %ld (%d,%d) (%d%d%d%d)[%x] (%d %d %d %d) %x %x %x %x\n",
                linecount, bltsizeH, bltsizeV,
                bltconUSEA(), bltconUSEB(), bltconUSEC(), bltconUSED(),
                bltcon0,
                bltamod, bltbmod, bltcmod, bltdmod,
                bltapt & agnus.ptrMask,
                bltbpt & agnus.ptrMask,
                bltcpt & agnus.ptrMask,
                bltdpt & agnus.ptrMask);
        }

        beginLineBlit(level);

    } else {

        if (BLT_CHECKSUM) {
            
            copycount++;
            check1 = check2 = util::fnvInit32();
            msg("Blit %ld (%d,%d) (%d%d%d%d)[%x] (%d %d %d %d) %x %x %x %x %s%s\n",
                copycount,
                bltsizeH, bltsizeV,
                bltconUSEA(), bltconUSEB(), bltconUSEC(), bltconUSED(),
                bltcon0,
                bltamod, bltbmod, bltcmod, bltdmod,
                bltapt & agnus.ptrMask,
                bltbpt & agnus.ptrMask,
                bltcpt & agnus.ptrMask,
                bltdpt & agnus.ptrMask,
                bltconDESC() ? "D" : "", bltconFE() ? "F" : "");
        }

        beginCopyBlit(level);
    }
}

void
Blitter::beginLineBlit(isize level)
{
    static u64 verbose = 0;

    if (verbose++ == 0) {
        debug(BLT_CHECKSUM, "Performing level %ld line blits.\n", level);
    }
    if (bltcon0 & BLTCON0_USEB) {
        xfiles("Performing line blit with channel B enabled\n");
    }
    if (bltsizeH != 2) {
        xfiles("Performing line blit with WIDTH = %d\n", bltsizeH);
    }
    
    switch (level) {
            
        case 0: beginFastLineBlit(); break;
        case 1: beginFakeLineBlit(); break;
        case 2: beginSlowLineBlit(); break;
            
        default:
            fatalError;
    }
}

void
Blitter::beginCopyBlit(isize level)
{
    static u64 verbose = 0;

    if (verbose++ == 0) {
        debug(BLT_CHECKSUM, "Performing level %ld copy blits.\n", level);
    }

    switch (level) {
            
        case 0: beginFastCopyBlit(); break;
        case 1: beginFakeCopyBlit(); break;
        case 2: beginSlowCopyBlit(); break;
            
        default:
            fatalError;
    }
}

void
Blitter::clearBusyFlag()
{
    debug(BLTTIM_DEBUG, "(%ld,%ld) Blitter bbusy\n", agnus.pos.v, agnus.pos.h);

    // Clear the Blitter busy flag
    bbusy = false;
}

void
Blitter::endBlit()
{
    debug(BLTTIM_DEBUG, "(%ld,%ld) Blitter terminates\n", agnus.pos.v, agnus.pos.h);
    
    running = false;
    if (BLT_MEM_GUARD) blitcount++;
    
    // Clear the Blitter slot
    agnus.cancel<SLOT_BLT>();
    
    // Dump checksums if requested
    debug(BLT_CHECKSUM,
          "check1: %x check2: %x ABCD: %x %x %x %x\n",
          check1, check2,
          bltapt & agnus.ptrMask, bltbpt & agnus.ptrMask,
          bltcpt & agnus.ptrMask, bltdpt & agnus.ptrMask);
    
    // Let the Copper know about the termination
    copper.blitterDidTerminate();
}

}
