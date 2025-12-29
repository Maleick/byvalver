#!/bin/bash

# Comprehensive script to extract shellcodes with null bytes from various formats

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

# Find files that likely contain shellcode by looking for patterns
all_files=($(find "$SHELLCODES_DIR" -type f \( -name "*.txt" -o -name "*.c" -o -name "*.cpp" -o -name "*.asm" -o -name "*.s" \)))

for file in "${all_files[@]}"; do
    if [ $count -ge 50 ]; then
        break
    fi
    
    extracted_from_current_file=0
    
    # Try multiple extraction patterns
    # Pattern 1: Look for strings with \x hex patterns
    while IFS= read -r line; do
        if [ $count -ge 50 ]; then
            break
        fi
        
        # Extract potential shellcode patterns from this line
        potential_shellcodes=$(echo "$line" | grep -oE '\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2}){4,}' 2>/dev/null)
        
        for shellcode in $potential_shellcodes; do
            if [ $count -ge 50 ]; then
                break
            fi
            
            hex_string=$(echo "$shellcode" | sed 's/\\x//g')
            if extract_and_save "$hex_string" "$file"; then
                extracted_from_current_file=1
                continue 2  # Continue the outer while loop
            fi
        done
    done < <(grep -i "shellcode\|\\\\x\|\\\\\\\\x" "$file" 2>/dev/null)
    
    if [ $count -ge 50 ]; then
        break
    fi

    # Pattern 2: Look for quoted strings that contain shellcode
    while IFS= read -r line; do
        if [ $count -ge 50 ]; then
            break
        fi
        
        # Extract content within quotes that has hex patterns
        quoted_content=$(echo "$line" | grep -oE '"[^"]*\\\\x[0-9a-fA-F]{2}[^"]*"' 2>/dev/null | sed 's/"//g')
        
        if [ -n "$quoted_content" ]; then
            hex_string=$(echo "$quoted_content" | sed 's/\\x//g')
            if extract_and_save "$hex_string" "$file"; then
                extracted_from_current_file=1
                continue 2  # Continue the outer while loop
            fi
        fi
    done < <(grep -i "shellcode\|=" "$file" 2>/dev/null)
    
    if [ $count -ge 50 ]; then
        break
    fi

    # Pattern 3: Look at the entire file for hex dump patterns
    hex_chunks=$(grep -oE '[0-9a-fA-F]\{2\} [0-9a-fA-F]\{2\} [0-9a-fA-F]\{2\}' "$file" | tr -d ' \n' 2>/dev/null)
    
    if [ -n "$hex_chunks" ] && [[ "$hex_chunks" == *"00"* ]] && [ ${#hex_chunks} -gt 10 ]; then
        if [ $((${#hex_chunks} % 2)) -ne 0 ]; then
            hex_chunks="${hex_chunks}0"  # Pad with 0 if odd length
        fi
        if extract_and_save "$hex_chunks" "$file"; then
            extracted_from_current_file=1
            continue
        fi
    fi

    if [ $count -ge 50 ]; then
        break
    fi

    # Pattern 4: Look for potential C-style shellcode arrays
    c_shellcode=$(grep -oE '0x[0-9a-fA-F]\{2\}[, \t\n\r]*0x[0-9a-fA-F]\{2\}[, \t\n\r]*0x[0-9a-fA-F]\{2\}' "$file" | tr -d 'x, \n\t\r' 2>/dev/null)
    
    if [ -n "$c_shellcode" ] && [[ "$c_shellcode" == *"00"* ]] && [ ${#c_shellcode} -gt 10 ]; then
        if [ $((${#c_shellcode} % 2)) -ne 0 ]; then
            c_shellcode="0${c_shellcode}"
        fi
        if extract_and_save "$c_shellcode" "$file"; then
            extracted_from_current_file=1
            continue
        fi
    fi
done

echo "Total extracted files with null bytes: $count"