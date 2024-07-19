#!/bin/bash

FILE="comp8567.txt"

# Check if file exists
if [ -e "$FILE" ]; then
  # Check if you are the owner of the file
  if [ "$(stat -c %U "$FILE")" = "$USER" ]; then
    # Check if the file has write permission
    if [ -w "$FILE" ]; then
      # Append text to the file
      echo "Welcome to COMP 8567, Summer 2024" >> "$FILE"
    else
      echo "The file exists but does not have write permission. Setting write permission now."
      chmod u+w "$FILE"
    fi
  else
    echo "You are not the owner of the file."
    exit 1
  fi
else
  echo "The file does not exist."
  exit 1
fi
