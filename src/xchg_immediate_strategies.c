#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

// Strategy 3: XCHG-based Immediate Loading
// Use XCHG (Exchange) instructions with temporary registers to load immediate values
// containing nulls without directly using immediate operands with nulls.

int can_handle_xchg_immediate_loading(cs_insn *insn) {
    // Check if this is a MOV instruction with immediate that contains null bytes
    if (insn->id != X86_INS_MOV) {
        return 0;
    }

    // Check if it has 2 operands and the second is an immediate
    if (insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *src_op = &insn->detail->x86.operands[1];
    if (src_op->type != X86_OP_IMM) {
        return 0;
    }

    // Check if the immediate contains null bytes
    uint32_t imm = (uint32_t)src_op->imm;
    for (int i = 0; i < 4; i++) {
        if (((imm >> (i * 8)) & 0xFF) == 0) {
            return 1; // Has null bytes in immediate
        }
    }

    return 0;
}

size_t get_size_xchg_immediate_loading(__attribute__((unused)) cs_insn *insn) {
    // The approach: MOV some_reg, constructed_value + XCHG target_reg, some_reg
    // MOV reg, imm (5-10 bytes) + XCHG reg, reg (1-2 bytes)
    return 15; // Conservative estimate
}

void generate_xchg_immediate_loading(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];  // destination register
    cs_x86_op *src_op = &insn->detail->x86.operands[1];  // source immediate

    if (dst_op->type != X86_OP_REG || src_op->type != X86_OP_IMM) {
        // Fallback if not in expected format
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    x86_reg target_reg = dst_op->reg;
    uint32_t imm = (uint32_t)src_op->imm;

    // Use a temporary register different from the target
    x86_reg temp_reg = X86_REG_ECX;
    if (target_reg == X86_REG_ECX) {
        temp_reg = X86_REG_EDX;  // Use EDX if ECX is the target
        if (target_reg == X86_REG_EDX) {
            temp_reg = X86_REG_EBX;  // Use EBX if both ECX and EDX are the target
        }
    }

    // MOV temp_reg, imm (using null-free construction)
    generate_mov_eax_imm(b, imm);

    // MOV temp_reg, EAX (to get the value in our chosen temp_reg)
    uint8_t mov_temp_eax[] = {0x89, 0x00};
    mov_temp_eax[1] = (get_reg_index(temp_reg) << 3) | get_reg_index(X86_REG_EAX);
    buffer_append(b, mov_temp_eax, 2);

    // XCHG target_reg, temp_reg
    if (get_reg_index(temp_reg) >= get_reg_index(target_reg)) {
        // Format: 87 /r (REX.W + 87 /r) where /r is reg1 (target) and r/m is reg2 (temp)
        uint8_t xchg_code[] = {0x87, 0x00};
        xchg_code[1] = (get_reg_index(target_reg) << 3) | get_reg_index(temp_reg);
        buffer_append(b, xchg_code, 2);
    } else {
        // Format: 87 /r where /r is reg2 (temp) and r/m is reg1 (target) 
        uint8_t xchg_code[] = {0x87, 0x00};
        xchg_code[1] = (get_reg_index(temp_reg) << 3) | get_reg_index(target_reg);
        buffer_append(b, xchg_code, 2);
    }
}

strategy_t xchg_immediate_loading_strategy = {
    .name = "xchg_immediate_loading",
    .can_handle = can_handle_xchg_immediate_loading,
    .get_size = get_size_xchg_immediate_loading,
    .generate = generate_xchg_immediate_loading,
    .priority = 60  // Medium priority
};

// Alternative XCHG approach: Use stack-based exchange
int can_handle_xchg_stack_loading(cs_insn *insn) {
    // Same as above - check for MOV with immediate containing nulls
    if (insn->id != X86_INS_MOV) {
        return 0;
    }

    if (insn->detail->x86.op_count != 2) {
        return 0;
    }

    cs_x86_op *src_op = &insn->detail->x86.operands[1];
    if (src_op->type != X86_OP_IMM) {
        return 0;
    }

    uint32_t imm = (uint32_t)src_op->imm;
    for (int i = 0; i < 4; i++) {
        if (((imm >> (i * 8)) & 0xFF) == 0) {
            return 1;
        }
    }

    return 0;
}

size_t get_size_xchg_stack_loading(__attribute__((unused)) cs_insn *insn) {
    // PUSH imm (constructed without nulls) + POP target_reg
    // This may involve: MOV EAX, imm + PUSH EAX + POP target_reg
    return 15; // Conservative estimate
}

void generate_xchg_stack_loading(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    if (dst_op->type != X86_OP_REG || src_op->type != X86_OP_IMM) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    x86_reg target_reg = dst_op->reg;
    uint32_t imm = (uint32_t)src_op->imm;

    // MOV EAX, imm (using null-free construction)
    generate_mov_eax_imm(b, imm);

    // PUSH EAX (to save the value)
    uint8_t push_eax[] = {0x50};
    buffer_append(b, push_eax, 1);

    // POP target_reg
    uint8_t pop_reg[] = {0x58};
    pop_reg[0] = pop_reg[0] + get_reg_index(target_reg);
    buffer_append(b, pop_reg, 1);
}

strategy_t xchg_stack_loading_strategy = {
    .name = "xchg_stack_loading",
    .can_handle = can_handle_xchg_stack_loading,
    .get_size = get_size_xchg_stack_loading,
    .generate = generate_xchg_stack_loading,
    .priority = 55  // Lower priority than direct XCHG
};

// Register the XCHG-based strategies
void register_xchg_immediate_loading_strategies() {
    register_strategy(&xchg_immediate_loading_strategy);
    register_strategy(&xchg_stack_loading_strategy);
}