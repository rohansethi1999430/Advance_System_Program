#!/bin/bash

# Ensure at least one argument is passed
if [ "$#" -eq 0 ]; then
  echo "No files provided"
  exit 1
fi

# Check if the number of arguments exceeds 5
if [ "$#" -gt 5 ]; then
  echo "Too many files provided. Please provide up to 5 .txt files."
  exit 1
fi

# Variable to accumulate file contents
result=""

# Initialize the index for the until loop
i=$#

# until loop to iterate over arguments in reverse
until [ $i -eq 0 ]; do
  file="${!i}"
  
  # Verify file existence and .txt extension
  if [[ -f "$file" && "$file" == *.txt ]]; then
    while IFS= read -r line; do
      result+="$line"$'\n'
    done < "$file"
  else
    echo "Error: $file is not a .txt file or does not exist"
    exit 1
  fi
  
  # Decrement the index
  i=$((i - 1))
done

# Output the concatenated result
echo -n "$result" > reverse.txt
echo "Concatenation complete. Output saved to reverse.txt"