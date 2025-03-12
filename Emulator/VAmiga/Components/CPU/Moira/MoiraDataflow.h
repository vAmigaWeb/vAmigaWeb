// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

/* Moira utilizes three layers to model dataflow inside the processor.
 *
 * Layer 1: Main entry points. Most instruction execution handlers call a
 *          layer 1 function to read or write their operands.
 *
 * Layer 2: Data providers. This layer provides functions for moving data back
 *          and forth between various locations.
 *
 * Layer 3: Memory interface. The functions from this layer perform the actual
 *          memory access.
 *
 * The following picture depicts the interplay between the different layers:
 *
 *                                      |
 * - - - - - - - - - - - - - - - - - - -|- - - - - - - - - - - - - - - - - - - -
 * Layer 1:                             V
 *                                    readOp
 *                                   (writeOp)
 *                                      |
 *  Addressing Mode M = 0---1---2---3---4---5---6---7---8---9---A---B
 *                     /    |   |   |   |   |   |   |   |   |   |    \
 *                    /     |   ---------------------------------     \
 *                   /      |           |                              \
 *                  /       |           V                               \
 *                 /        |       computeEA -----------                \
 *                /         |           |               |                 \
 * - - - - - - - -|- - - - -|- - - - - -|- - - - - - - -|- - - - - - - - -|- - -
 * Layer 2:       |         |           |               |                 |
 *                V         V           V               V                 V
 *              readD     readA      readM           readExt            readI
 *             (writeD)  (writeA)   (writeM)
 *                                      |
 * - - - - - - - - - - - - - - - - - - -|- - - - - - - - - - - - - - - - - - - -
 * Layer 3:                             |
 *                 Size S = B-----------W-----------L
 *                          |           |           |
 *                          V           V           V
 *                        read8       read16    2 x read16
 *                       (write8)    (write8)  (2 x write16)
 *
 */

/* Computes an effective address
 */
template <Core C, Mode M, Size S, Flags F = 0> u32 computeEA(u32 n);
template <Core C, Mode M, Size S, Flags F = 0> u32 computeEAbrief(u32 an);
template <Core C, Mode M, Size S, Flags F = 0> u32 computeEAfull(u32 an);

/* Reads an operand
 *
 * The source of the operand is determined by the addressing mode M. If the
 * operand is read from memory, the effective address is stored in variable ea.
 * If the source is a register or an immediate value, variable ea remains
 * untouched.
 */
template <Core C, Mode M, Size S = Word, Flags F = 0>
void readOp(int n, u32 *ea, u32 *result);


/* Writes an operand
 *
 * If parameter ea is omitted, the destination of the operand is determined
 * by the addressing mode M.
 */
template <Core C, Mode M, Size S, Flags F = 0> void writeOp(int n, u32 val);
template <Core C, Mode M, Size S, Flags F = 0> void writeOp(int n, u32 ea, u32 val);

// Emulates the address register modification for modes (An)+, (An)- (DEPRECATED)
template <Mode M, Size S> void updateAn(int n);
template <Mode M, Size S> void updateAnPI(int n);
template <Mode M, Size S> void updateAnPD(int n);
template <Mode M, Size S> void undoAn(int n);
template <Mode M, Size S> void undoAnPI(int n);
template <Mode M, Size S> void undoAnPD(int n);

// Emulates the address register modification for modes (An)+, (An)-
void updateAn(Mode M, Size S, int n);
void updateAnPI(Mode M, Size S, int n);
void updateAnPD(Mode M, Size S, int n);
void undoAn(Mode M, Size S, int n);
void undoAnPI(Mode M, Size S, int n);
void undoAnPD(Mode M, Size S, int n);

// Reads a value from program or data space, depending on the addressing mode
template <Core C, Mode M, Size S, Flags F = 0> u32 readM(u32 addr);

// Reads a value from a specific memory space
template <Core C, MemSpace MS, Size S, Flags F = 0> u32 read(u32 addr);

// Writes an operand to memory (without or with address error checking)
template <Core C, Mode M, Size S, Flags F = 0> void writeM(u32 addr, u32 val);

// Writes a value to a specific memory space
template <Core C, MemSpace MS, Size S, Flags F = 0> void write(u32 addr, u32 val);

// Reads an immediate value from memory
template <Core C, Size S> u32 readI();

// Pushes a value onto the stack
template <Core C, Size S, Flags F = 0> void push(u32 value);

// Pops a value from the stack
template <Core C, Size S, Flags F = 0> u32 pop();

// Checks whether the provided address should trigger an address error
template <Core C, Size S = Word> bool misaligned(u32 addr);

// Creates an address error stack frame
template <Flags F = 0> StackFrame makeFrame(u32 addr, u32 pc, u16 sr, u16 ird);
template <Flags F = 0> StackFrame makeFrame(u32 addr, u32 pc);
template <Flags F = 0> StackFrame makeFrame(u32 addr);

// Prefetches the next instruction
template <Core C, Flags F = 0> void prefetch();

// Performs a full prefetch cycle
template <Core C, Flags F = 0, int delay = 0> void fullPrefetch();

// prefetch replacement if loop mode is active
template <Core C> void noPrefetch(int delay = 0);

// Reads an extension word from memory
template <Core C> void readExt();
template <Core C, Size S> u32 readExt();

// Jumps to an exception vector
template <Core C, Flags F = 0> void jumpToVector(int nr);

// Computes the number of extension words provided in full extension format
int baseDispWords(u16 ext) const;
int outerDispWords(u16 ext) const;

// Computes the cycle penalty for extended 68020 addressing modes
template <Core C, Mode M, Size S> int penaltyCycles(u16 ext) const;
