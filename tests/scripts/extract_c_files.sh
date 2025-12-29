#!/bin/bash

# Script to extract additional shellcode files with null bytes from C files

SHELLCODES_DIR="/home/mrnob0dy666/byvalver_PUBLIC/shellcodes"
OUTPUT_DIR="/home/mrnob0dy666/byvalver_PUBLIC/5050"

# Find the current count
current_count=$(ls "$OUTPUT_DIR"/shellcode_*.bin 2>/dev/null | wc -l)
if [ -z "$current_count" ] || [ "$current_count" -lt 1 ]; then
    current_count=0
else
    current_count=$(printf "%02d" "$current_count")
fi

count=$current_count

# If we already have 50, exit
if [ $count -ge 50 ]; then
    echo "Already have 50 files, no need to continue."
    exit 0
fi

# Find C/CPP files that might contain shellcode
c_files=($(find "$SHELLCODES_DIR" -name "*.c" -o -name "*.cpp" | head -50))

for file in "${c_files[@]}"; do
    if [ $count -ge 50 ]; then
        break
    fi
    
    # Look for shellcode in C-style format like: "\x##\x##..."
    # This pattern looks for quoted strings with hex sequences
    hex_string=$(grep -oE '"[^"]*"' "$file" | grep -E '\\x[0-9a-fA-F]{2}' | head -1 | sed 's/"//g' | sed 's/\\x//g' 2>/dev/null)
    
    # If not found, try looking for the common C-style shellcode format
    if [ -z "$hex_string" ]; then
        # Look for patterns like 0x## in lists
        hex_list=$(grep -oE '0x[0-9a-fA-F]{2}' "$file" | head -50 | tr -d '\n' | sed 's/0x//g' 2>/dev/null)
        if [ -n "$hex_list" ] && [[ "$hex_list" == *"00"* ]]; then
            hex_string="$hex_list"
        fi
    fi
    
    if [ -n "$hex_string" ] && [ ${#hex_string} -gt 10 ]; then
        # Make sure it has even length for valid hex
        if [ $((${#hex_string} % 2)) -ne 0 ]; then
            # Truncate to even length
            hex_string=${hex_string:0:$((${#hex_string}-1))}
        fi
        
        output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((count+1))).bin"
        
        echo "$hex_string" | xxd -r -p > "$output_path" 2>/dev/null
        
        if [ -s "$output_path" ]; then
            # Check if this file contains null bytes (00)
            if xxd -p "$output_path" | grep -q '00'; then
                size=$(stat -c%s "$output_path")
                echo "Extracted: $output_path ($size bytes) with nulls from $(basename "$file")"
                ((count++))
            else
                rm "$output_path"  # Remove if no nulls found
            fi
        else
            rm "$output_path"  # Remove if file is empty
        fi
    fi
done

echo "Total extracted files with null bytes: $count"