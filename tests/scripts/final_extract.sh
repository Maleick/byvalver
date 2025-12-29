#!/bin/bash

# Script to extract shellcodes with null bytes by looking for the pattern at the end of files

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
    
    # Look for the shellcode pattern at the end of the file, often after "here is the shellcode" or similar
    # Extract the last non-empty lines of the file that might contain shellcode
    shellcode_line=""
    
    # Method 1: Look for the actual shellcode line (usually at the end of the file)
    shellcode_line=$(tail -20 "$file" | grep -oE '\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2})+' | head -10 | tr -d '\n' 2>/dev/null)
    
    # Method 2: If no shellcode found, look more broadly in the entire file
    if [ -z "$shellcode_line" ]; then
        # Look for patterns that usually contain shellcode in these files
        shellcode_line=$(grep -oE '\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2})+' "$file" | head -20 | tr -d '\n' 2>/dev/null)
    fi
    
    # Method 3: Look for quoted shellcode strings
    if [ -z "$shellcode_line" ]; then
        shellcode_line=$(grep -oE '"[^"]*\\\\x[0-9a-fA-F]{2}[^"]*"' "$file" | head -1 | sed 's/"//g' 2>/dev/null)
    fi
    
    if [ -n "$shellcode_line" ]; then
        # Remove the \x prefix to get hex string
        hex_string=$(echo "$shellcode_line" | sed 's/\\x//g' | sed 's/[^0-9a-fA-F]//g')
        
        # Only proceed if we have a reasonable length hex string
        if [ ${#hex_string} -gt 10 ]; then
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

# If we still need more files, try a different approach - look for files with hex bytes and "00" specifically
if [ $count -lt 50 ]; then
    echo "Still need $((50 - count)) more files, trying different approach..."
    
    # Look for files that contain both hex patterns and 00 sequences
    additional_files=$(find "$SHELLCODES_DIR" -name "*.txt" -exec grep -l "00" {} \; | grep -v -F -x -f <(printf '%s\n' "${files[@]}"))
    
    for file in $additional_files; do
        if [ $count -ge 50 ]; then
            break
        fi
        
        # Use the same extraction methods as above
        shellcode_line=""
        
        # Look for the shellcode pattern at the end of the file
        shellcode_line=$(tail -20 "$file" | grep -oE '\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2})+' | head -10 | tr -d '\n' 2>/dev/null)
        
        if [ -z "$shellcode_line" ]; then
            shellcode_line=$(grep -oE '\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2})+' "$file" | head -20 | tr -d '\n' 2>/dev/null)
        fi
        
        if [ -n "$shellcode_line" ]; then
            hex_string=$(echo "$shellcode_line" | sed 's/\\x//g' | sed 's/[^0-9a-fA-F]//g')
            
            if [ ${#hex_string} -gt 10 ]; then
                output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((count+1))).bin"
                
                echo "$hex_string" | xxd -r -p > "$output_path" 2>/dev/null
                
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