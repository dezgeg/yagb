#include "Cpu.hpp"
#include "Utils.hpp"

#include <string.h>
#include <stdio.h>

#define INSN_DBG(x) x
#define INSN_DBG_DECL() bool _branched = false; Word branchPc = 0; Regs _savedRegs = regs; _savedRegs.pc -= 1
#define INSN_BRANCH(newPc) (_branched = true, branchPc = regs.pc, regs.pc = (newPc))
#define INSN_DONE(cycles, ...) (log->logInsn(bus, &_savedRegs, cycles, _branched ? branchPc : regs.pc, __VA_ARGS__), cycles)

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

void Cpu::reset()
{
    regs = Regs();
    halted = false;
    stopped = false;
    interruptsEnabled = false;
}

long Cpu::tick()
{
    Byte opc = bus->memRead8(regs.pc++);
    switch (opc >> 6) {
        case 0: return executeInsn_0x_3x(opc);
        case 1: return executeInsn_4x_6x(opc);
        case 2: return executeInsn_7x_Bx(opc);
        case 3: return executeInsn_Cx_Fx(opc);
    }
    unreachable();
}

long Cpu::executeInsn_0x_3x(Byte opc)
{
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
            return INSN_DONE(20, "LD (a16), SP");
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
            return INSN_DONE(4, "DAA");
            unreachable();
        }
        case 0x37: {
            regs.flags.c = true;
            regs.flags.n = regs.flags.h = 0;
            return INSN_DONE(4, "SCF");
        }
        case 0x2f: {
            regs.a = ~regs.a;
            regs.flags.n = regs.flags.h = 0;
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
        case 0x0: case 0x8: {
            char buf[16];

            int delta = (SByte)bus->memRead8(regs.pc++);
            bool taken = evalConditional(opc, buf, "JR");
            if (taken)
                INSN_BRANCH(regs.pc + delta);

            return INSN_DONE(taken ? 12 : 8, "%s r8", buf);
        }
        case 0x1: {
            Word val = bus->memRead16(regs.pc);
            regs.pc += 2;
            regs.words[operand] = val;
            return INSN_DONE(12, "LD %s, d16", reg16SpStrings[operand]);
        }
        case 0x9: {
            regs.hl += regs.words[operand];
            return INSN_DONE(8, "ADD HL, %s", reg16SpStrings[operand]);
            // TODO: flags?
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
        case 0x4: case 0xC: {
            Byte tmp = LOAD8(byteOperand);
            STORE8(byteOperand, doAddSub(tmp, 1, false, false, false));
            return INSN_DONE(4 + 2 * ldst8ExtraCycles(byteOperand), "INC %s",
                             reg8Strings[byteOperand]);
        }
        case 0x5: case 0xD: {
            Byte tmp = LOAD8(byteOperand);
            STORE8(byteOperand, doAddSub(tmp, 1, true, false, false));
            return INSN_DONE(4 + 2 * ldst8ExtraCycles(byteOperand), "DEC %s",
                             reg8Strings[byteOperand]);
        }
        case 0x6: case 0xE: {
            Byte val = bus->memRead8(regs.pc++);
            STORE8(byteOperand, val);
            return INSN_DONE(4 + ldst8ExtraCycles(byteOperand), "LD %s, d8",
                             reg8Strings[byteOperand]);
        }
    }
    unreachable();
}

// Opcodes 4x..6x: Moves between 8-bit regs / (HL)
// Bottom 3 bits = source, next 3 bits destination. Order is: B C D E H L (HL) A
long Cpu::executeInsn_4x_6x(Byte opc)
{
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

bool Cpu::evalConditional(Byte opc, char* outDescr, const char* opcodeStr)
{
    // LSB set means unconditional, except JR r8 (0x18) is a special case.
    if (opc == 0x18 || opc & 1) {
        snprintf(outDescr, strlen(opcodeStr) + 1, "%s", opcodeStr);
        return true;
    }

    bool flagIsCarry = opc & 0x10;
    bool compareVal = opc & 0x08;
    snprintf(outDescr, strlen(opcodeStr) + sizeof(" NZ,"), "%s %s%c,", opcodeStr,
             compareVal ? "" : "N", flagIsCarry ? 'C' : 'Z');
    return bool(flagIsCarry ? regs.flags.c : regs.flags.z) == compareVal;
}

// TODO: not sane to have three bool parameters
Byte Cpu::doAddSub(unsigned lhs, unsigned rhs, bool isSub, bool withCarry, bool updateCarry=true)
{
    rhs = withCarry ? rhs + regs.flags.c : rhs;
    rhs = isSub ? ~rhs + 1 : rhs;

    unsigned sum = lhs + rhs;
    regs.flags.h = (lhs & 0xf) + (rhs & 0xf) > 0xf;
    if (updateCarry)
        regs.flags.c = sum > 0xff;
    regs.flags.z = Byte(sum) == 0;
    regs.flags.n = isSub;

    return sum;
}

Word Cpu::doAdd16(unsigned lhs, unsigned rhs)
{
    // set flags according to the MSB sum operation
    unsigned lsbSum = (lhs & 0xFF) + (rhs & 0xFF);
    doAddSub(lhs >> 8, (rhs >> 8) + lsbSum > 0xFF, false, false);
    return lhs + rhs;
}

Byte Cpu::doRotLeft(Byte v)
{
    regs.flags.z = regs.flags.n = regs.flags.h = 0;
    regs.flags.c = !!(v & 0x80);
    return (v << 1) | !!(v & 0x80);
}

Byte Cpu::doRotLeftWithCarry(Byte v)
{
    regs.flags.z = regs.flags.n = regs.flags.h = 0;
    bool oldMsb = v & 0x80;
    v = (v << 1) | regs.flags.c;
    regs.flags.c = oldMsb;

    return v;
}

Byte Cpu::doRotRight(Byte v)
{
    regs.flags.z = regs.flags.n = regs.flags.h = 0;
    regs.flags.c = !!(v & 0x01);
    return (v >> 1) | ((v & 0x01) << 7);
}

Byte Cpu::doRotRightWithCarry(Byte v)
{
    regs.flags.z = regs.flags.n = regs.flags.h = 0;
    bool oldLsb = v & 0x01;
    v = (v >> 1) | (regs.flags.c << 7);
    regs.flags.c = oldLsb;

    return v;
}

static const char* const aluopStrings[] = {
    "ADD", "ADC", "SUB", "SBC", "AND", "XOR", "OR", "CP",
};

Byte Cpu::doAluOp(int aluop, Byte lhs, Byte rhs)
{
    Byte v;
    switch (aluop) {
        case 0: return doAddSub(lhs, rhs, 0, 0);
        case 1: return doAddSub(lhs, rhs, 0, 1);
        case 2: return doAddSub(lhs, rhs, 1, 0);
        case 3: return doAddSub(lhs, rhs, 1, 1);

        case 4: v = lhs & rhs; regs.flags.z = regs.a == 0; regs.flags.n = 0; regs.flags.c = 0; regs.flags.h = 1; return v;
        case 5: v = lhs ^ rhs; regs.flags.z = regs.a == 0; regs.flags.n = 0; regs.flags.c = 0; regs.flags.h = 0; return v;
        case 6: v = lhs | rhs; regs.flags.z = regs.a == 0; regs.flags.n = 0; regs.flags.c = 0; regs.flags.h = 0; return v;
        case 7: doAddSub(lhs, rhs, 1, 0); return lhs; // SUB with result not saved
    }
    unreachable();
}

// Opcodes 7x..Bx: Accu-based 8-bit alu ops
// Bottom 3 bits = reg/(HL) operand, next 3 bits ALU op. Order is ADD, ADC, SUB, SBC, AND, XOR, OR, CP
long Cpu::executeInsn_7x_Bx(Byte opc)
{
    INSN_DBG_DECL();

    int operand = opc & 0x7;
    int aluop = (opc >> 3) & 0x7;

    regs.a = doAluOp(aluop, regs.a, LOAD8(operand));
    return INSN_DONE(4 + ldst8ExtraCycles(operand),
                     "%s %s", aluopStrings[aluop], reg8Strings[operand]);
}

long Cpu::executeInsn_Cx_Fx(Byte opc)
{
    INSN_DBG_DECL();

    switch (opc) {
        case 0xE0: {
            bus->memWrite8(0xff00 | bus->memRead8(regs.pc++), regs.a);
            return INSN_DONE(12, "LDH (a8), A");
        }
        case 0xF0: {
            regs.a = bus->memRead8(0xff00 | bus->memRead8(regs.pc++));
            return INSN_DONE(12, "LDH A, (a8)");
        }
        case 0xE8: {
            // TODO: nowhere is really documented how the flags are set in this case.
            Byte tmp = bus->memRead8(bus->memRead8(regs.pc++));
            regs.sp = doAdd16(regs.sp, (Word)(SByte)tmp);
            return INSN_DONE(16, "ADD SP, r8");
        }
        case 0xF8: {
            unreachable(); // TODO (affects flags)
            return INSN_DONE(12, "LD HL, SP+r8");
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
            bus->memWrite8(bus->memRead16(regs.pc), regs.a);
            regs.pc += 2;
            return INSN_DONE(16, "LDH (a16), A");
        }
        case 0xFA: {
            regs.a = bus->memRead8(bus->memRead16(regs.pc));
            regs.pc += 2;
            return INSN_DONE(16, "LDH A, (a16)");
        }
        case 0xF3: {
            interruptsEnabled = false;
            return INSN_DONE(4, "DI");
        }
        case 0xCB: {
            return executeTwoByteInsn();
        }
        case 0xFB: {
            interruptsEnabled = true;
            return INSN_DONE(4, "EI");
        }

        case 0xD3: case 0xDB: case 0xDD:
        case 0xE3: case 0xE4: case 0xEB: case 0xEC: case 0xED:
        case 0xF4: case 0xFC: case 0xFD:
            return INSN_DONE(100, "UNDEF");
    }

    Byte operand = (opc >> 4) & 0x3;
    Byte wideOperand = (opc >> 3) & 0x7;
    switch (opc & 0xf) {
        case 0x0: case 0x8: case 0x9: {
            char buf[16];
            bool unconditional = opc & 1;
            bool taken = evalConditional(opc, buf, "RET");
            if (taken) {
                INSN_BRANCH(bus->memRead16(regs.sp));
                regs.sp += 2;
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
        case 0x2: case 0xA: {
            Word addr = bus->memRead16(regs.pc);
            regs.pc += 2;

            char buf[16];
            if(evalConditional(opc, buf, "JP"))
                INSN_BRANCH(addr);
            return INSN_DONE(16, "%s a16", buf);
        }
        case 0xD:
        case 0x4: case 0xC: {
            Word addr = bus->memRead16(regs.pc);
            regs.pc += 2;

            char buf[16];
            bool taken = evalConditional(opc, buf, "CALL");
            if (taken) {
                regs.sp -= 2;
                bus->memWrite16(regs.sp, regs.pc);
                INSN_BRANCH(addr);
            }
            return INSN_DONE(taken ? 24 : 12, "%s a16", buf);
        }
        case 0x5: {
            regs.sp -= 2;
            bus->memWrite16(regs.sp, operand == 3 ? regs.af : regs.words[operand]);
            return INSN_DONE(16, "PUSH %s", reg16AfStrings[operand]);
        }
        case 0x6: case 0xE: {
            regs.a = doAluOp(wideOperand, regs.a, bus->memRead8(regs.pc++));
            return INSN_DONE(8, "%s d8", aluopStrings[wideOperand]);
        }
        case 0x7: case 0xF: {
            regs.sp -= 2;
            bus->memWrite16(regs.sp, regs.pc);
            INSN_BRANCH(wideOperand * 0x10);
            return INSN_DONE(16, "RST 0x%02x", regs.pc);
        }
    }
    unreachable();
}

long Cpu::executeTwoByteInsn()
{
    INSN_DBG_DECL();
    Byte opc = bus->memRead8(regs.pc++);
    const char* description;

    int operand = opc & 0x7;
    int category = opc >> 6;
    int bitIndex = ((opc >> 3) & 0x7);
    Byte bitMask = 1 << bitIndex;
    Byte value = LOAD8(operand);

    if (category == 0) {
        switch ((operand >> 3) & 0x7) {
            case 0: description = "RLC"; value = doRotLeft(value); break;
            case 1: description = "RRC"; value = doRotRight(value); break;
            case 2: description = "RL"; value = doRotLeftWithCarry(value); break;
            case 3: description = "RR"; value = doRotRightWithCarry(value); break;
            case 4: description = "SLA"; regs.flags.c = value & 0x80; value <<= 1; break;
            case 5: description = "SRA"; regs.flags.c = value & 0x01; value = ((SByte)value) >> 1; break;
            case 6: description = "SWAP"; value = ((value & 0xf) << 4) | (value >> 4); break;
            case 7: description = "SRL"; regs.flags.c = value & 0x01; value >>= 1; break;
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

    if (bitIndex < 0)
        return INSN_DONE(8 + ldst8ExtraCycles(operand), "%s %s",
                         description, reg8Strings[operand]);
    else
        return INSN_DONE(8 + 2 * ldst8ExtraCycles(operand), "%s %d, %s",
                         description, bitIndex, reg8Strings[operand]);
}
