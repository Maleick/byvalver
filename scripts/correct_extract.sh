#!/bin/bash

# Final script to extract 50 shellcode files with null bytes

SHELLCODES_DIR="/home/mrnob0dy666/byvalver_PUBLIC/shellcodes"
OUTPUT_DIR="/home/mrnob0dy666/byvalver_PUBLIC/5050"

# Clear and create output directory
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# Find all files that contain hex patterns and extract shellcode
all_files=($(find "$SHELLCODES_DIR" -name "*.txt" -exec grep -l "\\x" {} \;))

count=0

for file in "${all_files[@]}"; do
    if [ $count -ge 50 ]; then
        break
    fi
    
    # Extract potential shellcode hex patterns from the file
    hex_string=$(grep -oE '\\x[0-9a-fA-F]{2}' "$file" | tr -d '\n' | sed 's/\\x//g' 2>/dev/null)
    
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