#!/bin/bash

# Final comprehensive script to extract 50 shellcodes with null bytes

SHELLCODES_DIR="/home/mrnob0dy666/byvalver_PUBLIC/shellcodes"
OUTPUT_DIR="/home/mrnob0dy666/byvalver_PUBLIC/5050"

# Clear and create output directory
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

count=0

# Function to extract and save shellcode if it contains null bytes
extract_and_save() {
    local hex_string="$1"
    local source_file="$2"
    
    if [ ${#hex_string} -gt 10 ] && [[ $hex_string =~ [0-9a-fA-F]+ ]] && [ $((${#hex_string} % 2)) -eq 0 ]; then
        local output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((count+1))).bin"
        
        echo "$hex_string" | xxd -r -p > "$output_path" 2>/dev/null
        
        if [ -s "$output_path" ] && xxd -p "$output_path" | grep -q '00'; then
            local size=$(stat -c%s "$output_path")
            echo "Extracted: $output_path ($size bytes) with nulls from $(basename "$source_file")"
            ((count++))
            return 0
        else
            rm -f "$output_path"  # Remove if no nulls or empty
        fi
    fi
    return 1
}

# Find all files that might contain shellcode or hex data
all_files=($(find "$SHELLCODES_DIR" -type f \( -name "*.txt" -o -name "*.c" -o -name "*.cpp" -o -name "*.asm" -o -name "*.s" \)))

for file in "${all_files[@]}"; do
    if [ $count -ge 50 ]; then
        break
    fi
    
    # Try multiple extraction patterns for each file
    
    # Pattern 1: Look for strings with \x hex patterns specifically
    while IFS= read -r line; do
        if [ $count -ge 50 ]; then
            break
        fi
        potential_shellcodes=$(echo "$line" | grep -oE '\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2}){4,}' 2>/dev/null)
        for shellcode in $potential_shellcodes; do
            if [ $count -ge 50 ]; then
                break
            fi
            hex_string=$(echo "$shellcode" | sed 's/\\x//g')
            extract_and_save "$hex_string" "$file" && continue 2  # Continue outer loop
        done
    done < <(grep -i "shellcode\|\\\\x\|\\\\\\\\x" "$file" 2>/dev/null)
    
    if [ $count -ge 50 ]; then
        break
    fi

    # Pattern 2: Look for C-style quoted shellcodes
    while IFS= read -r line; do
        if [ $count -ge 50 ]; then
            break
        fi
        quoted_content=$(echo "$line" | grep -oE '"[^"]*\\\\x[0-9a-fA-F]{2}[^"]*"' 2>/dev/null | sed 's/"//g')
        if [ -n "$quoted_content" ]; then
            hex_string=$(echo "$quoted_content" | sed 's/\\x//g')
            extract_and_save "$hex_string" "$file" && continue 2  # Continue outer loop
        fi
    done < <(grep -i "shellcode\|=" "$file" 2>/dev/null)

    if [ $count -ge 50 ]; then
        break
    fi

    # Pattern 3: Look for hex dumps in the file (usually has addresses followed by hex)
    while IFS= read -r line; do
        if [ $count -ge 50 ]; then
            break
        fi
        # Extract hex from lines that look like disassembly dumps
        hex_dump=$(echo "$line" | grep -oE '[0-9a-fA-F]\{2\}[[:space:]]\{1,\}[0-9a-fA-F]\{2\}[[:space:]]\{1,\}[0-9a-fA-F]\{2\}' | tr -d ' \t' 2>/dev/null)
        if [ -n "$hex_dump" ] && [[ "$hex_dump" == *"00"* ]]; then
            # Check if this hex string has enough characters and is valid
            if [ ${#hex_dump} -gt 10 ]; then
                extract_and_save "$hex_dump" "$file" && continue 2  # Continue outer loop
            fi
        fi
    done < "$file"
done

# If we still don't have 50 files, do a more general search
if [ $count -lt 50 ]; then
    echo "Still need $((50 - count)) more files, doing general search for hex with nulls..."
    
    # Find files that contain hex characters and '00'
    for file in "${all_files[@]}"; do
        if [ $count -ge 50 ]; then
            break
        fi
        
        # Extract any hex sequence that contains 00
        # Look for any sequence of hex digits that might include nulls
        hex_seqs=$(grep -oE '[0-9a-fA-F]\{4,\}' "$file" | grep '00' | head -1 2>/dev/null)
        
        for hex_seq in $hex_seqs; do
            if [ $count -ge 50 ]; then
                break
            fi
            
            # Make sure length is even for valid hex
            if [ $((${#hex_seq} % 2)) -ne 0 ]; then
                # Truncate to even length if needed
                hex_seq=${hex_seq:0:$((${#hex_seq}-1))}
            fi
            
            if [ ${#hex_seq} -gt 10 ]; then
                extract_and_save "$hex_seq" "$file" && break  # Break inner loop to continue with next file
            fi
        done
    done
fi

echo "Final count: $count files with null bytes extracted to $OUTPUT_DIR"