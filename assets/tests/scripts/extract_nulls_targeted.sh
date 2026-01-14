#!/bin/bash

# Script to specifically extract shellcodes that contain null bytes (\x00)

SHELLCODES_DIR="/home/mrnob0dy666/byvalver_PUBLIC/shellcodes"
OUTPUT_DIR="/home/mrnob0dy666/byvalver_PUBLIC/5050"

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Function to extract shellcode containing null bytes from files
extract_shellcode_with_nulls() {
    local file="$1"
    local output_file="$2"
    
    # Method 1: Extract from quoted strings containing \x00
    local hex_content=$(grep -oE '"[^"]*\\\\x00[^"]*"' "$file" | head -1 | sed 's/"//g' 2>/dev/null)
    
    if [ -n "$hex_content" ]; then
        # Remove the \x prefix from each pair
        hex_content=$(echo "$hex_content" | sed 's/\\x//g')
        echo "$hex_content" | xxd -r -p > "$output_file" 2>/dev/null
        return $?
    fi
    
    # Method 2: Extract from any hex string containing 00
    hex_content=$(grep -oE '\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2})*\\\\x00(\\\\x[0-9a-fA-F]{2})*' "$file" | head -1 2>/dev/null)
    
    if [ -n "$hex_content" ]; then
        hex_content=$(echo "$hex_content" | sed 's/\\x//g')
        echo "$hex_content" | xxd -r -p > "$output_file" 2>/dev/null
        return $?
    fi
    
    # Method 3: General extraction if we find \x00 anywhere
    hex_content=$(grep -oE '\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2})+' "$file" | head -1 2>/dev/null)
    
    if [ -n "$hex_content" ]; then
        hex_content=$(echo "$hex_content" | sed 's/\\x//g')
        echo "$hex_content" | xxd -r -p > "$output_file" 2>/dev/null
        return $?
    fi
    
    return 1
}

# List files that contain \x00 explicitly (from our grep search)
null_files=(
    "/home/mrnob0dy666/byvalver_PUBLIC/shellcodes/bsd/13242.txt"
    "/home/mrnob0dy666/byvalver_PUBLIC/shellcodes/windows/13560.txt" 
    "/home/mrnob0dy666/byvalver_PUBLIC/shellcodes/windows_x86-64/48229.txt"
    "/home/mrnob0dy666/byvalver_PUBLIC/shellcodes/linux_x86/41635.txt"
    "/home/mrnob0dy666/byvalver_PUBLIC/shellcodes/linux_x86/51189.txt"
)

# Add other files from the shellcodes directory that might contain shellcode
other_files=()
while IFS= read -r -d '' file; do
    # Only add if not already in null_files
    found=0
    for nf in "${null_files[@]}"; do
        if [ "$file" = "$nf" ]; then
            found=1
            break
        fi
    done
    if [ $found -eq 0 ]; then
        other_files+=("$file")
    fi
done < <(find "$SHELLCODES_DIR" -name "*.txt" -type f -print0)

# Combine both arrays
all_files=("${null_files[@]}" "${other_files[@]}")

# Extract shellcode from each file until we have 50
processed=0
for file in "${all_files[@]}"; do
    if [ $processed -ge 50 ]; then
        break
    fi
    
    output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((processed+1))).bin"
    
    if extract_shellcode_with_nulls "$file" "$output_path"; then
        if [ -s "$output_path" ]; then
            # Verify the file actually contains null bytes
            if xxd -p "$output_path" | grep -q "00"; then
                size=$(stat -c%s "$output_path")
                echo "  -> Extracted: $output_path with null bytes ($size bytes)"
                ((processed++))
            else
                # If no nulls found after extraction, remove the file and continue
                rm "$output_path"
            fi
        else
            rm "$output_path"  # Remove empty files
        fi
    fi
done

# If we still need more files, use an alternative approach
if [ $processed -lt 50 ]; then
    echo "Still need $((50 - processed)) more files. Using alternative approach..."
    
    # Search for other potential shellcode files with nulls using a broader search
    remaining_files=()
    while IFS= read -r -d '' file; do
        remaining_files+=("$file")
    done < <(find "$SHELLCODES_DIR" -name "*.c" -o -name "*.cpp" -o -name "*.asm" -o -name "*.s" -print0)
    
    for file in "${remaining_files[@]}"; do
        if [ $processed -ge 50 ]; then
            break
        fi
        
        # Look for hex patterns in the file
        hex_content=$(grep -oE '\\\\x[0-9a-fA-F]{2}' "$file" | head -50 | tr -d '\n' | sed 's/\\x//g' 2>/dev/null)
        
        if [ -n "$hex_content" ] && [[ "$hex_content" == *"00"* ]]; then
            output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((processed+1))).bin"
            echo "$hex_content" | xxd -r -p > "$output_path" 2>/dev/null
            if [ -s "$output_path" ]; then
                size=$(stat -c%s "$output_path")
                echo "  -> Extracted from alternative file with nulls: $output_path ($size bytes)"
                ((processed++))
            else
                rm "$output_path"
            fi
        fi
    done
fi

echo "Final count: $processed files with null bytes saved to: $OUTPUT_DIR"