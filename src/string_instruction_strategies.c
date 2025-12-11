#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

// Strategy 4: String Instruction with Null Construction
// Use STOSB, STOSD, or similar string instructions with loops to construct 
// immediate values containing nulls in memory rather than through direct immediate encoding.

int can_handle_string_instruction_null_construct(__attribute__((unused)) cs_insn *insn) {
    // DISABLED: Implementation calls generate_mov_eax_imm() which may introduce nulls
    // Strategy claims to use STOSB byte-by-byte construction but doesn't implement it
    // See analysis report: 6,822 attempts with 0% success rate
    // Issue: https://github.com/mrnob0dy666/byvalver/issues/XXX
    return 0;
}

size_t get_size_string_instruction_null_construct(__attribute__((unused)) cs_insn *insn) {
    // The approach: Use ECX for counter + STOSB/STOSD instructions to build value byte-by-byte
    // This would involve: MOV ECX, 4 + MOV EDI, some_stack_location + MOV EAX, 0 + STOSB*4 + MOV target_reg, [constructed_location]
    // Approximate: ~20-30 bytes depending on implementation
    return 30; // Conservative estimate
}

void generate_string_instruction_null_construct(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];  // destination register
    cs_x86_op *src_op = &insn->detail->x86.operands[1];  // source immediate

    if (dst_op->type != X86_OP_REG || src_op->type != X86_OP_IMM) {
        // Fallback if not in expected format
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    x86_reg target_reg = dst_op->reg;
    uint32_t imm = (uint32_t)src_op->imm;

    // Strategy: Use STOSD to store the immediate value byte by byte
    // First, we need a memory location to store it
    
    // We can use the stack for temporary storage
    // PUSH target_reg to save it
    uint8_t push_target_reg = 0x50 + get_reg_index(target_reg);
    buffer_write_byte(b, push_target_reg);

    // MOV EAX, imm (the value we want to construct)
    generate_mov_eax_imm(b, imm);

    // We'll use STOSD to store EAX into [EDI], so EDI needs to point to our target location
    // To do this properly, we could use a stack location or manipulate memory
    // For this implementation, we'll store the value temporarily on the stack
    // and then load it back to the target register
    
    // PUSH EAX (to save the constructed value temporarily)
    uint8_t push_eax[] = {0x50};
    buffer_append(b, push_eax, 1);

    // POP target_reg (to restore the value to the target register)
    uint8_t pop_target_reg = 0x58 + get_reg_index(target_reg);
    buffer_append(b, &pop_target_reg, 1);
    
    // POP to restore the original target_reg value (if it had one we wanted to preserve)
    // Note: This implementation may not preserve original behavior perfectly
    // The proper implementation would need more complex stack manipulation
}

// Alternative approach: Use STOSB to build value byte by byte
int can_handle_byte_by_byte_construction(__attribute__((unused)) cs_insn *insn) {
    // DISABLED: Implementation calls generate_mov_eax_imm() which may introduce nulls
    // Strategy doesn't implement actual byte-by-byte construction as advertised
    // See analysis report: 6,822 attempts with 0% success rate
    // Issue: https://github.com/mrnob0dy666/byvalver/issues/XXX
    return 0;
}

size_t get_size_byte_by_byte_construction(__attribute__((unused)) cs_insn *insn) {
    // MOV ECX, count + MOV EDI, target + MOV AL/AX/EAX with bytes + STOSB/STOSW/STOSD * count
    // This varies significantly based on the value, but estimate around 25-35 bytes
    return 35; // Conservative estimate
}

void generate_byte_by_byte_construction(struct buffer *b, cs_insn *insn) {
    cs_x86_op *dst_op = &insn->detail->x86.operands[0];
    cs_x86_op *src_op = &insn->detail->x86.operands[1];

    if (dst_op->type != X86_OP_REG || src_op->type != X86_OP_IMM) {
        buffer_append(b, insn->bytes, insn->size);
        return;
    }

    x86_reg target_reg = dst_op->reg;
    uint32_t imm = (uint32_t)src_op->imm;
    
    // We'll construct the value by pushing individual bytes onto the stack
    // and then popping the full value into the target register
    
    // Save the target register
    uint8_t push_target = 0x50 + get_reg_index(target_reg);
    buffer_write_byte(b, push_target);

    // Push each byte of the immediate separately (in reverse order for little-endian)
    // The MOV instruction will push the value to the stack without null bytes in immediates
    generate_mov_eax_imm(b, imm);
    
    // Now move the value from EAX to the target register
    if (target_reg != X86_REG_EAX) {
        // MOV target_reg, EAX
        uint8_t mov_reg_eax[] = {0x89, 0x00};
        mov_reg_eax[1] = (get_reg_index(target_reg) << 3) | get_reg_index(X86_REG_EAX);
        buffer_append(b, mov_reg_eax, 2);
    }
    
    // Restore the original value of target_reg that we saved earlier
    // Note: This is slightly different from original since we're not restoring same value
    // This is a simplified approach that focuses on getting the immediate value without nulls
}

strategy_t string_instruction_null_construct_strategy = {
    .name = "string_instruction_null_construct",
    .can_handle = can_handle_string_instruction_null_construct,
    .get_size = get_size_string_instruction_null_construct,
    .generate = generate_string_instruction_null_construct,
    .priority = 45  // Lower priority, more complex approach
};

strategy_t byte_by_byte_construction_strategy = {
    .name = "byte_by_byte_construction",
    .can_handle = can_handle_byte_by_byte_construction,
    .get_size = get_size_byte_by_byte_construction,
    .generate = generate_byte_by_byte_construction,
    .priority = 40  // Even lower priority
};

// Register the string instruction strategies
void register_string_instruction_strategies() {
    register_strategy(&string_instruction_null_construct_strategy);
    register_strategy(&byte_by_byte_construction_strategy);
}