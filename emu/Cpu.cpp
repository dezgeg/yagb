#include "Cpu.hpp"
#include "Gameboy.hpp"
#include "Utils.hpp"

#define INSN_DBG(x) x

#define INSN_DBG_TRACE(...) (_startPc, gb->logInsn(__VA_ARGS__))

#define INSN_DBG_DECL() Word _startPc = regs.pc - 1;

static const char* const reg8Strings[] = {
    "B", "C", "D", "E", "H", "L", "(HL)", "A",
};

#define LOAD8(x) ((x) == 6 ? gb->memRead8(regs.hl) : regs.bytes[x])
#define STORE8(x, v) ((x) == 6 ? gb->memWrite8(regs.hl, v) : (void)(regs.bytes[x] = v))

void Cpu::reset()
{
    regs = Regs();
    halted = false;
}

void Cpu::tick()
{
    Byte opc = gb->memRead8(regs.pc++);
    switch (opc >> 6) {
        case 0: unreachable(); break;
        case 1: executeInsn_4x_6x(opc); break;
        case 2: executeInsn_7x_Bx(opc); break;
        case 3: unreachable(); break;
    }
}


// Opcodes 4x..6x: Moves between 8-bit regs / (HL)
// Bottom 3 bits = destination, next 3 bits source. Order is: B C D E H L (HL) A
void Cpu::executeInsn_4x_6x(Byte opc)
{
    INSN_DBG_DECL();

    // Exception: opc 0x76 == LD (HL), (HL) is HALT instead
    if (opc == 0x76) {
        INSN_DBG_TRACE("HALT");
        halted = true;
        return;
    }

    int src = (opc >> 3) & 0x7;
    int dest = opc & 0x7;
    INSN_DBG_TRACE("LD %s, %s", reg8Strings[dest], reg8Strings[src]);

    Byte val = LOAD8(src);
    STORE8(dest, val);
}

Byte Cpu::doAddSub(unsigned lhs, unsigned rhs, bool isSub, bool withCarry)
{
    rhs = withCarry ? rhs + 1 : rhs;
    rhs = isSub ? ~rhs + 1 : rhs;

    unsigned sum = lhs + rhs;
    regs.flags.h = (lhs & 0xf) + (rhs & 0xf) > 0xf;
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
