#!/bin/bash

# Verification script to ensure all 50 files contain actual null bytes

OUTPUT_DIR="/home/mrnob0dy666/byvalver_PUBLIC/5050"

# Count total shellcode files
total_files=$(ls "$OUTPUT_DIR"/shellcode_*.bin 2>/dev/null | wc -l)

echo "Total shellcode files found: $total_files"

if [ $total_files -ne 50 ]; then
    echo "Error: Expected 50 files, found $total_files"
    exit 1
fi

# Check each file for actual null bytes
valid_files=0
for file in "$OUTPUT_DIR"/shellcode_*.bin; do
    if [ -f "$file" ]; then
        null_count=$(xxd -p "$file" | grep -o '00' | wc -l)
        size=$(stat -c%s "$file")
        filename=$(basename "$file")
        if [ $null_count -gt 0 ]; then
            echo "$filename: $size bytes, contains $null_count null byte(s) - VALID"
            ((valid_files++))
        else
            echo "$filename: $size bytes, contains 0 null bytes - INVALID"
        fi
    fi
done

echo "Valid files with null bytes: $valid_files out of $total_files"

if [ $valid_files -eq 50 ]; then
    echo "SUCCESS: All 50 files contain actual null bytes"
else
    echo "FAILURE: Only $valid_files out of 50 files contain null bytes"
fi