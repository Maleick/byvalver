#include "new_strategies.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>

// Transformation strategy for MOV reg32, [reg32] instructions that contain null bytes
// Example: mov eax, [eax] (0x8B 0x00) -> transformed to null-byte-free sequence
bool transform_mov_reg_mem_self_can_handle(csh handle, cs_insn *insn) {
    if (!insn || insn->id != X86_INS_MOV) {
        return false;
    }

    // Check if the instruction is MOV reg32, [reg32]
    if (insn->detail->x86.op_count == 2) {
        cs_x86_op op0 = insn->detail->x86.operands[0];
        cs_x86_op op1 = insn->detail->x86.operands[1];

        // First operand should be a register
        if (op0.type == X86_OP_REG && 
            op0.size == 4 &&  // 32-bit register
            op1.type == X86_OP_MEM && 
            op1.mem.base != X86_REG_INVALID && 
            op1.mem.index == X86_REG_INVALID &&  // No index register
            op1.mem.scale == 1 &&  // Scale factor of 1
            op1.mem.disp == 0 &&   // No displacement (this creates the null byte issue)
            op0.reg == op1.mem.base) {  // Register and memory base are the same
            
            // Check if the original encoding contains null bytes
            for (int i = 0; i < insn->size; i++) {
                if (insn->bytes[i] == 0x00) {
                    return true;
                }
            }
        }
    }
    return false;
}

int transform_mov_reg_mem_self_get_size(csh handle, cs_insn *insn) {
    // The transformed sequence is:
    // push ecx           // 1 byte
    // lea ecx, [eax - 1] // 3 bytes (if eax, could be 2-3 depending on reg)
    // mov eax, [ecx + 1] // 3 bytes (if eax, could be 2-3 depending on reg)
    // pop ecx            // 1 byte
    // Total: ~8 bytes (may vary based on specific registers used)
    
    // Conservative estimate of 10 bytes to ensure enough space
    return 10;
}

int transform_mov_reg_mem_self_generate(csh handle, cs_insn *insn, unsigned char *output) {
    if (!output || !insn) {
        return 0;
    }

    // Get the register being used
    cs_x86_op op0 = insn->detail->x86.operands[0];  // destination register
    cs_x86_op op1 = insn->detail->x86.operands[1];  // source memory

    if (op0.type != X86_OP_REG || op1.type != X86_OP_MEM) {
        return 0;
    }

    unsigned char *ptr = output;
    int offset = 0;

    // push ecx (0x51) - save temporary register
    ptr[offset++] = 0x51;

    // lea ecx, [reg - 1] - load effective address with non-zero displacement
    // For eax: lea ecx, [eax - 1] = 0x8D, 0x48, 0xFF
    // For ebx: lea ecx, [ebx - 1] = 0x8D, 0x4B, 0xFF
    // For ecx: lea ecx, [ecx - 1] won't work since ecx is our temp register
    // For other registers: use the same approach but avoid conflicts
    
    int temp_reg = 1;  // Use ECX as temporary register (reg number 1)
    int src_reg = op1.mem.base - X86_REG_EAX;  // Convert to register number (0-7 for 32-bit regs)

    // If the source register is ECX, we need to use a different temporary register
    if (op1.mem.base == X86_REG_ECX) {
        // We'll need to use EDX as temp register (reg number 2)
        // push edx, lea edx, [reg - 1], mov reg, [edx + 1], pop edx
        
        // push edx (0x52)
        ptr[offset++] = 0x52;
        
        // lea edx, [src_reg - 1] (0x8D, 0x5? 0xFF)
        ptr[offset++] = 0x8D;
        ptr[offset++] = 0x50 | src_reg;  // ModR/M byte: 11-010-src_reg
        ptr[offset++] = 0xFF;  // -1 displacement
        
        // mov dest_reg, [edx + 1] (0x8B, 0x42, 0x01)
        ptr[offset++] = 0x8B;
        ptr[offset++] = 0x42 | ((op0.reg - X86_REG_EAX) << 3);  // ModR/M byte
        ptr[offset++] = 0x01;  // +1 displacement
        
        // pop edx (0x5A)
        ptr[offset++] = 0x5A;
    } else {
        // push ecx (0x51) - already done
        // lea ecx, [src_reg - 1] (0x8D, 0x4? 0xFF)
        ptr[offset++] = 0x8D;
        ptr[offset++] = 0x48 | src_reg;  // ModR/M byte: 11-001-src_reg, assumes ECX is temp reg
        ptr[offset++] = 0xFF;  // -1 displacement
        
        // mov dest_reg, [ecx + 1] (0x8B, 0x41, 0x01)
        ptr[offset++] = 0x8B;
        // Determine the ModR/M byte based on the destination register
        unsigned char modrm = 0x40 | (src_reg << 3) | (temp_reg); // [temp_reg + disp8]
        // Actually we need: mov dest_reg, [temp_reg + 1], so: 0x8B, (dest_reg << 3) | temp_reg + 0x40
        unsigned char dest_reg_num = op0.reg - X86_REG_EAX;
        ptr[offset++] = 0x41 | (dest_reg_num << 3);  // ModR/M: mod=01, r/m=temp_reg(ECX=1), reg=dest
        ptr[offset++] = 0x01;  // +1 displacement
        
        // pop ecx (0x59)
        ptr[offset++] = 0x59;
    }

    // Let's implement the general approach more carefully:
    // push temp_reg
    // lea temp_reg, [source_reg - 1]
    // mov dest_reg, [temp_reg + 1] 
    // pop temp_reg
    
    // Reset offset for correct implementation
    offset = 0;
    unsigned char temp_reg_code = 0x51; // ECX
    unsigned char temp_reg_num = 1;
    unsigned char src_reg_num = op1.mem.base - X86_REG_EAX;
    unsigned char dest_reg_num = op0.reg - X86_REG_EAX;
    
    // If source register is same as our temp register, use EDX instead
    if (op1.mem.base == X86_REG_ECX) {
        temp_reg_code = 0x52; // EDX
        temp_reg_num = 2;
        src_reg_num = op1.mem.base - X86_REG_EAX;
    }
    
    // push temp_reg
    ptr[offset++] = temp_reg_code;
    
    // lea temp_reg, [src_reg - 1]
    ptr[offset++] = 0x8D;
    ptr[offset++] = 0x48 | temp_reg_num;  // ModR/M: reg=temp_reg, r/m=src_reg  
    ptr[offset++] = 0xFF; // -1 displacement
    
    // mov dest_reg, [temp_reg + 1]
    ptr[offset++] = 0x8B;
    ptr[offset++] = 0x40 | (dest_reg_num << 3) | temp_reg_num; // ModR/M: reg=dest_reg, r/m=temp_reg
    ptr[offset++] = 0x01; // +1 displacement
    
    // pop temp_reg  
    if (op1.mem.base == X86_REG_ECX) {
        ptr[offset++] = 0x5A; // pop edx
    } else {
        ptr[offset++] = 0x59; // pop ecx
    }

    return offset;
}

// Define the strategy structure for MOV reg32, [reg32]
strategy_t transform_mov_reg_mem_self = {
    .priority = 100,  // High priority for this specific pattern
    .can_handle = transform_mov_reg_mem_self_can_handle,
    .get_size = transform_mov_reg_mem_self_get_size,
    .generate = transform_mov_reg_mem_self_generate
};

// Transformation strategy for ADD [mem], reg8 instructions that contain null bytes
// Example: add [eax], al (0x00 0x00) -> transformed to null-byte-free sequence
bool transform_add_mem_reg8_can_handle(csh handle, cs_insn *insn) {
    if (!insn || insn->id != X86_INS_ADD) {
        return false;
    }

    // Check if the instruction is ADD [mem], reg8
    if (insn->detail->x86.op_count == 2) {
        cs_x86_op op0 = insn->detail->x86.operands[0];
        cs_x86_op op1 = insn->detail->x86.operands[1];

        // First operand should be memory, second should be 8-bit register
        if (op0.type == X86_OP_MEM &&
            op1.type == X86_OP_REG &&
            op1.size == 1) {  // 8-bit register

            // Check if the original encoding contains null bytes
            for (int i = 0; i < insn->size; i++) {
                if (insn->bytes[i] == 0x00) {
                    return true;
                }
            }
        }
    }
    return false;
}

int transform_add_mem_reg8_get_size(csh handle, cs_insn *insn) {
    // The transformed sequence is approximately:
    // push temp_reg     // 1 byte
    // movzx temp_reg, [mem]  // 3-4 bytes (0x0F 0xB6)
    // add temp_reg, reg8     // 2-3 bytes
    // mov [mem], temp_reg_lo // 2-3 bytes
    // pop temp_reg    // 1 byte
    // Total: ~10-14 bytes

    // Conservative estimate of 15 bytes to ensure enough space
    return 15;
}

int transform_add_mem_reg8_generate(csh handle, cs_insn *insn, unsigned char *output) {
    if (!output || !insn) {
        return 0;
    }

    // Get the operands
    cs_x86_op op0 = insn->detail->x86.operands[0];  // destination memory
    cs_x86_op op1 = insn->detail->x86.operands[1];  // source register (8-bit)

    if (op0.type != X86_OP_MEM || op1.type != X86_OP_REG || op1.size != 1) {
        return 0;
    }

    unsigned char *ptr = output;
    int offset = 0;

    // Determine register numbers
    unsigned char mem_reg_num = op0.mem.base - X86_REG_EAX;
    unsigned char src_reg_num = op1.reg - X86_REG_EAX;
    unsigned char temp_reg_num = 1;  // Use ECX as temp (reg number 1)

    // If memory register is ECX, use EDX as temp register instead
    if (op0.mem.base == X86_REG_ECX) {
        temp_reg_num = 2;  // Use EDX
    }

    // push temp_reg (ECX or EDX)
    if (temp_reg_num == 1) {
        ptr[offset++] = 0x51; // push ecx
    } else {
        ptr[offset++] = 0x52; // push edx
    }

    // movzx temp_reg, byte ptr [mem_reg]
    // Opcode: 0x0F 0xB6 (MOVZX)
    ptr[offset++] = 0x0F;
    ptr[offset++] = 0xB6;

    // ModR/M byte: reg=temp_reg, r/m=mem_reg with [mem_reg] addressing (mod=00)
    unsigned char modrm = (temp_reg_num << 3) | mem_reg_num;
    ptr[offset++] = modrm;

    // add temp_reg, src_reg8
    // For 8-bit register addition: 0x00 /r pattern
    ptr[offset++] = 0x00;
    // ModR/M byte: reg=src_reg8, r/m=temp_reg (mod=11 for register-register)
    unsigned char add_modrm = 0xC0 | (src_reg_num << 3) | temp_reg_num;
    ptr[offset++] = add_modrm;

    // mov byte ptr [mem_reg], temp_reg_low8
    ptr[offset++] = 0x88;
    // ModR/M byte: reg=temp_reg, r/m=mem_reg
    unsigned char mov_modrm = (temp_reg_num << 3) | mem_reg_num;
    ptr[offset++] = mov_modrm;

    // pop temp_reg
    if (temp_reg_num == 1) {
        ptr[offset++] = 0x59; // pop ecx
    } else {
        ptr[offset++] = 0x5A; // pop edx
    }

    return offset;
}

// Define the strategy structure for ADD [mem], reg8
strategy_t transform_add_mem_reg8 = {
    .priority = 100,  // High priority for this specific pattern
    .can_handle = transform_add_mem_reg8_can_handle,
    .get_size = transform_add_mem_reg8_get_size,
    .generate = transform_add_mem_reg8_generate
};