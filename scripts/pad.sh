#!/usr/bin/env sh

set -e

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <input_file> <output_file> <alignment>"
    exit 1
fi

INPUT="$1"
OUTPUT="$2"
ALIGNMENT=$3

case "$ALIGNMENT" in
    ''|*[!0-9]*|0)
        echo "Error: alignment must be a positive integer."
        exit 1
        ;;
esac

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
