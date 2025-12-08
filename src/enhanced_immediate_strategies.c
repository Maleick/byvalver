/*
 * Enhanced Immediate Optimization Strategy
 *
 * PROBLEM: Immediate values in instructions can contain null bytes that need to be eliminated
 *          The current strategies may not handle all immediate value patterns effectively
 *
 * SOLUTIONS:
 *   1. Use advanced arithmetic combinations to construct immediate values
 *   2. Implement multi-step value construction to avoid null bytes
 *   3. Use XOR encoding with more sophisticated key selection
 *   4. Implement safe fallback mechanisms
 *
 * Priority: 80 (higher than most other immediate strategies)
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "strategy.h"
#include "utils.h"
#include <capstone/capstone.h>

/* Forward declarations */
extern void register_strategy(strategy_t *s);

/*
 * Helper function to determine if an instruction has immediate values with null bytes
 * This includes MOV reg, imm32, arithmetic with immediates, etc.
 */
static int has_immediate_with_nulls(cs_insn *insn) {
    if (!insn || !insn->detail) {
        return 0;
    }

    cs_x86 *x86 = &insn->detail->x86;

    // Check each operand for immediate values
    for (int i = 0; i < x86->op_count; i++) {
        if (x86->operands[i].type == X86_OP_IMM) {
            uint32_t imm = (uint32_t)x86->operands[i].imm;

            // Check if the immediate value contains null bytes
            for (int j = 0; j < 4; j++) {
                if (((imm >> (j * 8)) & 0xFF) == 0x00) {
                    return 1;  // Found null byte in immediate value
                }
            }
        }

        // Also check memory operands for displacement with nulls
        if (x86->operands[i].type == X86_OP_MEM) {
            cs_x86_op *op = &x86->operands[i];
            if (op->mem.disp != 0) {
                uint32_t disp = (uint32_t)op->mem.disp;
                for (int j = 0; j < 4; j++) {
                    if (((disp >> (j * 8)) & 0xFF) == 0x00) {
                        return 1;  // Found null byte in displacement
                    }
                }
            }
        }
    }

    return 0;
}

/*
 * Detect instructions with immediate values containing null bytes
 */
static int can_handle_enhanced_immediate(cs_insn *insn) {
    if (!insn || !insn->detail) {
        return 0;
    }

    // Check if instruction has immediate values with null bytes
    if (has_immediate_with_nulls(insn)) {
        return 1;
    }

    return 0;
}

/*
 * Calculate replacement size for enhanced immediate optimization
 */
static size_t get_size_enhanced_immediate(cs_insn *insn) {
    (void)insn;
    // Conservative estimate: MOV EAX, imm (5-20) + MOV reg, EAX (2) = 7-22 bytes
    // Using a larger size to account for complex transformations
    return 20;
}

/*
 * Advanced immediate construction using arithmetic and bitwise operations
 */
static int construct_immediate_advanced(struct buffer *b, uint32_t target) {
    // Try XOR encoding with various keys
    uint32_t xor_keys[] = {
        0x01010101, 0x11111111, 0x22222222, 0x33333333,
        0x44444444, 0x55555555, 0x66666666, 0x77777777,
        0x88888888, 0x99999999, 0xAAAAAAAA, 0xBBBBBBBB,
        0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0x12345678,
        0x87654321, 0xABCDEF01, 0xFEDCBA98, 0x01234567
    };

    for (size_t i = 0; i < sizeof(xor_keys)/sizeof(xor_keys[0]); i++) {
        uint32_t encoded = target ^ xor_keys[i];
        if (is_null_free(encoded) && is_null_free(xor_keys[i])) {
            // MOV EAX, encoded_value
            generate_mov_eax_imm(b, encoded);
            // XOR EAX, key
            uint8_t xor_eax_key[] = {0x35, 0, 0, 0, 0};  // XOR EAX, imm32
            memcpy(xor_eax_key + 1, &xor_keys[i], 4);
            buffer_append(b, xor_eax_key, 5);
            return 1;  // Success
        }
    }

    // Try ADD/SUB encoding
    uint32_t offsets[] = {
        0x01010101, 0x11111111, 0x22222222, 0x33333333,
        0x44444444, 0x55555555, 0x66666666, 0x77777777,
        0x01000000, 0x02000000, 0x04000000, 0x08000000,
        0x10000000, 0x20000000, 0x40000000, 0x80000000
    };

    for (size_t i = 0; i < sizeof(offsets)/sizeof(offsets[0]); i++) {
        // Try SUB: val1 - offset = target  =>  val1 = target + offset
        uint32_t val1 = target + offsets[i];
        if (is_null_free(val1) && is_null_free(offsets[i])) {
            generate_mov_eax_imm(b, val1);
            uint8_t sub_eax_key[] = {0x2D, 0, 0, 0, 0};  // SUB EAX, imm32
            memcpy(sub_eax_key + 1, &offsets[i], 4);
            buffer_append(b, sub_eax_key, 5);
            return 1;  // Success
        }

        // Try ADD: val1 + offset = target  =>  val1 = target - offset
        if (target >= offsets[i]) {
            val1 = target - offsets[i];
            if (is_null_free(val1) && is_null_free(offsets[i])) {
                generate_mov_eax_imm(b, val1);
                uint8_t add_eax_key[] = {0x05, 0, 0, 0, 0};  // ADD EAX, imm32
                memcpy(add_eax_key + 1, &offsets[i], 4);
                buffer_append(b, add_eax_key, 5);
                return 1;  // Success
            }
        }
    }

    // Try more complex multi-step construction
    // For example: clear EAX, then OR in each non-zero byte separately
    uint8_t bytes[4];
    memcpy(bytes, &target, 4);

    // Check if we can use multi-byte construction
    int non_zero_count = 0;
    for (int i = 0; i < 4; i++) {
        if (bytes[i] != 0) non_zero_count++;
    }

    if (non_zero_count <= 3) {  // If at least one byte is zero, multi-byte construction may be feasible
        // Clear EAX first
        uint8_t xor_eax_eax[] = {0x31, 0xC0};  // XOR EAX, EAX
        buffer_append(b, xor_eax_eax, 2);

        // Find first non-zero byte from MSB to LSB
        int first_nonzero = -1;
        for (int i = 3; i >= 0; i--) {
            if (bytes[i] != 0) {
                first_nonzero = i;
                break;
            }
        }

        if (first_nonzero != -1) {
            // Load first non-zero byte into AL using MOV AL, imm8
            uint8_t mov_al[] = {0xB0, bytes[first_nonzero]};  // MOV AL, imm8
            buffer_append(b, mov_al, 2);

            // Process remaining bytes
            for (int i = first_nonzero - 1; i >= 0; i--) {
                // Shift left by 8 bits to make room for next byte
                uint8_t shl_eax_8[] = {0xC1, 0xE0, 0x08};  // SHL EAX, 8
                buffer_append(b, shl_eax_8, 3);

                if (bytes[i] != 0) {
                    // OR in the non-zero byte using OR AL, imm8
                    uint8_t or_al[] = {0x0C, bytes[i]};  // OR AL, imm8
                    buffer_append(b, or_al, 2);
                }
                // Zero bytes don't need OR - the shift already placed 0x00 in AL
            }
            return 1;  // Success
        }
    }

    return 0;  // No successful construction method found
}

/*
 * Generate enhanced immediate optimization replacement
 */
static void generate_enhanced_immediate(struct buffer *b, cs_insn *insn) {
    if (!insn || !insn->detail) {
        return;
    }

    cs_x86 *x86 = &insn->detail->x86;

    // Handle different types of instructions with immediate values
    switch (insn->id) {
        case X86_INS_MOV: {
            // MOV reg, imm32 with null bytes in imm32
            if (x86->op_count == 2 && x86->operands[1].type == X86_OP_IMM) {
                uint32_t imm = (uint32_t)x86->operands[1].imm;
                x86_reg target_reg = x86->operands[0].reg;

                // Try to construct the immediate value using enhanced methods
                if (construct_immediate_advanced(b, imm)) {
                    // If target register is not EAX, move EAX to target reg
                    if (target_reg != X86_REG_EAX) {
                        uint8_t mov_reg_eax[] = {0x89, 0xC0};  // MOV reg, EAX
                        mov_reg_eax[1] = 0xC0 + (get_reg_index(target_reg) << 3) + get_reg_index(X86_REG_EAX);
                        buffer_append(b, mov_reg_eax, 2);
                    }
                } else {
                    // Fallback to original strategy if advanced construction fails
                    generate_mov_reg_imm(b, insn);
                }
            }
            break;
        }
        case X86_INS_ADD:
        case X86_INS_SUB:
        case X86_INS_AND:
        case X86_INS_OR:
        case X86_INS_XOR:
        case X86_INS_CMP: {
            // Arithmetic operations with immediate values
            if (x86->op_count == 2 && x86->operands[1].type == X86_OP_IMM) {
                uint32_t imm = (uint32_t)x86->operands[1].imm;
                x86_reg target_reg = x86->operands[0].reg;

                // Save original register if it's not EAX
                if (target_reg != X86_REG_EAX) {
                    uint8_t push_eax[] = {0x50};  // PUSH EAX
                    buffer_append(b, push_eax, 1);
                }

                // Construct the immediate value in EAX
                if (construct_immediate_advanced(b, imm)) {
                    // Perform the operation: op target_reg, EAX
                    uint8_t op_code;
                    switch (insn->id) {
                        case X86_INS_ADD: op_code = 0x01; break;  // ADD r/m32, r32
                        case X86_INS_SUB: op_code = 0x29; break;  // SUB r/m32, r32
                        case X86_INS_AND: op_code = 0x21; break;  // AND r/m32, r32
                        case X86_INS_OR:  op_code = 0x09; break;  // OR r/m32, r32
                        case X86_INS_XOR: op_code = 0x31; break;  // XOR r/m32, r32
                        case X86_INS_CMP: op_code = 0x39; break;  // CMP r/m32, r32
                        default: op_code = 0x01; break;  // Default to ADD
                    }

                    uint8_t final_code[] = {op_code, 0x00};
                    final_code[1] = (get_reg_index(target_reg) << 3) + get_reg_index(X86_REG_EAX);
                    
                    if (final_code[1] == 0x00) {
                        // If ModR/M creates null, use SIB addressing to avoid it
                        uint8_t sib_code[] = {op_code, 0x04, 0x20};
                        sib_code[1] = 0x04;  // mod=00, reg=reg_index, r/m=SIB
                        sib_code[2] = (0 << 6) | (4 << 3) | get_reg_index(X86_REG_EAX);  // SIB: scale=0, index=ESP, base=EAX
                        buffer_append(b, sib_code, 3);
                    } else {
                        buffer_append(b, final_code, 2);
                    }
                } else {
                    // Fallback to original strategy if advanced construction fails
                    generate_op_reg_imm(b, insn);
                }

                // Restore original EAX if needed
                if (target_reg != X86_REG_EAX) {
                    uint8_t pop_eax[] = {0x58};  // POP EAX
                    buffer_append(b, pop_eax, 1);
                }
            }
            break;
        }
        case X86_INS_PUSH: {
            // PUSH imm32 with null bytes in imm32
            if (x86->op_count == 1 && x86->operands[0].type == X86_OP_IMM) {
                uint32_t imm = (uint32_t)x86->operands[0].imm;

                // Try to construct the immediate value in EAX, then PUSH EAX
                if (construct_immediate_advanced(b, imm)) {
                    // PUSH EAX
                    uint8_t push_eax[] = {0x50};  // PUSH EAX
                    buffer_append(b, push_eax, 1);
                } else {
                    // Fallback to original strategy
                    generate_push_imm32(b, imm);
                }
            }
            break;
        }
        default:
            // For other instruction types with immediate values
            // Just fall back to standard strategies for now
            buffer_append(b, insn->bytes, insn->size);
            break;
    }
}

/* Strategy definition */
static strategy_t enhanced_immediate_strategy = {
    .name = "Enhanced Immediate Optimization",
    .can_handle = can_handle_enhanced_immediate,
    .get_size = get_size_enhanced_immediate,
    .generate = generate_enhanced_immediate,
    .priority = 80  // Higher priority than most other immediate strategies
};

/* Registration function */
void register_enhanced_immediate_strategies() {
    register_strategy(&enhanced_immediate_strategy);
}