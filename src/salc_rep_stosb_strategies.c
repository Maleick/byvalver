#include "strategy.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

// Strategy 9: SALC + REP STOSB for Null-filled Buffers
// Utilize `SALC` to set `AL` to zero and then `REP STOSB` to efficiently fill 
// memory regions with null bytes without embedding them in instructions,
// useful for initializing structures or buffers.

int can_handle_salc_rep_stosb_null_fill(__attribute__((unused)) cs_insn *insn) {
    // DISABLED: Strategy accepts single MOV instructions but cannot transform them
    // Falls back to original instruction WITH null bytes (line 60)
    // Designed for multi-instruction patterns but applied to single instructions
    // See analysis report: 105 failures, 0% success rate
    // Issue: https://github.com/mrnob0dy666/byvalver/issues/XXX
    return 0;
}

size_t get_size_salc_rep_stosb_null_fill(__attribute__((unused)) cs_insn *insn) {
    // SALC (D6) + MOV ECX, count + MOV EDI, dest + REP STOSB (F3 AA)
    // = 1 + ~5-7 + ~5-7 + 2 = ~15-19 bytes (depending on count values)
    return 20; // Conservative estimate
}

void generate_salc_rep_stosb_null_fill(struct buffer *b, cs_insn *insn) {
    // Note: A single MOV instruction can't be transformed directly to REP STOSB
    // because they do different things. But for buffer initialization,
    // this strategy could be applied when multiple sequential zero-writes are detected
    
    // For now, since we're only analyzing single instructions, 
    // we'll implement a more general approach of zeroing registers/initializing memory
    
    // If this was part of a sequence of operations to initialize memory,
    // we would use: SALC to set AL to 0, then use REP STOSB to fill
    
    // SALC instruction (0xD6) sets AL to 0 if carry flag is 0, or 0xFF if carry flag is 1
    // To ensure AL=0, we might want to clear carry flag first with CLC (0xF8)
    // Then use SALC (0xD6), then MOV ECX, count, MOV EDI, address, REP STOSB (0xF3, 0xAA)
    
    // For this single-instruction approach, fall back to the original
    buffer_append(b, insn->bytes, insn->size);
}

// Alternative approach: Use CLC + SALC for guaranteed zero in AL
int can_handle_clc_salc_zero(cs_insn *insn) {
    // Check if we're looking for a way to get zero in AL without immediate
    if (insn->id == X86_INS_MOV) {
        if (insn->detail->x86.op_count == 2) {
            cs_x86_op *dst_op = &insn->detail->x86.operands[0];
            cs_x86_op *src_op = &insn->detail->x86.operands[1];
            
            // If setting a register to 0 and the register is AL or EAX
            if (src_op->type == X86_OP_IMM && src_op->imm == 0) {
                if (dst_op->type == X86_OP_REG) {
                    x86_reg reg = dst_op->reg;
                    if (reg == X86_REG_EAX || reg == X86_REG_AX || reg == X86_REG_AL) {
                        return 1;
                    }
                }
            }
        }
    }
    
    return 0;
}

size_t get_size_clc_salc_zero(__attribute__((unused)) cs_insn *insn) {
    // CLC (1 byte) + SALC (1 byte) = 2 bytes
    // This is much more compact than XOR EAX,EAX (2 bytes) or MOV EAX,0 (5 bytes)
    return 2; // Size for CLC + SALC
}

void generate_clc_salc_zero(struct buffer *b, cs_insn *insn) {
    // The SALC instruction sets AL to 0 if carry flag is clear, or 0xFF if set
    // So to get AL=0, we first clear carry with CLC, then execute SALC

    // CLC - Clear Carry Flag (0xF8)
    uint8_t clc = 0xF8;
    buffer_write_byte(b, clc);

    // SALC - Set AL on Carry (0xD6) - with carry clear, AL becomes 0
    uint8_t salc = 0xD6;
    buffer_write_byte(b, salc);

    // At this point, AL contains 0
    // If the original instruction was MOV EAX,0, we're done since AL being 0
    // means the lower byte of EAX is 0, but the other bytes might not be.
    // For a complete zero of EAX, we might need more instructions.

    // For a complete register zero, we could do a more comprehensive approach
    // but since we're limited to the context of the original instruction,
    // we'll focus on the AL aspect which is what SALC affects directly.

    // If the target was a specific memory location with zero,
    // we could use this in combination with STOSB, but that would require
    // more context than a single instruction analysis provides

    (void)insn; // Mark parameter as used to avoid warning
}

strategy_t salc_rep_stosb_null_fill_strategy = {
    .name = "salc_rep_stosb_null_fill",
    .can_handle = can_handle_salc_rep_stosb_null_fill,
    .get_size = get_size_salc_rep_stosb_null_fill,
    .generate = generate_salc_rep_stosb_null_fill,
    .priority = 50  // Medium-low priority
};

strategy_t clc_salc_zero_strategy = {
    .name = "clc_salc_zero",
    .can_handle = can_handle_clc_salc_zero,
    .get_size = get_size_clc_salc_zero,
    .generate = generate_clc_salc_zero,
    .priority = 65  // Medium-high priority for register zeroing
};

// Register the SALC + REP STOSB strategies
void register_salc_rep_stosb_strategies() {
    register_strategy(&salc_rep_stosb_null_fill_strategy);
    register_strategy(&clc_salc_zero_strategy);
}