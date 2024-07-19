#!/bin/bash

DIRECTORY=$1
EXTENSION=$2

# Check if directory is provided
if [ -z "$DIRECTORY" ]; then
  echo "Usage: $0 <directory> [extension]"
  exit 1
fi

# Check if extension is provided
if [ -z "$EXTENSION" ]; then
  # Count all files in the directory
  FILE_COUNT=$(find "$DIRECTORY" -type f | wc -l)
else
  # Count files with the given extension in the directory
  FILE_COUNT=$(find "$DIRECTORY" -type f -name "*$EXTENSION" | wc -l)
fi

echo "Total number of files${EXTENSION:+ with extension $EXTENSION} in $DIRECTORY: $FILE_COUNT"
