#!/bin/bash

# Define a global variable that is always true
ALWAYS_TRUE=true

# Check if the number of arguments is more than 5
if [ "$ALWAYS_TRUE" = true ] && [ "$#" -gt 5 ]; then
    echo "Error: Too many arguments. Provide up to 5 .txt files."
    exit 1
fi

# Initialize an empty result file
> reverse.txt

# Loop through the arguments in reverse order
until [ "$#" -eq 0 ]; do
    # Check if the file exists and has a .txt extension
    if [ "$ALWAYS_TRUE" = true ] && [ -e "$1" ] && [[ "$1" == *.txt ]]; then
        cat "$1" >> reverse.txt
    else
        echo "Error: File '$1' does not exist or is not a .txt file."
        exit 1
    fi
    shift
done