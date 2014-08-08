#include "Cpu.hpp"
#include "Gameboy.hpp"
#include "Utils.hpp"

#define INSN_DBG(x) x

#define INSN_DBG_TRACE(...) (gb->logInsn(&_savedRegs, __VA_ARGS__))

#define INSN_DBG_DECL() Regs _savedRegs = regs; _savedRegs.pc -= 1

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

#define LOAD8(x) ((x) == 6 ? gb->memRead8(regs.hl) : regs.bytes[x])
#define STORE8(x, v) ((x) == 6 ? gb->memWrite8(regs.hl, v) : (void)(regs.bytes[x] = (v)))

#define LOAD8_AUTODEC(x) (gb->memRead8((x) == 2 ? regs.hl++ : (x) == 3 ? regs.hl-- : regs.words[x]))
#define STORE8_AUTODEC(x, v) (gb->memWrite16((x) == 2 ? regs.hl++ : (x) == 3 ? regs.hl-- : regs.words[x], (v)))

void Cpu::reset()
{
    regs = Regs();
    halted = false;
    stopped = false;
    interruptsEnabled = false;
}

void Cpu::tick()
{
    Byte opc = gb->memRead8(regs.pc++);
    switch (opc >> 6) {
        case 0: executeInsn_0x_3x(opc); break;
        case 1: executeInsn_4x_6x(opc); break;
        case 2: executeInsn_7x_Bx(opc); break;
        case 3: executeInsn_Cx_Fx(opc); break;
    }
}

void Cpu::executeInsn_0x_3x(Byte opc)
{
    INSN_DBG_DECL();

    // First, various special cases.
    switch (opc) {
        case 0x00: {
            INSN_DBG_TRACE("NOP");
            return;
        }
        case 0x10: {
            stopped = true;
            INSN_DBG_TRACE("STOP");
            return;
        }
        case 0x08: {
            Word addr = gb->memRead16(regs.pc);
            regs.pc += 2;
            gb->memWrite16(addr, regs.sp);
            INSN_DBG_TRACE("LD (a16), SP");
            return;
        }
        case 0x07: {
            regs.flags.c = !!(regs.a & 0x80);
            regs.a = (regs.a << 1) | !!(regs.a & 0x80);
            regs.flags.z = regs.flags.n = regs.flags.h = 0;
            INSN_DBG_TRACE("RLCA");
            return;
        }
        case 0x17: {
            bool oldMsb = regs.a & 0x80;
            regs.a = (regs.a << 1) | regs.flags.c;
            regs.flags.c = oldMsb;
            regs.flags.z = regs.flags.n = regs.flags.h = 0;
            INSN_DBG_TRACE("RLA");
            return;
        }
        case 0x0f: {
            regs.flags.c = !!(regs.a & 0x01);
            regs.a = (regs.a >> 1) | !!(regs.a & 0x01);
            regs.flags.z = regs.flags.n = regs.flags.h = 0;
            INSN_DBG_TRACE("RRCA");
            return;
        }
        case 0x1f: {
            bool oldLsb = regs.a & 0x01;
            regs.a = (regs.a >> 1) | (regs.flags.c << 7);
            regs.flags.c = oldLsb;
            regs.flags.z = regs.flags.n = regs.flags.h = 0;
            INSN_DBG_TRACE("RRA");
            return;
        }
        case 0x27: {
            INSN_DBG_TRACE("DAA");
            unreachable();
            return;
        }
        case 0x37: {
            regs.flags.c = true;
            regs.flags.n = regs.flags.h = 0;
            INSN_DBG_TRACE("SCF");
            return;
        }
        case 0x2f: {
            regs.a = ~regs.a;
            regs.flags.n = regs.flags.h = 0;
            INSN_DBG_TRACE("CPL");
            return;
        }
        case 0x3f: {
            regs.flags.c = !regs.flags.c;
            regs.flags.n = regs.flags.h = 0;
            INSN_DBG_TRACE("CCF");
            return;
        }
    }

    // Then the somewhat column-oriented opcodes
    Byte operand = (opc >> 4) & 0x3;
    Byte byteOperand = (operand << 1) | !!(opc & 0x8);
    switch (opc & 0xf) {
        case 0x0: case 0x8: {
            const char * const ccMap[] = { "??", "", "Z, ", "C, " };
            int cc = opc >> 4;
            bool notOk = cc == 1 ? 1 : cc == 2 ? regs.flags.z : regs.flags.c;

            int delta = (SByte)gb->memRead8(regs.pc++);
            if (notOk ^ !!(opc & 0x08))
                regs.pc += delta;

            INSN_DBG_TRACE("JR %s%sr8", notOk ? "N" : "", ccMap[cc]);
            return;
        }
        case 0x1: {
            Word val = gb->memRead16(regs.pc);
            regs.pc += 2;
            regs.words[operand] = val;
            INSN_DBG_TRACE("LD %s, d16", reg16SpStrings[operand]);
            return;
        }
        case 0x9: {
            regs.hl += regs.words[operand];
            INSN_DBG_TRACE("ADD HL, %s", reg16SpStrings[operand]);
            // TODO: flags?
            return;
        }
        case 0x2: {
            STORE8_AUTODEC(operand, regs.a);
            INSN_DBG_TRACE("LD %s, A", reg16AutodecStrings[operand]);
            return;
        }
        case 0xA: {
            regs.a = LOAD8_AUTODEC(operand);
            INSN_DBG_TRACE("LD A, %s", reg16AutodecStrings[operand]);
            return;
        }
        case 0x3: {
            regs.words[operand]++;
            INSN_DBG_TRACE("INC %s", reg16SpStrings[operand]);
            return;
        }
        case 0xB: {
            regs.words[operand]--;
            INSN_DBG_TRACE("DEC %s", reg16SpStrings[operand]);
            return;
        }
        case 0x4: case 0xC: {
            Byte tmp = LOAD8(byteOperand);
            STORE8(byteOperand, doAddSub(tmp, 1, false, false, false));
            INSN_DBG_TRACE("INC %s", reg8Strings[byteOperand]);
            return;
        }
        case 0x5: case 0xD: {
            Byte tmp = LOAD8(byteOperand);
            STORE8(byteOperand, doAddSub(tmp, 1, true, false, false));
            INSN_DBG_TRACE("DEC %s", reg8Strings[byteOperand]);
            return;
        }
        case 0x6: case 0xE: {
            Byte val = gb->memRead8(regs.pc++);
            STORE8(byteOperand, val);
            INSN_DBG_TRACE("LD %s, d8", reg8Strings[byteOperand]);
            return;
        }
    }
    unreachable();
}

// Opcodes 4x..6x: Moves between 8-bit regs / (HL)
// Bottom 3 bits = source, next 3 bits destination. Order is: B C D E H L (HL) A
void Cpu::executeInsn_4x_6x(Byte opc)
{
    INSN_DBG_DECL();

    // Exception: opc 0x76 == LD (HL), (HL) is HALT instead
    if (opc == 0x76) {
        INSN_DBG_TRACE("HALT");
        halted = true;
        return;
    }

    int dest = (opc >> 3) & 0x7;
    int src = opc & 0x7;
    INSN_DBG_TRACE("LD %s, %s", reg8Strings[dest], reg8Strings[src]);

    Byte val = LOAD8(src);
    STORE8(dest, val);
}

// TODO: not sane to have three bool parameters
Byte Cpu::doAddSub(unsigned lhs, unsigned rhs, bool isSub, bool withCarry, bool updateCarry=true)
{
    rhs = withCarry ? rhs + 1 : rhs;
    rhs = isSub ? ~rhs + 1 : rhs;

    unsigned sum = lhs + rhs;
    regs.flags.h = (lhs & 0xf) + (rhs & 0xf) > 0xf;
    if (updateCarry)
        regs.flags.c = sum > 0xff;
    regs.flags.z = Byte(sum) == 0;
    regs.flags.n = isSub;

    return sum;
}

static const char* const aluopStrings[] = {
    "ADD", "ADC", "SUB", "SBC", "AND", "XOR", "OR", "CP",
};
// Opcodes 7x..Bx: Accu-based 8-bit alu ops
// Bottom 3 bits = reg/(HL) operand, next 3 bits ALU op. Order is ADD, ADC, SUB, SBC, AND, XOR, OR, CP
void Cpu::executeInsn_7x_Bx(Byte opc)
{
    INSN_DBG_DECL();

    int operand = opc & 0x7;
    int aluop = (opc >> 3) & 0x7;
    INSN_DBG_TRACE("%s %s", aluopStrings[aluop], reg8Strings[operand]);

    Byte lhs = regs.a;
    Byte rhs = LOAD8(operand);
    switch (aluop) {
        case 0: regs.a = doAddSub(lhs, rhs, 0, 0); break;
        case 1: regs.a = doAddSub(lhs, rhs, 0, 1); break;
        case 2: regs.a = doAddSub(lhs, rhs, 1, 0); break;
        case 3: regs.a = doAddSub(lhs, rhs, 1, 1); break;

        case 4: regs.a = lhs & rhs; regs.flags.z = regs.a == 0; regs.flags.n = 0; regs.flags.c = 0; regs.flags.h = 1; break;
        case 5: regs.a = lhs ^ rhs; regs.flags.z = regs.a == 0; regs.flags.n = 0; regs.flags.c = 0; regs.flags.h = 0; break;
        case 6: regs.a = lhs | rhs; regs.flags.z = regs.a == 0; regs.flags.n = 0; regs.flags.c = 0; regs.flags.h = 0; break;
        case 7: doAddSub(lhs, rhs, 1, 0); break; // SUB with result not saved
    }
}

void Cpu::executeInsn_Cx_Fx(Byte opc)
{
    INSN_DBG_DECL();

    switch (opc) {
        case 0xE0: {
            gb->memWrite8(0xff00 | gb->memRead8(regs.pc++), regs.a);
            INSN_DBG_TRACE("LDH (a8), A");
            return;
        }
        case 0xF0: {
            regs.a = gb->memRead8(0xff00 | gb->memRead8(regs.pc++));
            INSN_DBG_TRACE("LDH A, (a8)");
            return;
        }
        case 0xE8: {
            unreachable(); // TODO (affects flags)
            INSN_DBG_TRACE("ADD SP, r8");
            return;
        }
        case 0xF8: {
            unreachable(); // TODO (affects flags)
            INSN_DBG_TRACE("LD HL, SP+r8");
            return;
        }
        case 0xE9: {
            regs.pc = regs.hl;
            INSN_DBG_TRACE("JP HL");
            return;
        }
        case 0xF9: {
            regs.sp = regs.hl;
            INSN_DBG_TRACE("LD SP, HL");
            return;
        }
        case 0xE2: {
            gb->memWrite8(0xff00 | regs.c, regs.a);
            INSN_DBG_TRACE("LDH (C), A");
            return;
        }
        case 0xF2: {
            regs.a = gb->memRead8(0xff00 | regs.c);
            INSN_DBG_TRACE("LDH A, (C)");
            return;
        }
        case 0xEA: {
            gb->memWrite8(gb->memRead16(regs.pc), regs.a);
            regs.pc += 2;
            INSN_DBG_TRACE("LDH (a16), A");
            return;
        }
        case 0xFA: {
            regs.a = gb->memRead8(gb->memRead16(regs.pc));
            regs.pc += 2;
            INSN_DBG_TRACE("LDH A, (a16)");
            return;
        }
        case 0xF3: {
            interruptsEnabled = false;
            INSN_DBG_TRACE("DI");
            return;
        }
        case 0xCB: {
            executeTwoByteInsn();
            return;
        }
        case 0xFB: {
            interruptsEnabled = true;
            INSN_DBG_TRACE("EI");
            return;
        }

        case 0xD3: case 0xDB: case 0xDD:
        case 0xE3: case 0xE4: case 0xEB: case 0xEC: case 0xED:
        case 0xF4: case 0xFC: case 0xFD:
            INSN_DBG_TRACE("UNDEF");
            return;
    }

    Byte operand = (opc >> 4) & 0x3;
    bool unconditional = false;
    switch (opc & 0xf) {
        case 0x0: case 0x8: case 0x9: {
            unreachable();
            return;
        }
        case 0x1: {
            Word value = gb->memRead16(regs.sp);
            regs.sp += 2;
            if (operand == 3) {
                regs.af = value;
                regs.flags.unimplemented = 0;
            } else {
                regs.words[operand] = value;
            }
            INSN_DBG_TRACE("POP %s", reg16AfStrings[operand]);
            return;
        }
        case 0x3:
            unconditional = true; // FALLTHRU
        case 0x2: case 0xA: {
            unreachable();
            return;
        }
        case 0xD:
            unconditional = true; // FALLTHRU
        case 0x4: case 0xC: {
            Word addr = gb->memRead16(regs.pc);
            regs.pc += 2;
            bool condbit = opc & 0x10 ? regs.flags.c : regs.flags.z;
            bool negate = !(opc & 0x8);
            if (negate)
                condbit = !condbit;
            if (unconditional || condbit) {
                regs.sp -= 2;
                gb->memWrite16(regs.sp, regs.pc);
                regs.pc = addr;
            }
            INSN_DBG_TRACE("CALL %sTODO, a16", negate ? "N" : "");
            return;
        }
        case 0x5: {
            regs.sp -= 2;
            gb->memWrite16(regs.sp, operand == 3 ? regs.af : regs.words[operand]);
            INSN_DBG_TRACE("PUSH %s", reg16AfStrings[operand]);
            return;
        }
        case 0x6: case 0xE: {
            unreachable();
            return;
        }
        case 0x7: case 0xF: {
            unreachable();
            return;
        }
    }
    unreachable();
}

void Cpu::executeTwoByteInsn()
{
    INSN_DBG_DECL();
    Byte opc = gb->memRead8(regs.pc++);
    const char* description;

    int operand = opc & 0x7;
    int category = opc >> 6;
    Byte bitMask = 1 << ((opc >> 3) & 0x7);
    Byte value = LOAD8(operand);

    if (category == 0) {
        unreachable(); // TODO
    } else if (category == 1) {
        regs.flags.n = 0;
        regs.flags.h = 1;
        regs.flags.z = (value & bitMask) == bitMask;
        description = "BIT";
    } else if (category == 2) {
        value &= ~bitMask;
        description = "RES";
    } else {
        value |= bitMask;
        description = "SET";
    }

    if (category != 1) // no writeback for BIT
        STORE8(operand, value);

    INSN_DBG_TRACE("%s %s", description, reg8Strings[operand]); // FIXME
}
