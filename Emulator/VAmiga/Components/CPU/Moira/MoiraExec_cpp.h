// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#define AVAILABILITY(core) \
if constexpr ((core) == C68010) { static_assert(C != C68000); } \
if constexpr ((core) == C68020) { static_assert(C != C68000 && C != C68010); } \
if constexpr (C == C68020) cp = 0; \
if constexpr (WILL_EXECUTE) willExecute(__func__, I, M, S, opcode);

#define FINALIZE \
if constexpr (DID_EXECUTE) didExecute(__func__, I, M, S, opcode);

#define SUPERVISOR_MODE_ONLY \
if (!reg.sr.s) { \
execException<C>(EXC_PRIVILEGE); \
CYCLES_68000(34) \
CYCLES_68010(38) \
CYCLES_68020(34) \
return; }

#define STD_AE_FRAME (\
(M == MODE_PD && S != Long) ? AE_INC_PC : \
(M == MODE_DI)              ? AE_DEC_PC : \
(M == MODE_IX)              ? AE_DEC_PC : \
(M == MODE_DIPC)            ? AE_DEC_PC : \
(M == MODE_IXPC)            ? AE_DEC_PC : 0)

template <Core C, Instr I, Mode M, Size S> void
Moira::execLineA(u16 opcode)
{
    AVAILABILITY(C68000)

    // Check if a software trap is set for this instruction
    if (debugger.swTraps.traps.contains(opcode)) {

        auto &trap = debugger.swTraps.traps[opcode];

        // Restore the original instruction
        reg.pc = reg.pc0;
        queue.irc = trap.instruction;
        prefetch<C>();

        // Inform the delegate
        softwareTrapReached(reg.pc0);
        return;
    }

    execException<C>(EXC_LINEA);

    CYCLES_68000(34)
    CYCLES_68010(4)
    CYCLES_68020(20)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execLineF(u16 opcode)
{
    AVAILABILITY(C68000)

    execException<C>(EXC_LINEF);

    CYCLES_68000(34)
    CYCLES_68010(38)
    CYCLES_68020(34)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execIllegal(u16 opcode)
{
    AVAILABILITY(C68000)

    execException<C>(EXC_ILLEGAL);

    CYCLES_68000(34)
    CYCLES_68010(38)
    CYCLES_68020(20)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execShiftRg(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    int cnt = readD(src) & 0x3F;
    int cyc = (S == Long ? 4 : 2) + 2 * cnt;

    prefetch<C, POLL>();
    SYNC(cyc);

    writeD<S>(dst, shift<C, I, S>(cnt, readD<S>(dst)));

    if constexpr (C == C68000 || C == C68010) {

        CYCLES(4 + cyc);

    } else {

        switch (I) {

            case ROL: case ROR: case ASL:   CYCLES(  8 + cnt ); break;
            case ROXL: case ROXR:           CYCLES( 12 + cnt ); break;
            default:                        CYCLES(  6 + cnt ); break;
        }
    }

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execShiftIm(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);
    int cnt = src ? src : 8;
    int cyc = (S == Long ? 4 : 2) + 2 * cnt;

    prefetch<C, POLL>();
    SYNC(cyc);

    writeD<S>(dst, shift<C, I, S>(cnt, readD<S>(dst)));

    if constexpr (C == C68000 || C == C68010) {

        CYCLES(4 + cyc);

    } else {

        switch (I) {

            case LSL: case LSR:             CYCLES(  4 ); break;
            case ROL: case ROR: case ASL:   CYCLES(  8 ); break;
            case ROXL: case ROXR:           CYCLES( 12 ); break;
            default:                        CYCLES(  6 ); break;
        }
    }

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execShiftEa(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = _____________xxx(opcode);

    u32 ea, data;
    readOp<C, M, S, STD_AE_FRAME>(src, &ea, &data);

    looping<I>() ? noPrefetch<C>(4) : prefetch<C, POLL>();
    writeM<C, M, S>(ea, shift<C, I, S>(1, data));

    if constexpr (I == ROR || I == ROL) {

        //           00  10  20        00  10  20        00  10  20
        //           .b  .b  .b        .w  .w  .w        .l  .l  .l
        CYCLES_AI   ( 0,  0,  0,       12, 12, 11,        0,  0,  0)
        CYCLES_PI   ( 0,  0,  0,       12, 12, 11,        0,  0,  0)
        CYCLES_PD   ( 0,  0,  0,       14, 14, 12,        0,  0,  0)
        CYCLES_DI   ( 0,  0,  0,       16, 16, 12,        0,  0,  0)
        CYCLES_IX   ( 0,  0,  0,       18, 18, 14,        0,  0,  0)
        CYCLES_AW   ( 0,  0,  0,       16, 16, 11,        0,  0,  0)
        CYCLES_AL   ( 0,  0,  0,       20, 20, 11,        0,  0,  0)

    } else if constexpr (I == ASL) {

        //           00  10  20        00  10  20        00  10  20
        //           .b  .b  .b        .w  .w  .w        .l  .l  .l
        CYCLES_AI   ( 0,  0,  0,       12, 12, 10,        0,  0,  0)
        CYCLES_PI   ( 0,  0,  0,       12, 12, 10,        0,  0,  0)
        CYCLES_PD   ( 0,  0,  0,       14, 14, 11,        0,  0,  0)
        CYCLES_DI   ( 0,  0,  0,       16, 16, 11,        0,  0,  0)
        CYCLES_IX   ( 0,  0,  0,       18, 18, 13,        0,  0,  0)
        CYCLES_AW   ( 0,  0,  0,       16, 16, 10,        0,  0,  0)
        CYCLES_AL   ( 0,  0,  0,       20, 20, 10,        0,  0,  0)

    } else {

        //           00  10  20        00  10  20        00  10  20
        //           .b  .b  .b        .w  .w  .w        .l  .l  .l
        CYCLES_AI   ( 0,  0,  0,       12, 12,  9,        0,  0,  0)
        CYCLES_PI   ( 0,  0,  0,       12, 12,  9,        0,  0,  0)
        CYCLES_PD   ( 0,  0,  0,       14, 14, 10,        0,  0,  0)
        CYCLES_DI   ( 0,  0,  0,       16, 16, 10,        0,  0,  0)
        CYCLES_IX   ( 0,  0,  0,       18, 18, 12,        0,  0,  0)
        CYCLES_AW   ( 0,  0,  0,       16, 16,  9,        0,  0,  0)
        CYCLES_AL   ( 0,  0,  0,       20, 20,  9,        0,  0,  0)
    }

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAbcdRg(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 result = bcd<C, I, Byte>(readD<Byte>(src), readD<Byte>(dst));

    prefetch<C, POLL>();
    SYNC(2);
    writeD<Byte>(dst, result);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 6,  6,  4,        0,  0,  0,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAbcdEa(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea1, ea2, data1, data2;
    readOp<C, M, S>(src, &ea1, &data1);
    POLL_IPL;
    readOp<C, M, S, IMPL_DEC>(dst, &ea2, &data2);

    u32 result = bcd<C, I, Byte>(data1, data2);
    looping<I>() ? noPrefetch<C>(4) : prefetch<C>();

    writeM<C, M, Byte>(ea2, result);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_PD   (18, 18, 16,        0,  0,  0,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAddEaRg(u16 opcode)
{
    AVAILABILITY(C68000)

    u32 ea, data, result;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    readOp<C, M, S, STD_AE_FRAME>(src, &ea, &data);

    result = addsub<C, I, S>(data, readD<S>(dst));
    writeD<S>(dst, result);

    if constexpr (C == C68000) {

        prefetch<C, POLL>();
        if constexpr (S == Long) SYNC(2 + (isMemMode(M) ? 0 : 2));

    } else {

        POLL_IPL;
        if constexpr (S == Long) SYNC(2);
        looping<I>() ? noPrefetch<C>(6) : prefetch<C>();

    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 4,  4,  2,        4,  4,  2,        8,  6,  2)
    CYCLES_AN   ( 0,  0,  0,        4,  4,  2,        8,  6,  2)
    CYCLES_AI   ( 8,  8,  6,        8,  8,  6,       14, 14,  6)
    CYCLES_PI   ( 8,  8,  6,        8,  8,  6,       14, 14,  6)
    CYCLES_PD   (10, 10,  7,       10, 10,  7,       16, 16,  7)
    CYCLES_DI   (12, 12,  7,       12, 12,  7,       18, 18,  7)
    CYCLES_IX   (14, 14,  9,       14, 14,  9,       20, 20,  9)
    CYCLES_AW   (12, 12,  6,       12, 12,  6,       18, 18,  6)
    CYCLES_AL   (16, 16,  6,       16, 16,  6,       22, 22,  6)
    CYCLES_DIPC (12, 12,  7,       12, 12,  7,       18, 18,  7)
    CYCLES_IXPC (14, 14,  9,       14, 14,  9,       20, 20,  9)
    CYCLES_IM   ( 8,  8,  4,        8,  8,  4,       16, 14,  6)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAddRgEa(u16 opcode)
{
    AVAILABILITY(C68000)

    u32 ea, data, result;

    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    readOp<C, M, S, STD_AE_FRAME>(dst, &ea, &data);
    result = addsub<C, I, S>(readD<S>(src), data);

    looping<I>() ? noPrefetch<C>(2) : prefetch<C, POLL>();

    writeM<C, M, S>(ea, result);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_AI   (12, 12,  8,       12, 12,  8,       20, 20,  8)
    CYCLES_PI   (12, 12,  8,       12, 12,  8,       20, 20,  8)
    CYCLES_PD   (14, 14,  9,       14, 14,  9,       22, 22,  9)
    CYCLES_DI   (16, 16,  9,       16, 16,  9,       24, 24,  9)
    CYCLES_IX   (18, 18, 11,       18, 18, 11,       26, 26, 11)
    CYCLES_AW   (16, 16,  8,       16, 16,  8,       24, 24,  8)
    CYCLES_AL   (20, 20,  8,       20, 20,  8,       28, 28,  8)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAdda(u16 opcode)
{
    AVAILABILITY(C68000)

    u32 ea, data, result;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    readOp<C, M, S, STD_AE_FRAME>(src, &ea, &data);
    data = SEXT<S>(data);

    result = (I == ADDA || I == ADDA_LOOP) ? U32_ADD(readA(dst), data) : U32_SUB(readA(dst), data);
    writeA(dst, result);

    if constexpr (C == C68000) {

        prefetch<C, POLL>();
        SYNC(2);
        if constexpr (S == Word || isRegMode(M) || isImmMode(M)) SYNC(2);

    } else {

        SYNC(2);
        POLL_IPL;
        if constexpr (S == Word || isRegMode(M) || isImmMode(M)) SYNC(2);
        if (looping<I>()) {
            if (M == MODE_DN && S == Long) noPrefetch<C>(6);
            else if (M == MODE_AI && S == Long) noPrefetch<C>(6);
            else if (M == MODE_AI && S != Long) noPrefetch<C>(4);
            else if (M == MODE_PI && S == Long) noPrefetch<C>(6);
            else if (M == MODE_PI && S != Long) noPrefetch<C>(4);
            else if (M == MODE_PD && S == Long) noPrefetch<C>(6);
            else if (M == MODE_PD && S != Long) noPrefetch<C>(4);
            else noPrefetch<C>();
        } else {
            prefetch<C>();
        }
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 0,  0,  0,        8,  8,  2,        8,  6,  2)
    CYCLES_AN   ( 0,  0,  0,        8,  8,  2,        8,  6,  2)
    CYCLES_AI   ( 0,  0,  0,       12, 12,  6,       14, 14,  6)
    CYCLES_PI   ( 0,  0,  0,       12, 12,  6,       14, 14,  6)
    CYCLES_PD   ( 0,  0,  0,       14, 14,  7,       16, 16,  7)
    CYCLES_DI   ( 0,  0,  0,       16, 16,  7,       18, 18,  7)
    CYCLES_IX   ( 0,  0,  0,       18, 18,  9,       20, 20,  9)
    CYCLES_AW   ( 0,  0,  0,       16, 16,  6,       18, 18,  6)
    CYCLES_AL   ( 0,  0,  0,       20, 20,  6,       22, 22,  6)
    CYCLES_DIPC ( 0,  0,  0,       16, 16,  7,       18, 18,  7)
    CYCLES_IXPC ( 0,  0,  0,       18, 18,  9,       20, 20,  9)
    CYCLES_IM   ( 0,  0,  0,       12, 12,  4,       16, 14,  6)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAddiRg(u16 opcode)
{
    AVAILABILITY(C68000)

    u32 src = readI<C, S>();
    int dst = _____________xxx(opcode);

    u32 ea, data, result;
    readOp<C, M, S>(dst, &ea, &data);

    result = addsub<C, I, S>(src, data);
    prefetch<C, POLL>();

    if constexpr (S == Long) { SYNC_68000(4); SYNC_68010(2); }
    writeD<S>(dst, result);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   (8,   8,  2,        8,  8,  2,       16, 14,  2)
    CYCLES_AI   (16, 16,  8,       16, 16,  8,       28, 28,  8)
    CYCLES_PI   (16, 16,  8,       16, 16,  8,       28, 28,  8)
    CYCLES_PD   (18, 18,  9,       18, 18,  9,       30, 30,  9)
    CYCLES_DI   (20, 20,  9,       20, 20,  9,       32, 32,  9)
    CYCLES_IX   (22, 22, 11,       22, 22, 11,       34, 34, 11)
    CYCLES_AW   (20, 20,  8,       20, 20,  8,       32, 32,  8)
    CYCLES_AL   (24, 24,  8,       24, 24,  8,       36, 36,  8)
    CYCLES_IM   (22, 22, 11,       22, 22, 11,       34, 34, 11)

    FINALIZE
}


template <Core C, Instr I, Mode M, Size S> void
Moira::execAddiEa(u16 opcode)
{
    AVAILABILITY(C68000)

    u32 src = readI<C, S>();
    int dst = _____________xxx(opcode);

    u32 ea, data, result;
    readOp<C, M, S, STD_AE_FRAME>(dst, &ea, &data);

    result = addsub<C, I, S>(src, data);
    prefetch<C>();

    writeOp<C, M, S, POLL>(dst, ea, result);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   (8,   8,  2,        8,  8,  2,       16, 14,  2)
    CYCLES_AI   (16, 16,  8,       16, 16,  8,       28, 28,  8)
    CYCLES_PI   (16, 16,  8,       16, 16,  8,       28, 28,  8)
    CYCLES_PD   (18, 18,  9,       18, 18,  9,       30, 30,  9)
    CYCLES_DI   (20, 20,  9,       20, 20,  9,       32, 32,  9)
    CYCLES_IX   (22, 22, 11,       22, 22, 11,       34, 34, 11)
    CYCLES_AW   (20, 20,  8,       20, 20,  8,       32, 32,  8)
    CYCLES_AL   (24, 24,  8,       24, 24,  8,       36, 36,  8)
    CYCLES_IM   (22, 22, 11,       22, 22, 11,       34, 34, 11)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAddqDn(u16 opcode)
{
    AVAILABILITY(C68000)

    i8  src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    if (src == 0) src = 8;
    u32 result = addsub<C, I, S>(src, readD<S>(dst));
    prefetch<C, POLL>();

    if constexpr (S == Long) SYNC(4);
    writeD<S>(dst, result);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 4,  4,  2,        4,  4,  2,        8,  8,  2)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAddqAn(u16 opcode)
{
    AVAILABILITY(C68000)

    i8  src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    if (src == 0) src = 8;
    u32 result = (I == ADDQ) ? U32_ADD(readA(dst), src) : U32_SUB(readA(dst), src);
    prefetch<C, POLL>();
    SYNC(4);
    writeA(dst, result);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_AN   ( 0,  0,  0,        8,  4,  2,        8,  8,  2)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAddqEa(u16 opcode)
{
    AVAILABILITY(C68000)

    i8  src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    u32 ea, data, result;
    readOp<C, M, S, STD_AE_FRAME>(dst, &ea, &data);

    if (src == 0) src = 8;
    result = addsub<C, I, S>(src, data);
    prefetch<C, POLL>();

    writeOp<C, M, S>(dst, ea, result);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_AI   (12, 12,  8,       12, 12,  8,       20, 20,  8)
    CYCLES_PI   (12, 12,  8,       12, 12,  8,       20, 20,  8)
    CYCLES_PD   (14, 14,  9,       14, 14,  9,       22, 22,  9)
    CYCLES_DI   (16, 16,  9,       16, 16,  9,       24, 24,  9)
    CYCLES_IX   (18, 18, 11,       18, 18, 11,       26, 26, 11)
    CYCLES_AW   (16, 16,  8,       16, 16,  8,       24, 24,  8)
    CYCLES_AL   (20, 20,  8,       20, 20,  8,       28, 28,  8)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAddxRg(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 result = addsub<C, I, S>(readD<S>(src), readD<S>(dst));

    if constexpr (C == C68000) {

        prefetch<C, POLL>();
        if constexpr (S == Long) SYNC(4);

    } else {

        POLL_IPL;
        if constexpr (S == Long) SYNC(2);
        prefetch<C>();
    }

    writeD<S>(dst, result);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 4,  4,  2,        4,  4,  2,        8,  6,  2)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAddxEa(u16 opcode)
{
    AVAILABILITY(C68000)

    const u64 flags =
    (S == Word) ? AE_INC_PC : (S == Long) ? AE_INC_PC | AE_INC_A : 0;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea1, ea2, data1, data2;

    try {
        readOp<C, M, S, flags>(src, &ea1, &data1);

    } catch (const AddressError &exc) {

        // Rectify stack frame
        if constexpr (S == Long) undoAnPD<M,S>(src);
        throw exc;
    }
    if constexpr (S != Long) POLL_IPL;

    try {
        readOp<C, M, S, flags|IMPL_DEC> (dst, &ea2, &data2);

    } catch (const AddressError &exc) {

        // Rectify stack frame
        if constexpr (S == Long) undoAnPD<M,S>(dst);
        throw exc;
    }

    u32 result = addsub<C, I, S>(data1, data2);

    if constexpr (S == Long && C == C68000 && !MIMIC_MUSASHI) {

        writeM<C, M, Word, POLL>(ea2 + 2, result & 0xFFFF);
        looping<I>() ? noPrefetch<C>() : prefetch<C>();
        writeM<C, M, Word>(ea2, result >> 16);

    } else if constexpr (S == Long && C == C68010 && !MIMIC_MUSASHI) {

        writeM<C, M, S>(ea2, result);
        looping<I>() ? noPrefetch<C>(S == Long ? 0 : 2) : prefetch<C, POLL>();

    } else {

        looping<I>() ? noPrefetch<C>(S == Long ? 0 : 2) : prefetch<C>();
        writeM<C, M, S>(ea2, result);
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_PD   (18, 18, 12,       18, 18, 12,       30, 30, 12)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAndEaRg(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea, data;
    readOp<C, M, S, STD_AE_FRAME>(src, &ea, &data);

    u32 result = logic<C, I, S>(data, readD<S>(dst));
    writeD<S>(dst, result);

    if constexpr (C == C68000) {

        looping<I>() ? noPrefetch<C>() : prefetch<C, POLL>();
        if constexpr (S == Long) SYNC(isRegMode(M) || isImmMode(M) ? 4 : 2);

    } else {

        POLL_IPL;
        if constexpr (S == Long) SYNC(2);
        looping<I>() ? noPrefetch<C>(6) : prefetch<C>();
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 4,  4,  2,        4,  4,  2,        8,  6,  2)
    CYCLES_AI   ( 8,  8,  6,        8,  8,  6,       14, 14,  6)
    CYCLES_PI   ( 8,  8,  6,        8,  8,  6,       14, 14,  6)
    CYCLES_PD   (10, 10,  7,       10, 10,  7,       16, 16,  7)
    CYCLES_DI   (12, 12,  7,       12, 12,  7,       18, 18,  7)
    CYCLES_IX   (14, 14,  9,       14, 14,  9,       20, 20,  9)
    CYCLES_AW   (12, 12,  6,       12, 12,  6,       18, 18,  6)
    CYCLES_AL   (16, 16,  6,       16, 16,  6,       22, 22,  6)
    CYCLES_DIPC (12, 12,  7,       12, 12,  7,       18, 18,  7)
    CYCLES_IXPC (14, 14,  9,       14, 14,  9,       20, 20,  9)
    CYCLES_IM   ( 8,  8,  4,        8,  8,  4,       16, 14,  6)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAndRgEa(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    u32 ea, data;
    readOp<C, M, S, STD_AE_FRAME>(dst, &ea, &data);

    u32 result = logic<C, I, S>(readD<S>(src), data);
    looping<I>() ? noPrefetch<C>(2) : prefetch<C, POLL>();

    if constexpr (S == Long && isRegMode(M)) { SYNC_68000(4); SYNC_68010(2); }

    if constexpr (MIMIC_MUSASHI) {
        writeOp<C, M, S>(dst, ea, result);
    } else {
        writeOp<C, M, S, REVERSE>(dst, ea, result);
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 4,  4,  2,        4,  4,  2,        8,  6,  2)
    CYCLES_AI   (12, 12,  8,       12, 12,  8,       20, 20,  8)
    CYCLES_PI   (12, 12,  8,       12, 12,  8,       20, 20,  8)
    CYCLES_PD   (14, 14,  9,       14, 14,  9,       22, 22,  9)
    CYCLES_DI   (16, 16,  9,       16, 16,  9,       24, 24,  9)
    CYCLES_IX   (18, 18, 11,       18, 18, 11,       26, 26, 11)
    CYCLES_AW   (16, 16,  8,       16, 16,  8,       24, 24,  8)
    CYCLES_AL   (20, 20,  8,       20, 20,  8,       28, 28,  8)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAndiRg(u16 opcode)
{
    AVAILABILITY(C68000)

    u32 src = readI<C, S>();
    int dst = _____________xxx(opcode);

    u32 result = logic<C, I, S>(src, readD<S>(dst));
    prefetch<C, POLL>();

    if constexpr (S == Long) { SYNC_68000(4); SYNC_68010(2); }
    writeD<S>(dst, result);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 8,  8,  2,        8,  8,  2,       16, 14,  2)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAndiEa(u16 opcode)
{
    AVAILABILITY(C68000)

    u32 ea, data, result;

    u32 src = readI<C, S>();
    int dst = _____________xxx(opcode);

    readOp<C, M, S, STD_AE_FRAME>(dst, &ea, &data);

    result = logic<C, I, S>(src, data);
    prefetch<C, POLL>();

    if constexpr (MIMIC_MUSASHI) {
        writeOp<C, M, S>(dst, ea, result);
    } else {
        writeOp<C, M, S, REVERSE>(dst, ea, result);
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_AI   (16, 16,  8,       16, 16,  8,       28, 28,  8)
    CYCLES_PI   (16, 16,  8,       16, 16,  8,       28, 28,  8)
    CYCLES_PD   (18, 18,  9,       18, 18,  9,       30, 30,  9)
    CYCLES_DI   (20, 20,  9,       20, 20,  9,       32, 32,  9)
    CYCLES_IX   (22, 22, 11,       22, 22, 11,       34, 34, 11)
    CYCLES_AW   (20, 20,  8,       20, 20,  8,       32, 32,  8)
    CYCLES_AL   (24, 24,  8,       24, 24,  8,       36, 36,  8)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAndiccr(u16 opcode)
{
    AVAILABILITY(C68000)

    if constexpr (C == C68000) {

        u32 src = readI<C, S>();
        u8  dst = getCCR();
        setCCR(u8(logic<C, I, S>(src, dst)));
        SYNC(8);
        (void)read<C, MEM_DATA, Word>(reg.pc + 2);

    } else {

        SYNC(8);
        u32 src = readI<C, S>();
        u8  dst = getCCR();
        setCCR(u8(logic<C, I, S>(src, dst)));
    }

    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IM   (20, 16, 12,        0,  0,  0,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAndisr(u16 opcode)
{
    AVAILABILITY(C68000)

    SUPERVISOR_MODE_ONLY

    u32 src = readI<C, S>();
    u16 dst = getSR();

    SYNC_68000(8);
    SYNC_68010(4);

    u32 result = logic<C, I, S>(src, dst);
    setSR((u16)result);

    (void)read<C, MEM_DATA, Word>(reg.pc + 2);
    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IM   ( 0,  0,  0,       20, 16, 12,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execBra(u16 opcode)
{
    AVAILABILITY(S == Long ? C68020 : C68000)

    u32 oldpc = reg.pc;
    u32 disp = S == Byte ? (u8)opcode : queue.irc;

    SYNC(2);

    if constexpr (S == Long) {

        readExt<C>();
        disp = disp << 16 | queue.irc;
    }

    u32 newpc = U32_ADD(oldpc, SEXT<S>(disp));

    // Check for address error
    if (misaligned<C>(newpc)) {
        throw AddressError(makeFrame(newpc));
    }

    reg.pc = newpc;
    fullPrefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP   (10, 10, 10,       10, 10, 10,       10, 10, 10)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execBcc(u16 opcode)
{
    AVAILABILITY(S == Long ? C68020 : C68000)

    u32 oldpc = reg.pc;

    SYNC(2);

    if (cond<I>()) {

        u32 disp = S == Byte ? (u8)opcode : queue.irc;

        if (S == Long) {

            readExt<C>();
            disp = disp << 16 | queue.irc;
        }

        u32 newpc = U32_ADD(oldpc, SEXT<S>(disp));

        // Check for address error
        if (misaligned<C>(newpc)) {
            throw AddressError(makeFrame(newpc));
        }

        // Take branch
        reg.pc = newpc;
        fullPrefetch<C, POLL>();

        //           00  10  20        00  10  20        00  10  20
        //           .b  .b  .b        .w  .w  .w        .l  .l  .l
        CYCLES_IP   (10, 10,  6,       10, 10,  6,        0,  0,  6)

    } else {

        // Fall through to next instruction
        if constexpr (C == C68000) SYNC(2);
        if constexpr (S == Word || S == Long) readExt<C>();
        if constexpr (S == Long) readExt<C>();
        prefetch<C, POLL>();

        //           00  10  20        00  10  20        00  10  20
        //           .b  .b  .b        .w  .w  .w        .l  .l  .l
        CYCLES_IP   ( 8,  6,  4,       12, 10,  6,        0,  0,  6)
    }

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execBitDxDy(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    u8 b = readD(src) & 0b11111;
    [[maybe_unused]] auto c = cyclesBit<C, I>(b);

    u32 data = readD(dst);
    data = bit<C, I>(data, b);

    prefetch<C, POLL>();

    SYNC(c);
    if constexpr (I != BTST) writeD(dst, data);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   (4+c,4+c, 4,        0,  0,  0,        0,  0,  0)
    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execBitDxEa(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    u8 b = readD(src) & 0b111;

    u32 ea, data;
    readOp<C, M, Byte>(dst, &ea, &data);

    data = bit<C, I>(data, b);

    if constexpr (I == BCLR && C == C68010) { SYNC(2); }

    prefetch<C, POLL>();
    if constexpr (I != BTST) writeM<C, M, Byte>(ea, data);

    [[maybe_unused]] auto c = I == BTST ? 0 : I == BCLR && C == C68010 ? 6 : 4;

    //             00    10    20        00  10  20        00  10  20
    //             .b    .b    .b        .w  .w  .w        .l  .l  .l
    CYCLES_AI   ( 8+c,  8+c,    8,        0,  0,  0,        0,  0,  0)
    CYCLES_PD   (10+c, 10+c,    9,        0,  0,  0,        0,  0,  0)
    CYCLES_PI   ( 8+c,  8+c,    8,        0,  0,  0,        0,  0,  0)
    CYCLES_DI   (12+c, 12+c,    9,        0,  0,  0,        0,  0,  0)
    CYCLES_IX   (14+c, 14+c,   11,        0,  0,  0,        0,  0,  0)
    CYCLES_AW   (12+c, 12+c,    8,        0,  0,  0,        0,  0,  0)
    CYCLES_AL   (16+c, 16+c,    8,        0,  0,  0,        0,  0,  0)
    CYCLES_DIPC (12+c, 12+c,    9,        0,  0,  0,        0,  0,  0)
    CYCLES_IXPC (14+c, 14+c,   11,        0,  0,  0,        0,  0,  0)
    CYCLES_IM   ( 8+c,  8+c,    6,        0,  0,  0,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execBitImDy(u16 opcode)
{
    AVAILABILITY(C68000)

    u8  src = (u8)readI<C, S>();
    int dst = _____________xxx(opcode);

    src &= 0b11111;
    u32 data = readD(dst);
    data = bit<C, I>(data, src);

    prefetch<C, POLL>();

    [[maybe_unused]] auto c = cyclesBit<C, I>(src);
    SYNC(c);
    if constexpr (I != BTST) writeD(dst, data);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   (8+c,8+c, 4,        0,  0,  0,        0,  0,  0)
    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execBitImEa(u16 opcode)
{
    AVAILABILITY(C68000)

    u8  src = (u8)readI<C, S>();
    int dst = _____________xxx(opcode);

    src &= 0b111;
    u32 ea, data;
    readOp<C, M, S>(dst, &ea, &data);

    data = bit<C, I>(data, src);

    prefetch<C, POLL>();
    if constexpr (I != BTST) writeM<C, M, S>(ea, data);

    [[maybe_unused]] auto c = I == BTST ? 0 : 4;

    //             00    10    20        00  10  20        00  10  20
    //             .b    .b    .b        .w  .w  .w        .l  .l  .l
    CYCLES_AI   (12+c, 12+c,    8,        0,  0,  0,        0,  0,  0)
    CYCLES_PD   (14+c, 14+c,    9,        0,  0,  0,        0,  0,  0)
    CYCLES_PI   (12+c, 12+c,    8,        0,  0,  0,        0,  0,  0)
    CYCLES_DI   (16+c, 16+c,    9,        0,  0,  0,        0,  0,  0)
    CYCLES_IX   (18+c, 18+c,   11,        0,  0,  0,        0,  0,  0)
    CYCLES_AW   (16+c, 16+c,    8,        0,  0,  0,        0,  0,  0)
    CYCLES_AL   (20+c, 20+c,    8,        0,  0,  0,        0,  0,  0)
    CYCLES_DIPC (16+c, 16+c,    9,        0,  0,  0,        0,  0,  0)
    CYCLES_IXPC (18+c, 18+c,   11,        0,  0,  0,        0,  0,  0)
    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execBitFieldDn(u16 opcode)
{
    AVAILABILITY(C68020)

    u16 ext = (u16)readI<C, Word>();

    int dy     = _____________xxx (opcode);
    int dn     = _xxx____________ (ext);
    int of     = _____xxxxx______ (ext);
    int doBit  = ____x___________ (ext);
    int wi     = ___________xxxxx (ext);
    int dwBit  = __________x_____ (ext);

    // If do or dw is set, offset or width are taken from data registers
    int rawOffset = doBit ? reg.d[of & 0b111] : of;
    int rawWidth = dwBit ? reg.d[wi & 0b111] : wi;

    // Crop offset and map width to 32, 1 ... 31
    int offset = rawOffset & 0b11111;
    int width = ((rawWidth - 1) & 0b11111) + 1;

    // Create the bit mask
    u32 mask = std::rotr(u32(0xFFFFFFFF00000000 >> width), offset);

    u32 data = readD(dy);
    u32 result, insert;

    switch (I) {

        case BFCHG:
        case BFCLR:
        case BFSET:

            result = bitfield<I>(data, offset, width, mask);
            writeD(dy, result);

            //           00  10  20        00  10  20        00  10  20
            //           .b  .b  .b        .w  .w  .w        .l  .l  .l
            CYCLES_DN   ( 0,  0,  0,        0,  0,  0,        0,  0, 12)
            break;

        case BFEXTS:
        case BFEXTU:

            data = std::rotl(data, offset);
            result = bitfield<I>(data, offset, width, mask);
            writeD(dn, result);

            //           00  10  20        00  10  20        00  10  20
            //           .b  .b  .b        .w  .w  .w        .l  .l  .l
            CYCLES_DN   ( 0,  0,  0,        0,  0,  0,        0,  0,  8)
            break;

        case BFFFO:
        {
            data = std::rotl(data, offset);
            result = bitfield<I>(data, rawOffset, width, mask);
            writeD(dn, result);

            //           00  10  20        00  10  20        00  10  20
            //           .b  .b  .b        .w  .w  .w        .l  .l  .l
            CYCLES_DN   ( 0,  0,  0,        0,  0,  0,        0,  0, 18)
            break;
        }
        case BFINS:

            insert = readD(dn);
            insert = u32(insert << (32 - width));

            reg.sr.n = NBIT<S>(insert);
            reg.sr.z = ZERO<S>(insert);
            reg.sr.v = 0;
            reg.sr.c = 0;

            insert = std::rotr((u32)insert, offset);
            writeD(dy, (data & ~mask) | insert);

            //           00  10  20        00  10  20        00  10  20
            //           .b  .b  .b        .w  .w  .w        .l  .l  .l
            CYCLES_DN   ( 0,  0,  0,        0,  0,  0,        0,  0, 10)
            break;

        case BFTST:

            (void)bitfield<I>(data, offset, width, mask);

            //           00  10  20        00  10  20        00  10  20
            //           .b  .b  .b        .w  .w  .w        .l  .l  .l
            CYCLES_DN   ( 0,  0,  0,        0,  0,  0,        0,  0,  6)
            break;

        default:
            fatalError;
    }

    prefetch<C, POLL>();

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execBitFieldEa(u16 opcode)
{
    AVAILABILITY(C68020)

    u16 ext = (u16)readI<C, Word>();

    int dy     = _____________xxx (opcode);
    int dn     = _xxx____________ (ext);
    int offset = _____xxxxx______ (ext);
    int doBit  = ____x___________ (ext);
    int width  = ___________xxxxx (ext);
    int dwBit  = __________x_____ (ext);

    // If Do or Dw is set, offset or width are taken from data registers
    if (doBit) offset = reg.d[offset & 0b111];
    if (dwBit) width = reg.d[width & 0b111];

    // Map width to 32, 1 ... 31
    width = ((width - 1) & 0b11111) + 1;

    u32 oldOffset = offset;
    u32 result, insert;

    // Compute the effective address
    u32 ea = computeEA<C, M, S>(dy);

    ea += offset / 8;
    offset %= 8;
    if(offset < 0) {

        offset += 8;
        ea--;
    }

    // Create bit masks
    u64 baseMask = (0xFFFFFFFF00000000 << (32 - width)) >> offset;
    auto mask = u32(baseMask >> 32);
    auto mask8 = u8(baseMask >> 24);

    u32 data = readM<C, M, S>(ea);

    switch (I) {

        case BFCHG:
        case BFCLR:
        case BFSET:

            result = bitfield<I>(data, offset, width, mask);
            writeM<C, M, S>(ea, result);

            if((width + offset) > 32) {

                data = readM<C, M, Byte>(ea + 4);

                if constexpr (I == BFCHG) writeM<C, M, Byte>(ea + 4, data ^ mask8);
                if constexpr (I == BFCLR) writeM<C, M, Byte>(ea + 4, data & ~mask8);
                if constexpr (I == BFSET) writeM<C, M, Byte>(ea + 4, data | mask8);

                reg.sr.z &= ZERO<Byte>(data & mask8);
            }

            //           00  10  20        00  10  20        00  10  20
            //           .b  .b  .b        .w  .w  .w        .l  .l  .l
            CYCLES_AI   ( 0,  0,  0,        0,  0,  0,        0,  0, 24)
            CYCLES_DI   ( 0,  0,  0,        0,  0,  0,        0,  0, 25)
            CYCLES_IX   ( 0,  0,  0,        0,  0,  0,        0,  0, 27)
            CYCLES_AW   ( 0,  0,  0,        0,  0,  0,        0,  0, 24)
            CYCLES_AL   ( 0,  0,  0,        0,  0,  0,        0,  0, 24)
            break;

        case BFEXTS:
        case BFEXTU:

            data = CLIP<Long>(data << offset);

            if((offset + width) > 32) {
                data |= (readM<C, M, Byte>(ea + 4) << offset) >> 8;
            }

            result = bitfield<I>(data, offset, width, mask);
            writeD(dn, result);

            //           00  10  20        00  10  20        00  10  20
            //           .b  .b  .b        .w  .w  .w        .l  .l  .l
            CYCLES_AI   ( 0,  0,  0,        0,  0,  0,        0,  0, 19)
            CYCLES_DI   ( 0,  0,  0,        0,  0,  0,        0,  0, 20)
            CYCLES_IX   ( 0,  0,  0,        0,  0,  0,        0,  0, 22)
            CYCLES_AW   ( 0,  0,  0,        0,  0,  0,        0,  0, 19)
            CYCLES_AL   ( 0,  0,  0,        0,  0,  0,        0,  0, 19)
            CYCLES_DIPC ( 0,  0,  0,        0,  0,  0,        0,  0, 20)
            CYCLES_IXPC ( 0,  0,  0,        0,  0,  0,        0,  0, 22)
            break;

        case BFFFO:

            data = CLIP<Long>(data << offset);

            if((offset + width) > 32) {

                data |= (readM<C, M, Byte>(ea + 4) << offset) >> 8;
            }

            result = bitfield<I>(data, oldOffset, width, mask);
            writeD(dn, result);

            //           00  10  20        00  10  20        00  10  20
            //           .b  .b  .b        .w  .w  .w        .l  .l  .l
            CYCLES_AI   ( 0,  0,  0,        0,  0,  0,        0,  0, 32)
            CYCLES_DI   ( 0,  0,  0,        0,  0,  0,        0,  0, 33)
            CYCLES_IX   ( 0,  0,  0,        0,  0,  0,        0,  0, 35)
            CYCLES_AW   ( 0,  0,  0,        0,  0,  0,        0,  0, 32)
            CYCLES_AL   ( 0,  0,  0,        0,  0,  0,        0,  0, 32)
            CYCLES_DIPC ( 0,  0,  0,        0,  0,  0,        0,  0, 33)
            CYCLES_IXPC ( 0,  0,  0,        0,  0,  0,        0,  0, 35)
            break;

        case BFINS:
        {
            insert = readD(dn);
            insert = u32(insert << (32 - width));

            reg.sr.n = NBIT<S>(insert);
            reg.sr.z = ZERO<S>(insert);
            reg.sr.v = 0;
            reg.sr.c = 0;

            writeM<C, M, S>(ea, (data & ~mask) | insert >> offset);

            if((width + offset) > 32) {

                data = readM<C, M, Byte>(ea + 4);
                u8 insert8 = u8(readD(dn) << (8 - ((width + offset) - 32)));
                writeM<C, M, Byte>(ea + 4, (data & ~mask8) | insert8);
                reg.sr.z &= ZERO<Byte>(insert8);
            }

            //           00  10  20        00  10  20        00  10  20
            //           .b  .b  .b        .w  .w  .w        .l  .l  .l
            CYCLES_AI   ( 0,  0,  0,        0,  0,  0,        0,  0, 21)
            CYCLES_DI   ( 0,  0,  0,        0,  0,  0,        0,  0, 22)
            CYCLES_IX   ( 0,  0,  0,        0,  0,  0,        0,  0, 24)
            CYCLES_AW   ( 0,  0,  0,        0,  0,  0,        0,  0, 21)
            CYCLES_AL   ( 0,  0,  0,        0,  0,  0,        0,  0, 21)
            break;
        }
        case BFTST:

            (void)bitfield<I>(data, offset, width, mask);

            if((width + offset) > 32) {

                u8 data2 = u8(readM<C, M, Byte>(ea + 4));
                reg.sr.z &= ZERO<Byte>(data2 & mask8);
            }

            //           00  10  20        00  10  20        00  10  20
            //           .b  .b  .b        .w  .w  .w        .l  .l  .l
            CYCLES_AI   ( 0,  0,  0,        0,  0,  0,        0,  0, 17)
            CYCLES_DI   ( 0,  0,  0,        0,  0,  0,        0,  0, 18)
            CYCLES_IX   ( 0,  0,  0,        0,  0,  0,        0,  0, 20)
            CYCLES_AW   ( 0,  0,  0,        0,  0,  0,        0,  0, 17)
            CYCLES_AL   ( 0,  0,  0,        0,  0,  0,        0,  0, 17)
            CYCLES_DIPC ( 0,  0,  0,        0,  0,  0,        0,  0, 18)
            CYCLES_IXPC ( 0,  0,  0,        0,  0,  0,        0,  0, 20)
            break;

        default:
            fatalError;
    }

    prefetch<C, POLL>();

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execBkpt(u16 opcode)
{
    AVAILABILITY(C68010)

    if (MIMIC_MUSASHI) {

        execException<C>(EXC_ILLEGAL);

    } else {

        execException<C>(EXC_BKPT);
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP   ( 0,  0,  0,        0,  0,  0,        0, 38, 20 );

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execBsr(u16 opcode)
{
    AVAILABILITY(S == Long ? C68020 : C68000)

    u32 oldpc = reg.pc;
    u32 disp = S == Byte ? (u8)opcode : queue.irc;

    if (S == Long) {

        readExt<C>();
        disp = disp << 16 | queue.irc;
    }

    u32 newpc = U32_ADD(oldpc, SEXT<S>(disp));
    u32 retpc = U32_ADD(reg.pc, S == Long || S == Word ? 2 : 0);

    SYNC(2);

    if (C == C68000) {

        // Check for address errors
        if (misaligned<C>(reg.sp)) {
            reg.sp -= 4;
            throw AddressError(makeFrame<AE_WRITE|AE_DATA>(reg.sp));
        }
        if (misaligned<C>(newpc)) {
            throw AddressError(makeFrame(newpc));
        }

        // Save return address on stack
        push<C, Long>(retpc);

        // Jump to new address
        reg.pc = newpc;
        fullPrefetch<C, POLL>();

    } else {

        // Check for address errors
        if (misaligned<C>(reg.sp)) {
            writeBuffer = 0;
            throw AddressError(makeFrame<AE_WRITE|AE_DATA>(newpc));

        }
        if (misaligned<C>(newpc)) {
            throw AddressError(makeFrame(newpc));
        }

        // Save return address on stack
        push<C, Long, POLL>(retpc);

        // Jump to new address
        reg.pc = newpc;

        fullPrefetch<C>();
    }


    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP   (18, 18,  7,       18, 18,  7,       18, 18,  7)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCallm(u16 opcode)
{
    AVAILABILITY(C68020)

    int dst = ( _____________xxx(opcode) );
    u32 ea, data;

    readOp<C, M, Byte>(dst, &ea, &data);
    readExt<C>();
    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_AI   ( 0,  0,  0,        0,  0,  0,        0,  0, 64)
    CYCLES_DI   ( 0,  0,  0,        0,  0,  0,        0,  0, 65)
    CYCLES_IX   ( 0,  0,  0,        0,  0,  0,        0,  0, 67)
    CYCLES_AW   ( 0,  0,  0,        0,  0,  0,        0,  0, 64)
    CYCLES_AL   ( 0,  0,  0,        0,  0,  0,        0,  0, 64)
    CYCLES_DIPC ( 0,  0,  0,        0,  0,  0,        0,  0, 65)
    CYCLES_IXPC ( 0,  0,  0,        0,  0,  0,        0,  0, 67)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCas(u16 opcode)
{
    AVAILABILITY(C68020)

    int dst = _____________xxx(opcode);
    int dc  = _____________xxx(queue.irc);
    int du  = _______xxx______(queue.irc);

    u32 ea, data;

    readExt<C>();
    readOp<C, M, S, STD_AE_FRAME>(dst, &ea, &data);

    auto compare = readD(dc);

    // Set flags
    cmp<C, S>(CLIP<S>(compare), data);

    if (!reg.sr.z) {

        writeD(dc, CLEAR<S>(compare) | data);

        //           00  10  20        00  10  20        00  10  20
        //           .b  .b  .b        .w  .w  .w        .l  .l  .l
        CYCLES_AI   ( 0,  0, 16,        0,  0, 16,        0,  0, 16)
        CYCLES_PI   ( 0,  0, 16,        0,  0, 16,        0,  0, 16)
        CYCLES_PD   ( 0,  0, 17,        0,  0, 17,        0,  0, 17)
        CYCLES_DI   ( 0,  0, 17,        0,  0, 17,        0,  0, 17)
        CYCLES_IX   ( 0,  0, 19,        0,  0, 19,        0,  0, 19)
        CYCLES_AW   ( 0,  0, 16,        0,  0, 16,        0,  0, 16)
        CYCLES_AL   ( 0,  0, 16,        0,  0, 16,        0,  0, 16)

    } else {

        writeM<C, M, S>(ea, readD<S>(du));

        //           00  10  20        00  10  20        00  10  20
        //           .b  .b  .b        .w  .w  .w        .l  .l  .l
        CYCLES_AI   ( 0,  0, 19,        0,  0, 19,        0,  0, 19)
        CYCLES_PI   ( 0,  0, 19,        0,  0, 19,        0,  0, 19)
        CYCLES_PD   ( 0,  0, 20,        0,  0, 20,        0,  0, 20)
        CYCLES_DI   ( 0,  0, 20,        0,  0, 20,        0,  0, 20)
        CYCLES_IX   ( 0,  0, 22,        0,  0, 22,        0,  0, 22)
        CYCLES_AW   ( 0,  0, 19,        0,  0, 19,        0,  0, 19)
        CYCLES_AL   ( 0,  0, 19,        0,  0, 19,        0,  0, 19)
    }

    prefetch<C, POLL>();

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCas2(u16 opcode)
{
    AVAILABILITY(C68020)

    int dc1 = _____________xxx(queue.irc);
    int du1 = _______xxx______(queue.irc);
    int rn1 = xxxx____________(queue.irc);

    readExt<C>();

    int dc2 = _____________xxx(queue.irc);
    int du2 = _______xxx______(queue.irc);
    int rn2 = xxxx____________(queue.irc);

    readExt<C>();

    u32 ea1 = reg.r[rn1];
    u32 ea2 = reg.r[rn2];
    u32 data1 = readM<C, M, S>(ea1);
    u32 data2 = readM<C, M, S>(ea2);

    auto compare1 = readD(dc1);
    auto compare2 = readD(dc2);

    // Set flags
    cmp<C, S>(CLIP<S>(compare1), data1);

    if (reg.sr.z) {

        // Set flags
        cmp<C, S>(CLIP<S>(compare2), data2);
        if (reg.sr.z) {

            writeM<C, M, S>(ea1, reg.d[du1]);
            writeM<C, M, S>(ea2, reg.d[du2]);

            prefetch<C, POLL>();
            CYCLES_68020 (15);
            return;
        }
    }

    if (MIMIC_MUSASHI) {

        if (rn1 & 0x8) {
            writeD(dc1, SEXT<S>(data1));
        } else {
            writeD<S>(dc1, data1);
        }
        if (rn2 & 0x8) {
            writeD(dc2, SEXT<S>(data2));
        } else {
            writeD<S>(dc2, data2);
        }

    } else {

        writeD<S>(dc1, data1);
        writeD<S>(dc2, data2);
    }

    prefetch<C, POLL>();

    CYCLES_68020 (12)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execChk(u16 opcode)
{
    AVAILABILITY(S == Long ? C68020 : C68000)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);
    u32 ea = 0, data, dy;
    [[maybe_unused]] auto c = clock;

    try {

        readOp<C, M, S>(src, &ea, &data);

    } catch (const AddressError &) {

        // Rectify the stack frame
        if (C == C68000) {

            SYNC(2);
            throw AddressError(makeFrame<STD_AE_FRAME>(ea));

        } else {

            readBuffer = (u16)readM<C, M, S>(ea & ~1);
            updateAnPI<M, S>(src);
            if (isAbsMode(M) || M == MODE_AI || M == MODE_PI || M == MODE_PD) {

                SYNC(2);
                throw AddressError(makeFrame<AE_SET_RW|AE_SET_DF>(ea));

            } else {

                SYNC(2);
                throw AddressError(makeFrame<AE_DEC_PC|AE_SET_RW|AE_SET_DF>(ea));
            }
        }
    }
    dy = readD<S>(dst);

    SYNC_68000(6);

    if (MIMIC_MUSASHI) {

        reg.sr.z = ZERO<S>(dy);
        reg.sr.v = 0;
        reg.sr.c = 0;

    } else {

        reg.sr.n = 0;
        setUndefinedCHK<C, S>(SEXT<S>(data), SEXT<S>(dy));
    }

    if (SEXT<S>(dy) > SEXT<S>(data)) {

        switch (C) {

            case C68000:
            case C68020:

                SYNC(MIMIC_MUSASHI ? 10 - (int)(clock - c) : 2);
                break;

            case C68010:

                prefetch<C, POLL>();
                SYNC_68010(4);
                break;
        }
        reg.sr.n = NBIT<S>(dy);
        execException<C>(EXC_CHK);


        CYCLES_68000(40)
        CYCLES_68010(44)
        CYCLES_68020(40)
        return;
    }

    if (SEXT<S>(dy) < 0) {

        switch (C) {

            case C68000:
            case C68020:

                SYNC(MIMIC_MUSASHI ? 10 - (int)(clock - c) : 4);
                break;

            case C68010:

                prefetch<C, POLL>();
                SYNC_68010(6);
        }
        reg.sr.n = MIMIC_MUSASHI ? NBIT<S>(dy) : 1;
        execException<C>(EXC_CHK);

        CYCLES_68000(40)
        CYCLES_68010(44)
        CYCLES_68020(40)
        return;
    }

    prefetch<C, POLL>();
    SYNC_68010(4);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 0,  0,  0,       10,  8,  8,        0,  0,  8)
    CYCLES_AI   ( 0,  0,  0,       14, 12, 12,        0,  0, 12)
    CYCLES_PI   ( 0,  0,  0,       14, 12, 12,        0,  0, 12)
    CYCLES_PD   ( 0,  0,  0,       16, 14, 13,        0,  0, 13)
    CYCLES_DI   ( 0,  0,  0,       18, 16, 13,        0,  0, 13)
    CYCLES_IX   ( 0,  0,  0,       20, 18, 15,        0,  0, 15)
    CYCLES_AW   ( 0,  0,  0,       18, 16, 12,        0,  0, 12)
    CYCLES_AL   ( 0,  0,  0,       22, 20, 12,        0,  0, 12)
    CYCLES_DIPC ( 0,  0,  0,       18, 16, 13,        0,  0, 13)
    CYCLES_IXPC ( 0,  0,  0,       20, 18, 15,        0,  0, 15)
    CYCLES_IM   ( 0,  0,  0,       14, 12, 10,        0,  0, 12)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execChkCmp2(u16 opcode)
{
    AVAILABILITY(C68020)

    u32 ext = queue.irc;
    int src = _____________xxx(opcode);
    int dst = xxxx____________(ext);
    u32 ea, data1, data2;

    readExt<C>();

    readOp<C, M, S>(src, &ea, &data1);
    data2 = readM<C, M, S>(ea + S);

    if (MIMIC_MUSASHI) {

        auto bound1 = ((M == 9 || M == 10) && S == Byte) ? (i32)data1 : SEXT<S>(data1);
        auto bound2 = ((M == 9 || M == 10) && S == Byte) ? (i32)data2 : SEXT<S>(data2);
        i32 compare = readR<S>(dst);
        if (dst < 8) compare = SEXT<S>(compare);

        if (bound1 < bound2) {
            reg.sr.c = compare < bound1 || compare > bound2;
        } else {
            reg.sr.c = compare > bound2 || compare < bound1;
        }
        reg.sr.z = compare == bound1 || compare == bound2;

    } else {

        i32 bound1 = SEXT<S>(data1);
        i32 bound2 = SEXT<S>(data2);
        i32 compare = dst < 8 ? SEXT<S>(readR(dst)) : readR(dst);

        if (bound1 <= bound2) {
            reg.sr.c = compare < bound1 || compare > bound2;
        } else {
            reg.sr.c = compare < bound1 && compare > bound2;
        }
        reg.sr.z = compare == bound1 || compare == bound2;
        setUndefinedCHK2<C, S>(bound1, bound2, compare);
    }

    if ((ext & 0x800) && reg.sr.c) {

        execException<C>(EXC_CHK);
        CYCLES_68020(40)

    } else {

        prefetch<C, POLL>();

        //           00  10  20        00  10  20        00  10  20
        //           .b  .b  .b        .w  .w  .w        .l  .l  .l
        CYCLES_AI   ( 0,  0, 22,        0,  0, 22,        0,  0, 22)
        CYCLES_DI   ( 0,  0, 23,        0,  0, 23,        0,  0, 23)
        CYCLES_IX   ( 0,  0, 25,        0,  0, 25,        0,  0, 25)
        CYCLES_AW   ( 0,  0, 22,        0,  0, 22,        0,  0, 22)
        CYCLES_AL   ( 0,  0, 22,        0,  0, 22,        0,  0, 22)
        CYCLES_DIPC ( 0,  0, 23,        0,  0, 23,        0,  0, 23)
        CYCLES_IXPC ( 0,  0, 23,        0,  0, 23,        0,  0, 23)
    }

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execClr(u16 opcode)
{
    AVAILABILITY(C68000)

    if constexpr (C == C68000 || C == C68020) {

        int dst = _____________xxx(opcode);

        u32 ea, data;
        readOp<C, M, S, STD_AE_FRAME>(dst, &ea, &data);

        prefetch<C, POLL>();

        if constexpr (S == Long && isRegMode(M)) SYNC(2);

        if constexpr (MIMIC_MUSASHI) {
            writeOp<C, M, S>(dst, ea, 0);
        } else {
            writeOp<C, M, S, REVERSE>(dst, ea, 0);
        }

        reg.sr.n = 0;
        reg.sr.z = 1;
        reg.sr.v = 0;
        reg.sr.c = 0;
    }

    if constexpr (C == C68010) {

        int dst = _____________xxx(opcode);

        u32 ea = computeEA<C, M, S>(dst);

        switch (M) {

            case MODE_AI:

                writeOp<C, M, S, POLL | AE_INC_PC>(dst, ea, 0);
                looping<I>() ? noPrefetch<C>() : prefetch<C>();
                if (looping<I>()) loopModeDelay = 4;
                break;

            case MODE_PI:

                writeOp<C, M, S, POLL | AE_INC_PC>(dst, ea, 0);
                updateAnPI<M, S>(dst);
                looping<I>() ? noPrefetch<C>() : prefetch<C>();
                if (looping<I>()) loopModeDelay = 4;
                break;

            case MODE_PD:

                if constexpr (S == Long) {
                    writeOp<C, M, S, REVERSE | POLL | AE_INC_PC | AE_INC_A>(dst, ea, 0);
                } else {
                    writeOp<C, M, S, REVERSE | POLL | AE_INC_PC>(dst, ea, 0);
                }
                updateAnPD<M, S>(dst);
                looping<I>() ? noPrefetch<C>() : prefetch<C>();
                if (looping<I>()) loopModeDelay = 4;
                break;

            case MODE_DI:

                reg.sr.n = 0;
                reg.sr.z = 1;
                reg.sr.v = 0;
                reg.sr.c = 0;

                prefetch<C, POLL>();
                if constexpr (S == Long) {
                    writeOp<C, M, S, REVERSE | AE_INC_PC | AE_INC_A>(dst, ea, 0);
                } else {
                    writeOp<C, M, S, REVERSE | AE_INC_PC>(dst, ea, 0);
                }
                break;

            case MODE_IX:

                reg.sr.n = 0;
                reg.sr.z = 1;
                reg.sr.v = 0;
                reg.sr.c = 0;

                SYNC(2);
                prefetch<C,POLL>();
                if constexpr (S == Long) {
                    writeOp<C, M, S, REVERSE | AE_INC_PC | AE_INC_A>(dst, ea, 0);
                } else {
                    writeOp<C, M, S, REVERSE | AE_INC_PC>(dst, ea, 0);
                }
                break;

            case MODE_AW:
            case MODE_AL:

                prefetch<C, POLL>();
                writeOp<C, M, S, REVERSE>(dst, ea, 0);
                break;

            default:

                writeOp<C, M, S, REVERSE>(dst, ea, 0);
                prefetch<C, POLL>();
        }

        reg.sr.n = 0;
        reg.sr.z = 1;
        reg.sr.v = 0;
        reg.sr.c = 0;
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 4,  4,  2,        4,  4,  2,        6,  6,  2)
    CYCLES_AI   (12,  8,  8,       12,  8,  8,       20, 12,  8)
    CYCLES_PI   (12,  8,  8,       12,  8,  8,       20, 12,  8)
    CYCLES_PD   (14, 10,  9,       14, 10,  9,       22, 14,  9)
    CYCLES_DI   (16, 12,  9,       16, 12,  9,       24, 16,  9)
    CYCLES_IX   (18, 14, 11,       18, 14, 11,       26, 20, 11)
    CYCLES_AW   (16, 12,  8,       16, 12,  8,       24, 16,  8)
    CYCLES_AL   (20, 14,  8,       20, 14,  8,       28, 20,  8)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCmp(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea, data;
    readOp<C, M, S, STD_AE_FRAME>(src, &ea, &data);

    if (C == C68000) {

        cmp<C, S>(data, readD<S>(dst));
        prefetch<C, POLL>();

        if constexpr (S == Long) SYNC(2);

    } else {

        if constexpr (S == Long && isImmMode(M)) SYNC(2);
        if constexpr (S == Long && isRegMode(M)) SYNC(2);

        cmp<C, S>(data, readD<S>(dst));
        POLL_IPL;
        looping<I>() ? noPrefetch<C>(S == Long ? 4 : 2) : prefetch<C>();
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 4,  4,  2,        4,  4,  2,        6,  6,  2)
    CYCLES_AN   ( 0,  0,  0,        4,  4,  2,        6,  6,  2)
    CYCLES_AI   ( 8,  8,  6,        8,  8,  6,       14, 14,  6)
    CYCLES_PI   ( 8,  8,  6,        8,  8,  6,       14, 14,  6)
    CYCLES_PD   (10, 10,  7,       10, 10,  7,       16, 16,  7)
    CYCLES_DI   (12, 12,  7,       12, 12,  7,       18, 18,  7)
    CYCLES_IX   (14, 14,  9,       14, 14,  9,       20, 20,  9)
    CYCLES_AW   (12, 12,  6,       12, 12,  6,       18, 18,  6)
    CYCLES_AL   (16, 16,  6,       16, 16,  6,       22, 22,  6)
    CYCLES_DIPC (12, 12,  7,       12, 12,  7,       18, 18,  7)
    CYCLES_IXPC (14, 14,  9,       14, 14,  9,       20, 20,  9)
    CYCLES_IM   ( 8,  8,  4,        8,  8,  4,       14, 14,  6)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCmpa(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea, data;
    readOp<C, M, S, STD_AE_FRAME>(src, &ea, &data);

    data = SEXT<S>(data);
    cmp<C, Long>(data, readA(dst));

    if constexpr (C == C68000) {

        looping<I>() ? noPrefetch<C>() : prefetch<C, POLL>();
        SYNC(2);

    } else {

        SYNC(2);
        POLL_IPL;
        looping<I>() ? noPrefetch<C>(2) : prefetch<C>();
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 0,  0,  0,        6,  6,  4,        6,  6,  4)
    CYCLES_AN   ( 0,  0,  0,        6,  6,  4,        6,  6,  4)
    CYCLES_AI   ( 0,  0,  0,       10, 10,  8,       14, 14,  8)
    CYCLES_PI   ( 0,  0,  0,       10, 10,  8,       14, 14,  8)
    CYCLES_PD   ( 0,  0,  0,       12, 12,  9,       16, 16,  9)
    CYCLES_DI   ( 0,  0,  0,       14, 14,  9,       18, 18,  9)
    CYCLES_IX   ( 0,  0,  0,       16, 16, 11,       20, 20, 11)
    CYCLES_AW   ( 0,  0,  0,       14, 14,  8,       18, 18,  8)
    CYCLES_AL   ( 0,  0,  0,       18, 18,  8,       22, 22,  8)
    CYCLES_DIPC ( 0,  0,  0,       14, 14,  9,       18, 18,  9)
    CYCLES_IXPC ( 0,  0,  0,       16, 16, 11,       20, 20, 11)
    CYCLES_IM   ( 0,  0,  0,       10, 10,  6,       14, 14,  8)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCmpiRg(u16 opcode)
{
    AVAILABILITY(C68000)

    u32 src = readI<C, S>();
    int dst = _____________xxx(opcode);

    prefetch<C, POLL>();

    if constexpr (S == Long && C == C68000) SYNC(2);
    cmp<C, S>(src, readD<S>(dst));

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   (8,   8,  2,        8,  8,  2,       14, 12,  2)
    CYCLES_AI   (12, 12,  6,       12, 12,  6,       20, 20,  6)
    CYCLES_PI   (12, 12,  6,       12, 12,  6,       20, 20,  6)
    CYCLES_PD   (14, 14,  7,       14, 14,  7,       22, 22,  7)
    CYCLES_DI   (16, 16,  7,       16, 16,  7,       24, 24,  7)
    CYCLES_IX   (18, 18,  9,       18, 18,  9,       26, 26,  9)
    CYCLES_AW   (16, 16,  6,       16, 16,  6,       24, 24,  6)
    CYCLES_AL   (20, 20,  6,       20, 20,  6,       28, 28,  6)
    CYCLES_DIPC ( 0,  0,  7,        0,  0,  7,        0,  0,  7)
    CYCLES_IXPC ( 0,  0,  9,        0,  0,  9,        0,  0,  9)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCmpiEa(u16 opcode)
{
    AVAILABILITY(C68000)

    u32 src = readI<C, S>();
    int dst = _____________xxx(opcode);

    u32 ea, data;
    readOp<C, M, S, STD_AE_FRAME>(dst, &ea, &data);
    prefetch<C, POLL>();

    cmp<C, S>(src, data);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   (8,   8,  2,        8,  8,  2,       14, 12,  2)
    CYCLES_AI   (12, 12,  6,       12, 12,  6,       20, 20,  6)
    CYCLES_PI   (12, 12,  6,       12, 12,  6,       20, 20,  6)
    CYCLES_PD   (14, 14,  7,       14, 14,  7,       22, 22,  7)
    CYCLES_DI   (16, 16,  7,       16, 16,  7,       24, 24,  7)
    CYCLES_IX   (18, 18,  9,       18, 18,  9,       26, 26,  9)
    CYCLES_AW   (16, 16,  6,       16, 16,  6,       24, 24,  6)
    CYCLES_AL   (20, 20,  6,       20, 20,  6,       28, 28,  6)
    CYCLES_DIPC ( 0, 16,  7,        0, 16,  7,        0, 24,  7)
    CYCLES_IXPC ( 0, 18,  9,        0, 18,  9,        0, 26,  9)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCmpm(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea1, ea2, data1, data2;

    if (C == C68000) {

        readOp<C, M, S, AE_INC_PC>(src, &ea1, &data1);
        POLL_IPL;
        readOp<C, M, S, AE_INC_PC>(dst, &ea2, &data2);

        cmp<C, S>(data1, data2);
        prefetch<C>();

    } else {

        readOp<C, M, S, AE_INC_PC>(src, &ea1, &data1);
        readOp<C, M, S, AE_INC_PC>(dst, &ea2, &data2);
        POLL_IPL;

        cmp<C, S>(data1, data2);
        looping<I>() ? noPrefetch<C>(S == Long ? 2 : 0) : prefetch<C>();
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_PI   (12, 12,  9,       12, 12,  9,       20, 20,  9)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCpBcc(u16 opcode)
{
    AVAILABILITY(C68020)

    execLineF<C, I, M, S>(opcode);

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCpDbcc(u16 opcode)
{
    AVAILABILITY(C68020)

    execLineF<C, I, M, S>(opcode);

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCpGen(u16 opcode)
{
    AVAILABILITY(C68020)

    execLineF<C, I, M, S>(opcode);

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCpRestore(u16 opcode)
{
    AVAILABILITY(C68020)
    SUPERVISOR_MODE_ONLY

    execLineF<C, I, M, S>(opcode);

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCpRestoreInvalid(u16 opcode)
{
    AVAILABILITY(C68020)

    execLineF<C, I, M, S>(opcode);

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCpSave(u16 opcode)
{
    AVAILABILITY(C68020)
    SUPERVISOR_MODE_ONLY

    execLineF<C, I, M, S>(opcode);

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCpScc(u16 opcode)
{
    AVAILABILITY(C68020)

    execLineF<C, I, M, S>(opcode);

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCpTrapcc(u16 opcode)
{
    AVAILABILITY(C68020)

    execLineF<C, I, M, S>(opcode);

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execDbcc(u16 opcode)
{
    AVAILABILITY(C68000)

    auto exec68000 = [&]() {

        SYNC(2);
        if (!cond<I>()) {

            int dn = _____________xxx(opcode);
            u32 newpc = U32_ADD(reg.pc, (i16)queue.irc);

            bool takeBranch = readD<Word>(dn) != 0;

            // Check for address error
            if (misaligned<C, S>(newpc)) {
                throw AddressError(makeFrame<AE_INC_PC>(newpc, newpc));
            }

            // Decrement loop counter
            writeD<Word>(dn, U32_SUB(readD<Word>(dn), 1));

            // Branch
            if (takeBranch) {
                reg.pc = newpc;
                fullPrefetch<C, POLL>();
                CYCLES_68000(10);
                CYCLES_68020(6);

            } else {

                (void)read<C, MEM_PROG, Word>(reg.pc + 2);
                CYCLES_68000(14);
                CYCLES_68020(10);

                reg.pc += 2;
                fullPrefetch<C, POLL>();
            }

        } else {

            SYNC(2);
            CYCLES_68000(12);
            CYCLES_68020(6);

            // Fall through to next instruction
            reg.pc += 2;
            fullPrefetch<C, POLL>();
        }
    };

    auto exec68010 = [&]() {

        if (!cond<I>()) {

            SYNC(2);

            int dn = _____________xxx(opcode);
            i16 disp = (i16)queue.irc;

            u32 newpc = U32_ADD(reg.pc, disp);

            bool takeBranch = readD<Word>(dn) != 0;

            // Check for address error
            if (misaligned<C, S>(newpc)) {
                throw AddressError(makeFrame<AE_INC_PC>(newpc, newpc));
            }

            // Decrement loop counter
            writeD<Word>(dn, U32_SUB(readD<Word>(dn), 1));

            // Branch
            if (takeBranch) {

                reg.pc = newpc;
                fullPrefetch<C, POLL>();

                if (loop[queue.ird] && disp == -4) {

                    // Enter loop mode
                    flags |= CPU_IS_LOOPING;
                    queue.irc = opcode;
                }

                if (MIMIC_MUSASHI) SYNC(2);
                CYCLES_68010(12);
                return;

            } else {

                (void)read<C, MEM_PROG, Word>(reg.pc + 2);
                SYNC(MIMIC_MUSASHI ? 4 : 2);
                CYCLES_68010(8);

                reg.pc += 2;
                fullPrefetch<C, POLL>();
            }

        } else {

            SYNC(2);
            CYCLES_68010(2);

            reg.pc += 2;
            fullPrefetch<C, POLL>();
        }

        CYCLES_68010(10);
    };

    auto execLoop = [&]() {

        if (!cond<I>()) {

            int dn = _____________xxx(opcode);
            u32 newpc = U32_ADD(reg.pc, -4);

            bool takeBranch = readD<Word>(dn) != 0;

            // Check for address error
            if (misaligned<C, S>(newpc)) {
                throw AddressError(makeFrame<AE_INC_PC>(newpc, newpc));
            }

            // Decrement loop counter
            writeD<Word>(dn, U32_SUB(readD<Word>(dn), 1));

            // Branch
            if (takeBranch) {

                SYNC(6);
                reg.pc = newpc;
                reg.pc0 = reg.pc;
                queue.ird = queue.irc;
                queue.irc = opcode;
                POLL_IPL;
                return;

            } else {

                // Fall through to next instruction
                if (loopModeDelay) SYNC(loopModeDelay);
                loopModeDelay = 2;
                reg.pc += 2;
                fullPrefetch<C, POLL>();
                flags &= ~CPU_IS_LOOPING;
            }

        } else {

            SYNC(4);
            // Fall through to next instruction
            reg.pc += 2;
            fullPrefetch<C, POLL>();
            flags &= ~CPU_IS_LOOPING;
        }
    };

    switch (C) {

        case C68000: exec68000(); break;
        case C68010: looping<I>() ? execLoop() : exec68010(); break;
        case C68020: exec68000(); break;

        default:
            fatalError;
    }

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execExgDxDy(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    std::swap(reg.d[src], reg.d[dst]);

    prefetch<C, POLL>();
    SYNC(2);

    CYCLES_IP ( 0,  0,  0,       0,  0,  0,      6,  6,  2)

    FINALIZE

}

template <Core C, Instr I, Mode M, Size S> void
Moira::execExgAxDy(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    std::swap(reg.a[src], reg.d[dst]);

    prefetch<C, POLL>();
    SYNC(2);

    CYCLES_IP ( 0,  0,  0,       0,  0,  0,      6,  6,  2)

    FINALIZE

}

template <Core C, Instr I, Mode M, Size S> void
Moira::execExgAxAy(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    std::swap(reg.a[src], reg.a[dst]);

    prefetch<C, POLL>();
    SYNC(2);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP   ( 0,  0,  0,        0,  0,  0,        6,  6,  2)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execExt(u16 opcode)
{
    AVAILABILITY(C68000)

    int n = _____________xxx(opcode);

    u32 dn = readD(n);
    dn = (S == Long) ? SEXT<Word>(dn) : SEXT<Byte>(dn);

    writeD<S>(n, dn);
    reg.sr.n = NBIT<S>(dn);
    reg.sr.z = ZERO<S>(dn);
    reg.sr.v = 0;
    reg.sr.c = 0;

    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 0,  0,  0,        4,  4,  4,        4,  4,  4)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execExtb(u16 opcode)
{
    AVAILABILITY(C68020)

    int n = _____________xxx(opcode);

    u32 dn = readD(n);
    dn = SEXT<Byte>(dn);

    writeD(n, dn);
    reg.sr.n = NBIT<S>(dn);
    reg.sr.z = ZERO<S>(dn);
    reg.sr.v = 0;
    reg.sr.c = 0;

    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 0,  0,  0,        0,  0,  0,        0,  0,  4)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execJmp(u16 opcode)
{
    AVAILABILITY(C68000)

    u32 oldpc = reg.pc;

    int src = _____________xxx(opcode);
    u32 ea  = computeEA<C, M, Long, SKIP_LAST_RD>(src);

    [[maybe_unused]] const int delay[] = { 0,0,0,0,0,2,4,2,0,2,4,0 };
    SYNC(delay[M]);

    // Check for address error
    if (misaligned<C, Word>(ea)) {
        throw AddressError(makeFrame(ea, oldpc));
    }

    // Jump to new address
    reg.pc = ea;

    // Fill the prefetch queue
    fullPrefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_AI   ( 0,  0,  0,        0,  0,  0,        8,  8,  4)
    CYCLES_DI   ( 0,  0,  0,        0,  0,  0,       10, 10,  5)
    CYCLES_IX   ( 0,  0,  0,        0,  0,  0,       14, 14,  7)
    CYCLES_AW   ( 0,  0,  0,        0,  0,  0,       10, 10,  4)
    CYCLES_AL   ( 0,  0,  0,        0,  0,  0,       12, 12,  4)
    CYCLES_DIPC ( 0,  0,  0,        0,  0,  0,       10, 10,  5)
    CYCLES_IXPC ( 0,  0,  0,        0,  0,  0,       14, 14,  7)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execJsr(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = _____________xxx(opcode);
    u32 ea  = computeEA<C, M, Long, SKIP_LAST_RD>(src);

    [[maybe_unused]] const int delay[] = { 0,0,0,0,0,2,4,2,0,2,4,0 };
    SYNC(delay[M]);

    switch (C) {

        case C68000:

            // Check for address errors
            if (isDspMode(M) && misaligned<C>(ea)) {
                throw AddressError(makeFrame<AE_DEC_PC>(ea));
            }
            if (misaligned<C>(ea)) {
                throw AddressError(makeFrame(ea));
            }

            // Save return address on stack
            push<C, Long>(reg.pc);

            // Jump to new address
            reg.pc = ea;

            queue.irc = (u16)read<C, MEM_PROG, Word>(ea);
            prefetch<C, POLL>();
            break;

        default:

            // Check for address errors
            if (misaligned<C>(ea) && misaligned<C>(reg.sp)) {

                if (M == MODE_AI) {

                    queue.irc = (u16)read<C, MEM_PROG, Word>(ea & ~1);
                    throw AddressError(makeFrame<AE_SET_IF|AE_SET_RW>(ea));
                }

                if (isAbsMode(M)) {

                    auto frame = makeFrame<AE_SET_IF|AE_SET_RW>(ea);
                    frame.pc -= 4;
                    throw AddressError(frame);
                }
                if (isDspMode(M)) {

                    throw AddressError(makeFrame<AE_DEC_PC|AE_SET_IF|AE_SET_RW>(ea));

                } else {

                    throw AddressError(makeFrame(ea));
                }
            }

            if (misaligned<C>(ea)) {

                if (isDspMode(M)) {
                    throw AddressError(makeFrame<AE_SET_IF|AE_SET_RW>(ea));
                } else {
                    throw AddressError(makeFrame(ea));
                }
            }

            if (misaligned<C>(reg.sp)) {

                prefetch<C>();
                reg.sp -= 4;
                writeBuffer = u16(reg.pc >> 16);
                throw AddressError(makeFrame<AE_DATA>(reg.sp));
            }

            // Save return address on stack
            push<C, Long, POLL>(reg.pc);

            // Jump to new address
            reg.pc = ea;

            queue.irc = (u16)read<C, MEM_PROG, Word>(ea);
            prefetch<C>();
            break;
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_AI   ( 0,  0,  0,        0,  0,  0,       16, 16,  4)
    CYCLES_DI   ( 0,  0,  0,        0,  0,  0,       18, 18,  5)
    CYCLES_IX   ( 0,  0,  0,        0,  0,  0,       22, 22,  7)
    CYCLES_AW   ( 0,  0,  0,        0,  0,  0,       18, 18,  4)
    CYCLES_AL   ( 0,  0,  0,        0,  0,  0,       20, 20,  4)
    CYCLES_DIPC ( 0,  0,  0,        0,  0,  0,       18, 18,  5)
    CYCLES_IXPC ( 0,  0,  0,        0,  0,  0,       22, 22,  7)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execLea(u16 opcode)
{
    AVAILABILITY(C68000)
    
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    reg.a[dst] = computeEA<C, M, S>(src);
    if (isIdxMode(M)) SYNC(2);

    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 0,  0,  0,        0,  0,  0,        0,  0,  0)
    CYCLES_AN   ( 0,  0,  0,        0,  0,  0,        0,  0,  0)
    CYCLES_AI   ( 0,  0,  0,        0,  0,  0,        4,  4,  6)
    CYCLES_PI   ( 0,  0,  0,        0,  0,  0,        0,  0,  0)
    CYCLES_PD   ( 0,  0,  0,        0,  0,  0,        0,  0,  0)
    CYCLES_DI   ( 0,  0,  0,        0,  0,  0,        8,  8,  7)
    CYCLES_IX   ( 0,  0,  0,        0,  0,  0,       12, 12,  9)
    CYCLES_AW   ( 0,  0,  0,        0,  0,  0,        8,  8,  6)
    CYCLES_AL   ( 0,  0,  0,        0,  0,  0,       12, 12,  6)
    CYCLES_DIPC ( 0,  0,  0,        0,  0,  0,        8,  8,  7)
    CYCLES_IXPC ( 0,  0,  0,        0,  0,  0,       12, 12,  9)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execLink(u16 opcode)
{
    AVAILABILITY(S == Long ? C68020 : C68000)

    u16 ird  = getIRD();
    u32 sp   = getSP() - 4;

    int ax   = _____________xxx(opcode);
    i32 disp = SEXT <S> (readI<C, S>());

    // Check for address error
    if (misaligned<C>(sp)) {

        writeBuffer = u16(readA(ax) >> 16);
        writeA(ax, sp);
        throw AddressError(makeFrame<AE_DATA|AE_WRITE>(sp, getPC() + 2, getSR(), ird));
    }

    POLL_IPL;

    // Write to stack
    push <C, Long>(readA(ax) - ((MIMIC_MUSASHI && ax == 7) ? 4 : 0));

    // Modify address register and stack pointer
    writeA(ax, sp);
    reg.sp = U32_ADD(reg.sp, disp);

    prefetch<C>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP   ( 0,  0,  0,       16, 16,  5,        0,  0,  6);

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove0(u16 opcode)
{
    AVAILABILITY(C68000)

    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    readOp<C, M, S, STD_AE_FRAME>(src, &ea, &data);

    reg.sr.n = NBIT<S>(data);
    reg.sr.z = ZERO<S>(data);
    reg.sr.v = 0;
    reg.sr.c = 0;

    writeOp<C, MODE_DN, S>(dst, data);

    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 4,  4,  2,        4,  4,  2,        4,  4,  2)
    CYCLES_AN   ( 4,  4,  2,        4,  4,  2,        4,  4,  2)
    CYCLES_AI   ( 8,  8,  6,        8,  8,  6,       12, 12,  6)
    CYCLES_PI   ( 8,  8,  6,        8,  8,  6,       12, 12,  6)
    CYCLES_PD   (10, 10,  7,       10, 10,  7,       14, 14,  7)
    CYCLES_DI   (12, 12,  7,       12, 12,  7,       16, 16,  7)
    CYCLES_IX   (14, 14,  9,       14, 14,  9,       18, 18,  9)
    CYCLES_AW   (12, 12,  6,       12, 12,  6,       16, 16,  6)
    CYCLES_AL   (16, 16,  6,       16, 16,  6,       20, 20,  6)
    CYCLES_DIPC (12, 12,  7,       12, 12,  7,       16, 16,  7)
    CYCLES_IXPC (14, 14,  9,       14, 14,  9,       18, 18,  9)
    CYCLES_IM   ( 8,  8,  4,        8,  8,  4,       12, 12,  6)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove2(u16 opcode)
{
    AVAILABILITY(C68000)

    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    readOp<C, M, S, STD_AE_FRAME>(src, &ea, &data);

    if constexpr (C == C68000) {

        if constexpr (!isMemMode(M) && S == Long) {

            writeOp<C, MODE_AI, S, AE_INC_PC|POLL>(dst, data);

            reg.sr.n = NBIT<S>(data);
            reg.sr.z = ZERO<S>(data);
            reg.sr.v = 0;
            reg.sr.c = 0;

            looping<I>() ? noPrefetch<C>() : prefetch<C>();

        } else {

            reg.sr.n = NBIT<Word>(data);
            reg.sr.z = ZERO<Word>(data);
            reg.sr.v = 0;
            reg.sr.c = 0;

            writeOp<C, MODE_AI, S, AE_INC_PC>(dst, data);

            reg.sr.n = NBIT<S>(data);
            reg.sr.z = ZERO<S>(data);

            looping<I>() ? noPrefetch<C>() : prefetch<C, POLL>();
        }

    } else {

        if constexpr (!isMemMode(M)) {

            POLL_IPL;

            writeOp<C, MODE_AI, S, AE_INC_PC>(dst, data);

            reg.sr.n = NBIT<S>(data);
            reg.sr.z = ZERO<S>(data);
            reg.sr.v = 0;
            reg.sr.c = 0;

            looping<I>() ? noPrefetch<C>() : prefetch<C>();

        } else {

            reg.sr.n = NBIT<Word>(data);
            reg.sr.z = ZERO<Word>(data);
            reg.sr.v = 0;
            reg.sr.c = 0;

            writeOp<C, MODE_AI, S, AE_INC_PC>(dst, data);

            reg.sr.n = NBIT<S>(data);
            reg.sr.z = ZERO<S>(data);

            looping<I>() ? noPrefetch<C>() : prefetch<C, POLL>();
            if (looping<I>() && S == Long) loopModeDelay = 0;
        }
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 8,  8,  4,        8,  8,  4,       12, 12,  4)
    CYCLES_AN   ( 8,  8,  4,        8,  8,  4,       12, 12,  4)
    CYCLES_AI   (12, 12,  8,       12, 12,  8,       20, 20,  8)
    CYCLES_PI   (12, 12,  8,       12, 12,  8,       20, 20,  8)
    CYCLES_PD   (14, 14,  9,       14, 14,  9,       22, 22,  9)
    CYCLES_DI   (16, 16,  9,       16, 16,  9,       24, 24,  9)
    CYCLES_IX   (18, 18, 11,       18, 18, 11,       26, 26, 11)
    CYCLES_AW   (16, 16,  8,       16, 16,  8,       24, 24,  8)
    CYCLES_AL   (20, 20,  8,       20, 20,  8,       28, 28,  8)
    CYCLES_DIPC (16, 16,  9,       16, 16,  9,       24, 24,  9)
    CYCLES_IXPC (18, 18, 11,       18, 18, 11,       26, 26, 11)
    CYCLES_IM   (12, 12,  6,       12, 12,  6,       20, 20,  8)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove3(u16 opcode)
{
    AVAILABILITY(C68000)

    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    readOp<C, M, S, STD_AE_FRAME>(src, &ea, &data);

    if constexpr (C == C68000) {

        if constexpr (S == Long && !isMemMode(M)) {

            writeOp<C, MODE_PI, S, AE_INC_PC|POLL>(dst, data);

            reg.sr.n = NBIT<S>(data);
            reg.sr.z = ZERO<S>(data);
            reg.sr.v = 0;
            reg.sr.c = 0;

            looping<I>() ? noPrefetch<C>() : prefetch<C>();

        } else {

            reg.sr.n = NBIT<Word>(data);
            reg.sr.z = ZERO<Word>(data);
            reg.sr.v = 0;
            reg.sr.c = 0;

            writeOp<C, MODE_PI, S, AE_INC_PC|POLL>(dst, data);

            reg.sr.n = NBIT<S>(data);
            reg.sr.z = ZERO<S>(data);

            looping<I>() ? noPrefetch<C>() : prefetch<C>();
        }

    } else {

        if constexpr (!isMemMode(M)) {

            POLL_IPL;
            writeOp<C, MODE_PI, S, AE_INC_PC>(dst, data);

            reg.sr.n = NBIT<S>(data);
            reg.sr.z = ZERO<S>(data);
            reg.sr.v = 0;
            reg.sr.c = 0;

            looping<I>() ? noPrefetch<C>() : prefetch<C>();

        } else {

            reg.sr.n = NBIT<Word>(data);
            reg.sr.z = ZERO<Word>(data);
            reg.sr.v = 0;
            reg.sr.c = 0;

            writeOp<C, MODE_PI, S, AE_INC_PC|POLL>(dst, data);
            looping<I>() ? noPrefetch<C>() : prefetch<C>();
            if (looping<I>() && S == Long) loopModeDelay = 0;

            reg.sr.n = NBIT<S>(data);
            reg.sr.z = ZERO<S>(data);
        }
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 8,  8,  4,        8,  8,  4,       12, 12,  4)
    CYCLES_AN   ( 8,  8,  4,        8,  8,  4,       12, 12,  4)
    CYCLES_AI   (12, 12,  8,       12, 12,  8,       20, 20,  8)
    CYCLES_PI   (12, 12,  8,       12, 12,  8,       20, 20,  8)
    CYCLES_PD   (14, 14,  9,       14, 14,  9,       22, 22,  9)
    CYCLES_DI   (16, 16,  9,       16, 16,  9,       24, 24,  9)
    CYCLES_IX   (18, 18, 11,       18, 18, 11,       26, 26, 11)
    CYCLES_AW   (16, 16,  8,       16, 16,  8,       24, 24,  8)
    CYCLES_AL   (20, 20,  8,       20, 20,  8,       28, 28,  8)
    CYCLES_DIPC (16, 16,  9,       16, 16,  9,       24, 24,  9)
    CYCLES_IXPC (18, 18, 11,       18, 18, 11,       26, 26, 11)
    CYCLES_IM   (12, 12,  6,       12, 12,  6,       20, 20,  8)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove4(u16 opcode)
{
    AVAILABILITY(C68000)

    u16 ird = getIRD();
    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    /* Source: http://pasti.fxatari.com/68kdocs/68kPrefetch.html
     *
     * "When the destination addressing mode is pre-decrement, steps 4 and 5
     *  above are inverted. So it behaves like a read modify instruction and it
     *  is a class 0 instruction. Note: The behavior is the same disregarding
     *  transfer size (byte, word or long), and disregarding the source
     *  addressing mode."
     */
    readOp<C, M, S, STD_AE_FRAME>(src, &ea, &data);

    // Determine next address error stack frame format
    const u64 flags0 = AE_WRITE | AE_DATA;
    const u64 flags1 = AE_WRITE | AE_DATA | AE_SET_CB3;
    const u64 flags2 = AE_WRITE | AE_DATA;
    int format = (S == Long) ? 0 : (reg.sr.s || reg.sr.c) ? 2 : 1;

    reg.sr.n = NBIT<S>(data);
    reg.sr.z = ZERO<S>(data);
    reg.sr.v = 0;
    reg.sr.c = 0;

    looping<I>() ? noPrefetch<C>(2) : prefetch<C, POLL>();
    if (looping<I>() && S == Long) loopModeDelay = 0;

    ea = computeEA<C, MODE_PD, S, IMPL_DEC>(dst);
    writeBuffer = u16(data);

    // Check for address error
    if (misaligned<C, S>(ea)) {

        if constexpr (S != Long) updateAn<MODE_PD, S>(dst);
        if (format == 0) { throw AddressError(makeFrame<flags0>(ea + 2, reg.pc + 2, getSR(), ird)); }
        if (format == 1) { SYNC(2); throw AddressError(makeFrame<flags1>(ea, reg.pc + 2)); }
        if (format == 2) { SYNC(2); throw AddressError(makeFrame<flags2>(ea, reg.pc + 2)); }
    }

    writeM<C, MODE_PD, S, REVERSE>(ea, data);
    updateAn<MODE_PD, S>(dst);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 8,  8,  5,        8,  8,  5,       12, 14,  5)
    CYCLES_AN   ( 0,  0,  0,        8,  8,  5,       12, 14,  5)
    CYCLES_AI   (12, 12,  9,       12, 12,  9,       20, 22,  9)
    CYCLES_PI   (12, 12,  9,       12, 12,  9,       20, 22,  9)
    CYCLES_PD   (14, 14, 10,       14, 14, 10,       22, 24, 10)
    CYCLES_DI   (16, 16, 10,       16, 16, 10,       24, 26, 10)
    CYCLES_IX   (18, 18, 12,       18, 18, 12,       26, 28, 12)
    CYCLES_AW   (16, 16,  9,       16, 16,  9,       24, 26,  9)
    CYCLES_AL   (20, 20,  9,       20, 20,  9,       28, 30,  9)
    CYCLES_DIPC (16, 16, 10,       16, 16, 10,       24, 26, 10)
    CYCLES_IXPC (18, 18, 12,       18, 18, 12,       26, 28, 12)
    CYCLES_IM   (12, 12,  7,       12, 12,  7,       20, 22,  9)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove5(u16 opcode)
{
    AVAILABILITY(C68000)

    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    readOp<C, M, S, STD_AE_FRAME>(src, &ea, &data);

    if constexpr (S == Long && !isMemMode(M)) {

        reg.sr.n = NBIT<Word>(data >> 16);
        reg.sr.z = ZERO<Word>(data >> 16) && reg.sr.z;

        writeOp<C, MODE_DI, S, POLL>(dst, data);

        reg.sr.n = NBIT<S>(data);
        reg.sr.z = ZERO<S>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        prefetch<C>();

    } else {

        reg.sr.n = NBIT<S>(data);
        reg.sr.z = ZERO<S>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        writeOp<C, MODE_DI, S>(dst, data);
        prefetch<C, POLL>();
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   (12, 12,  5,       12, 12,  5,       16, 16,  5)
    CYCLES_AN   (12, 12,  5,       12, 12,  5,       16, 16,  5)
    CYCLES_AI   (16, 16,  9,       16, 16,  9,       24, 24,  9)
    CYCLES_PI   (16, 16,  9,       16, 16,  9,       24, 24,  9)
    CYCLES_PD   (18, 18, 10,       18, 18, 10,       26, 26, 10)
    CYCLES_DI   (20, 20, 10,       20, 20, 10,       28, 28, 10)
    CYCLES_IX   (22, 22, 12,       22, 22, 12,       30, 30, 12)
    CYCLES_AW   (20, 20,  9,       20, 20,  9,       28, 28,  9)
    CYCLES_AL   (24, 24,  9,       24, 24,  9,       32, 32,  9)
    CYCLES_DIPC (20, 20, 10,       20, 20, 10,       28, 28, 10)
    CYCLES_IXPC (22, 22, 12,       22, 22, 12,       30, 30, 12)
    CYCLES_IM   (16, 16,  7,       16, 16,  7,       24, 24,  9)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove6(u16 opcode)
{
    AVAILABILITY(C68000)

    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    readOp<C, M, S, STD_AE_FRAME>(src, &ea, &data);

    if constexpr (S == Long && !isMemMode(M)) {

        reg.sr.n = NBIT<Word>(data >> 16);
        reg.sr.z = ZERO<Word>(data >> 16) && reg.sr.z;

        writeOp<C, MODE_IX, S, POLL>(dst, data);

        reg.sr.n = NBIT<S>(data);
        reg.sr.z = ZERO<S>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        prefetch<C>();

    } else {

        reg.sr.n = NBIT<S>(data);
        reg.sr.z = ZERO<S>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        writeOp<C, MODE_IX, S>(dst, data);
        prefetch<C, POLL>();
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   (14, 14,  7,       14, 14,  7,       18, 18,  7)
    CYCLES_AN   (14, 14,  7,       14, 14,  7,       18, 18,  7)
    CYCLES_AI   (18, 18, 11,       18, 18, 11,       26, 26, 11)
    CYCLES_PI   (18, 18, 11,       18, 18, 11,       26, 26, 11)
    CYCLES_PD   (20, 20, 12,       20, 20, 12,       28, 28, 12)
    CYCLES_DI   (22, 22, 12,       22, 22, 12,       30, 30, 12)
    CYCLES_IX   (24, 24, 14,       24, 24, 14,       32, 32, 14)
    CYCLES_AW   (22, 22, 11,       22, 22, 11,       30, 30, 11)
    CYCLES_AL   (26, 26, 11,       26, 26, 11,       34, 34, 11)
    CYCLES_DIPC (22, 22, 12,       22, 22, 12,       30, 30, 12)
    CYCLES_IXPC (24, 24, 14,       24, 24, 14,       32, 32, 14)
    CYCLES_IM   (18, 18,  9,       18, 18,  9,       26, 26, 11)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove7(u16 opcode)
{
    AVAILABILITY(C68000)

    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    readOp<C, M, S, STD_AE_FRAME>(src, &ea, &data);

    reg.sr.n = NBIT<S>(u64(data));
    reg.sr.z = ZERO<S>(u64(data));
    reg.sr.v = 0;
    reg.sr.c = 0;

    writeOp<C, MODE_AW, S>(dst, data);
    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   (12, 12,  4,       12, 12,  4,       16, 16,  4)
    CYCLES_AN   (12, 12,  4,       12, 12,  4,       16, 16,  4)
    CYCLES_AI   (16, 16,  8,       16, 16,  8,       24, 24,  8)
    CYCLES_PI   (16, 16,  8,       16, 16,  8,       24, 24,  8)
    CYCLES_PD   (18, 18,  9,       18, 18,  9,       26, 26,  9)
    CYCLES_DI   (20, 20,  9,       20, 20,  9,       28, 28,  9)
    CYCLES_IX   (22, 22, 11,       22, 22, 11,       30, 30, 11)
    CYCLES_AW   (20, 20,  8,       20, 20,  8,       28, 28,  8)
    CYCLES_AL   (24, 24,  8,       24, 24,  8,       32, 32,  8)
    CYCLES_DIPC (20, 20,  9,       20, 20,  9,       28, 28,  9)
    CYCLES_IXPC (22, 22, 11,       22, 22, 11,       30, 30, 11)
    CYCLES_IM   (16, 16,  6,       16, 16,  6,       24, 24,  8)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove8(u16 opcode)
{
    AVAILABILITY(C68000)

    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    /* Source: http://pasti.fxatari.com/68kdocs/68kPrefetch.html
     *
     * "When the destination addressing mode is long absolute and the source
     *  operand is any memory addr.mode, step 4 is interleaved in the middle of
     *  step 3. Step 3 only performs a single prefetch in this case. The other
     *  prefetch cycle that is normally performed at that step is deferred
     *  after the write cycles. So, two prefetch cycles are performed after the
     *  write ones. It is a class 2 instruction. Note: The behavior is the same
     *  disregarding transfer size (byte, word or long). But if the source
     *  operand is a data or address register, or immediate, then the behavior
     *  is the same as other MOVE variants (class 1 instruction)."
     */
    if (isMemMode(M)) {

        readOp<C, M, S, STD_AE_FRAME>(src, &ea, &data);

        reg.sr.n = NBIT<Word>(data);
        reg.sr.z = ZERO<Word>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        u32 ea2 = queue.irc << 16;
        readExt<C>();
        ea2 |= queue.irc;

        // Check for address error
        if (misaligned<C, S>(ea2)) {
            throw AddressError(makeFrame<AE_WRITE|AE_DATA>(ea2));
        }

        reg.sr.n = NBIT<S>(data);
        reg.sr.z = ZERO<S>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        writeM<C, MODE_AL, S>(ea2, data);
        readExt<C>();

    } else {

        readOp<C, M, S>(src, &ea, &data);

        reg.sr.n = NBIT<S>(data);
        reg.sr.z = ZERO<S>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        writeOp<C, MODE_AL, S>(dst, data);
    }

    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   (16, 16,  6,       16, 16,  6,       20, 20,  6)
    CYCLES_AN   (16, 16,  6,       16, 16,  6,       20, 20,  6)
    CYCLES_AI   (20, 20, 10,       20, 20, 10,       28, 28, 10)
    CYCLES_PI   (20, 20, 10,       20, 20, 10,       28, 28, 10)
    CYCLES_PD   (22, 22, 11,       22, 22, 11,       30, 30, 11)
    CYCLES_DI   (24, 24, 11,       24, 24, 11,       32, 32, 11)
    CYCLES_IX   (26, 26, 13,       26, 26, 13,       34, 34, 13)
    CYCLES_AW   (24, 24, 10,       24, 24, 10,       32, 32, 10)
    CYCLES_AL   (28, 28, 10,       28, 28, 10,       36, 36, 10)
    CYCLES_DIPC (24, 24, 11,       24, 24, 11,       32, 32, 11)
    CYCLES_IXPC (26, 26, 13,       26, 26, 13,       34, 34, 13)
    CYCLES_IM   (20, 20,  8,       20, 20,  8,       28, 28, 10)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMovea(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea = 0, data;

    try { readOp<C, M, S>(src, &ea, &data); } catch (AddressError &exc) {

        // Rectify the stack frame
        exc.stackFrame = makeFrame<STD_AE_FRAME|AE_SET_RW|AE_SET_DF>(ea);
        throw exc;
    }

    prefetch<C, POLL>();
    writeA(dst, SEXT<S>(data));

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 4,  4,  2,        4,  4,  2,        4,  4,  2)
    CYCLES_AN   ( 4,  4,  2,        4,  4,  2,        4,  4,  2)
    CYCLES_AI   ( 8,  8,  6,        8,  8,  6,       12, 12,  6)
    CYCLES_PI   ( 8,  8,  6,        8,  8,  6,       12, 12,  6)
    CYCLES_PD   (10, 10,  7,       10, 10,  7,       14, 14,  7)
    CYCLES_DI   (12, 12,  7,       12, 12,  7,       16, 16,  7)
    CYCLES_IX   (14, 14,  9,       14, 14,  9,       18, 18,  9)
    CYCLES_AW   (12, 12,  6,       12, 12,  6,       16, 16,  6)
    CYCLES_AL   (16, 16,  6,       16, 16,  6,       20, 20,  6)
    CYCLES_DIPC (12, 12,  7,       12, 12,  7,       16, 16,  7)
    CYCLES_IXPC (14, 14,  9,       14, 14,  9,       18, 18,  9)
    CYCLES_IM   ( 8,  8,  4,        8,  8,  4,       12, 12,  6)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMovecRcRx(u16 opcode)
{
    AVAILABILITY(C68000)
    SUPERVISOR_MODE_ONLY

    SYNC(4);

    auto arg = readI<C, Word>();
    int dst = xxxx____________(arg);

    if constexpr (C == C68010) {

        auto rc = arg & 0xFFF;

        if (rc != 0x000 && rc != 0x001 && rc != 0x800 && rc != 0x801) {

            execIllegal<C, I, M, S>(opcode);
            return;
        }
    }
    if constexpr (C == C68020) {

        auto rc = arg & 0xFFF;

        if (rc != 0x000 && rc != 0x001 && rc != 0x800 && rc != 0x801 &&
            rc != 0x002 && rc != 0x802 && rc != 0x803 && rc != 0x804) {

            execIllegal<C, I, M, S>(opcode);
            return;
        }
    }

    switch (arg & 0x0FFF) {

        case 0x000: reg.r[dst] = getSFC(); break;
        case 0x001: reg.r[dst] = getDFC(); break;
        case 0x800: reg.r[dst] = getUSP(); break;
        case 0x801: reg.r[dst] = getVBR(); break;
        case 0x002: reg.r[dst] = getCACR(); break;
        case 0x802: reg.r[dst] = getCAAR(); break;
        case 0x803: reg.r[dst] = reg.sr.m ? getSP() : getMSP(); break;
        case 0x804: reg.r[dst] = reg.sr.m ? getISP() : getSP(); break;
    }

    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP (   0,  0,  0,        0,  0,  0,        0, 12,  6);

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMovecRxRc(u16 opcode)
{
    AVAILABILITY(C68010)
    SUPERVISOR_MODE_ONLY

    SYNC(2);

    auto arg = readI<C, Word>();
    int  src = xxxx____________(arg);

    if constexpr (C == C68010) {

        auto reg = arg & 0xFFF;

        if (reg != 0x000 && reg != 0x001 && reg != 0x800 && reg != 0x801) {

            execIllegal<C, I, M, S>(opcode);
            return;
        }
    }
    if constexpr (C == C68020) {

        auto reg = arg & 0xFFF;

        if (reg != 0x000 && reg != 0x001 && reg != 0x800 && reg != 0x801 &&
            reg != 0x002 && reg != 0x802 && reg != 0x803 && reg != 0x804) {

            execIllegal<C, I, M, S>(opcode);
            return;
        }
    }

    u32  val = readR(src);
    switch (arg & 0x0FFF) {

        case 0x000: setSFC(val); break;
        case 0x001: setDFC(val); break;
        case 0x800: setUSP(val); break;
        case 0x801: setVBR(val); break;
        case 0x002: setCACR(val); break;
        case 0x802: setCAAR(val); break;
        case 0x803: reg.sr.m ? setSP(val) : setMSP(val); break;
        case 0x804: reg.sr.m ? setISP(val) : setSP(val); break;
    }

    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP   ( 0,  0,  0,        0,  0,  0,        0, 10, 12);

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMovemEaRg(u16 opcode)
{
    AVAILABILITY(C68000)

    int src  = _____________xxx(opcode);
    u16 mask = (u16)readI<C, Word>();
    u32 ea   = computeEA<C, M, S>(src);

    int cnt = 0;

    // Check for address error
    if (misaligned<C, S>(ea)) {

        setFC<M>();
        if constexpr (M == MODE_IX || M == MODE_IXPC) {
            throw AddressError(makeFrame<AE_DEC_PC|AE_SET_DF|AE_SET_RW>(ea));
        } else {
            throw AddressError(makeFrame<AE_INC_PC|AE_SET_DF|AE_SET_RW>(ea));
        }
    }

    if constexpr (S == Long) (void)read<C, MEM_DATA, Word>(ea);

    if constexpr (M == 3) {     // (An)+

        for (int i = 0; i <= 15; i++) {

            // Only proceed if the mask bit is set
            if ((mask & (1 << i)) == 0) continue;

            writeR(i, SEXT<S>(readM<C, M, S>(ea)));
            ea += S;
            cnt++;
        }

        writeA(src, ea);

    } else {

        for(int i = 0; i <= 15; i++) {

            // Only proceed if the mask bit is set
            if ((mask & (1 << i)) == 0) continue;

            writeR(i, SEXT<S>(readM<C, M, S>(ea)));
            ea += S;
            cnt++;
        }
    }
    if constexpr (S == Word) (void)read<C, MEM_DATA, Word>(ea);

    prefetch<C, POLL>();

    auto c = (C == C68020 || S == Word) ? 4 * cnt : 8 * cnt;

    //           00  10  20        00    10    20        00    10    20
    //           .b  .b  .b        .w    .w    .w        .l    .l    .l
    CYCLES_AI   ( 0,  0,  0,     12+c, 12+c, 12+c,     12+c, 12+c, 12+c)
    CYCLES_PI   ( 0,  0,  0,     12+c, 12+c,  8+c,     12+c, 12+c,  8+c)
    CYCLES_DI   ( 0,  0,  0,     16+c, 16+c, 13+c,     16+c, 16+c, 13+c)
    CYCLES_IX   ( 0,  0,  0,     18+c, 18+c, 15+c,     18+c, 18+c, 15+c)
    CYCLES_AW   ( 0,  0,  0,     16+c, 16+c, 12+c,     16+c, 16+c, 12+c)
    CYCLES_AL   ( 0,  0,  0,     20+c, 20+c, 12+c,     20+c, 20+c, 12+c)
    CYCLES_DIPC ( 0,  0,  0,     16+c, 16+c,  9+c,     16+c, 16+c,  9+c)
    CYCLES_IXPC ( 0,  0,  0,     18+c, 18+c, 11+c,     18+c, 18+c, 11+c)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMovemRgEa(u16 opcode)
{
    AVAILABILITY(C68000)

    int dst  = _____________xxx(opcode);
    u16 mask = (u16)readI<C, Word>();

    int cnt = 0;

    if constexpr (M == 4) {     // -(An)

        u32 ea = readA(dst);

        for (int i = 15; i >= 0; i--) {

            // Only proceed if the mask bit is set
            if ((mask & (0x8000 >> i)) == 0) continue;

            // Check for address error
            if (misaligned<C, S>(ea)) {

                setFC<M>();
                readBuffer = mask;
                writeBuffer = u16(reg.r[i] & 0xFFFF);
                throw AddressError(makeFrame<AE_INC_PC|AE_WRITE>(U32_SUB(ea, 2)));
            }

            // Write register contents into memory
            ea -= S;
            if constexpr (C == C68020 && !MIMIC_MUSASHI) writeA(dst, ea);
            writeM<C, M, S, MIMIC_MUSASHI ? REVERSE : 0>(ea, reg.r[i]);
            cnt++;
        }
        if constexpr (C != C68020 || MIMIC_MUSASHI) writeA(dst, ea);

    } else {

        u32 ea = computeEA<C, M, S>(dst);

        for(int i = 0; i < 16; i++) {

            // Only proceed if the mask bit is set
            if ((mask & (1 << i)) == 0) continue;

            // Check for address error
            if (misaligned<C, S>(ea)) {

                setFC<M>();
                readBuffer = mask;
                writeBuffer = S == Long ? u16(reg.r[i] >> 16) : u16(reg.r[i] & 0xFFFF);
                throw AddressError(makeFrame<AE_INC_PC|AE_WRITE>(ea));
            }

            // Write register contents into memory
            writeM<C, M, S>(ea, reg.r[i]);
            ea += S;
            cnt++;
        }
    }
    prefetch<C, POLL>();

    auto c = (C == C68020 || S == Word) ? 4 * cnt : 8 * cnt;

    //           00  10  20        00    10    20        00    10    20
    //           .b  .b  .b        .w    .w    .w        .l    .l    .l
    CYCLES_AI   ( 0,  0,  0,      8+c,  8+c,  8+c,      8+c,  8+c,  8+c)
    CYCLES_PD   ( 0,  0,  0,      8+c,  8+c,  4+c,      8+c,  8+c,  4+c)
    CYCLES_DI   ( 0,  0,  0,     12+c, 12+c,  9+c,     12+c, 12+c,  9+c)
    CYCLES_IX   ( 0,  0,  0,     14+c, 14+c, 11+c,     14+c, 14+c, 11+c)
    CYCLES_AW   ( 0,  0,  0,     12+c, 12+c,  8+c,     12+c, 12+c,  8+c)
    CYCLES_AL   ( 0,  0,  0,     16+c, 16+c,  8+c,     16+c, 16+c,  8+c)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMovepDxEa(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    u32 ea = computeEA<C, M, S>(dst);
    u32 dx = readD(src);

    switch (S) {

        case Long:
        {
            writeM<C, M, Byte>(ea, (dx >> 24) & 0xFF); ea += 2;
            writeM<C, M, Byte>(ea, (dx >> 16) & 0xFF); ea += 2;
            [[fallthrough]];
        }
        case Word:
        {
            writeM<C, M, Byte>(ea, (dx >>  8) & 0xFF); ea += 2;
            writeM<C, M, Byte>(ea, (dx >>  0) & 0xFF);
        }
    }
    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DI   ( 0,  0,  0,       16, 16, 11,       24, 24, 17)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMovepEaDx(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea = computeEA<C, M, S>(src);
    u32 dx = 0;

    switch (S) {

        case Long:
        {
            dx |= read<C, MEM_DATA, Byte>(ea) << 24; ea += 2;
            dx |= read<C, MEM_DATA, Byte>(ea) << 16; ea += 2;
            [[fallthrough]];
        }
        case Word:
        {
            dx |= read<C, MEM_DATA, Byte>(ea) << 8; ea += 2;
            POLL_IPL;
            dx |= read<C, MEM_DATA, Byte>(ea) << 0;
        }

    }
    writeD<S>(dst, dx);
    prefetch<C>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DI   ( 0,  0,  0,       16, 16, 12,       24, 24, 18)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveq(u16 opcode)
{
    AVAILABILITY(C68000)

    i8  src = (i8)(opcode & 0xFF);
    int dst = ____xxx_________(opcode);

    writeD <Long> (dst, (i32)src);

    reg.sr.n = NBIT<Byte>(src);
    reg.sr.z = ZERO<Byte>(src);
    reg.sr.v = 0;
    reg.sr.c = 0;

    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IM   ( 0,  0,  0,        0,  0,  0,        4,  4,  2)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoves(u16 opcode)
{
    AVAILABILITY(C68010)
    SUPERVISOR_MODE_ONLY

    auto old = queue.irc;

    if (queue.irc & 0x800) {    // Rg -> Ea

        auto arg = readI<C, Word>();
        int src = xxxx____________(arg);
        int dst = _____________xxx(opcode);

        auto value = readR<S>(src);

        u32 ea = computeEA<C, M, S>(dst);
        updateAn<M, S>(dst);

        // Take care of some special cases
        if (M == MODE_PI && src == (dst | 0x8)) {

            // MOVES An,(An)+
            value += dst == 7 ? (S == Long ? 4 : 2) : S;
        }
        if (M == MODE_PD && src == (dst | 0x8)) {

            // MOVES An,-(An)
            value -= dst == 7 ? (S == Long ? 4 : 2) : S;
        }

        if constexpr (M == MODE_AI) SYNC(6);
        if constexpr (M == MODE_PI) SYNC(8);
        if constexpr (M == MODE_PD) SYNC(6);
        if constexpr (M == MODE_DI) SYNC(4);
        if constexpr (M == MODE_IX) SYNC(6);
        if constexpr (M == MODE_AW) SYNC(4);
        if constexpr (M == MODE_AL) SYNC(4);

        // Make the DFC register visible on the FC pins
        fcSource = 2;

        // writeOp<C, M, S>(dst, value);
        try {
            writeM<C, M, S, AE_INC_PC>(ea, value);
        } catch (AddressError &exc) {

            writeBuffer = (S == Long ? u16(value >> 16) : u16(value & 0xFFFF));

            // EXPERIMENTAL: CLEAN THIS UP (RENAME stackFrame.ird to irc?!)
            fcSource = 0;
            queue.irc = old;
            throw exc;
        }

        // Switch back to the old FC pin values
        fcSource = 0;

        if (S == Long && (cpuModel == M68020 || cpuModel == M68EC020)) cp += 2;

    } else {                    // Ea -> Rg

        auto arg = readI<C, Word>();
        int src = _____________xxx(opcode);
        int dst = xxxx____________(arg);

        // u32 ea, data;
        // readOp<C, M, S, STD_AE_FRAME | SKIP_READ>(src, &ea, &data);
        u32 ea;
        try {

            ea = computeEA<C, M, S>(src);
            updateAn<M, S>(src);

        } catch (AddressError &exc) {

            exc.stackFrame.ird = old;
            throw exc;
        }

        // Make the SFC register visible on the FC pins
        fcSource = 1;

        if constexpr (M == MODE_AI) SYNC(6);
        if constexpr (M == MODE_PI) SYNC(8);
        if constexpr (M == MODE_PD) SYNC(6);
        if constexpr (M == MODE_DI) SYNC(4);
        if constexpr (M == MODE_IX) SYNC(6);
        if constexpr (M == MODE_AW) SYNC(4);
        if constexpr (M == MODE_AL) SYNC(4);

        u32 data = readM<C, M, S>(ea);

        if (dst < 8) {
            writeR<S>(dst, data);
        } else {
            writeR<Long>(dst, SEXT<S>(data));
        }

        // Switch back to the old FC pin values
        fcSource = 0;

        if (cpuModel == M68020 || cpuModel == M68EC020) cp += 2;
    }

    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_AI   ( 0, 18,  9,        0, 18,  9,        0, 22,  9)
    CYCLES_PI   ( 0, 18,  9,        0, 18,  9,        0, 22,  9)
    CYCLES_PD   ( 0, 20, 10,        0, 20, 10,        0, 28, 10)
    CYCLES_DI   ( 0, 26, 10,        0, 26, 10,        0, 32, 10)
    CYCLES_IX   ( 0, 30, 12,        0, 30, 12,        0, 36, 12)
    CYCLES_AW   ( 0, 26,  9,        0, 26,  9,        0, 32,  9)
    CYCLES_AL   ( 0, 30,  9,        0, 30,  9,        0, 36,  9)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveCcrRg(u16 opcode)
{
    AVAILABILITY(C68010)

    int dst = _____________xxx(opcode);

    u32 ea, data;
    readOp<C, M, S>(dst, &ea, &data);
    prefetch<C, POLL>();

    writeD <S> (dst, getCCR());

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 0,  0,  0,        0,  4,  4,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveCcrEa(u16 opcode)
{
    AVAILABILITY(C68010)

    int dst = _____________xxx(opcode);

    if (M == MODE_AI) SYNC(2);
    if (M == MODE_PI) SYNC(4);
    if (M == MODE_PD) SYNC(2);

    auto val = getCCR();

    // Compute effective address
    u32 ea = computeEA<C, M, S>(dst);

    // Emulate -(An) register modification
    updateAnPD<M, S>(dst);

    prefetch<C>();

    if (misaligned<C>(ea)) {

        writeBuffer = val & 0xFFFF;
        updateAnPI<M, S>(dst);
        setFC<M>();
        throw AddressError(makeFrame<AE_WRITE|AE_INC_PC>(ea));
    }

    // Write to effective address
    writeM<C, M, S, POLL>(ea, val);

    // Emulate (An)+ register modification
    updateAnPI<M, S>(dst);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_AI   ( 0,  0,  0,        0, 12,  8,        0,  0,  0)
    CYCLES_PI   ( 0,  0,  0,        0, 12,  8,        0,  0,  0)
    CYCLES_PD   ( 0,  0,  0,        0, 14,  9,        0,  0,  0)
    CYCLES_DI   ( 0,  0,  0,        0, 16,  9,        0,  0,  0)
    CYCLES_IX   ( 0,  0,  0,        0, 18, 11,        0,  0,  0)
    CYCLES_AW   ( 0,  0,  0,        0, 16,  8,        0,  0,  0)
    CYCLES_AL   ( 0,  0,  0,        0, 20,  8,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveToCcr(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = _____________xxx(opcode);

    u32 ea, data;
    readOp<C, M, S, STD_AE_FRAME>(src, &ea, &data);

    SYNC(4);
    setCCR((u8)data);

    (void)read<C, MEM_PROG, Word>(reg.pc + 2);
    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 0,  0,  0,       12, 12,  4,        0,  0,  0)
    CYCLES_AI   ( 0,  0,  0,       16, 16,  8,        0,  0,  0)
    CYCLES_PI   ( 0,  0,  0,       16, 16,  8,        0,  0,  0)
    CYCLES_PD   ( 0,  0,  0,       18, 18,  9,        0,  0,  0)
    CYCLES_DI   ( 0,  0,  0,       20, 20,  9,        0,  0,  0)
    CYCLES_IX   ( 0,  0,  0,       22, 22, 11,        0,  0,  0)
    CYCLES_AW   ( 0,  0,  0,       20, 20,  8,        0,  0,  0)
    CYCLES_AL   ( 0,  0,  0,       24, 24,  8,        0,  0,  0)
    CYCLES_DIPC ( 0,  0,  0,       20, 20,  9,        0,  0,  0)
    CYCLES_IXPC ( 0,  0,  0,       22, 22, 11,        0,  0,  0)
    CYCLES_IM   ( 0,  0,  0,       16, 16,  6,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveSrRg(u16 opcode)
{
    AVAILABILITY(C68000)
    if constexpr (C != C68000) SUPERVISOR_MODE_ONLY

        int dst = _____________xxx(opcode);

    u32 ea, data;
    readOp<C, M, S>(dst, &ea, &data);
    prefetch<C, POLL>();

    if constexpr (C == C68000) SYNC(2);
    writeD<S>(dst, getSR());

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 0,  0,  0,        6,  4,  8,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveSrEa(u16 opcode)
{
    AVAILABILITY(C68000)
    if constexpr (C != C68000) SUPERVISOR_MODE_ONLY

        int dst = _____________xxx(opcode);

    if (C == C68000) {

        u32 ea, data;
        readOp<C, M, S, STD_AE_FRAME>(dst, &ea, &data);
        prefetch<C>();

        writeOp<C, M, S, POLL>(dst, ea, getSR());

    } else {

        if (M == MODE_AI) SYNC(2);
        if (M == MODE_PI) SYNC(4);
        if (M == MODE_PD) SYNC(2);

        auto val = getSR();
        u32 ea = computeEA<C, M, S>(dst);

        // Emulate -(An) register modification
        updateAnPD<M, S>(dst);

        prefetch<C>();

        if (misaligned<C>(ea)) {

            writeBuffer = val & 0xFFFF;
            updateAnPI<M, S>(dst);
            setFC<M>();
            throw AddressError(makeFrame<AE_WRITE|AE_INC_PC>(ea));
        }

        // Write to effective address
        writeM<C, M, S, POLL>(ea, val);

        // Emulate (An)+ register modification
        updateAnPI<M, S>(dst);

        /*
        writeOp<C, M, S, POLL>(dst, getSR());
        prefetch<C>();
        */
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 0,  0,  0,        6,  4,  8,        0,  0,  0)
    CYCLES_AI   ( 0,  0,  0,       12, 12, 12,        0,  0,  0)
    CYCLES_PI   ( 0,  0,  0,       12, 12, 12,        0,  0,  0)
    CYCLES_PD   ( 0,  0,  0,       14, 14, 13,        0,  0,  0)
    CYCLES_DI   ( 0,  0,  0,       16, 16, 13,        0,  0,  0)
    CYCLES_IX   ( 0,  0,  0,       18, 18, 15,        0,  0,  0)
    CYCLES_AW   ( 0,  0,  0,       16, 16, 12,        0,  0,  0)
    CYCLES_AL   ( 0,  0,  0,       20, 20, 12,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveToSr(u16 opcode)
{
    AVAILABILITY(C68000)

    SUPERVISOR_MODE_ONLY

    int src = _____________xxx(opcode);

    u32 ea, data;
    readOp<C, M, S, STD_AE_FRAME>(src, &ea, &data);

    SYNC(4);
    setSR((u16)data);

    (void)read<C, MEM_PROG, Word>(reg.pc + 2);
    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 0,  0,  0,       12, 12,  8,        0,  0,  0)
    CYCLES_AI   ( 0,  0,  0,       16, 16, 12,        0,  0,  0)
    CYCLES_PI   ( 0,  0,  0,       16, 16, 12,        0,  0,  0)
    CYCLES_PD   ( 0,  0,  0,       18, 18, 13,        0,  0,  0)
    CYCLES_DI   ( 0,  0,  0,       20, 20, 13,        0,  0,  0)
    CYCLES_IX   ( 0,  0,  0,       22, 22, 15,        0,  0,  0)
    CYCLES_AW   ( 0,  0,  0,       20, 20, 12,        0,  0,  0)
    CYCLES_AL   ( 0,  0,  0,       24, 24, 12,        0,  0,  0)
    CYCLES_DIPC ( 0,  0,  0,       20, 20, 13,        0,  0,  0)
    CYCLES_IXPC ( 0,  0,  0,       22, 22, 15,        0,  0,  0)
    CYCLES_IM   ( 0,  0,  0,       16, 16, 10,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveUspAn(u16 opcode)
{
    AVAILABILITY(C68000)
    SUPERVISOR_MODE_ONLY

    int an = _____________xxx(opcode);

    if constexpr (C >= C68010) SYNC(2);

    prefetch<C, POLL>();
    writeA(an, getUSP());

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP   ( 0,  0,  0,        0,  0,  0,        4,  6,  2)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveAnUsp(u16 opcode)
{
    AVAILABILITY(C68000)
    SUPERVISOR_MODE_ONLY

    int an = _____________xxx(opcode);

    if constexpr (C >= C68010) SYNC(2);

    prefetch<C, POLL>();
    setUSP(readA(an));

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP   ( 0,  0,  0,        0,  0,  0,        4,  6,  2)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMuls(u16 opcode)
{
    AVAILABILITY(C68000)

    if constexpr (MIMIC_MUSASHI) {
        execMulsMusashi<C, I, M, S>(opcode);
    } else {
        execMulsMoira<C, I, M, S>(opcode);
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 0,  0,  0,       54,  32,  27,      0,  0,  0)
    CYCLES_AI   ( 0,  0,  0,       58,  36,  31,      0,  0,  0)
    CYCLES_PI   ( 0,  0,  0,       58,  36,  31,      0,  0,  0)
    CYCLES_PD   ( 0,  0,  0,       60,  38,  32,      0,  0,  0)
    CYCLES_DI   ( 0,  0,  0,       62,  40,  32,      0,  0,  0)
    CYCLES_IX   ( 0,  0,  0,       64,  42,  34,      0,  0,  0)
    CYCLES_AW   ( 0,  0,  0,       62,  40,  31,      0,  0,  0)
    CYCLES_AL   ( 0,  0,  0,       66,  44,  31,      0,  0,  0)
    CYCLES_DIPC ( 0,  0,  0,       62,  40,  32,      0,  0,  0)
    CYCLES_IXPC ( 0,  0,  0,       64,  42,  34,      0,  0,  0)
    CYCLES_IM   ( 0,  0,  0,       58,  36,  29,      0,  0,  0)

    FINALIZE;
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMulsMoira(u16 opcode)
{
    u32 ea, data, result;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    readOp<C, M, Word, STD_AE_FRAME>(src, &ea, &data);

    if (C == C68000) {

        prefetch<C, POLL>();
        result = muls<C>(data, readD<Word>(dst));
        [[maybe_unused]] auto cycles = cyclesMul<C, I>(u16(data));
        SYNC(cycles);

    } else {

        result = muls<C>(data, readD<Word>(dst));
        [[maybe_unused]] auto cycles = cyclesMul<C, I>(u16(data));
        SYNC(cycles);
        prefetch<C, POLL>();
    }

    writeD(dst, result);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMulsMusashi(u16 opcode)
{
    u32 ea, data, result;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    readOp<C, M, Word>(src, &ea, &data);

    prefetch<C, POLL>();
    result = muls<C>(data, readD<Word>(dst));

    if constexpr (I == MULU) { SYNC_68000(50); SYNC_68010(26); }
    if constexpr (I == MULS) { SYNC_68000(50); SYNC_68010(28); }

    writeD(dst, result);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMulu(u16 opcode)
{
    AVAILABILITY(C68000)

    if constexpr (MIMIC_MUSASHI) {
        execMuluMusashi<C, I, M, S>(opcode);
    } else {
        execMuluMoira<C, I, M, S>(opcode);
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 0,  0,  0,       54,  30,  27,      0,  0,  0)
    CYCLES_AI   ( 0,  0,  0,       58,  34,  31,      0,  0,  0)
    CYCLES_PI   ( 0,  0,  0,       58,  34,  31,      0,  0,  0)
    CYCLES_PD   ( 0,  0,  0,       60,  36,  32,      0,  0,  0)
    CYCLES_DI   ( 0,  0,  0,       62,  38,  32,      0,  0,  0)
    CYCLES_IX   ( 0,  0,  0,       64,  40,  34,      0,  0,  0)
    CYCLES_AW   ( 0,  0,  0,       62,  38,  31,      0,  0,  0)
    CYCLES_AL   ( 0,  0,  0,       66,  42,  31,      0,  0,  0)
    CYCLES_DIPC ( 0,  0,  0,       62,  38,  32,      0,  0,  0)
    CYCLES_IXPC ( 0,  0,  0,       64,  40,  34,      0,  0,  0)
    CYCLES_IM   ( 0,  0,  0,       58,  34,  29,      0,  0,  0)

    FINALIZE;
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMuluMoira(u16 opcode)
{
    u32 ea, data, result;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    readOp<C, M, Word, STD_AE_FRAME>(src, &ea, &data);

    if (C == C68000) {

        prefetch<C, POLL>();
        result = mulu<C>(data, readD<Word>(dst));
        [[maybe_unused]] auto cycles = cyclesMul<C, I>(u16(data));
        SYNC(cycles);

    } else {

        result = mulu<C>(data, readD<Word>(dst));
        [[maybe_unused]] auto cycles = cyclesMul<C, I>(u16(data));
        SYNC(cycles);
        prefetch<C, POLL>();
    }

    writeD(dst, result);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMuluMusashi(u16 opcode)
{
    u32 ea, data, result;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    readOp<C, M, Word>(src, &ea, &data);

    prefetch<C, POLL>();
    result = mulu<C>(data, readD<Word>(dst));

    if constexpr (I == MULU) { SYNC_68000(50); SYNC_68010(26); }
    if constexpr (I == MULS) { SYNC_68000(50); SYNC_68010(28); }

    writeD(dst, result);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMull(u16 opcode)
{
    AVAILABILITY(C68020)

    if constexpr (MIMIC_MUSASHI) {
        execMullMusashi<C, I, M, S>(opcode);
    } else {
        execMullMoira<C, I, M, S>(opcode);
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 0,  0,  0,        0,  0, 43,        0,  0, 43)
    CYCLES_AI   ( 0,  0,  0,        0,  0, 47,        0,  0, 47)
    CYCLES_PI   ( 0,  0,  0,        0,  0, 47,        0,  0, 47)
    CYCLES_PD   ( 0,  0,  0,        0,  0, 48,        0,  0, 48)
    CYCLES_DI   ( 0,  0,  0,        0,  0, 48,        0,  0, 48)
    CYCLES_IX   ( 0,  0,  0,        0,  0, 50,        0,  0, 50)
    CYCLES_AW   ( 0,  0,  0,        0,  0, 47,        0,  0, 47)
    CYCLES_AL   ( 0,  0,  0,        0,  0, 47,        0,  0, 47)
    CYCLES_DIPC ( 0,  0,  0,        0,  0, 48,        0,  0, 48)
    CYCLES_IXPC ( 0,  0,  0,        0,  0, 50,        0,  0, 50)
    CYCLES_IM   ( 0,  0,  0,        0,  0, 47,        0,  0, 47)

    FINALIZE;
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMullMoira(u16 opcode)
{
    u64 result;
    u32 ea, data;
    u16 ext = (u16)readI<C, Word>();

    int src = _____________xxx(opcode);
    int dh  = _____________xxx(ext);
    int dl  = _xxx____________(ext);

    readOp<C, M, S>(src, &ea, &data);

    prefetch<C, POLL>();

    switch (____xx__________(ext)) {

        case 0b00: { // Unsigned 32 bit

            result = mullu<C, Word>(data, readD(dl));

            writeD(dl, u32(result));
            break;
        }
        case 0b01: // Unsigned 64 bit

            result = mullu<C, Long>(data, readD(dl));

            // Note: 68040 switched the write-order
            writeD(dl, u32(result));
            writeD(dh, u32(result >> 32));
            break;

        case 0b10: // Signed 32 bit

            result = mulls<C, Word>(data, readD(dl));
            writeD(dl, u32(result));
            break;

        case 0b11: // Signed 64 bit

            result = mulls<C, Long>(data, readD(dl));

            // Note: 68040 switched the write-order
            writeD(dl, u32(result));
            writeD(dh, u32(result >> 32));
            break;
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMullMusashi(u16 opcode)
{
    u64 result;
    u32 ea, data;
    u16 ext = (u16)readI<C, Word>();

    int src = _____________xxx(opcode);
    int dh  = _____________xxx(ext);
    int dl  = _xxx____________(ext);

    readOp<C, M, S>(src, &ea, &data);

    prefetch<C, POLL>();

    switch (____xx__________(ext)) {

        case 0b00:

            result = mullu<C, Word>(data, readD(dl));
            writeD(dl, u32(result));
            break;

        case 0b01:

            result = mullu<C, Long>(data, readD(dl));
            writeD(dh, u32(result >> 32));
            writeD(dl, u32(result));
            break;

        case 0b10:

            result = mulls<C, Word>(data, readD(dl));
            writeD(dl, u32(result));
            break;

        case 0b11:

            result = mulls<C, Long>(data, readD(dl));
            writeD(dh, u32(result >> 32));
            writeD(dl, u32(result));
            break;
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execDivs(u16 opcode)
{
    AVAILABILITY(C68000)

    bool divByZero = false;

    if constexpr (MIMIC_MUSASHI) {
        execDivsMusashi<C, I, M, S>(opcode, &divByZero);
    } else {
        execDivsMoira<C, I, M, S>(opcode, &divByZero);
    }

    if (divByZero) {

        CYCLES_68000(38);
        CYCLES_68010(44);
        CYCLES_68020(38);

    } else {

        //           00  10  20        00  10  20        00  10  20
        //           .b  .b  .b        .w  .w  .w        .l  .l  .l
        CYCLES_DN   ( 0,  0,  0,      158,122, 56,        0,  0,  0)
        CYCLES_AI   ( 0,  0,  0,      162,126, 60,        0,  0,  0)
        CYCLES_PI   ( 0,  0,  0,      162,126, 60,        0,  0,  0)
        CYCLES_PD   ( 0,  0,  0,      164,128, 61,        0,  0,  0)
        CYCLES_DI   ( 0,  0,  0,      166,130, 61,        0,  0,  0)
        CYCLES_IX   ( 0,  0,  0,      168,132, 63,        0,  0,  0)
        CYCLES_AW   ( 0,  0,  0,      166,130, 60,        0,  0,  0)
        CYCLES_AL   ( 0,  0,  0,      170,134, 60,        0,  0,  0)
        CYCLES_DIPC ( 0,  0,  0,      166,130, 61,        0,  0,  0)
        CYCLES_IXPC ( 0,  0,  0,      168,132, 63,        0,  0,  0)
        CYCLES_IM   ( 0,  0,  0,      162,126, 58,        0,  0,  0)
    }

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execDivsMoira(u16 opcode, bool *divByZero)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea = 0, divisor, result;

    try { readOp<C, M, Word>(src, &ea, &divisor); } catch (AddressError &exc) {

        // Rectify the stack frame
        if (C == C68000) {

            SYNC(2);
            exc.stackFrame = makeFrame<STD_AE_FRAME>(ea);

        } else {

            readBuffer = (u16)readM<C, M, S>(ea & ~1);
            updateAnPI<M, S>(src);
            if (isAbsMode(M) || M == MODE_AI || M == MODE_PI || M == MODE_PD) {
                SYNC(2);
                exc.stackFrame = makeFrame<AE_SET_RW|AE_SET_DF>(ea);
            } else {
                SYNC(2);
                exc.stackFrame = makeFrame<AE_DEC_PC|AE_SET_RW|AE_SET_DF>(ea);
            }
        }
        throw exc;
    }

    u32 dividend = readD(dst);

    if (divisor == 0) {

        reg.sr.n = 0;
        reg.sr.z = 1;
        reg.sr.v = 0;
        reg.sr.c = 0;
        setDivZeroDIVS<C, S>(dividend);

        SYNC(8);
        execException<C>(EXC_DIVIDE_BY_ZERO);
        *divByZero = true;
        return;
    }

    result = divsMoira<C>(dividend, divisor);
    writeD(dst, result);
    prefetch<C, POLL>();

    [[maybe_unused]] auto cycles = cyclesDiv<C, I>(dividend, (u16)divisor) - 4;
    SYNC(cycles);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execDivsMusashi(u16 opcode, bool *divByZero)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    [[maybe_unused]] i64 c = clock;

    u32 ea, divisor, result;
    readOp<C, M, Word>(src, &ea, &divisor);

    if (divisor == 0) {

        if constexpr (C == C68000) {
            SYNC(8 - (int)(clock - c));
        } else {
            SYNC(10 - (int)(clock - c));
        }
        execException<C>(EXC_DIVIDE_BY_ZERO);
        *divByZero = true;
        return;
    }

    u32 dividend = readD(dst);
    result = divsMusashi<C>(dividend, divisor);

    SYNC_68000(154);
    SYNC_68010(118);

    writeD(dst, result);
    prefetch<C, POLL>();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execDivu(u16 opcode)
{
    AVAILABILITY(C68000)

    bool divByZero = false;

    if constexpr (MIMIC_MUSASHI) {
        execDivuMusashi<C, I, M, S>(opcode, &divByZero);
    } else {
        execDivuMoira<C, I, M, S>(opcode, &divByZero);
    }

    if (divByZero) {

        CYCLES_68000(38);
        CYCLES_68010(44);
        CYCLES_68020(38);

    } else {

        //           00  10  20        00  10  20        00  10  20
        //           .b  .b  .b        .w  .w  .w        .l  .l  .l
        CYCLES_DN   ( 0,  0,  0,      140,108, 44,       0,  0,  0)
        CYCLES_AI   ( 0,  0,  0,      144,112, 48,       0,  0,  0)
        CYCLES_PI   ( 0,  0,  0,      144,112, 48,       0,  0,  0)
        CYCLES_PD   ( 0,  0,  0,      146,114, 49,       0,  0,  0)
        CYCLES_DI   ( 0,  0,  0,      148,116, 49,       0,  0,  0)
        CYCLES_IX   ( 0,  0,  0,      150,118, 51,       0,  0,  0)
        CYCLES_AW   ( 0,  0,  0,      148,116, 48,       0,  0,  0)
        CYCLES_AL   ( 0,  0,  0,      152,120, 48,       0,  0,  0)
        CYCLES_DIPC ( 0,  0,  0,      148,116, 49,       0,  0,  0)
        CYCLES_IXPC ( 0,  0,  0,      150,118, 51,       0,  0,  0)
        CYCLES_IM   ( 0,  0,  0,      144,112, 46,       0,  0,  0)
    }

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execDivuMoira(u16 opcode, bool *divByZero)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea = 0, divisor, result;

    try { readOp<C, M, Word>(src, &ea, &divisor); } catch (AddressError &exc) {

        // Rectify the stack frame
        if (C == C68000) {

            SYNC(2);
            exc.stackFrame = makeFrame<STD_AE_FRAME>(ea);
            throw exc;

        } else {

            readBuffer = (u16)readM<C, M, S>(ea & ~1);
            updateAnPI<M, S>(src);
            SYNC(2);
            if (isAbsMode(M) || M == MODE_AI || M == MODE_PI || M == MODE_PD) {
                exc.stackFrame = makeFrame<AE_SET_RW|AE_SET_DF>(ea);
            } else {
                exc.stackFrame = makeFrame<AE_DEC_PC|AE_SET_RW|AE_SET_DF>(ea);
            }
            throw exc;
        }
    }

    u32 dividend = readD(dst);

    // Check for division by zero
    if (divisor == 0) {

        reg.sr.n = NBIT<Long>(dividend);
        reg.sr.z = (dividend & 0xFFFF0000) == 0;
        reg.sr.v = 0;
        reg.sr.c = 0;
        setDivZeroDIVU<C, S>(dividend);

        SYNC(8);
        execException<C>(EXC_DIVIDE_BY_ZERO);
        *divByZero = true;
        return;
    }

    result = divuMoira<C>(dividend, divisor);
    writeD(dst, result);
    prefetch<C, POLL>();

    [[maybe_unused]] auto cycles = cyclesDiv<C, I>(dividend, (u16)divisor) - 4;
    SYNC(cycles);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execDivuMusashi(u16 opcode, bool *divByZero)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    [[maybe_unused]] i64 c = clock;
    u32 ea, divisor, result;
    readOp<C, M, Word>(src, &ea, &divisor);

    // Check for division by zero
    if (divisor == 0) {
        if constexpr (C == C68000) {
            SYNC(8 - (int)(clock - c));
        } else {
            SYNC(10 - (int)(clock - c));
        }
        execException<C>(EXC_DIVIDE_BY_ZERO);
        *divByZero = true;
        return;
    }

    u32 dividend = readD(dst);
    result = divuMusashi<C>(dividend, divisor);

    SYNC_68000(136);
    SYNC_68010(104);

    writeD(dst, result);
    prefetch<C, POLL>();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execDivl(u16 opcode)
{
    AVAILABILITY(C68020)

    bool success;
    bool divByZero = false;

    if constexpr (MIMIC_MUSASHI) {
        success = execDivlMusashi<C, I, M, S>(opcode, &divByZero);
    } else {
        success = execDivlMoira<C, I, M, S>(opcode, &divByZero);
    }

    if (success) {

        //           00  10  20        00  10  20        00  10  20
        //           .b  .b  .b        .w  .w  .w        .l  .l  .l
        CYCLES_DN   ( 0,  0,  0,        0,  0,  0,        0,  0, 84)
        CYCLES_AI   ( 0,  0,  0,        0,  0,  0,        0,  0, 88)
        CYCLES_PI   ( 0,  0,  0,        0,  0,  0,        0,  0, 88)
        CYCLES_PD   ( 0,  0,  0,        0,  0,  0,        0,  0, 89)
        CYCLES_DI   ( 0,  0,  0,        0,  0,  0,        0,  0, 89)
        CYCLES_IX   ( 0,  0,  0,        0,  0,  0,        0,  0, 91)
        CYCLES_AW   ( 0,  0,  0,        0,  0,  0,        0,  0, 88)
        CYCLES_AL   ( 0,  0,  0,        0,  0,  0,        0,  0, 88)
        CYCLES_DIPC ( 0,  0,  0,        0,  0,  0,        0,  0, 89)
        CYCLES_IXPC ( 0,  0,  0,        0,  0,  0,        0,  0, 91)
        CYCLES_IM   ( 0,  0,  0,        0,  0,  0,        0,  0, 88)

    } else if (divByZero) {

        CYCLES_68000(38);
        CYCLES_68010(44);
        CYCLES_68020(38);
    }

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> bool
Moira::execDivlMoira(u16 opcode, bool *divByZero)
{
    u64 dividend;
    u32 ea, divisor;
    u16 ext = (u16)readI<C, Word>();

    int src = _____________xxx(opcode);
    int dh  = _____________xxx(ext);
    int dl  = _xxx____________(ext);

    try {
        readOp<C, M, S>(src, &ea, &divisor);
    } catch(...) {
        // TODO: Change return type from bool to void
        return false;
    }

    if (ext & 0x400) {
        dividend = (u64)readD(dh) << 32 | readD(dl);
    } else {
        dividend = readD(dl);
    }

    if (divisor == 0) {

        if (ext & 0x800) {

            reg.sr.c = 0;
            setDivZeroDIVSL<C, S>(i64(dividend));

        } else {

            reg.sr.v = 1;
            reg.sr.c = 0;
            setDivZeroDIVUL<C, S>(i64(dividend));
        }

        execException<C>(EXC_DIVIDE_BY_ZERO);
        *divByZero = true;
        return false;
    }

    prefetch<C, POLL>();

    switch (____xx__________(ext)) {

        case 0b00:
        {
            auto result = divluMoira<Word>(dividend, divisor);

            writeD(dh, result.second);
            writeD(dl, result.first);
            break;
        }
        case 0b01:
        {
            auto result = divluMoira<Long>(dividend, divisor);

            if(!reg.sr.v) {

                writeD(dh, result.second);
                writeD(dl, result.first);

            } else {

                setUndefinedDIVUL<C, S>(dividend, divisor);
            }
            break;
        }
        case 0b10:
        {
            // auto result = divlsMusashi<Word>(dividend, divisor);
            auto result = divlsMoira<Word>(dividend, divisor);

            writeD(dh, result.second);
            writeD(dl, result.first);
            break;
        }
        case 0b11:
        {
            auto result = divlsMoira<Long>(dividend, divisor);

            if(!reg.sr.v) {

                writeD(dh, result.second);
                writeD(dl, result.first);

            } else {

                setUndefinedDIVSL<C, S>(dividend, divisor);
            }
            break;
        }
    }

    return true;
}

template <Core C, Instr I, Mode M, Size S> bool
Moira::execDivlMusashi(u16 opcode, bool *divByZero)
{
    u64 dividend;
    u32 ea, divisor;
    u16 ext = (u16)readI<C, Word>();

    int src = _____________xxx(opcode);
    int dh  = _____________xxx(ext);
    int dl  = _xxx____________(ext);

    try {
        readOp<C, M, S>(src, &ea, &divisor);
    } catch(...) {
        // TODO: Change return type from bool to void
        return false;
    }

    if (divisor == 0) {

        execException<C>(EXC_DIVIDE_BY_ZERO);
        *divByZero = true;
        return false;
    }

    prefetch<C, POLL>();

    switch (____xx__________(ext)) {

        case 0b00:
        {
            dividend = readD(dl);

            auto result = divluMusashi<Word>(dividend, divisor);
            writeD(dh, result.second);
            writeD(dl, result.first);
            break;
        }
        case 0b01:
        {
            dividend = (u64)readD(dh) << 32 | readD(dl);
            auto result = divluMusashi<Long>(dividend, divisor);
            if(!reg.sr.v) {

                writeD(dh, result.second);
                writeD(dl, result.first);
            }
            break;
        }
        case 0b10:
        {
            dividend = readD(dl);
            auto result = divlsMusashi<Word>(dividend, divisor);
            writeD(dh, result.second);
            writeD(dl, result.first);
            break;
        }
        case 0b11:
        {
            dividend = (u64)readD(dh) << 32 | readD(dl);
            auto result = divlsMusashi<Long>(dividend, divisor);

            if(!reg.sr.v) {

                writeD(dh, result.second);
                writeD(dl, result.first);
            }
            break;
        }
    }

    return true;
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execNbcdRg(u16 opcode)
{
    AVAILABILITY(C68000)

    int reg = _____________xxx(opcode);

    looping<I>() ? noPrefetch<C>() : prefetch<C, POLL>();
    SYNC(2);
    writeD<Byte>(reg, bcd<C, SBCD, Byte>(readD<Byte>(reg), 0));

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 6,  6,  6,        0,  0,  0,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execNbcdEa(u16 opcode)
{
    AVAILABILITY(C68000)

    int reg = _____________xxx(opcode);

    u32 ea, data;
    readOp<C, M, Byte>(reg, &ea, &data);

    if (looping<I>()) {
        if (M == MODE_AI) noPrefetch<C>(4);
        else if (M == MODE_PI) noPrefetch<C>(4);
        else if (M == MODE_PD) noPrefetch<C>(4);
        else noPrefetch<C>(2);
    } else {
        prefetch<C, POLL>();
    }
    writeM<C, M, Byte>(ea, bcd<C, SBCD, Byte>(data, 0));

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_AI   (12, 12, 10,        0,  0,  0,        0,  0,  0)
    CYCLES_PI   (12, 12, 10,        0,  0,  0,        0,  0,  0)
    CYCLES_PD   (14, 14, 11,        0,  0,  0,        0,  0,  0)
    CYCLES_DI   (16, 16, 11,        0,  0,  0,        0,  0,  0)
    CYCLES_IX   (18, 18, 13,        0,  0,  0,        0,  0,  0)
    CYCLES_AW   (16, 16, 10,        0,  0,  0,        0,  0,  0)
    CYCLES_AL   (20, 20, 10,        0,  0,  0,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execNegRg(u16 opcode)
{
    AVAILABILITY(C68000)

    int dst = ( _____________xxx(opcode) );
    u32 ea, data;

    readOp<C, M, S>(dst, &ea, &data);

    data = logic<C, I, S>(data);
    prefetch<C, POLL>();

    if constexpr (S == Long) SYNC(2);
    writeD<S>(dst, data);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 4,  4,  2,        4,  4,  2,        6,  6,  2)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execNegEa(u16 opcode)
{
    AVAILABILITY(C68000)

    int dst = ( _____________xxx(opcode) );
    u32 ea, data;

    readOp<C, M, S, STD_AE_FRAME>(dst, &ea, &data);

    data = logic<C, I, S>(data);
    looping<I>() ? noPrefetch<C>(2) : prefetch<C, POLL>();

    if constexpr (MIMIC_MUSASHI) {
        writeOp<C, M, S>(dst, ea, data);
    } else {
        writeOp<C, M, S, REVERSE>(dst, ea, data);
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_AI   (12, 12,  8,       12, 12,  8,       20, 20,  8)
    CYCLES_PI   (12, 12,  8,       12, 12,  8,       20, 20,  8)
    CYCLES_PD   (14, 14,  9,       14, 14,  9,       22, 22,  9)
    CYCLES_DI   (16, 16,  9,       16, 16,  9,       24, 24,  9)
    CYCLES_IX   (18, 18, 11,       18, 18, 11,       26, 26, 11)
    CYCLES_AW   (16, 16,  8,       16, 16,  8,       24, 24,  8)
    CYCLES_AL   (20, 20,  8,       20, 20,  8,       28, 28,  8)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execNop(u16 opcode)
{
    AVAILABILITY(C68000)

    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP   ( 0,  0,  0,        0,  0,  0,        4,  4,  2)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execPackDn(u16 opcode)
{
    AVAILABILITY(C68020)

    int dx  = _____________xxx(opcode);
    int dy  = ____xxx_________(opcode);

    u16 adj = (u16)readI<C, Word>();
    u32 src = readD(dx) + adj;
    u32 dst = (src >> 4 & 0xF0) | (src & 0x0F);

    writeD<Byte>(dy, dst);
    prefetch<C>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 0,  0,  0,        0,  0,  6,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execPackPd(u16 opcode)
{
    AVAILABILITY(C68020)

    int ax  = _____________xxx(opcode);
    int ay  = ____xxx_________(opcode);

    if (MIMIC_MUSASHI) {

        u32 ea1, data1;
        readOp<C, M, Byte>(ax, &ea1, &data1);

        u16 adj = (u16)readI<C, Word>();

        u32 ea2, data2;
        readOp<C, M, Byte>(ax, &ea2, &data2);

        u32 src = (data1 << 8 | data2) + adj;

        writeOp<C, M, Byte>(ay, (src >> 4 & 0xF0) | (src & 0x0F));

    } else {

        reg.a[ax]--;
        auto data1 = read<C, MEM_DATA, Byte>(readA(ax));

        u16 adj = (u16)readI<C, Word>();

        reg.a[ax]--;
        auto data2 = read<C, MEM_DATA, Byte>(readA(ax));

        u32 src = (data2 << 8 | data1) + adj;
        u32 dst = (src >> 4 & 0xF0) | (src & 0x0F);

        writeOp<C, M, Byte>(ay, dst);
    }

    prefetch<C>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_PD   ( 0,  0,  0,        0,  0, 13,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execPea(u16 opcode)
{
    AVAILABILITY(C68000)

    int src = _____________xxx(opcode);

    u32 ea = computeEA<C, M, Long>(src);

    if (isIdxMode(M)) SYNC(2);

    // Check for address error
    if (misaligned<C>(reg.sp)) {

        U32_DEC(reg.sp, S);

        if (C == C68000) {

            if (isAbsMode(M)) {
                throw AddressError(makeFrame<AE_WRITE|AE_DATA>(reg.sp));
            } else {
                throw AddressError(makeFrame<AE_WRITE|AE_DATA|AE_INC_PC>(reg.sp));
            }

        } else {

            writeBuffer = u16(ea >> 16);
            if (isAbsMode(M)) {
                readBuffer = queue.irc;
                throw AddressError(makeFrame<AE_WRITE|AE_DATA>(reg.sp, U32_SUB(reg.pc, 4)));
            } else if (isDspMode(M)) {
                prefetch<C>();
                throw AddressError(makeFrame<AE_WRITE|AE_DATA|AE_DEC_PC>(reg.sp));
            } else {
                prefetch<C>();
                throw AddressError(makeFrame<AE_WRITE|AE_DATA>(reg.sp));
            }
        }
    }

    if (isAbsMode(M)) {

        if (C == C68000) {

            push <C, Long> (ea);
            prefetch<C, POLL>();

        } else {

            POLL_IPL;
            push <C, Long> (ea);
            prefetch<C>();
        }

    } else if (isIdxMode(M)) {

        if (C == C68000) {

            POLL_IPL;
            prefetch<C>();
            push <C, Long> (ea);

        } else {

            prefetch<C, POLL>();
            push <C, Long> (ea);
        }

    } else {

        prefetch<C, POLL>();
        push <C, Long>(ea);
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_AI   ( 0,  0,  0,        0,  0,  0,       12, 12,  9)
    CYCLES_DI   ( 0,  0,  0,        0,  0,  0,       16, 16, 10)
    CYCLES_IX   ( 0,  0,  0,        0,  0,  0,       20, 20, 12)
    CYCLES_AW   ( 0,  0,  0,        0,  0,  0,       16, 16,  9)
    CYCLES_AL   ( 0,  0,  0,        0,  0,  0,       20, 20,  9)
    CYCLES_DIPC ( 0,  0,  0,        0,  0,  0,       16, 16, 10)
    CYCLES_IXPC ( 0,  0,  0,        0,  0,  0,       20, 20, 12)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execReset(u16 opcode)
{
    AVAILABILITY(C68000)

    SUPERVISOR_MODE_ONLY

    SYNC_68000(128);
    SYNC_68010(126);

    prefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP   ( 0,  0,  0,        0,  0,  0,      132,130,518)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execRtd(u16 opcode)
{
    AVAILABILITY(C68010)

    // Check for address error
    if (misaligned<C>(reg.sp)) {

        setFC<M>();
        readBuffer = u16(readM<C, M, Word>(reg.sp & ~1));
        throw AddressError(makeFrame<AE_SET_RW|AE_SET_DF>(reg.sp));
    }

    u32 newpc = readM<C, M, Long>(reg.sp);
    reg.sp += 4 + i16(queue.irc);

    // Check for address error
    if (misaligned<C>(newpc)) {
        throw AddressError(makeFrame<AE_PROG>(newpc));
    }

    setPC(newpc);
    fullPrefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP   ( 0,  0,  0,        0,  0,  0,        0, 16, 10)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execRte(u16 opcode)
{
    AVAILABILITY(C68000)
    SUPERVISOR_MODE_ONLY

    u16 newsr = 0;
    u32 newpc = 0;

    switch (C) {

        case C68000:
        {
            // TODO: Use pop instead of read (?)
            newsr = (u16)read<C, MEM_DATA, Word>(reg.sp);
            reg.sp += 2;

            newpc = read<C, MEM_DATA, Long>(reg.sp);
            reg.sp += 4;
            break;
        }
        case C68010:
        {
            // TODO: Use pop instead of read (?)
            u16 format = (u16)read<C, MEM_DATA, Word>(reg.sp + 6);

            // Check the frame format
            switch (format >> 12) {

                case 0b0000: // Short format

                    newsr = (u16)read<C, MEM_DATA, Word>(reg.sp + 0);
                    if ((format & 0xFF) == 0x10) {
                        // SYNC(4); // ???? TODO: LOOKS WRONG
                    }
                    newpc = read<C, MEM_DATA, Long>(reg.sp + 2);
                    reg.sp += 8;

                    break;

                case 0b1000: // Long format (keep on reading)
                {
                    newsr = (u16)read<C, MEM_DATA, Word>(reg.sp + 0);
                    if ((format & 0xFF) == 0x10) {
                        // SYNC(4); // ???? TODO: LOOKS WRONG
                    }
                    newpc = read<C, MEM_DATA, Long>(reg.sp + 2);

                    (void)read<C, MEM_DATA, Word>(reg.sp + 8); // special status word
                    (void)read<C, MEM_DATA, Long>(reg.sp + 10); // fault address

                    u16 value = (u16)read<C, MEM_DATA, Word>(reg.sp + 26); // internal information, 16 words
                    u16 version = (value >> 10) & 0xF;

                    if (version != 0) { // TODO: PUT IN CPU VERSION NUMBER (GET FROM REAL CPU)

                        SYNC(4);
                        execException(EXC_FORMAT_ERROR);
                        return;
                    }

                    (void)read<C, MEM_DATA, Word>(reg.sp + 28); // internal information, 16 words

                    (void)read<C, MEM_DATA, Word>(reg.sp + 14); // unused/reserved
                    (void)read<C, MEM_DATA, Word>(reg.sp + 16); // data output buffer
                    (void)read<C, MEM_DATA, Word>(reg.sp + 18); // unused/reserved
                    (void)read<C, MEM_DATA, Word>(reg.sp + 20); // data input buffer
                    (void)read<C, MEM_DATA, Word>(reg.sp + 22); // unused/reserved
                    (void)read<C, MEM_DATA, Word>(reg.sp + 24); // instruction input buffer

                    (void)read<C, MEM_DATA, Long>(reg.sp + 34); // internal information, 16 words
                    (void)read<C, MEM_DATA, Long>(reg.sp + 36); // internal information, 16 words
                    (void)read<C, MEM_DATA, Long>(reg.sp + 38); // internal information, 16 words
                    (void)read<C, MEM_DATA, Long>(reg.sp + 42); // internal information, 16 words
                    (void)read<C, MEM_DATA, Long>(reg.sp + 46); // internal information, 16 words
                    (void)read<C, MEM_DATA, Long>(reg.sp + 50); // internal information, 16 words
                    (void)read<C, MEM_DATA, Long>(reg.sp + 54); // internal information, 16 words

                    reg.sp += 58;
                    break;
                }
                default: // Format error

                    if (!MIMIC_MUSASHI) {

                        reg.sr.n = (format >> 12) & 0b1000;
                        reg.sr.z = 0;
                        reg.sr.v = 0;
                    }

                    SYNC(4);
                    (void)read<C, MEM_DATA, Long>(reg.sp + 2);

                    // reg.sr.c = 1; // Check test case Exceptions/StackFrame/stackframe2
                    execException(EXC_FORMAT_ERROR);
                    return;
            }
            break;
        }
        case C68020:
        {
            while (1) {

                u16 format = (u16)(read<C, MEM_DATA, Word>(reg.sp + 6) >> 12);

                if (format == 0b000) {  // Standard frame

                    // TODO: Use pop instead of read

                    newsr = (u16)read<C, MEM_DATA, Word>(reg.sp);
                    reg.sp += 2;

                    newpc = read<C, MEM_DATA, Long>(reg.sp);
                    reg.sp += 4;

                    (void)read<C, MEM_DATA, Word>(reg.sp);
                    reg.sp += 2;
                    break;

                } else if (format == 0b001) {  // Throwaway frame

                    // TODO: Use pop instead of read

                    newsr = (u16)read<C, MEM_DATA, Word>(reg.sp);
                    reg.sp += 2;

                    (void)read<C, MEM_DATA, Long>(reg.sp);
                    reg.sp += 4;

                    (void)read<C, MEM_DATA, Word>(reg.sp);
                    reg.sp += 2;

                    setSR(newsr);
                    continue;

                } else if (format == 0b010) {  // Trap

                    // TODO: Use pop instead of read

                    newsr = (u16)read<C, MEM_DATA, Word>(reg.sp);
                    reg.sp += 2;

                    newpc = read<C, MEM_DATA, Long>(reg.sp);
                    reg.sp += 4;

                    (void)read<C, MEM_DATA, Word>(reg.sp);
                    reg.sp += 2;

                    (void)read<C, MEM_DATA, Long>(reg.sp);
                    reg.sp += 4;
                    break;

                } else if (format == 0b1011) {

                    // Status register
                    newsr = (u16)pop<C, Word>();

                    // Program counter
                    newpc = pop<C, Long>();

                    // 1011 | Vector offset
                    (void)pop<C, Word>();

                    // Internal register
                    (void)pop<C, Word>();

                    // Special status register
                    (void)pop<C, Word>();

                    // Instruction pipe stage C
                    (void)pop<C, Word>();

                    // Instruction pipe stage B
                    (void)pop<C, Word>();

                    // Data cycle fault address
                    (void)pop<C, Long>();

                    // Internal registers
                    (void)pop<C, Word>();
                    (void)pop<C, Word>();

                    // Data output buffer
                    (void)pop<C, Long>();

                    // Internal registers
                    (void)pop<C, Long>();
                    (void)pop<C, Long>();

                    // Stage B address
                    (void)pop<C, Long>();

                    // Internal registers
                    (void)pop<C, Long>();

                    // Data input buffer
                    (void)pop<C, Long>();

                    // Internal registers
                    (void)pop<C, Word>();
                    (void)pop<C, Long>();

                    // Version#, Internal information
                    (void)pop<C, Word>();

                    // Internal registers
                    (void)pop<C, Long>();
                    (void)pop<C, Long>();
                    (void)pop<C, Long>();
                    (void)pop<C, Long>();
                    (void)pop<C, Long>();
                    (void)pop<C, Long>();
                    (void)pop<C, Long>();
                    (void)pop<C, Long>();
                    (void)pop<C, Long>();
                    break;

                } else {

                    execException(EXC_FORMAT_ERROR);
                    CYCLES_68020(4)
                    return;
                }
            }
            break;
        }
    }

    setSR(newsr);

    // Check for address error
    if (misaligned<C>(newpc)) {
        throw AddressError(makeFrame<AE_PROG>(newpc));
    }

    setPC(newpc);
    fullPrefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP   ( 0,  0,  0,        0,  0,  0,       20, 24, 20)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execRtm(u16 opcode)
{
    AVAILABILITY(C68020)

    fullPrefetch<C, POLL>();

    CYCLES_68020(19)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execRtr(u16 opcode)
{
    AVAILABILITY(C68000)

    // Check for address error
    if (misaligned<C>(reg.sp)) {

        setFC<M>();
        readBuffer = u16(readM<C, M, Word>(reg.sp & ~1));
        throw AddressError(makeFrame<AE_SET_RW|AE_SET_DF>(reg.sp));
    }

    u16 newccr = (u16)readM<C, M, Word>(reg.sp);

    reg.sp += 2;

    u32 newpc = read<C, MEM_DATA, Long>(reg.sp);
    reg.sp += 4;

    setCCR((u8)newccr);

    // Check for address error
    if (misaligned<C>(newpc)) {
        throw AddressError(makeFrame<AE_PROG>(newpc));
    }

    setPC(newpc);
    fullPrefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP   ( 0,  0,  0,        0,  0,  0,       20, 20, 14)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execRts(u16 opcode)
{
    AVAILABILITY(C68000)

    // Check for address error
    if (misaligned<C>(reg.sp)) {

        setFC<M>();
        readBuffer = u16(readM<C, M, Word>(reg.sp & ~1));
        throw AddressError(makeFrame<AE_SET_RW|AE_SET_DF>(reg.sp));
    }

    u32 newpc = readM<C, M, Long>(reg.sp);
    reg.sp += 4;

    // Check for address error
    if (misaligned<C>(newpc)) {
        throw AddressError(makeFrame<AE_PROG>(newpc));
    }

    setPC(newpc);
    fullPrefetch<C, POLL>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP   ( 0,  0,  0,        0,  0,  0,       16, 16, 10)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execSccRg(u16 opcode)
{
    AVAILABILITY(C68000)

    int dst = ( _____________xxx(opcode) );
    u32 ea, data;

    readOp<C, M, S>(dst, &ea, &data);

    data = cond<I>() ? 0xFF : 0;
    prefetch<C, POLL>();
    if constexpr (C == C68000) { if (data) SYNC(2); }

    writeD<S>(dst, data);

    //              00     10  20        00  10  20        00  10  20
    //              .b     .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   (data?6:4,  4,  4,        0,  0,  0,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execSccEa(u16 opcode)
{
    AVAILABILITY(C68000)

    int dst = ( _____________xxx(opcode) );
    u32 ea, data;

    if constexpr (C == C68000) {

        readOp<C, M, Byte>(dst, &ea, &data);

        prefetch<C, POLL>();
        data = cond<I>() ? 0xFF : 0;
        writeOp<C, M, Byte>(dst, ea, data);

    } else {

        // readOp<C, M, Byte, SKIP_READ>(dst, &ea, &data);
        ea = computeEA<C, M, S>(dst);
        updateAn<M, S>(dst);

        if constexpr (M == MODE_AI) SYNC(2);
        if constexpr (M == MODE_PI) SYNC(4);
        if constexpr (M == MODE_PD) SYNC(2);
        if constexpr (M == MODE_IX) SYNC(2);

        prefetch<C, POLL>();
        data = cond<I>() ? 0xFF : 0;
        writeOp<C, M, S>(dst, ea, data);
    }

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_AI   (12, 12, 10,        0,  0,  0,        0,  0,  0)
    CYCLES_PI   (12, 12, 10,        0,  0,  0,        0,  0,  0)
    CYCLES_PD   (14, 14, 11,        0,  0,  0,        0,  0,  0)
    CYCLES_DI   (16, 16, 11,        0,  0,  0,        0,  0,  0)
    CYCLES_IX   (18, 18, 13,        0,  0,  0,        0,  0,  0)
    CYCLES_AW   (16, 16, 10,        0,  0,  0,        0,  0,  0)
    CYCLES_AL   (20, 20, 10,        0,  0,  0,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execStop(u16 opcode)
{
    AVAILABILITY(C68000)

    SUPERVISOR_MODE_ONLY

    u16 src = (u16)readI<C, Word>();

    setSR(src);
    flags |= CPU_IS_STOPPED;
    reg.pc0 = reg.pc;

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP   ( 0,  0,  0,        4,  4,  8,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execSwap(u16 opcode)
{
    AVAILABILITY(C68000)

    int rg  = ( _____________xxx(opcode) );
    u32 dat = readD(rg);

    prefetch<C, POLL>();

    dat = (dat >> 16) | (dat & 0xFFFF) << 16;
    writeD(rg, dat);

    reg.sr.n = NBIT<Long>(dat);
    reg.sr.z = ZERO<Long>(dat);
    reg.sr.v = 0;
    reg.sr.c = 0;

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 0,  0,  0,        4,  4,  4,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execTasRg(u16 opcode)
{
    AVAILABILITY(C68000)

    int dst = ( _____________xxx(opcode) );

    u32 ea, data;
    readOp<C, M, Byte>(dst, &ea, &data);

    reg.sr.n = NBIT<Byte>(data);
    reg.sr.z = ZERO<Byte>(data);
    reg.sr.v = 0;
    reg.sr.c = 0;

    data |= 0x80;
    writeD<S>(dst, data);

    prefetch<C, POLL>();

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execTasEa(u16 opcode)
{
    AVAILABILITY(C68000)

    int dst = ( _____________xxx(opcode) );

    u32 ea, data;

    if (C == C68000) {

        readOp<C, M, Byte>(dst, &ea, &data);

        reg.sr.n = NBIT<Byte>(data);
        reg.sr.z = ZERO<Byte>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;
        data |= 0x80;

        if (!isRegMode(M)) SYNC(2);
        writeOp<C, M, S>(dst, ea, data);

        prefetch<C, POLL>();

    } else {

        if constexpr (M == MODE_AI) SYNC(2);
        if constexpr (M == MODE_PI) SYNC(4);
        if constexpr (M == MODE_PD) SYNC(2);
        if constexpr (M == MODE_IX) SYNC(2);

        readOp<C, M, Byte>(dst, &ea, &data);

        reg.sr.n = NBIT<Byte>(data);
        reg.sr.z = ZERO<Byte>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;
        data |= 0x80;

        // if (!isRegMode(M)) SYNC(2);
        writeOp<C, M, S>(dst, ea, data);

        prefetch<C, POLL>();
        SYNC(2);
    }

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execTrap(u16 opcode)
{
    AVAILABILITY(C68000)

    int nr = ____________xxxx(opcode);

    SYNC(4);
    execException<C>(EXC_TRAP, nr);

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP   ( 0,  0,  0,        0,  0,  0,       34, 38, 20)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execTrapv(u16 opcode)
{
    AVAILABILITY(C68000)

    if (reg.sr.v) {

        if (C == C68000) {
            (void)read<C, MEM_PROG, Word>(reg.pc + 2);
        } else {
            (void)read<C, MEM_PROG, Word>(reg.pc + 2);
            SYNC(2);
        }
        execException<C>(EXC_TRAPV);

        //           00  10  20        00  10  20        00  10  20
        //           .b  .b  .b        .w  .w  .w        .l  .l  .l
        CYCLES_IP   ( 0,  0,  0,        0,  0,  0,       34, 34, 20)

    } else {

        prefetch<C, POLL>();

        //           00  10  20        00  10  20        00  10  20
        //           .b  .b  .b        .w  .w  .w        .l  .l  .l
        CYCLES_IP   ( 0,  0,  0,        0,  0,  0,        4,  4,  4)
    }

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execTrapcc(u16 opcode)
{
    AVAILABILITY(C68020)

    switch (opcode & 0b111) {

        case 0b010: (void)readI<C, Word>(); break;
        case 0b011: (void)readI<C, Long>(); break;
    }

    if (cond<I>()) {

        execException<C>(EXC_TRAPV);
        CYCLES_68020(20);
        return;
    }

    switch (opcode & 0b111) {

        case 0b100: CYCLES_68020(4); break;
        case 0b010: CYCLES_68020(6); break;
        case 0b011: CYCLES_68020(8); break;
    }

    prefetch<C, POLL>();

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execTst(u16 opcode)
{
    AVAILABILITY(C68000)

    int rg = _____________xxx(opcode);

    u32 ea, data;
    readOp<C, M, S, STD_AE_FRAME>(rg, &ea, &data);

    reg.sr.n = NBIT<S>(data);
    reg.sr.z = ZERO<S>(data);
    reg.sr.v = 0;
    reg.sr.c = 0;

    looping<I>() ? noPrefetch<C>(S == Long ? 4 : 2) : prefetch<C, POLL>();
    if (looping<I>() && S == Long) loopModeDelay = 0;

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 4,  4,  2,        4,  4,  2,        4,  4,  2)
    CYCLES_AN   ( 0,  0,  0,        0,  0,  2,        4,  4,  2)
    CYCLES_AI   ( 8,  8,  6,        8,  8,  6,       12, 12,  6)
    CYCLES_PI   ( 8,  8,  6,        8,  8,  6,       12, 12,  6)
    CYCLES_PD   (10, 10,  7,       10, 10,  7,       14, 14,  7)
    CYCLES_DI   (12, 12,  7,       12, 12,  7,       16, 16,  7)
    CYCLES_IX   (14, 14,  9,       14, 14,  9,       18, 18,  9)
    CYCLES_AW   (12, 12,  6,       12, 12,  6,       16, 16,  6)
    CYCLES_AL   (16, 16,  6,       16, 16,  6,       20, 20,  6)
    CYCLES_DIPC ( 0,  0,  7,        0,  0,  7,        0,  0,  7)
    CYCLES_IXPC ( 0,  0,  9,        0,  0,  9,        0,  0,  9)
    CYCLES_IM   ( 0,  0,  6,        0,  0,  6,        0,  0,  6)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execUnlk(u16 opcode)
{
    AVAILABILITY(C68000)

    int an = _____________xxx(opcode);

    // Check for address error
    if (misaligned<C>(readA(an))) {
        throw AddressError(makeFrame<AE_DATA|AE_INC_PC|AE_SET_DF|AE_SET_RW>(readA(an)));
    }

    // Move address register to stack pointer
    reg.sp = readA(an);

    // Update address register
    u32 ea, data;
    readOp<C, MODE_AI, Long, AE_DATA|AE_INC_PC|POLL>(7, &ea, &data);
    writeA(an, data);

    if (an != 7) reg.sp += 4;
    prefetch<C>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_IP   ( 0,  0,  0,       12, 12,  6,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execUnpkDn(u16 opcode)
{
    AVAILABILITY(C68020)

    int dx  = _____________xxx(opcode);
    int dy  = ____xxx_________(opcode);

    u16 adj = (u16)readI<C, Word>();
    u32 src = readD(dx);

    u32 dst = ((src << 4 & 0x0F00) | (src & 0x000F)) + adj;
    writeD<Word>(dy, dst);

    prefetch<C>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_DN   ( 0,  0,  0,        0,  0,  8,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execUnpkPd(u16 opcode)
{
    AVAILABILITY(C68020)

    int ax = _____________xxx(opcode);
    int ay = ____xxx_________(opcode);

    u32 ea, data;
    readOp<C, M, Byte>(ax, &ea, &data);

    u16 adj = (u16)readI<C, Word>();
    u32 dst = ((data << 4 & 0x0F00) | (data & 0x000F)) + adj;

    if (MIMIC_MUSASHI) {

        writeOp<C, M, Byte>(ay, dst >> 8 & 0xFF);
        writeOp<C, M, Byte>(ay, dst & 0xFF);

    } else {

        reg.a[ay]--;
        write<C, MEM_DATA, Byte>(readA(ay), dst & 0xFF);

        reg.a[ay]--;
        write<C, MEM_DATA, Byte>(readA(ay), dst >> 8 & 0xFF);
    }

    prefetch<C>();

    //           00  10  20        00  10  20        00  10  20
    //           .b  .b  .b        .w  .w  .w        .l  .l  .l
    CYCLES_PD   ( 0,  0,  0,        0,  0, 13,        0,  0,  0)

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCinv(u16 opcode)
{
    AVAILABILITY(C68020)

    execLineF<C, I, M, S>(opcode);

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCpush(u16 opcode)
{
    AVAILABILITY(C68020)

    execLineF<C, I, M, S>(opcode);

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove16PiPi(u16 opcode)
{
    AVAILABILITY(C68020)

    execLineF<C, I, M, S>(opcode);

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove16PiAl(u16 opcode)
{
    AVAILABILITY(C68020)

    execLineF<C, I, M, S>(opcode);

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove16AlPi(u16 opcode)
{
    AVAILABILITY(C68020)

    execLineF<C, I, M, S>(opcode);

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove16AiAl(u16 opcode)
{
    AVAILABILITY(C68020)

    execLineF<C, I, M, S>(opcode);

    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove16AlAi(u16 opcode)
{
    AVAILABILITY(C68020)

    execLineF<C, I, M, S>(opcode);

    FINALIZE
}

#include "MoiraExecMMU_cpp.h"
#include "MoiraExecFPU_cpp.h"

#undef AVAILABILITY
#undef SUPERVISOR_MODE_ONLY
#undef FINALIZE
