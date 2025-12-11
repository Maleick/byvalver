#!/bin/bash

# Simple direct approach to extract shellcodes with nulls
SHELLCODES_DIR="/home/mrnob0dy666/byvalver_PUBLIC/shellcodes"
OUTPUT_DIR="/home/mrnob0dy666/byvalver_PUBLIC/5050"

# Clear and create output directory
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

count=0

# List of known files that contain shellcode with nulls
files_with_nulls=(
    "windows/13560.txt"
    "bsd/13242.txt" 
    "windows_x86-64/48229.txt"
    "linux_x86/51189.txt"
    "linux_x86/41635.txt"
    "linux_x86/13333.txt"   # Let's check this one too
    "linux_x86/13334.txt"
    "windows_x86/35793.txt"
    "windows_x86/47980.txt" 
    "windows_x86/13507.txt"
    "windows_x86/13524.txt"
    "macos/51177.txt"
    "macos/51178.txt"
    "windows/13581.txt"
    "windows/13582.txt"
    "arm/46736.txt"
    "osx/38065.txt"
    "system_z/38075.txt"
    "linux/51191.txt"
    "windows_x86-64/35794.txt"
    "windows_x86-64/48252.txt"
    "generator/46746.txt"
    "generator/46789.txt"
    "generator/46800.txt"
    "freebsd_x86-64/43502.txt"
    "freebsd_x86-64/43503.txt"
    "freebsd_x86/13262.txt"
    "freebsd_x86/13263.txt"
    "freebsd_x86/13264.txt"
    "linux_x86/13416.txt"
    "linux_x86/13577.txt"
    "linux_x86/13661.txt"
    "linux_x86/37285.txt"
    "linux_x86/37289.txt"
    "linux_x86/37297.txt"
    "linux_x86/40026.txt"
    "linux_x86/41757.txt"
    "linux_x86/43758.txt"
    "linux_x86/45538.txt"
    "linux_x86/46704.txt"
    "linux_x86/46801.txt"
    "linux_x86/46994.txt"
    "linux_x86/47108.txt"
    "linux_x86/47530.txt"
    "linux_x86/48243.txt"
    "linux_x86-64/38150.txt"
    "linux_x86-64/41883.txt"
    "linux_x86-64/47784.txt"
    "linux_mips/27132.txt"
    "ios/13290.txt"
)

# Process each file
for file_path in "${files_with_nulls[@]}"; do
    if [ $count -ge 50 ]; then
        break
    fi
    
    full_path="$SHELLCODES_DIR/$file_path"
    
    if [ ! -f "$full_path" ]; then
        continue
    fi
    
    # Extract shellcode from the file (look for \x patterns)
    # Get the content that looks like shellcode (after "shellcode" marker or at end of file)
    shellcode_content=$(sed -n '/[Ss]hellcode:/,$p' "$full_path" | grep -oE '\\\\x[0-9a-fA-F]{2}' | tr -d '\n' | sed 's/\\x//g' 2>/dev/null)
    
    # If the above doesn't work, try to find it at the end of the file
    if [ -z "$shellcode_content" ]; then
        shellcode_content=$(tail -20 "$full_path" | grep -oE '\\\\x[0-9a-fA-F]{2}' | tr -d '\n' | sed 's/\\x//g' 2>/dev/null)
    fi
    
    # If still not found, look for any pattern with hex that contains 00
    if [ -z "$shellcode_content" ]; then
        # Look for content after common shellcode indicators
        content=$(sed -n '/\\\\x/p' "$full_path")
        if [ -n "$content" ]; then
            shellcode_content=$(echo "$content" | grep -oE '\\\\x[0-9a-fA-F]{2}' | head -20 | tr -d '\n' | sed 's/\\x//g' 2>/dev/null)
        fi
    fi
    
    if [ -n "$shellcode_content" ]; then
        output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((count+1))).bin"
        
        # Make sure the hex string length is even
        if [ $((${#shellcode_content} % 2)) -ne 0 ]; then
            # Truncate to even length
            shellcode_content=${shellcode_content:0:$((${#shellcode_content}-1))}
        fi
        
        if [ ${#shellcode_content} -gt 10 ]; then
            echo "$shellcode_content" | xxd -r -p > "$output_path" 2>/dev/null
            
            if [ -s "$output_path" ] && xxd -p "$output_path" | grep -q '00'; then
                size=$(stat -c%s "$output_path")
                echo "Extracted: $output_path ($size bytes) with nulls from $file_path"
                ((count++))
            else
                rm -f "$output_path"  # Remove if no nulls found
            fi
        fi
    fi
done

echo "Final count: $count files extracted"