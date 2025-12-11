#!/bin/bash

# Extract ALL shellcodes with null bytes, not just 50

SHELLCODES_DIR="/home/mrnob0dy666/byvalver_PUBLIC/shellcodes"
OUTPUT_DIR="/home/mrnob0dy666/byvalver_PUBLIC/5050"

# Clear and create output directory
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# Find all files containing \x00 patterns
files_with_nulls=($(find "$SHELLCODES_DIR" -type f \( -name "*.txt" -o -name "*.c" -o -name "*.cpp" -o -name "*.asm" -o -name "*.s" \) -exec grep -l '\\x00' {} \;))

processed=0

for file in "${files_with_nulls[@]}"; do
    output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((processed+1))).bin"
    
    # Method 1: Handle files with shellcode in C-style format: char shellcode[] = "\x...";
    if grep -q "shellcode\[\]" "$file" || grep -q "shellcode.*=" "$file"; then
        # Extract the content between quotes after shellcode assignment
        shellcode_hex=$(sed -n '/shellcode.*=/,/;/ s/[^"]*"\([^"]*\)".*/\1/p' "$file" | grep -oE '\\x[0-9a-fA-F]{2}' | tr -d '\n' 2>/dev/null)
        
        if [ -z "$shellcode_hex" ]; then
            # Alternative: Extract from multi-line quoted string (files with continuation)
            shellcode_content=$(awk '
            /shellcode.*=/ { in_shellcode = 1; next }
            in_shellcode && /"/ && !start_found { 
                start_found = 1; 
                content = $0; 
                gsub(/^[^"]*"|"[^"]*$/, "", content);
                next 
            }
            in_shellcode && start_found {
                gsub(/^"|"$/, "", $0);
                content = content $0;
                if ($0 ~ /"/) {
                    in_shellcode = 0;
                    print content;
                    exit;
                }
            }
            ' "$file" 2>/dev/null)
            
            if [ -n "$shellcode_content" ]; then
                shellcode_hex=$(echo "$shellcode_content" | grep -oE '\\x[0-9a-fA-F]{2}' | tr -d '\n' 2>/dev/null)
            fi
        fi
        
    # Method 2: Handle files with raw hex format at the end 
    elif tail -10 "$file" | grep -q '\\x'; then
        # Get the last line that contains hex pattern
        shellcode_hex=$(tail -10 "$file" | grep -oE '\\x[0-9a-fA-F]{2}' | tr -d '\n' 2>/dev/null)
    else
        # Method 3: General extraction for any \x00 containing hex in the file
        shellcode_hex=$(grep -oE '\\x[0-9a-fA-F]{2}' "$file" | head -50 | tr -d '\n' 2>/dev/null)
    fi
    
    # Process if shellcode hex was found
    if [ -n "$shellcode_hex" ] && [ ${#shellcode_hex} -gt 10 ]; then
        # Remove \x prefix
        hex_string=$(echo "$shellcode_hex" | sed 's/\\x//g' 2>/dev/null)
        
        # Ensure even length for valid hex
        if [ $((${#hex_string} % 2)) -ne 0 ]; then
            hex_string=${hex_string:0:$((${#hex_string}-1))}
        fi
        
        # Convert to binary
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
                echo "Extracted: $output_path ($size bytes) with $null_count null byte(s) from $(basename "$file")"
                ((processed++))
            else
                rm "$output_path"  # Remove if no actual nulls
            fi
        else
            rm "$output_path"  # Remove if empty
        fi
    fi
done

echo "Final count: $processed files with actual null bytes extracted"