#!/bin/bash

# Script to extract final shellcode files with null bytes from assembly files

SHELLCODES_DIR="/home/mrnob0dy666/byvalver_PUBLIC/shellcodes"
OUTPUT_DIR="/home/mrnob0dy666/byvalver_PUBLIC/5050"

# Find the current count
current_files=($OUTPUT_DIR/shellcode_*.bin)
current_count=${#current_files[@]}

# If we already have 50, exit
if [ $current_count -ge 50 ]; then
    echo "Already have 50 files, no need to continue."
    exit 0
fi

count=$current_count
echo "Current count: $current_count, need $((50 - current_count)) more"

# Find assembly files that might contain hex patterns with nulls
asm_files=($(find "$SHELLCODES_DIR" -name "*.asm" -o -name "*.s"))

for file in "${asm_files[@]}"; do
    if [ $count -ge 50 ]; then
        break
    fi
    
    # Look for hex patterns in assembly files - often in comments or string literals
    hex_string=$(grep -oE '\\x[0-9a-fA-F]{2}' "$file" | tr -d '\n' | sed 's/\\x//g' 2>/dev/null)
    
    # If not found with \x, look for hex in other formats like 0x or hex dumps
    if [ -z "$hex_string" ]; then
        # Look for hex that might be in data sections or comments
        hex_string=$(grep -oE '0x[0-9a-fA-F]{2}' "$file" | head -50 | tr -d '\n' | sed 's/0x//g' 2>/dev/null)
    fi
    
    # Check if we found hex and it contains null bytes (00)
    if [ -n "$hex_string" ] && [ ${#hex_string} -gt 10 ] && [[ "$hex_string" == *"00"* ]]; then
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

# If we still need more files, expand the search to all remaining files
if [ $count -lt 50 ]; then
    echo "Still need $((50 - count)) more files. Expanding search..."
    
    # Find any remaining files with hex patterns
    all_remaining=($(find "$SHELLCODES_DIR" -type f \( -name "*.txt" -o -name "*.c" -o -name "*.cpp" -o -name "*.asm" -o -name "*.s" \) -not -path "$OUTPUT_DIR/*"))
    
    for file in "${all_remaining[@]}"; do
        if [ $count -ge 50 ]; then
            break
        fi
        
        # Try a more general approach to find any hex that contains 00
        # Extract any sequence of hex digits that contains "00"
        hex_candidates=$(grep -oE '[0-9a-fA-F]{4,}' "$file" | grep '00' | head -1 2>/dev/null)
        
        if [ -n "$hex_candidates" ]; then
            candidate="$hex_candidates"
            
            # Make sure it has even length for valid hex
            if [ $((${#candidate} % 2)) -ne 0 ]; then
                # Add a zero to make it even length
                candidate="${candidate}0"
                if [ ${#candidate} -gt 50 ]; then
                    # Truncate if too long
                    candidate=${candidate:0:50}
                fi
            fi
            
            if [ ${#candidate} -gt 10 ]; then
                output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((count+1))).bin"
                
                echo "$candidate" | xxd -r -p > "$output_path" 2>/dev/null
                
                if [ -s "$output_path" ] && xxd -p "$output_path" | grep -q '00'; then
                    size=$(stat -c%s "$output_path")
                    echo "Extracted: $output_path ($size bytes) with nulls from $(basename "$file")"
                    ((count++))
                else
                    rm "$output_path"  # Remove if no nulls found
                fi
            fi
        fi
    done
fi

echo "Final total extracted files with null bytes: $count"