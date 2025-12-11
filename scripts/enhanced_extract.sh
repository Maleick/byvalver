#!/bin/bash

# Enhanced script to extract shellcode from various formats and convert to binary files

SHELLCODES_DIR="/home/mrnob0dy666/byvalver_PUBLIC/shellcodes"
OUTPUT_DIR="/home/mrnob0dy666/byvalver_PUBLIC/5050"

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Function to convert hex string to binary
hex_to_bin() {
    local hex_string="$1"
    local output_file="$2"
    
    # Remove any whitespace and common prefixes
    hex_string=$(echo "$hex_string" | tr -d '\n' | sed 's/0x//gI' | sed 's/\\x//gI' | sed 's/\\//g')
    
    # Remove common separators
    hex_string=$(echo "$hex_string" | tr -d ' \t\r\n,;')
    
    # Convert hex to binary
    echo "$hex_string" | xxd -r -p > "$output_file" 2>/dev/null || echo -n "" > "$output_file"
}

# Function to extract shellcode from a file and convert to binary
extract_shellcode() {
    local input_file="$1"
    local output_file="$2"
    local extracted=false
    
    # Method 1: Look for shellcode in C-style format: "\x##\x##..."
    if grep -oE '"[^"]*\\\\x[0-9a-fA-F]{2}[^"]*"' "$input_file" | head -1 > /dev/null; then
        local shellcode_line=$(grep -oE '"[^"]*\\\\x[0-9a-fA-F]{2}[^"]*"' "$input_file" | head -1)
        echo "$shellcode_line" | sed 's/"//g' | xxd -r -p > "$output_file" 2>/dev/null
        if [ -s "$output_file" ]; then
            extracted=true
        fi
    fi
    
    # Method 2: Look for shellcode in format \x##\x##...
    if [ "$extracted" = false ]; then
        if grep -oE '\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2})+' "$input_file" | head -1 > /dev/null; then
            local shellcode_line=$(grep -oE '\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2})+' "$input_file" | head -1)
            echo "$shellcode_line" | sed 's/\\x//g' | xxd -r -p > "$output_file" 2>/dev/null
            if [ -s "$output_file" ]; then
                extracted=true
            fi
        fi
    fi
    
    # Method 3: Look for raw hex bytes separated by spaces
    if [ "$extracted" = false ]; then
        # Try to find sequences of hex bytes in objdump format or similar
        if grep -oE '[0-9a-fA-F]{2} [0-9a-fA-F]{2} [0-9a-fA-F]{2}' "$input_file" > /dev/null; then
            # Extract hex bytes from lines that look like objdump output
            grep -E '[0-9a-fA-F]{2} [0-9a-fA-F]{2,}' "$input_file" | \
            sed 's/^[0-9a-fA-F]*[ \t]*//' | \
            sed 's/[ \t][ \t]*[0-9a-fA-F][0-9a-fA-F]:.*$//' | \
            tr -d ' \t\n\r' | \
            xxd -r -p > "$output_file" 2>/dev/null
            if [ -s "$output_file" ]; then
                extracted=true
            fi
        fi
    fi
    
    # Method 4: Look for shellcode in a more general string format
    if [ "$extracted" = false ]; then
        # Look for hex sequences in quotes that may not have \x format
        if grep -oE '"[0-9a-fA-F]{2,}"' "$input_file" | head -1 > /dev/null; then
            local shellcode_line=$(grep -oE '"[0-9a-fA-F]{2,}"' "$input_file" | head -1)
            echo "$shellcode_line" | sed 's/"//g' | xxd -r -p > "$output_file" 2>/dev/null
            if [ -s "$output_file" ]; then
                extracted=true
            fi
        fi
    fi
    
    # If no extraction succeeded, create an empty file
    if [ "$extracted" = false ]; then
        touch "$output_file"
    fi
}

# Enhanced function to get more shellcode files by looking for other candidates
find_more_shellcodes() {
    # Find potential shellcode files by looking for files with assembly, hex, or specific patterns
    find "$SHELLCODES_DIR" -type f -name "*.txt" -o -name "*.c" -o -name "*.cpp" -o -name "*.asm" -o -name "*.s" | \
    head -50
}

# Get a comprehensive list of potential shellcode files
shellcode_files=()
while IFS= read -r -d '' file; do
    shellcode_files+=("$file")
done < <(find_more_shellcodes | tr '\n' '\0')

# Limit to 50 files if we have more
if [ ${#shellcode_files[@]} -gt 50 ]; then
    # Truncate array
    shellcode_files=("${shellcode_files[@]:0:50}")
fi

# Extract each shellcode
for i in "${!shellcode_files[@]}"; do
    input_path="${shellcode_files[i]}"
    output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((i+1))).bin"
    
    if [ -f "$input_path" ]; then
        echo "Extracting shellcode from: $(basename "$input_path")"
        extract_shellcode "$input_path" "$output_path"
        if [ -s "$output_path" ]; then
            echo "  -> Saved to: $output_path ($(stat -c%s "$output_path") bytes)"
        else
            echo "  -> Failed to extract shellcode from: $(basename "$input_path")"
            # Create an empty file to maintain count
            touch "$output_path"
        fi
    else
        echo "File not found: $input_path"
        # Create an empty file to maintain count
        touch "$output_path"
    fi
done

echo "Extraction complete. Files saved to: $OUTPUT_DIR"