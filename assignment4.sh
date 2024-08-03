#!/bin/bash

log_message() {
    echo "$1" >> $HOME/backup/backup.log
}

# Initialize counters for backup files
cbup_count=1
ibup_count=1
dbup_count=1

# Function to perform complete backup
perform_complete_backup() {
    FIND_USER_HOME="$HOME"
    FIND_TYPE="-type f"
    FIND_FILE_TYPES="\( $file_types \)"
    FIND_PRINT0="-print0"

    log_message "User Home: $FIND_USER_HOME"
    log_message "Type: $FIND_TYPE"
    log_message "File Types: $FIND_FILE_TYPES"
    log_message "Print0: $FIND_PRINT0"

    FIND_COMMAND="find $FIND_USER_HOME $FIND_TYPE $FIND_FILE_TYPES $FIND_PRINT0"
    log_message "Executing: $FIND_COMMAND"

    files=$(eval $FIND_COMMAND | xargs -0)
    if [ -n "$files" ]; then
        for file in $files; do
            log_message "Found file for complete backup: $file"
        done
        tar_file=$HOME/backup/cbup24s/cbup24s-$cbup_count.tar
        log_message "Creating tar file: $tar_file"
        tar -cvf $tar_file $files
        log_message "$(date '+%a %d %b%Y %T %p %Z') cbup24s-$cbup_count.tar was created"
        touch /tmp/last_full_backup
        cbup_count=$((cbup_count + 1))
    else
        log_message "$(date '+%a %d %b%Y %T %p %Z') No files found for complete backup"
    fi
}

# Function to perform incremental backup
perform_incremental_backup() {
    FIND_USER_HOME="$HOME"
    FIND_TYPE="-type f"
    FIND_FILE_TYPES="\( $file_types \)"
    FIND_NEWER="-newer /tmp/last_backup"
    FIND_PRINT0="-print0"

    log_message "User Home: $FIND_USER_HOME"
    log_message "Type: $FIND_TYPE"
    log_message "File Types: $FIND_FILE_TYPES"
    log_message "Newer Than: $FIND_NEWER"
    log_message "Print0: $FIND_PRINT0"

    FIND_COMMAND="find $FIND_USER_HOME $FIND_TYPE $FIND_FILE_TYPES $FIND_NEWER $FIND_PRINT0"
    log_message "Executing: $FIND_COMMAND"

    files=$(eval $FIND_COMMAND | xargs -0)
    if [ -n "$files" ]; then
        for file in $files; do
            log_message "Found file for incremental backup: $file"
        done
        tar_file=$HOME/backup/ibup24s/ibup24s-$ibup_count.tar
        log_message "Creating tar file: $tar_file"
        tar -cvf $tar_file $files
        log_message "$(date '+%a %d %b%Y %T %p %Z') ibup24s-$ibup_count.tar was created"
        touch /tmp/last_backup
        ibup_count=$((ibup_count + 1))
    else
        log_message "$(date '+%a %d %b%Y %T %p %Z') No changes - Incremental backup was not created"
    fi
}

# Function to perform differential backup
perform_differential_backup() {
    FIND_USER_HOME="$HOME"
    FIND_TYPE="-type f"
    FIND_FILE_TYPES="\( $file_types \)"
    FIND_NEWER="-newer /tmp/last_diff_backup"
    FIND_PRINT0="-print0"

    log_message "User Home: $FIND_USER_HOME"
    log_message "Type: $FIND_TYPE"
    log_message "File Types: $FIND_FILE_TYPES"
    log_message "Newer Than: $FIND_NEWER"
    log_message "Print0: $FIND_PRINT0"

    FIND_COMMAND="find $FIND_USER_HOME $FIND_TYPE $FIND_FILE_TYPES $FIND_NEWER $FIND_PRINT0"
    log_message "Executing: $FIND_COMMAND"

    files=$(eval $FIND_COMMAND | xargs -0)
    if [ -n "$files" ]; then
        for file in $files; do
            log_message "Found file for differential backup: $file"
        done
        tar_file=$HOME/backup/dbup24s/dbup24s-$dbup_count.tar
        log_message "Creating tar file: $tar_file"
        tar -cvf $tar_file $files
        log_message "$(date '+%a %d %b%Y %T %p %Z') dbup24s-$dbup_count.tar was created"
        touch /tmp/last_diff_backup
        dbup_count=$((dbup_count + 1))
    else
        log_message "$(date '+%a %d %b%Y %T %p %Z') No changes - Differential backup was not created"
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
log_message "Final constructed file_types parameter: $file_types"

# Main loop to perform backups
while true; do
    perform_complete_backup
    touch /tmp/last_backup
    touch /tmp/last_diff_backup
    sleep 20  # Sleep for 2 minutes

    perform_incremental_backup
    sleep 20  # Sleep for 2 minutes

    perform_incremental_backup
    sleep 20  # Sleep for 2 minutes

    perform_differential_backup
    sleep 20  # Sleep for 2 minutes

    perform_incremental_backup
    sleep 20  # Sleep for 2 minutes
done