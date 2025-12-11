#!/bin/bash

# Proper verification script to ensure all 50 files contain actual null bytes

OUTPUT_DIR="/home/mrnob0dy666/byvalver_PUBLIC/5050"

# Count total shellcode files
total_files=$(ls "$OUTPUT_DIR"/shellcode_*.bin 2>/dev/null | wc -l)

echo "Total shellcode files found: $total_files"

# Check each file for actual null bytes using Python for accuracy
valid_files=0
for file in "$OUTPUT_DIR"/shellcode_*.bin; do
    if [ -f "$file" ]; then
        filename=$(basename "$file")
        size=$(stat -c%s "$file")
        
        # Use Python to count actual null bytes
        null_count=$(python3 -c "
import sys
with open('$file', 'rb') as f:
    data = f.read()
    print(data.count(0))
" 2>/dev/null)
        
        if [ "$null_count" -gt 0 ]; then
            echo "$filename: $size bytes, contains $null_count actual null byte(s) - VALID"
            ((valid_files++))
        else
            echo "$filename: $size bytes, contains 0 actual null bytes - INVALID"
        fi
    fi
done

echo "Valid files with actual null bytes: $valid_files out of $total_files"

if [ $valid_files -eq 50 ]; then
    echo "SUCCESS: All 50 files contain actual null bytes"
else
    echo "FAILURE: Only $valid_files out of 50 files contain actual null bytes"
fi