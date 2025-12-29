#!/bin/bash

# Script to find and extract shellcodes that contain null bytes

SHELLCODES_DIR="/home/mrnob0dy666/byvalver_PUBLIC/shellcodes"
OUTPUT_DIR="/home/mrnob0dy666/byvalver_PUBLIC/5050"

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Function to check if a file contains null bytes after extracting shellcode
has_null_bytes() {
    local file="$1"
    python3 -c "
import sys
with open('$1', 'rb') as f:
    data = f.read()
    print(0 in [b for b in data])
" 2>/dev/null
}

# Function to extract shellcode from files that contain it in the format "\x##\x##..."
extract_shellcode_from_format() {
    local file="$1"
    local output_file="$2"
    
    # Extract shellcode from format like:
    # shellcode  = "\x55\x89\xe5\x83\xec\x60\x31\xc0\x66\xb8\x73\x73\x50\x68\x64\x64\x72\x65\x68\x72\x6f\x63\x41\x68\x47\x65"
    # shellcode += "\x74\x50\x89\x65\xfc\x31\xc0\x64\x8b\x40\x30\x8b\x40\x0c\x8b\x40\x1c\x89\xc3\x8b\x03\x89\xc3\x8b\x03\x8b"
    
    # Extract hex values that match the pattern and convert to binary
    local hex_content=""
    # Look for patterns that contain multiple hex bytes
    hex_content=$(grep -oE '"\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2})+"' "$file" | tr -d '\n"' 2>/dev/null)
    
    if [ -n "$hex_content" ]; then
        # Remove the \x prefix from each pair
        hex_content=$(echo "$hex_content" | sed 's/\\x//g')
        echo "$hex_content" | xxd -r -p > "$output_file" 2>/dev/null
        return $?
    else
        # Alternative: look for the pattern without the quote characters
        hex_content=$(grep -oE '\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2})+' "$file" | tr -d '\n' 2>/dev/null)
        if [ -n "$hex_content" ]; then
            hex_content=$(echo "$hex_content" | sed 's/\\x//g')
            echo "$hex_content" | xxd -r -p > "$output_file" 2>/dev/null
            return $?
        fi
    fi
    
    return 1
}

# Look for all txt files that might contain shellcode
all_files=()
while IFS= read -r -d '' file; do
    all_files+=("$file")
done < <(find "$SHELLCODES_DIR" -name "*.txt" -type f -print0)

# Process each file and extract those that result in binaries with null bytes
processed=0
for file in "${all_files[@]}"; do
    if [ $processed -ge 50 ]; then
        break
    fi
    
    output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((processed+1))).bin"
    
    if extract_shellcode_from_format "$file" "$output_path"; then
        if [ -s "$output_path" ]; then
            # Check if this binary contains null bytes
            if has_null_bytes "$output_path"; then
                size=$(stat -c%s "$output_path")
                echo "  -> Extracted with nulls: $output_path ($size bytes)"
                ((processed++))
            else
                # If no nulls, remove the file and continue
                rm "$output_path"
            fi
        else
            rm "$output_path"  # Remove empty files
        fi
    fi
done

# If we didn't reach 50, try alternative methods to find shellcodes with nulls
if [ $processed -lt 50 ]; then
    remaining=$((50 - processed))
    echo "Need $remaining more files. Searching for alternative formats..."
    
    # Find C files that contain shellcode
    c_files=()
    while IFS= read -r -d '' file; do
        c_files+=("$file")
    done < <(find "$SHELLCODES_DIR" -name "*.c" -type f -print0)
    
    for file in "${c_files[@]}"; do
        if [ $processed -ge 50 ]; then
            break
        fi
        
        # Look for shellcode format in C files
        hex_content=$(grep -oE '"[^"]*\\\\x00[^"]*"' "$file" | head -1 | sed 's/"//g' 2>/dev/null)
        
        if [ -n "$hex_content" ]; then
            output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((processed+1))).bin"
            echo "$hex_content" | sed 's/\\x//g' | xxd -r -p > "$output_path" 2>/dev/null
            if [ -s "$output_path" ] && has_null_bytes "$output_path"; then
                size=$(stat -c%s "$output_path")
                echo "  -> Extracted from C file with nulls: $output_path ($size bytes)"
                ((processed++))
            else
                rm "$output_path"  # Remove if no nulls
            fi
        fi
    done
fi

# If we still don't have 50, fill with whatever we can find that has nulls
if [ $processed -lt 50 ]; then
    remaining=$((50 - processed))
    echo "Filling remaining $remaining slots with any shellcodes containing nulls..."
    
    # Try to find any shellcode with null bytes using different patterns
    for file in "${all_files[@]}"; do
        if [ $processed -ge 50 ]; then
            break
        fi
        
        # Try to extract hex bytes using a more general approach
        hex_content=$(grep -oE '[0-9a-fA-F]{2}' "$file" | tr -d '\n' | head -c 2000 2>/dev/null)
        
        if [ -n "$hex_content" ] && [ ${#hex_content} -gt 20 ]; then
            output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((processed+1))).bin"
            echo "$hex_content" | xxd -r -p > "$output_path" 2>/dev/null
            if [ -s "$output_path" ] && has_null_bytes "$output_path"; then
                size=$(stat -c%s "$output_path")
                echo "  -> Extracted general content with nulls: $output_path ($size bytes)"
                ((processed++))
            else
                rm "$output_path"  # Remove if no nulls or empty
            fi
        fi
    done
fi

echo "Extraction complete. $processed files with null bytes saved to: $OUTPUT_DIR"