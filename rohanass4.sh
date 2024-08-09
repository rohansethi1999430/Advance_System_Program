#!/bin/bash

log_message() {
    # Format the log message with a timestamp and append it to the log file
    echo "$(date '+%a %d %b%Y %T %p %Z') $1" >> $HOME/backup/backup.log
}

# Initialize counters for backup files
cbup_count=1
ibup_count=1
dbup_count=1

# Function to perform complete backup
perform_complete_backup() {
    FIND_USER_HOME="$HOME"
    FIND_TYPE="-type f"
    FIND_EXCLUDE="-not -path \"$HOME/.vscode-server/*\" -not -path \"$HOME/.vscode-server/data/User/History/*\" -not -path \"$HOME/.cache/*\" -not -path \"$HOME/.local/share/Trash/*\""
    FIND_PRINT0="-print0"

    if [ -n "$file_types" ]; then
        FIND_FILE_TYPES="\( $file_types \)"
    else
        FIND_FILE_TYPES=""
    fi

    FIND_COMMAND="find $FIND_USER_HOME $FIND_TYPE $FIND_FILE_TYPES $FIND_EXCLUDE $FIND_PRINT0"
    
    files=$(eval $FIND_COMMAND | xargs -0)
    
    num_files=$(echo "$files" | wc -w)
    log_message "Number of files found for complete backup: $num_files"

    if [ -n "$files" ]; then
        tar_file=$HOME/backup/cbup24s/cbup24s-$cbup_count.tar
        log_message "Creating tar file: $tar_file"
        tar -cvf $tar_file $files
        log_message "cbup24s-$cbup_count.tar was created"
        touch /tmp/last_full_backup
        cbup_count=$((cbup_count + 1))
    else
        log_message "No files found for complete backup"
    fi
}

# Function to perform incremental backup
perform_incremental_backup() {
    FIND_USER_HOME="$HOME"
    FIND_TYPE="-type f"
    FIND_NEWER="-newer /tmp/last_backup"
    FIND_EXCLUDE="-not -path \"$HOME/.vscode-server/*\" -not -path \"$HOME/.vscode-server/data/User/History/*\" -not -path \"$HOME/.cache/*\" -not -path \"$HOME/.local/share/Trash/*\""
    FIND_PRINT0="-print0"

    if [ -n "$file_types" ]; then
        FIND_FILE_TYPES="\( $file_types \)"
    else
        FIND_FILE_TYPES=""
    fi

    FIND_COMMAND="find $FIND_USER_HOME $FIND_TYPE $FIND_FILE_TYPES $FIND_NEWER $FIND_EXCLUDE $FIND_PRINT0"
    
    files=$(eval $FIND_COMMAND | xargs -0)
    
    num_files=$(echo "$files" | wc -w)
    log_message "Number of files found for incremental backup: $num_files"

    if [ -n "$files" ]; then
        tar_file=$HOME/backup/ibup24s/ibup24s-$ibup_count.tar
        log_message "Creating tar file: $tar_file"
        tar -cvf $tar_file $files
        log_message "ibup24s-$ibup_count.tar was created"
        touch /tmp/last_backup
        ibup_count=$((ibup_count + 1))
    else
        log_message "No changes - Incremental backup was not created"
    fi
}

# Function to perform differential backup
perform_differential_backup() {
    FIND_USER_HOME="$HOME"
    FIND_TYPE="-type f"
    FIND_NEWER="-newer /tmp/last_diff_backup"
    FIND_EXCLUDE="-not -path \"$HOME/.vscode-server/*\" -not -path \"$HOME/.vscode-server/data/User/History/*\" -not -path \"$HOME/.cache/*\" -not -path \"$HOME/.local/share/Trash/*\""
    FIND_PRINT0="-print0"

    if [ -n "$file_types" ]; then
        FIND_FILE_TYPES="\( $file_types \)"
    else
        FIND_FILE_TYPES=""
    fi

    FIND_COMMAND="find $FIND_USER_HOME $FIND_TYPE $FIND_FILE_TYPES $FIND_NEWER $FIND_EXCLUDE $FIND_PRINT0"
    
    files=$(eval $FIND_COMMAND | xargs -0)
    
    num_files=$(echo "$files" | wc -w)
    log_message "Number of files found for differential backup: $num_files"

    if [ -n "$files" ]; then
        tar_file=$HOME/backup/dbup24s/dbup24s-$dbup_count.tar
        log_message "Creating tar file: $tar_file"
        tar -cvf $tar_file $files
        log_message "dbup24s-$dbup_count.tar was created"
        touch /tmp/last_diff_backup
        dbup_count=$((dbup_count + 1))
    else
        log_message "No changes - Differential backup was not created"
    fi
}

# Check and create necessary directories
mkdir -p $HOME/backup/cbup24s
mkdir -p $HOME/backup/ibup24s
mkdir -p $HOME/backup/dbup24s

# Determine file types to backup
file_types=""
for arg in "$@"; do
    log_message "Adding file type parameter: -name '*$arg'"
    file_types="$file_types -o -name '*$arg'"
done

# Remove leading -o for find command
file_types="${file_types:3}"

# Main loop to perform backups
while true; do
    perform_complete_backup
    touch /tmp/last_backup
    touch /tmp/last_diff_backup
    sleep 20  # Sleep for 20 seconds

    perform_incremental_backup
    sleep 20  # Sleep for 20 seconds

    perform_incremental_backup
    sleep 20  # Sleep for 20 seconds

    perform_differential_backup
    sleep 20  # Sleep for 20 seconds

    perform_incremental_backup
    sleep 20  # Sleep for 20 seconds
done