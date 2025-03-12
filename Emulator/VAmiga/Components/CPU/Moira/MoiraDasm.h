// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

//
// Auxiliary functions
//

// Reads a data value from memory without side-effects
template <Size S = Word> u32 dasmRead(u32 addr) const;

// Increments addr and reads a data value from memory without side-effects
template <Size S = Word> u32 dasmIncRead(u32 &addr) const;

// Assembles an operand
template <Mode M, Size S = Word> Ea<M, S>Op(u16 reg, u32 &pc) const;
