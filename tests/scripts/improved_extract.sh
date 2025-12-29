#!/bin/bash

# Improved script to extract shellcodes with null bytes

SHELLCODES_DIR="/home/mrnob0dy666/byvalver_PUBLIC/shellcodes"
OUTPUT_DIR="/home/mrnob0dy666/byvalver_PUBLIC/5050"

# Clear and create output directory
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# Get all files with \x patterns (potential shellcode)
files=($(find "$SHELLCODES_DIR" -name "*.txt" -exec grep -l "\\\\x" {} \;))

count=0

for file in "${files[@]}"; do
    if [ $count -ge 50 ]; then
        break
    fi
    
    # Look specifically for shellcode after common markers
    shellcode_line=""
    
    # Method 1: Look for lines that start with shellcode or contain shellcode pattern
    shellcode_line=$(grep -E 'shellcode.*=.*"\\\\x[0-9a-fA-F]{2}' "$file" | head -1 | grep -oE '"\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2})+"' | sed 's/"//g' 2>/dev/null)
    
    # Method 2: If not found, look for lines with \x patterns that are likely shellcode
    if [ -z "$shellcode_line" ]; then
        # Look for lines that have many \x patterns (likely shellcode)
        shellcode_line=$(grep -oE '\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2}){5,}' "$file" | head -1 2>/dev/null)
    fi
    
    # Method 3: If still not found, look for text after markers like "shellcode:"
    if [ -z "$shellcode_line" ]; then
        # Extract everything after common shellcode markers
        content_after_marker=$(sed -n '/[Ss]hellcode.*:/,$p' "$file")
        shellcode_line=$(echo "$content_after_marker" | grep -oE '\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2}){2,}' | head -10 | tr -d '\n' 2>/dev/null)
    fi
    
    # Method 4: Look for content after "here is the shellcode" patterns
    if [ -z "$shellcode_line" ]; then
        content_after_marker=$(sed -n '/here is the shellcode/,$p' "$file")
        shellcode_line=$(echo "$content_after_marker" | grep -oE '\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2}){2,}' | head -10 | tr -d '\n' 2>/dev/null)
    fi
    
    if [ -n "$shellcode_line" ]; then
        # Remove the \x prefix to get hex string
        hex_string=$(echo "$shellcode_line" | sed 's/\\x//g' | sed 's/[^0-9a-fA-F]//g')
        
        # Only proceed if we have a reasonable length hex string and it contains 00 (null bytes)
        if [ ${#hex_string} -gt 10 ] && [[ "$hex_string" == *"00"* ]]; then
            output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((count+1))).bin"
            
            # Convert hex string to binary
            echo "$hex_string" | xxd -r -p > "$output_path" 2>/dev/null
            
            if [ -s "$output_path" ]; then
                # Verify this file actually contains null bytes by checking if xxd contains '00'
                if xxd -p "$output_path" | grep -q '00'; then
                    size=$(stat -c%s "$output_path")
                    echo "Extracted: $output_path ($size bytes) with nulls from $(basename "$file")"
                    ((count++))
                else
                    rm "$output_path"  # Remove if no actual nulls found
                fi
            fi
        fi
    fi
done

# If we still need more files, try even more broadly
if [ $count -lt 50 ]; then
    # Find all txt files (not just ones with \x) and search more broadly
    all_txt_files=($(find "$SHELLCODES_DIR" -name "*.txt" -type f))
    
    for file in "${all_txt_files[@]}"; do
        if [ $count -ge 50 ]; then
            break
        fi
        
        # Check if we've already processed this file
        already_processed=0
        for processed_file in "${files[@]}"; do
            if [ "$file" = "$processed_file" ]; then
                already_processed=1
                break
            fi
        done
        
        if [ $already_processed -eq 1 ]; then
            continue
        fi
        
        # Look for any hex patterns that might include nulls
        potential_hex=$(grep -oE '[0-9a-fA-F]{2}' "$file" | head -50 | tr -d '\n' 2>/dev/null)
        
        if [ -n "$potential_hex" ] && [ ${#potential_hex} -gt 20 ] && [[ "$potential_hex" == *"00"* ]]; then
            output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((count+1))).bin"
            
            # Make sure the hex string is valid and even length
            if [ $((${#potential_hex} % 2)) -eq 0 ]; then
                echo "$potential_hex" | xxd -r -p > "$output_path" 2>/dev/null
                
                if [ -s "$output_path" ] && xxd -p "$output_path" | grep -q '00'; then
                    size=$(stat -c%s "$output_path")
                    echo "Extracted: $output_path ($size bytes) with nulls from $(basename "$file")"
                    ((count++))
                fi
            fi
        fi
    done
fi

echo "Total extracted files with null bytes: $count"