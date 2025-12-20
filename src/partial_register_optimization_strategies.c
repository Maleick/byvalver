/*
 * Partial Register Optimization Strategies
 *
 * PROBLEM: Loading immediate values into 32-bit registers often generates null bytes:
 * - mov eax, 0x00000042 → B8 42 00 00 00 (3 null bytes in immediate)
 * - mov ebx, 0x00001234 → BB 34 12 00 00 (2 null bytes)
 * - mov ecx, 0x00004200 → B9 00 42 00 00 (2 null bytes)
 *
 * SOLUTION: Use partial register access (AL, AH, AX) to load values without null padding:
 * - 8-bit register (AL): Loads values 0x00-0xFF
 * - 8-bit high register (AH): Loads values into bits 8-15
 * - 16-bit register (AX): Loads values 0x0000-0xFFFF
 *
 * APPLICABILITY: Very high (80%+ of shellcode loads small immediate values)
 * PRIORITY: 89 (Foundational - affects most immediate loads)
 */

#include "partial_register_optimization_strategies.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

// Helper function to get 8-bit low register (AL, BL, CL, DL, etc.)
static x86_reg get_8bit_low_register(x86_reg reg32) {
    switch (reg32) {
        case X86_REG_EAX: return X86_REG_AL;
        case X86_REG_EBX: return X86_REG_BL;
        case X86_REG_ECX: return X86_REG_CL;
        case X86_REG_EDX: return X86_REG_DL;
        case X86_REG_ESI: return X86_REG_SIL;  // x64
        case X86_REG_EDI: return X86_REG_DIL;  // x64
        case X86_REG_EBP: return X86_REG_BPL;  // x64
        case X86_REG_ESP: return X86_REG_SPL;  // x64
        default: return X86_REG_INVALID;
    }
}

// Helper function to get 8-bit high register (AH, BH, CH, DH) - x86 only
static x86_reg get_8bit_high_register(x86_reg reg32) {
    switch (reg32) {
        case X86_REG_EAX: return X86_REG_AH;
        case X86_REG_EBX: return X86_REG_BH;
        case X86_REG_ECX: return X86_REG_CH;
        case X86_REG_EDX: return X86_REG_DH;
        default: return X86_REG_INVALID;  // Only AX, BX, CX, DX have high bytes
    }
}

// Helper function to get 16-bit register (AX, BX, CX, DX, etc.)
static x86_reg get_16bit_register(x86_reg reg32) {
    switch (reg32) {
        case X86_REG_EAX: return X86_REG_AX;
        case X86_REG_EBX: return X86_REG_BX;
        case X86_REG_ECX: return X86_REG_CX;
        case X86_REG_EDX: return X86_REG_DX;
        case X86_REG_ESI: return X86_REG_SI;
        case X86_REG_EDI: return X86_REG_DI;
        case X86_REG_EBP: return X86_REG_BP;
        case X86_REG_ESP: return X86_REG_SP;
        default: return X86_REG_INVALID;
    }
}

// Helper function to encode MOV reg8, imm8
static void encode_mov_r8_imm8(struct buffer *b, x86_reg reg8, uint8_t imm) {
    uint8_t opcode = 0xB0;  // MOV AL, imm8 base opcode

    // Calculate register offset (AL=0, CL=1, DL=2, BL=3, AH=4, CH=5, DH=6, BH=7)
    uint8_t reg_offset = 0;
    switch (reg8) {
        case X86_REG_AL: reg_offset = 0; break;
        case X86_REG_CL: reg_offset = 1; break;
        case X86_REG_DL: reg_offset = 2; break;
        case X86_REG_BL: reg_offset = 3; break;
        case X86_REG_AH: reg_offset = 4; break;
        case X86_REG_CH: reg_offset = 5; break;
        case X86_REG_DH: reg_offset = 6; break;
        case X86_REG_BH: reg_offset = 7; break;
        default: return;  // Invalid register
    }

    uint8_t insn[] = {opcode + reg_offset, imm};
    buffer_append(b, insn, 2);
}

// Helper function to encode MOV reg16, imm16
static void encode_mov_r16_imm16(struct buffer *b, x86_reg reg16, uint16_t imm) {
    uint8_t prefix = 0x66;  // Operand-size prefix for 16-bit operation
    uint8_t opcode = 0xB8;  // MOV AX, imm16 base opcode

    // Calculate register offset
    uint8_t reg_offset = 0;
    switch (reg16) {
        case X86_REG_AX: reg_offset = 0; break;
        case X86_REG_CX: reg_offset = 1; break;
        case X86_REG_DX: reg_offset = 2; break;
        case X86_REG_BX: reg_offset = 3; break;
        case X86_REG_SP: reg_offset = 4; break;
        case X86_REG_BP: reg_offset = 5; break;
        case X86_REG_SI: reg_offset = 6; break;
        case X86_REG_DI: reg_offset = 7; break;
        default: return;  // Invalid register
    }

    uint8_t insn[] = {prefix, opcode + reg_offset, (uint8_t)(imm & 0xFF), (uint8_t)(imm >> 8)};
    buffer_append(b, insn, 4);
}

// Helper function to encode XOR reg32, reg32 (zero register)
static void encode_xor_r32_r32(struct buffer *b, x86_reg reg32) {
    uint8_t opcode = 0x33;  // XOR r32, r/m32
    uint8_t modrm = 0xC0;   // MOD=11 (register), REG=reg, R/M=reg

    // Calculate register value (0-7)
    uint8_t reg_val = 0;
    switch (reg32) {
        case X86_REG_EAX: reg_val = 0; break;
        case X86_REG_ECX: reg_val = 1; break;
        case X86_REG_EDX: reg_val = 2; break;
        case X86_REG_EBX: reg_val = 3; break;
        case X86_REG_ESP: reg_val = 4; break;
        case X86_REG_EBP: reg_val = 5; break;
        case X86_REG_ESI: reg_val = 6; break;
        case X86_REG_EDI: reg_val = 7; break;
        default: return;  // Invalid register
    }

    modrm |= (reg_val << 3) | reg_val;  // Set both REG and R/M to same register

    uint8_t insn[] = {opcode, modrm};
    buffer_append(b, insn, 2);
}

/*
 * Strategy 1: Load 8-bit immediate into low byte (AL, BL, CL, DL)
 *
 * Handles: mov eax, 0x00000042 → xor eax, eax; mov al, 0x42
 */
int can_handle_partial_reg_8bit_low(cs_insn *insn) {
    if (insn->id != X86_INS_MOV || insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    // Must be: MOV reg32, imm
    if (dst_op->type != X86_OP_REG || src_op->type != X86_OP_IMM) {
        return 0;
    }

    // Must be 32-bit register destination
    if (dst_op->size != 4) {
        return 0;
    }

    // Check if we can access 8-bit low register
    if (get_8bit_low_register(dst_op->reg) == X86_REG_INVALID) {
        return 0;
    }

    uint32_t imm = (uint32_t)src_op->imm;

    // Value must fit in 8 bits (0x00000000-0x000000FF)
    if (imm > 0xFF) {
        return 0;
    }

    // Original instruction must have bad characters
    if (!has_null_bytes(insn)) {
        return 0;
    }

    return 1;
}

size_t get_size_partial_reg_8bit_low(cs_insn *insn) {
    (void)insn;
    return 4;  // XOR reg32, reg32 (2 bytes) + MOV reg8, imm8 (2 bytes)
}

void generate_partial_reg_8bit_low(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    x86_reg dst_reg32 = dst_op->reg;
    x86_reg dst_reg8 = get_8bit_low_register(dst_reg32);
    uint8_t imm8 = (uint8_t)(src_op->imm & 0xFF);

    // xor dst_reg32, dst_reg32
    encode_xor_r32_r32(b, dst_reg32);

    // mov dst_reg8, imm8
    encode_mov_r8_imm8(b, dst_reg8, imm8);
}

/*
 * Strategy 2: Load 8-bit immediate into high byte (AH, BH, CH, DH)
 *
 * Handles: mov eax, 0x00004200 → xor eax, eax; mov ah, 0x42
 */
int can_handle_partial_reg_8bit_high(cs_insn *insn) {
    if (insn->id != X86_INS_MOV || insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    // Must be: MOV reg32, imm
    if (dst_op->type != X86_OP_REG || src_op->type != X86_OP_IMM) {
        return 0;
    }

    // Must be 32-bit register destination
    if (dst_op->size != 4) {
        return 0;
    }

    // Check if we can access 8-bit high register (only AX, BX, CX, DX)
    if (get_8bit_high_register(dst_op->reg) == X86_REG_INVALID) {
        return 0;
    }

    uint32_t imm = (uint32_t)src_op->imm;

    // Value must be in high byte position (0x0000XX00)
    // Lower 8 bits must be 0, upper 16 bits must be 0, bits 8-15 can be any value
    if ((imm & 0x00FF) != 0 || (imm & 0xFFFF0000) != 0) {
        return 0;
    }

    uint8_t high_byte = (uint8_t)((imm >> 8) & 0xFF);
    if (high_byte == 0) {
        return 0;  // Would just be zero, use 8-bit low strategy instead
    }

    // Original instruction must have bad characters
    if (!has_null_bytes(insn)) {
        return 0;
    }

    return 1;
}

size_t get_size_partial_reg_8bit_high(cs_insn *insn) {
    (void)insn;
    return 4;  // XOR reg32, reg32 (2 bytes) + MOV reg8_high, imm8 (2 bytes)
}

void generate_partial_reg_8bit_high(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    x86_reg dst_reg32 = dst_op->reg;
    x86_reg dst_reg8_high = get_8bit_high_register(dst_reg32);
    uint8_t imm8 = (uint8_t)((src_op->imm >> 8) & 0xFF);

    // xor dst_reg32, dst_reg32
    encode_xor_r32_r32(b, dst_reg32);

    // mov dst_reg8_high, imm8
    encode_mov_r8_imm8(b, dst_reg8_high, imm8);
}

/*
 * Strategy 3: Load 16-bit immediate into 16-bit register (AX, BX, CX, DX)
 *
 * Handles: mov eax, 0x00001234 → xor eax, eax; mov ax, 0x1234
 */
int can_handle_partial_reg_16bit(cs_insn *insn) {
    if (insn->id != X86_INS_MOV || insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    // Must be: MOV reg32, imm
    if (dst_op->type != X86_OP_REG || src_op->type != X86_OP_IMM) {
        return 0;
    }

    // Must be 32-bit register destination
    if (dst_op->size != 4) {
        return 0;
    }

    // Check if we can access 16-bit register
    if (get_16bit_register(dst_op->reg) == X86_REG_INVALID) {
        return 0;
    }

    uint32_t imm = (uint32_t)src_op->imm;

    // Value must fit in 16 bits (0x00000000-0x0000FFFF)
    if (imm > 0xFFFF) {
        return 0;
    }

    // Must have at least one null byte in upper 16 bits
    if ((imm & 0xFFFF0000) == 0) {
        // Check if immediate itself contains nulls
        uint16_t imm16 = (uint16_t)(imm & 0xFFFF);
        if (!is_bad_char_free_byte((uint8_t)(imm16 & 0xFF)) ||
            !is_bad_char_free_byte((uint8_t)(imm16 >> 8))) {
            // Immediate has bad chars, can't help with 16-bit load
            return 0;
        }
    }

    // Original instruction must have bad characters
    if (!has_null_bytes(insn)) {
        return 0;
    }

    return 1;
}

size_t get_size_partial_reg_16bit(cs_insn *insn) {
    (void)insn;
    return 6;  // XOR reg32, reg32 (2 bytes) + MOV reg16, imm16 (4 bytes with 0x66 prefix)
}

void generate_partial_reg_16bit(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    x86_reg dst_reg32 = dst_op->reg;
    x86_reg dst_reg16 = get_16bit_register(dst_reg32);
    uint16_t imm16 = (uint16_t)(src_op->imm & 0xFFFF);

    // xor dst_reg32, dst_reg32
    encode_xor_r32_r32(b, dst_reg32);

    // mov dst_reg16, imm16
    encode_mov_r16_imm16(b, dst_reg16, imm16);
}

// Define the strategy structures as external symbols for the registry
strategy_t partial_register_optimization_strategy = {
    .name = "Partial Register Optimization",
    .can_handle = can_handle_partial_register_optimization,  // Use the main handler
    .get_size = get_size_partial_register_optimization,
    .generate = generate_partial_register_optimization,
    .priority = 89
};

// Main strategy interface functions
int can_handle_partial_register_optimization(cs_insn *insn) {
    // Check if this is a MOV instruction with immediate that contains bad characters
    if (insn->id != X86_INS_MOV || insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    if (dst_op->type != X86_OP_REG || src_op->type != X86_OP_IMM) {
        return 0;
    }

    // Only handle 32-bit registers for partial register optimization
    if (dst_op->reg < X86_REG_EAX || dst_op->reg > X86_REG_EDI) {
        return 0;
    }

    // Check if immediate contains bad characters
    uint32_t imm = (uint32_t)src_op->imm;
    return !is_bad_char_free(imm);
}

size_t get_size_partial_register_optimization(cs_insn *insn) {
    // Conservative estimate: 3-6 bytes for partial register approach
    (void)insn; // Suppress unused parameter warning
    return 6;
}

void generate_partial_register_optimization(struct buffer *b, cs_insn *insn) {
    // Use the first available partial register approach
    // For now, just call one of the existing implementations
    // This will be expanded to try different approaches
    if (insn && b) {
        generate_partial_reg_8bit_low(b, insn); // Use the existing implementation
    }
}

