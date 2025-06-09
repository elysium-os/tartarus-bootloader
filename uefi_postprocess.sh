#!/usr/bin/env sh

set -e

ALIGNMENT=4096

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <input_binary> <output_binary>"
    exit 1
fi

INPUT="$1"
OUTPUT="$2"

if [ ! -f "$INPUT" ]; then
    echo "Error: Input file '$INPUT' does not exist or is not a regular file."
    exit 1
fi

SIZE=$(stat -c%s "$INPUT")
REMAINDER=$((SIZE % ALIGNMENT))
if [ "$REMAINDER" -eq 0 ]; then
    PADDING=0
else
    PADDING=$((ALIGNMENT - REMAINDER))
fi

cp "$INPUT" "$OUTPUT"

if [ "$PADDING" -gt 0 ]; then
    dd if=/dev/zero bs=1 count="$PADDING" >> "$OUTPUT" 2>/dev/null
fi
