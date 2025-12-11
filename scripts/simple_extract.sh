#!/bin/bash

# Simple script to extract shellcodes with null bytes from files that contain them

SHELLCODES_DIR="/home/mrnob0dy666/byvalver_PUBLIC/shellcodes"
OUTPUT_DIR="/home/mrnob0dy666/byvalver_PUBLIC/5050"

# Clear and create output directory
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# Array of files known to contain shellcode with null bytes based on grep search
files_with_shellcode=(
    "/home/mrnob0dy666/byvalver_PUBLIC/shellcodes/windows/13560.txt"
    "/home/mrnob0dy666/byvalver_PUBLIC/shellcodes/bsd/13242.txt"
    "/home/mrnob0dy666/byvalver_PUBLIC/shellcodes/windows_x86-64/48229.txt"
    "/home/mrnob0dy666/byvalver_PUBLIC/shellcodes/linux_x86/41635.txt"
    "/home/mrnob0dy666/byvalver_PUBLIC/shellcodes/linux_x86/51189.txt"
)

# Find additional files that may contain shellcode with null bytes
additional_files=$(find "$SHELLCODES_DIR" -name "*.txt" -exec grep -l '\\x00' {} \; 2>/dev/null | head -45)

for file in $additional_files; do
    files_with_shellcode+=("$file")
done

# Extract shellcode from each file
count=0
for file in "${files_with_shellcode[@]}"; do
    if [ $count -ge 50 ]; then
        break
    fi
    
    # Look for shellcode pattern like \x64\xA1\x30\x00\x00\x00...
    shellcode_line=$(grep -oE '\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2})*' "$file" | head -10 | tr -d '\n' 2>/dev/null)
    
    if [ -n "$shellcode_line" ]; then
        # If the line doesn't have the \x format, try to find the actual shellcode section
        if [ -z "$(echo "$shellcode_line" | grep '\\x')" ]; then
            # Look for content after common identifiers like "shellcode" or "here is the shellcode"
            shellcode_section=$(sed -n '/here is the shellcode/,$p' "$file" | grep -oE '\\\\x[0-9a-fA-F]{2}' | tr -d '\n' 2>/dev/null)
            if [ -n "$shellcode_section" ]; then
                shellcode_line="$shellcode_section"
            fi
        fi
        
        if [ -n "$shellcode_line" ]; then
            # Remove the \x prefix to get hex string
            hex_string=$(echo "$shellcode_line" | sed 's/\\x//g' | sed 's/[^0-9a-fA-F]//g')
            
            # Only proceed if the hex string contains 00 (null bytes)
            if [[ "$hex_string" == *"00"* ]]; then
                output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((count+1))).bin"
                
                # Convert hex string to binary
                echo "$hex_string" | xxd -r -p > "$output_path" 2>/dev/null
                
                if [ -s "$output_path" ]; then
                    size=$(stat -c%s "$output_path")
                    echo "Extracted: $output_path ($size bytes) from $(basename "$file")"
                    ((count++))
                fi
            fi
        fi
    fi
done

# If we need more files, find any text files that have hex patterns with nulls
if [ $count -lt 50 ]; then
    remaining=$((50 - count))
    echo "Need $remaining more files, searching more broadly..."
    
    all_txt_files=$(find "$SHELLCODES_DIR" -name "*.txt" -type f 2>/dev/null | head -100)
    
    for file in $all_txt_files; do
        if [ $count -ge 50 ]; then
            break
        fi
        
        # Look for any hex patterns that might include nulls
        # Look for lines that could contain shellcode (with multiple hex values)
        potential_shellcode=$(grep -i -oE '([0-9a-fA-F]{2}[ ,]?\s*){5,}' "$file" | head -1 | sed 's/[^0-9a-fA-F]//g' 2>/dev/null)
        
        if [ -n "$potential_shellcode" ] && [[ "$potential_shellcode" == *"00"* ]]; then
            output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((count+1))).bin"
            
            # Make sure the hex string is valid and even length
            if [ $((${#potential_shellcode} % 2)) -eq 0 ]; then
                echo "$potential_shellcode" | xxd -r -p > "$output_path" 2>/dev/null
                
                if [ -s "$output_path" ]; then
                    size=$(stat -c%s "$output_path")
                    echo "Extracted: $output_path ($size bytes) from $(basename "$file")"
                    ((count++))
                fi
            fi
        fi
    done
fi

echo "Total extracted files with null bytes: $count"