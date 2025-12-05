#include "strategy.h"
#include "utils.h"
#include <stdio.h>

// CALL imm32 strategy
int can_handle_call_imm(cs_insn *insn) {
    return (insn->id == X86_INS_CALL && insn->detail->x86.op_count == 1 &&
            insn->detail->x86.operands[0].type == X86_OP_IMM &&
            has_null_bytes(insn));
}

size_t get_size_call_imm(cs_insn *insn) {
    return get_call_imm_size(insn);
}

void generate_call_imm_strat(struct buffer *b, cs_insn *insn) {
    generate_call_imm(b, insn);
}

strategy_t call_imm_strategy = {
    .name = "call_imm",
    .can_handle = can_handle_call_imm,
    .get_size = get_size_call_imm,
    .generate = generate_call_imm_strat,
    .priority = 8
};

// CALL [disp32] strategy - for indirect calls with displacement containing null bytes
int can_handle_call_mem_disp32(cs_insn *insn) {
    return (insn->id == X86_INS_CALL && insn->detail->x86.op_count == 1 &&
            insn->detail->x86.operands[0].type == X86_OP_MEM &&
            insn->detail->x86.operands[0].mem.base == X86_REG_INVALID &&
            insn->detail->x86.operands[0].mem.index == X86_REG_INVALID &&
            has_null_bytes(insn));
}

size_t get_size_call_mem_disp32(cs_insn *insn) {
    // MOV EAX, disp32 + CALL EAX = 5 + 2 = 7 bytes (approximation)
    return get_mov_reg_imm_size(insn) + 2;  // Adjust based on actual implementation
}

void generate_call_mem_disp32(struct buffer *b, cs_insn *insn) {
    uint32_t addr = (uint32_t)insn->detail->x86.operands[0].mem.disp;

    // MOV EAX, addr (null-free construction)
    generate_mov_eax_imm(b, addr);

    // CALL EAX
    uint8_t call_eax[] = {0xFF, 0xD0}; // CALL EAX
    buffer_append(b, call_eax, 2);
}

strategy_t call_mem_disp32_strategy = {
    .name = "call_mem_disp32",
    .can_handle = can_handle_call_mem_disp32,
    .get_size = get_size_call_mem_disp32,
    .generate = generate_call_mem_disp32,
    .priority = 8
};

// JMP [disp32] strategy - for indirect jumps with displacement containing null bytes
int can_handle_jmp_mem_disp32(cs_insn *insn) {
    return (insn->id == X86_INS_JMP && insn->detail->x86.op_count == 1 &&
            insn->detail->x86.operands[0].type == X86_OP_MEM &&
            insn->detail->x86.operands[0].mem.base == X86_REG_INVALID &&
            insn->detail->x86.operands[0].mem.index == X86_REG_INVALID &&
            has_null_bytes(insn));
}

size_t get_size_jmp_mem_disp32(cs_insn *insn) {
    // MOV EAX, disp32 + JMP EAX = 5 + 2 = 7 bytes (approximation)
    return get_mov_reg_imm_size(insn) + 2;  // Adjust based on actual implementation
}

void generate_jmp_mem_disp32(struct buffer *b, cs_insn *insn) {
    uint32_t addr = (uint32_t)insn->detail->x86.operands[0].mem.disp;

    // MOV EAX, addr (null-free construction)
    generate_mov_eax_imm(b, addr);

    // JMP EAX
    uint8_t jmp_eax[] = {0xFF, 0xE0}; // JMP EAX
    buffer_append(b, jmp_eax, 2);
}

strategy_t jmp_mem_disp32_strategy = {
    .name = "jmp_mem_disp32",
    .can_handle = can_handle_jmp_mem_disp32,
    .get_size = get_size_jmp_mem_disp32,
    .generate = generate_jmp_mem_disp32,
    .priority = 8
};

// Generic strategy for any instruction with memory operands having displacement containing nulls
int can_handle_generic_mem_null_disp(cs_insn *insn) {
    // Check for any instruction with memory operand containing null bytes in displacement
    if (insn->detail->x86.op_count > 0) {
        for (int i = 0; i < insn->detail->x86.op_count; i++) {
            if (insn->detail->x86.operands[i].type == X86_OP_MEM &&
                insn->detail->x86.operands[i].mem.base == X86_REG_INVALID &&
                insn->detail->x86.operands[i].mem.index == X86_REG_INVALID) {

                uint32_t disp = (uint32_t)insn->detail->x86.operands[i].mem.disp;

                // Check if displacement has null bytes
                int has_null_in_disp = 0;
                for (int j = 0; j < 4; j++) {
                    if (((disp >> (j * 8)) & 0xFF) == 0) {
                        has_null_in_disp = 1;
                        break;
                    }
                }

                if (has_null_in_disp) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

size_t get_size_generic_mem_null_disp(__attribute__((unused)) cs_insn *insn) {
    return 20; // Conservative estimate for complex memory operations
}

void generate_generic_mem_null_disp(struct buffer *b, cs_insn *insn) {
    // This is a catch-all for memory operations with null displacement
    // Handle based on instruction type, with proper error handling

    // Find the memory operand
    int mem_op_index = -1;
    for (int i = 0; i < insn->detail->x86.op_count; i++) {
        if (insn->detail->x86.operands[i].type == X86_OP_MEM &&
            insn->detail->x86.operands[i].mem.base == X86_REG_INVALID &&
            insn->detail->x86.operands[i].mem.index == X86_REG_INVALID) {
            mem_op_index = i;
            break;
        }
    }

    if (mem_op_index == -1) {
        // No matching memory operand found, should not happen if can_handle passed
        return;
    }

    uint32_t addr = (uint32_t)insn->detail->x86.operands[mem_op_index].mem.disp;

    // MOV EAX, addr (null-free construction)
    generate_mov_eax_imm(b, addr);

    // Handle different instruction types
    if (insn->id == X86_INS_MOV) {
        if (mem_op_index == 0) { // Destination is memory [disp32]
            if (insn->detail->x86.operands[1].type == X86_OP_REG) {
                uint8_t src_reg = insn->detail->x86.operands[1].reg;
                uint8_t reg_index = get_reg_index(src_reg);
                // Use SIB byte to avoid null in ModR/M: [EAX] using SIB byte format
                uint8_t code[] = {0x89, 0x04, 0x20}; // MOV [EAX], reg using SIB
                code[1] = 0x04 | (reg_index << 3); // ModR/M: reg=reg_index, r/m=100 (SIB follows)
                // SIB: scale=00, index=ESP(100 - special no-index), base=000 (EAX) = [EAX]
                buffer_append(b, code, 3);
            }
        } else { // Source is memory [disp32]
            if (insn->detail->x86.operands[0].type == X86_OP_REG) {
                uint8_t dst_reg = insn->detail->x86.operands[0].reg;
                uint8_t reg_index = get_reg_index(dst_reg);
                // Use SIB byte to avoid null in ModR/M: [EAX] using SIB byte format
                uint8_t code[] = {0x8B, 0x04, 0x20}; // MOV reg, [EAX] using SIB
                code[1] = 0x04 | (reg_index << 3); // ModR/M: reg=reg_index, r/m=100 (SIB follows)
                // SIB: scale=00, index=ESP(100 - special no-index), base=000 (EAX) = [EAX]
                buffer_append(b, code, 3);
            }
        }
    } else if (insn->id == X86_INS_PUSH) {
        if (insn->detail->x86.operands[0].type == X86_OP_MEM) {
            // PUSH [EAX] - push from memory location
            // Use SIB byte to avoid null: PUSH [EAX] using SIB
            uint8_t code[] = {0xFF, 0x34, 0x20}; // PUSH [EAX] using SIB
            buffer_append(b, code, 3);
        }
    } else if (insn->id == X86_INS_CMP) {
        // For CMP [disp32], reg
        if (insn->detail->x86.operands[1].type == X86_OP_REG) {
            uint8_t reg = insn->detail->x86.operands[1].reg;
            uint8_t reg_index = get_reg_index(reg);
            // Use SIB byte to avoid null: [EAX] using SIB byte format
            uint8_t code[] = {0x39, 0x04, 0x20}; // CMP [EAX], reg using SIB
            code[1] = 0x04 | (reg_index << 3); // ModR/M: reg=reg_index, r/m=100 (SIB follows)
            // SIB: scale=00, index=ESP(100 - special no-index), base=000 (EAX) = [EAX]
            buffer_append(b, code, 3);
        } else if (insn->detail->x86.operands[0].type == X86_OP_REG &&
                   insn->detail->x86.operands[1].type == X86_OP_MEM) {
            // For CMP reg, [disp32] - compare reg with memory
            uint8_t reg = insn->detail->x86.operands[0].reg;
            uint8_t reg_index = get_reg_index(reg);
            // Use SIB byte to avoid null: [EAX] using SIB byte format
            uint8_t code[] = {0x3B, 0x04, 0x20}; // CMP reg, [EAX] using SIB
            code[1] = 0x04 | (reg_index << 3); // ModR/M: reg=reg_index, r/m=100 (SIB follows)
            // SIB: scale=00, index=ESP(100 - special no-index), base=000 (EAX) = [EAX]
            buffer_append(b, code, 3);
        }
    } else if (insn->id == X86_INS_ADD || insn->id == X86_INS_SUB ||
               insn->id == X86_INS_AND || insn->id == X86_INS_OR ||
               insn->id == X86_INS_XOR) {
        // For arithmetic operations on memory with null bytes in displacement
        // Determine which operand is the register (the other is memory)
        cs_x86_op *reg_op = NULL;
        if (insn->detail->x86.operands[0].type == X86_OP_REG) {
            reg_op = &insn->detail->x86.operands[0];
        } else if (insn->detail->x86.operands[1].type == X86_OP_REG) {
            reg_op = &insn->detail->x86.operands[1];
        }

        if (reg_op != NULL) {
            uint8_t reg = reg_op->reg;
            uint8_t reg_index = get_reg_index(reg);
            uint8_t opcode;

            switch(insn->id) {
                case X86_INS_ADD: opcode = 0x01; break;  // ADD r/m32, r32
                case X86_INS_SUB: opcode = 0x29; break;  // SUB r/m32, r32
                case X86_INS_AND: opcode = 0x21; break;  // AND r/m32, r32
                case X86_INS_OR:  opcode = 0x09; break;  // OR r/m32, r32
                case X86_INS_XOR: opcode = 0x31; break;  // XOR r/m32, r32
                default: opcode = 0x01; break; // Default to ADD
            }

            // Use SIB byte to avoid null: [EAX] using SIB byte format
            uint8_t code[] = {opcode, 0x04, 0x20}; // op [EAX], reg using SIB
            code[1] = 0x04 | (reg_index << 3); // ModR/M: reg=reg_index, r/m=100 (SIB follows)
            // SIB: scale=00, index=ESP(100 - special no-index), base=000 (EAX) = [EAX]
            buffer_append(b, code, 3);
        }
    } else if (insn->id == X86_INS_NOP) {
        // NOP with null bytes in displacement - replace with equivalent no-op
        // Do nothing - just skip the NOP since it's just a no-operation
    } else if (insn->id == X86_INS_INC || insn->id == X86_INS_DEC) {
        // Handle INC/DEC memory operations with null displacement
        uint8_t code[] = {0xFF, 0x04, 0x20};
        if (insn->id == X86_INS_INC) code[1] = 0x04 | 0x00;  // INC uses /0: reg=000
        else code[1] = 0x04 | 0x08;  // DEC uses /1: reg=001
        // SIB: scale=00, index=ESP(100 - special no-index), base=000 (EAX)
        buffer_append(b, code, 3);
    } else {
        // For unsupported instructions, fall back to original behavior or emit warning
        fprintf(stderr, "[WARN] Unsupported instruction in generic_mem_null_disp: %s\n",
                insn->mnemonic);
        // As a fallback, we can try the original instruction - but this may still contain nulls
        // It's better to skip and let another strategy handle it
    }
}

strategy_t generic_mem_null_disp_strategy = {
    .name = "generic_mem_null_disp",
    .can_handle = can_handle_generic_mem_null_disp,
    .get_size = get_size_generic_mem_null_disp,
    .generate = generate_generic_mem_null_disp,
    .priority = 3 // Lowest priority - use as absolute last resort
};

void register_jump_strategies() {
    register_strategy(&call_imm_strategy);
    register_strategy(&call_mem_disp32_strategy);
    register_strategy(&jmp_mem_disp32_strategy);
    register_strategy(&generic_mem_null_disp_strategy);
}