#!/bin/bash

# Script to extract shellcodes with actual null bytes from files that contain \x00 patterns

SHELLCODES_DIR="/home/mrnob0dy666/byvalver_PUBLIC/shellcodes"
OUTPUT_DIR="/home/mrnob0dy666/byvalver_PUBLIC/5050"

# Clear and create output directory
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# Find all files containing \x00 patterns
files_with_nulls=($(find "$SHELLCODES_DIR" -type f \( -name "*.txt" -o -name "*.c" -o -name "*.cpp" -o -name "*.asm" -o -name "*.s" \) -exec grep -l '\\x00' {} \;))

processed=0

for file in "${files_with_nulls[@]}"; do
    if [ $processed -ge 50 ]; then
        break
    fi
    
    # Extract the specific shellcode that contains \x00 patterns
    # Look for quoted strings with shellcode or lines with multiple hex patterns
    
    # Method 1: Look for quoted strings containing hex patterns with nulls
    shellcode_string=$(grep -oE '"[^"]*\\\\x[0-9a-fA-F]{2}[^"]*"' "$file" | grep '\\x00' | head -1 | sed 's/"//g' 2>/dev/null)
    
    if [ -n "$shellcode_string" ]; then
        # Remove \x prefix to get hex string
        hex_string=$(echo "$shellcode_string" | sed 's/\\x//g' | sed 's/[^0-9a-fA-F]//g' 2>/dev/null)
    else
        # Method 2: Extract hex patterns from \x format specifically looking for those with 00
        # Get all \x patterns in the file
        all_hex=$(grep -oE '\\\\x[0-9a-fA-F]{2}' "$file" | tr -d '\n' 2>/dev/null)
        if [ -n "$all_hex" ]; then
            hex_string=$(echo "$all_hex" | sed 's/\\x//g' 2>/dev/null)
        else
            # Method 3: Look for the specific shellcode after markers like "shellcode:"
            if grep -qi "shellcode" "$file"; then
                # Extract content after shellcode markers that might contain the actual shellcode
                shellcode_section=$(sed -n '/[Ss]hellcode.*:/,$p' "$file")
                hex_match=$(echo "$shellcode_section" | grep -oE '\\\\x[0-9a-fA-F]{2}' | tr -d '\n' 2>/dev/null)
                if [ -n "$hex_match" ]; then
                    hex_string=$(echo "$hex_match" | sed 's/\\x//g' 2>/dev/null)
                fi
            fi
        fi
    fi
    
    # Process hex string if found
    if [ -n "$hex_string" ] && [ ${#hex_string} -gt 10 ]; then
        # Ensure even length for valid hex
        if [ $((${#hex_string} % 2)) -ne 0 ]; then
            hex_string=${hex_string:0:$((${#hex_string}-1))}
        fi
        
        output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((processed+1))).bin"
        
        echo "$hex_string" | xxd -r -p > "$output_path" 2>/dev/null
        
        if [ -s "$output_path" ]; then
            # Use Python to accurately count actual null bytes
            null_count=$(python3 -c "
import sys
with open('$output_path', 'rb') as f:
    data = f.read()
    print(data.count(0))
" 2>/dev/null)
            
            if [ "$null_count" -gt 0 ]; then
                size=$(stat -c%s "$output_path")
                echo "Valid shellcode with $null_count null byte(s): $output_path from $(basename "$file")"
                ((processed++))
            else
                rm "$output_path"  # Remove if no actual nulls
            fi
        else
            rm "$output_path"  # Remove if empty
        fi
    fi
done

echo "Successfully extracted $processed shellcode files with actual null bytes"