#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

// Strategy 1: PUSH Immediate with SIB Addressing
// Transform PUSH operations with immediate values containing nulls by first loading
// the value into a register using null-free instructions, then PUSHing the register

int can_handle_push_immediate_nulls(cs_insn *insn) {
    // Check if this is a PUSH instruction with immediate operand
    if (insn->id != X86_INS_PUSH) {
        return 0;
    }

    // Check if the operand is an immediate value
    if (insn->detail->x86.op_count != 1) {
        return 0;
    }

    if (insn->detail->x86.operands[0].type != X86_OP_IMM) {
        return 0;
    }

    // Check if the immediate value contains null bytes
    uint32_t imm = (uint32_t)insn->detail->x86.operands[0].imm;

    for (int i = 0; i < 4; i++) {
        if (((imm >> (i * 8)) & 0xFF) == 0x00) {
            return 1; // Has null bytes in immediate
        }
    }

    return 0;
}

size_t get_size_push_immediate_nulls(__attribute__((unused)) cs_insn *insn) {
    // MOV EAX, imm32 (using null-free construction) + PUSH EAX
    // Null-free construction of immediate can vary, but assume it's about 9-15 bytes maximum
    // MOV EAX, imm (null-free) = ~9-12 bytes, PUSH reg = 1 byte
    return 15; // Conservative estimate
}

void generate_push_immediate_nulls(struct buffer *b, cs_insn *insn) {
    uint32_t imm = (uint32_t)insn->detail->x86.operands[0].imm;

    // Load the immediate value into EAX using null-free construction
    generate_mov_eax_imm(b, imm);

    // PUSH EAX
    uint8_t push_eax = 0x50; // PUSH EAX opcode
    buffer_write_byte(b, push_eax);
}

strategy_t push_immediate_nulls_strategy = {
    .name = "push_immediate_nulls",
    .can_handle = can_handle_push_immediate_nulls,
    .get_size = get_size_push_immediate_nulls,
    .generate = generate_push_immediate_nulls,
    .priority = 75  // Medium-high priority
};

// Alternative implementation using register other than EAX
int can_handle_push_immediate_nulls_alt(cs_insn *insn) {
    // Same as above but can be used as an alternative strategy
    return can_handle_push_immediate_nulls(insn);
}

size_t get_size_push_immediate_nulls_alt(__attribute__((unused)) cs_insn *insn) {
    // MOV ECX, imm32 + PUSH ECX (or other general purpose register)
    return 15; // Conservative estimate
}

void generate_push_immediate_nulls_alt(struct buffer *b, cs_insn *insn) {
    uint32_t imm = (uint32_t)insn->detail->x86.operands[0].imm;

    // Load the immediate value into ECX using null-free construction
    cs_insn temp_insn = *insn;
    temp_insn.detail->x86.operands[0].reg = X86_REG_ECX;
    temp_insn.detail->x86.operands[1].imm = imm;

    generate_mov_eax_imm(b, imm);

    // MOV ECX, EAX (since MOV reg, imm32 may have nulls, use MOV reg, reg)
    uint8_t mov_ecx_eax[] = {0x89, 0xC1}; // MOV ECX, EAX
    buffer_append(b, mov_ecx_eax, 2);

    // PUSH ECX
    uint8_t push_ecx = 0x51; // PUSH ECX opcode
    buffer_write_byte(b, push_ecx);
}

strategy_t push_immediate_nulls_alt_strategy = {
    .name = "push_immediate_nulls_alt",
    .can_handle = can_handle_push_immediate_nulls_alt,
    .get_size = get_size_push_immediate_nulls_alt,
    .generate = generate_push_immediate_nulls_alt,
    .priority = 70  // Medium priority, lower than main strategy
};

// Register the PUSH immediate strategies
void register_push_immediate_strategies() {
    register_strategy(&push_immediate_nulls_strategy);
    register_strategy(&push_immediate_nulls_alt_strategy);
}