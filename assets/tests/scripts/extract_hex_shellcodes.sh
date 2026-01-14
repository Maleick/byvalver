#!/bin/bash

# Script to extract shellcode from files that contain hex strings in format: "\x##\x##..."

SHELLCODES_DIR="/home/mrnob0dy666/byvalver_PUBLIC/shellcodes"
OUTPUT_DIR="/home/mrnob0dy666/byvalver_PUBLIC/5050"

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

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
    hex_content=$(grep -oE '"\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2})+"' "$file" | tr -d '\n"')
    
    if [ -n "$hex_content" ]; then
        # Remove the \x prefix from each pair
        hex_content=$(echo "$hex_content" | sed 's/\\x//g')
        echo "$hex_content" | xxd -r -p > "$output_file" 2>/dev/null
        return $?
    else
        # Alternative: look for the pattern without the quote characters
        hex_content=$(grep -oE '\\\\x[0-9a-fA-F]{2}(\\\\x[0-9a-fA-F]{2})+' "$file" | tr -d '\n')
        if [ -n "$hex_content" ]; then
            hex_content=$(echo "$hex_content" | sed 's/\\x//g')
            echo "$hex_content" | xxd -r -p > "$output_file" 2>/dev/null
            return $?
        fi
    fi
    
    return 1
}

# Get a list of all txt files that might contain shellcode
all_files=()
while IFS= read -r -d '' file; do
    all_files+=("$file")
done < <(find "$SHELLCODES_DIR" -name "*.txt" -type f -print0)

# Sort files by modification time to get the most recent ones first, then take top 50
sorted_files=()
while IFS= read -r line; do
    sorted_files+=("$line")
done < <(for file in "${all_files[@]}"; do
    stat -c '%Y %n' "$file"
done | sort -nr | cut -d' ' -f2- | head -50)

# Extract shellcode from each file
for i in "${!sorted_files[@]}"; do
    input_path="${sorted_files[i]}"
    output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((i+1))).bin"
    
    echo "Processing: $(basename "$input_path")"
    
    if extract_shellcode_from_format "$input_path" "$output_path"; then
        if [ -s "$output_path" ]; then
            echo "  -> Extracted: $output_path ($(stat -c%s "$output_path") bytes)"
        else
            echo "  -> Failed to create binary (empty file)"
            # Create an empty file anyway
            touch "$output_path"
        fi
    else
        # Try alternative extraction methods
        # Method 1: Look for any hex string patterns in the file
        hex_content=$(grep -oE '[0-9a-fA-F]{2}' "$input_path" | tr -d '\n')
        if [ -n "$hex_content" ] && [ ${#hex_content} -gt 10 ]; then  # Only if we have significant content
            echo "$hex_content" | xxd -r -p > "$output_path" 2>/dev/null
            if [ -s "$output_path" ]; then
                echo "  -> Extracted using alternative method: $output_path ($(stat -c%s "$output_path") bytes)"
            else
                touch "$output_path"
                echo "  -> No shellcode extracted using alternative method"
            fi
        else
            echo "  -> No shellcode found in file"
            touch "$output_path"  # Create an empty file to maintain count
        fi
    fi
done

echo "Extraction complete. Files saved to: $OUTPUT_DIR"