#include "Cpu.hpp"
#include "Serializer.hpp"

#include <stdio.h>

#ifndef CONFIG_NO_INSN_TRACE
#define INSN_DBG(x) (log->insnLoggingEnabled ? (void)(x) : (void)0)
#define INSN_DBG_DECL() bool _branched = false; Word branchPc = 0; Regs _savedRegs = regs; _savedRegs.pc -= 1
#define INSN_BRANCH(newPc) (_branched = true, branchPc = regs.pc, regs.pc = (newPc))
#define INSN_DONE(cycles, ...) (log->logInsn(bus, &_savedRegs, cycles, _branched ? branchPc : regs.pc, __VA_ARGS__), cycles)
#else
#define INSN_DBG(x)
#define INSN_DBG_DECL()
#define INSN_BRANCH(newPc) regs.pc = (newPc)
#define INSN_DONE(cycles, ...) cycles
#endif

static const char* const reg8Strings[] = {
        "B", "C", "D", "E", "H", "L", "(HL)", "A",
};

static const char* const reg16SpStrings[] = {
        "BC", "DE", "HL", "SP",
};

static const char* const reg16AfStrings[] = {
        "BC", "DE", "HL", "AF",
};

static const char* const reg16AutodecStrings[] = {
        "(BC)", "(DE)", "(HL)+", "(HL)-",
};

// 'x' is an instruction encoding for one of the following: B C D E H L (HL) A
// The '^ 1' does the endian swap for little-endian host
#define LOAD8(x) ((x) == 6 ? bus->memRead8(regs.hl) : (x) == 7 ? regs.a : regs.bytes[(x) ^ 1])
#define STORE8(x, v) ((x) == 6 ? bus->memWrite8(regs.hl, v) : (void)(((x) == 7 ? regs.a : regs.bytes[(x) ^ 1]) = (v)))

#define ldst8ExtraCycles(x) ((x) == 6 ? 4 : 0)

#define LOAD8_AUTODEC(x) (bus->memRead8((x) == 2 ? regs.hl++ : (x) == 3 ? regs.hl-- : regs.words[x]))
#define STORE8_AUTODEC(x, v) (bus->memWrite8((x) == 2 ? regs.hl++ : (x) == 3 ? regs.hl-- : regs.words[x], (v)))

void Cpu::reset() {
    regs = Regs();
    halted = false;
    stopped = false;
}

long Cpu::tick() {
    Byte irqs = bus->getPendingIrqs();
    if (irqs) {
        halted = stopped = false;
    }

    if (regs.irqsEnabled && irqs) {
        int irq = ffs(irqs ^ (irqs & (irqs - 1))) - 1;
        bus->ackIrq((Irq)irq);
        log->logDebug("Handling IRQ %d", irq);

        regs.sp -= 2;
        bus->memWrite16(regs.sp, regs.pc);
        regs.pc = 0x40 + irq * 0x8;
        regs.irqsEnabled = false;
        return 12; // TODO: what's the delay?
    }

    if (halted || stopped) {
        return 4;
    }

    Byte opc = bus->memRead8(regs.pc++);
    switch (opc >> 6) {
        case 0:
            return executeInsn_0x_3x(opc);
        case 1:
            return executeInsn_4x_6x(opc);
        case 2:
            return executeInsn_7x_Bx(opc);
        case 3:
            return executeInsn_Cx_Fx(opc);
    }
    unreachable();
}

bool Cpu::evalConditional(Byte opc, char* outDescr, const char* opcodeStr) {
    // LSB set means unconditional, except JR r8 (0x18) is a special case.
    if (opc == 0x18 || opc & 1) {
        INSN_DBG(snprintf(outDescr, strlen(opcodeStr) + 1, "%s", opcodeStr));
        return true;
    }

    bool flagIsCarry = opc & 0x10;
    bool compareVal = opc & 0x08;
    INSN_DBG(snprintf(outDescr, strlen(opcodeStr) + sizeof(" NZ,"), "%s %s%c,", opcodeStr,
            compareVal ? "" : "N", flagIsCarry ? 'C' : 'Z'));
    return bool(flagIsCarry ? regs.flags.c : regs.flags.z) == compareVal;
}

enum AddSubFlags {
    AS_IsSub = 1 << 0,
    AS_WithCarry = 1 << 1,
    AS_UpdateCarry = 1 << 2,
    AS_UpdateZero = 1 << 3,
};

Byte Cpu::doAddSub(unsigned lhs, unsigned rhs, unsigned flags) {
    unsigned carry = (flags & AS_WithCarry) && regs.flags.c;
    unsigned result;
    if (!(flags & AS_IsSub)) {
        regs.flags.h = (lhs & 0xf) + (rhs & 0xf) + carry > 0xf;
        result = lhs + rhs + carry;
    } else {
        regs.flags.h = (lhs & 0xf) - (rhs & 0xf) - carry > 0xf;
        result = lhs - rhs - carry;
    }

    if (flags & AS_UpdateCarry) {
        regs.flags.c = result > 0xff;
    }
    if (flags & AS_UpdateZero) {
        regs.flags.z = Byte(result) == 0;
    }
    regs.flags.n = !!(flags & AS_IsSub);

    return result;
}

Word Cpu::doAdd16(unsigned lhs, unsigned rhs) {
    // set flags according to the MSB sum operation
    unsigned lsbSum = (lhs & 0xFF) + (rhs & 0xFF);
    regs.flags.c = lsbSum > 0xFF; // Slight hack?
    doAddSub(lhs >> 8, rhs >> 8, AS_WithCarry | AS_UpdateCarry);
    return lhs + rhs;
}

Byte Cpu::doRotLeft(Byte v) {
    regs.flags.z = regs.flags.n = regs.flags.h = 0;
    regs.flags.c = !!(v & 0x80);
    return (v << 1) | !!(v & 0x80);
}

Byte Cpu::doRotLeftWithCarry(Byte v) {
    regs.flags.z = regs.flags.n = regs.flags.h = 0;
    bool oldMsb = v & 0x80;
    v = (v << 1) | regs.flags.c;
    regs.flags.c = oldMsb;

    return v;
}

Byte Cpu::doRotRight(Byte v) {
    regs.flags.z = regs.flags.n = regs.flags.h = 0;
    regs.flags.c = !!(v & 0x01);
    return (v >> 1) | ((v & 0x01) << 7);
}

Byte Cpu::doRotRightWithCarry(Byte v) {
    regs.flags.z = regs.flags.n = regs.flags.h = 0;
    bool oldLsb = v & 0x01;
    v = (v >> 1) | (regs.flags.c << 7);
    regs.flags.c = oldLsb;

    return v;
}

static const char* const aluopStrings[] = {
        "ADD", "ADC", "SUB", "SBC", "AND", "XOR", "OR", "CP",
};

Byte Cpu::doAluOp(int aluop, Byte lhs, Byte rhs) {
    Byte v;
    switch (aluop) {
        case 0:
            return doAddSub(lhs, rhs, AS_UpdateCarry | AS_UpdateZero);
        case 1:
            return doAddSub(lhs, rhs, AS_UpdateCarry | AS_UpdateZero | AS_WithCarry);
        case 2:
            return doAddSub(lhs, rhs, AS_UpdateCarry | AS_UpdateZero | AS_IsSub);
        case 3:
            return doAddSub(lhs, rhs, AS_UpdateCarry | AS_UpdateZero | AS_IsSub | AS_WithCarry);

        case 4:
            v = lhs & rhs;
            regs.flags.z = v == 0;
            regs.flags.n = 0;
            regs.flags.c = 0;
            regs.flags.h = 1;
            return v;
        case 5:
            v = lhs ^ rhs;
            regs.flags.z = v == 0;
            regs.flags.n = 0;
            regs.flags.c = 0;
            regs.flags.h = 0;
            return v;
        case 6:
            v = lhs | rhs;
            regs.flags.z = v == 0;
            regs.flags.n = 0;
            regs.flags.c = 0;
            regs.flags.h = 0;
            return v;
        case 7:
            doAddSub(lhs, rhs, AS_IsSub | AS_UpdateCarry | AS_UpdateZero);
            return lhs; // SUB with result not saved
    }
    unreachable();
}

long Cpu::executeInsn_0x_3x(Byte opc) {
    INSN_DBG_DECL();

    // First, various special cases.
    switch (opc) {
        case 0x00: {
            return INSN_DONE(4, "NOP");
        }
        case 0x10: {
            stopped = true;
            return INSN_DONE(4, "STOP");
        }
        case 0x08: {
            Word addr = bus->memRead16(regs.pc);
            regs.pc += 2;
            bus->memWrite16(addr, regs.sp);
            return INSN_DONE(20, "LD (0x%04x), SP", addr);
        }
        case 0x07: {
            regs.a = doRotLeft(regs.a);
            return INSN_DONE(4, "RLCA");
        }
        case 0x17: {
            regs.a = doRotLeftWithCarry(regs.a);
            return INSN_DONE(4, "RLA");
        }
        case 0x0f: {
            regs.a = doRotRight(regs.a);
            return INSN_DONE(4, "RRCA");
        }
        case 0x1f: {
            regs.a = doRotRightWithCarry(regs.a);
            return INSN_DONE(4, "RRA");
        }
        case 0x27: {
            // TODO: flags aren't still being set correctly?
            Byte corr = 0;
            if (regs.flags.h || (regs.a & 0xf) >= 10) {
                corr += 0x06;
            }
            if (regs.flags.c || regs.a > 0x99) {
                corr += 0x60;
            }

            regs.flags.c = corr >= 0x60;
            regs.a = doAddSub(regs.a, corr,
                    AS_UpdateZero | (regs.flags.n ? AS_IsSub : 0));

            return INSN_DONE(4, "DAA");
        }
        case 0x37: {
            regs.flags.c = true;
            regs.flags.n = regs.flags.h = 0;
            return INSN_DONE(4, "SCF");
        }
        case 0x2f: {
            regs.a = ~regs.a;
            regs.flags.n = regs.flags.h = 1;
            return INSN_DONE(4, "CPL");
        }
        case 0x3f: {
            regs.flags.c = !regs.flags.c;
            regs.flags.n = regs.flags.h = 0;
            return INSN_DONE(4, "CCF");
        }
    }

    // Then the somewhat column-oriented opcodes
    Byte operand = (opc >> 4) & 0x3;
    Byte byteOperand = (operand << 1) | !!(opc & 0x8);
    switch (opc & 0xf) {
        case 0x0:
        case 0x8: {
            char buf[16];

            int delta = (SByte)bus->memRead8(regs.pc++);
            bool taken = evalConditional(opc, buf, "JR");
            if (taken)
                INSN_BRANCH(regs.pc + delta);

            return INSN_DONE(taken ? 12 : 8, "%s 0x%04x", buf, regs.pc);
        }
        case 0x1: {
            Word val = bus->memRead16(regs.pc);
            regs.pc += 2;
            regs.words[operand] = val;
            return INSN_DONE(12, "LD %s, 0x%04x", reg16SpStrings[operand], val);
        }
        case 0x9: {
            regs.hl = doAdd16(regs.hl, regs.words[operand]); // TODO: are flags really correct?
            return INSN_DONE(8, "ADD HL, %s", reg16SpStrings[operand]);
        }
        case 0x2: {
            STORE8_AUTODEC(operand, regs.a);
            return INSN_DONE(8, "LD %s, A", reg16AutodecStrings[operand]);
        }
        case 0xA: {
            regs.a = LOAD8_AUTODEC(operand);
            return INSN_DONE(8, "LD A, %s", reg16AutodecStrings[operand]);
        }
        case 0x3: {
            regs.words[operand]++;
            return INSN_DONE(8, "INC %s", reg16SpStrings[operand]);
        }
        case 0xB: {
            regs.words[operand]--;
            return INSN_DONE(8, "DEC %s", reg16SpStrings[operand]);
        }
        case 0x4:
        case 0xC: {
            Byte tmp = LOAD8(byteOperand);
            STORE8(byteOperand, doAddSub(tmp, 1, AS_UpdateZero));
            return INSN_DONE(4 + 2 * ldst8ExtraCycles(byteOperand), "INC %s",
                    reg8Strings[byteOperand]);
        }
        case 0x5:
        case 0xD: {
            Byte tmp = LOAD8(byteOperand);
            STORE8(byteOperand, doAddSub(tmp, 1, AS_IsSub | AS_UpdateZero));
            return INSN_DONE(4 + 2 * ldst8ExtraCycles(byteOperand), "DEC %s",
                    reg8Strings[byteOperand]);
        }
        case 0x6:
        case 0xE: {
            Byte val = bus->memRead8(regs.pc++);
            STORE8(byteOperand, val);
            return INSN_DONE(4 + ldst8ExtraCycles(byteOperand), "LD %s, 0x%02x",
                    reg8Strings[byteOperand], val);
        }
    }
    unreachable();
}

// Opcodes 4x..6x: Moves between 8-bit regs / (HL)
// Bottom 3 bits = source, next 3 bits destination. Order is: B C D E H L (HL) A
long Cpu::executeInsn_4x_6x(Byte opc) {
    INSN_DBG_DECL();

    // Exception: opc 0x76 == LD (HL), (HL) is HALT instead
    if (opc == 0x76) {
        halted = true;
        return INSN_DONE(4, "HALT");
    }

    int dest = (opc >> 3) & 0x7;
    int src = opc & 0x7;

    Byte val = LOAD8(src);
    STORE8(dest, val);
    return INSN_DONE(4 + ldst8ExtraCycles(src) + ldst8ExtraCycles(dest),
            "LD %s, %s", reg8Strings[dest], reg8Strings[src]);
}

// Opcodes 7x..Bx: Accu-based 8-bit alu ops
// Bottom 3 bits = reg/(HL) operand, next 3 bits ALU op. Order is ADD, ADC, SUB, SBC, AND, XOR, OR, CP
long Cpu::executeInsn_7x_Bx(Byte opc) {
    INSN_DBG_DECL();

    int operand = opc & 0x7;
    int aluop = (opc >> 3) & 0x7;

    regs.a = doAluOp(aluop, regs.a, LOAD8(operand));
    return INSN_DONE(4 + ldst8ExtraCycles(operand),
            "%s %s", aluopStrings[aluop], reg8Strings[operand]);
}

long Cpu::executeInsn_Cx_Fx(Byte opc) {
    INSN_DBG_DECL();

    switch (opc) {
        case 0xE0: {
            Word address = 0xff00 | bus->memRead8(regs.pc++);
            bus->memWrite8(address, regs.a);
            return INSN_DONE(12, "LDH (0x%04x), A", address);
        }
        case 0xF0: {
            Word address = 0xff00 | bus->memRead8(regs.pc++);
            regs.a = bus->memRead8(address);
            return INSN_DONE(12, "LDH A, (0x%04x)", address);
        }
        case 0xE8: {
            // TODO: nowhere is really documented how the flags are set in this case.
            SByte tmp = (SByte)bus->memRead8(regs.pc++);
            regs.sp = doAdd16(regs.sp, (Word)tmp);
            regs.flags.z = 0;
            return INSN_DONE(16, "ADD SP, %d", tmp);
        }
        case 0xF8: {
            // TODO: not sure about these flags either
            SByte tmp = (SByte)bus->memRead8(regs.pc++);
            regs.hl = doAdd16(regs.sp, (Word)tmp);
            regs.flags.z = 0;
            return INSN_DONE(12, "LD HL, SP + %d", tmp);
        }
        case 0xE9: {
            INSN_BRANCH(regs.hl);
            return INSN_DONE(4, "JP HL");
        }
        case 0xF9: {
            regs.sp = regs.hl;
            return INSN_DONE(8, "LD SP, HL");
        }
        case 0xE2: {
            bus->memWrite8(0xff00 | regs.c, regs.a);
            return INSN_DONE(8, "LDH (C), A");
        }
        case 0xF2: {
            regs.a = bus->memRead8(0xff00 | regs.c);
            return INSN_DONE(8, "LDH A, (C)");
        }
        case 0xEA: {
            Word address = bus->memRead16(regs.pc);
            bus->memWrite8(address, regs.a);
            regs.pc += 2;
            return INSN_DONE(16, "LD (0x%04x), A", address);
        }
        case 0xFA: {
            Word address = bus->memRead16(regs.pc);
            regs.a = bus->memRead8(address);
            regs.pc += 2;
            return INSN_DONE(16, "LD A, (0x%04x)", address);
        }
        case 0xF3: {
            regs.irqsEnabled = false;
            return INSN_DONE(4, "DI");
        }
        case 0xCB: {
            return executeTwoByteInsn();
        }
        case 0xFB: {
            regs.irqsEnabled = true;
            return INSN_DONE(4, "EI");
        }

        case 0xD3:
        case 0xDB:
        case 0xDD:
        case 0xE3:
        case 0xE4:
        case 0xEB:
        case 0xEC:
        case 0xED:
        case 0xF4:
        case 0xFC:
        case 0xFD:
            return INSN_DONE(100, "UNDEF");
    }

    Byte operand = (opc >> 4) & 0x3;
    Byte wideOperand = (opc >> 3) & 0x7;
    switch (opc & 0xf) {
        case 0x0:
        case 0x8:
        case 0x9: {
            char buf[16];
            bool unconditional = opc & 1;
            bool taken = evalConditional(opc, buf, "RET");
            if (taken) {
                INSN_BRANCH(bus->memRead16(regs.sp));
                regs.sp += 2;
            }
            if (opc == 0xd9) {
                regs.irqsEnabled = true;
                strcpy(buf, "RETI");
            }
            return INSN_DONE(unconditional ? 16 : taken ? 20 : 8, "%s", buf);
        }
        case 0x1: {
            Word value = bus->memRead16(regs.sp);
            regs.sp += 2;
            if (operand == 3) {
                regs.af = value;
                regs.flags.unimplemented = 0;
            } else {
                regs.words[operand] = value;
            }
            return INSN_DONE(12, "POP %s", reg16AfStrings[operand]);
        }
        case 0x3:
        case 0x2:
        case 0xA: {
            Word addr = bus->memRead16(regs.pc);
            regs.pc += 2;

            char buf[16];
            bool taken = evalConditional(opc, buf, "JP");
            if (taken)
                INSN_BRANCH(addr);
            return INSN_DONE(taken ? 16 : 12, "%s 0x%04x", buf, addr);
        }
        case 0xD:
        case 0x4:
        case 0xC: {
            Word addr = bus->memRead16(regs.pc);
            regs.pc += 2;

            char buf[16];
            bool taken = evalConditional(opc, buf, "CALL");
            if (taken) {
                regs.sp -= 2;
                bus->memWrite16(regs.sp, regs.pc);
                INSN_BRANCH(addr);
            }
            return INSN_DONE(taken ? 24 : 12, "%s 0x%04x", buf, addr);
        }
        case 0x5: {
            regs.sp -= 2;
            bus->memWrite16(regs.sp, operand == 3 ? regs.af : regs.words[operand]);
            return INSN_DONE(16, "PUSH %s", reg16AfStrings[operand]);
        }
        case 0x6:
        case 0xE: {
            Byte value = bus->memRead8(regs.pc++);
            regs.a = doAluOp(wideOperand, regs.a, value);
            return INSN_DONE(8, "%s 0x%02x", aluopStrings[wideOperand], value);
        }
        case 0x7:
        case 0xF: {
            regs.sp -= 2;
            bus->memWrite16(regs.sp, regs.pc);
            INSN_BRANCH(wideOperand * 0x08);
            return INSN_DONE(16, "RST 0x%02x", regs.pc);
        }
    }
    unreachable();
}

long Cpu::executeTwoByteInsn() {
    INSN_DBG_DECL();
    Byte opc = bus->memRead8(regs.pc++);
    const char* description __attribute__((unused));

    int operand = opc & 0x7;
    int category = opc >> 6;
    int bitIndex = ((opc >> 3) & 0x7);
    Byte bitMask = 1 << bitIndex;
    Byte value = LOAD8(operand);

    if (category == 0) {
        switch ((opc >> 3) & 0x7) {
            case 0:
                description = "RLC";
                value = doRotLeft(value);
                break;
            case 1:
                description = "RRC";
                value = doRotRight(value);
                break;
            case 2:
                description = "RL";
                value = doRotLeftWithCarry(value);
                break;
            case 3:
                description = "RR";
                value = doRotRightWithCarry(value);
                break;
            case 4:
                description = "SLA";
                regs.flags.c = !!(value & 0x80);
                value <<= 1;
                break;
            case 5:
                description = "SRA";
                regs.flags.c = value & 0x01;
                value = ((SByte)value) >> 1;
                break;
            case 6:
                description = "SWAP";
                value = ((value & 0xf) << 4) | (value >> 4);
                regs.flags.c = 0;
                break;
            case 7:
                description = "SRL";
                regs.flags.c = value & 0x01;
                value >>= 1;
                break;
        }
        regs.flags.n = regs.flags.h = 0;
        regs.flags.z = value == 0;
        bitIndex = -1;
    } else if (category == 1) {
        description = "BIT";
        regs.flags.n = 0;
        regs.flags.h = 1;
        regs.flags.z = !(value & bitMask);
    } else if (category == 2) {
        description = "RES";
        value &= ~bitMask;
    } else {
        description = "SET";
        value |= bitMask;
    }

    if (category != 1) // no writeback for BIT
        STORE8(operand, value);

    if (bitIndex < 0) {
        return INSN_DONE(8 + 2 * ldst8ExtraCycles(operand), "%s %s",
                description, reg8Strings[operand]);
    } else {
        return INSN_DONE(8 + 2 * ldst8ExtraCycles(operand), "%s %d, %s",
                description, bitIndex, reg8Strings[operand]);
    }
}

void Cpu::serialize(Serializer& ser) {
    ser.handleObject("Cpu.regs", regs);
    ser.handleObject("Cpu.halted", halted);
    ser.handleObject("Cpu.stopped", stopped);
}
