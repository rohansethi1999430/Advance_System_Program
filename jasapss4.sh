#!/bin/bash

# Define the root directory to back up and the backup destinations
SOURCE_DIR="/home/chauha5a"
BACKUP_DIR="$HOME/backup"
FULL_BACKUP_DIR="$BACKUP_DIR/cbup24s"
INCREMENTAL_BACKUP_DIR="$BACKUP_DIR/ibup24s"
DIFFERENTIAL_BACKUP_DIR="$BACKUP_DIR/dbup24s"
LOG_FILE="$BACKUP_DIR/backup.log"

# Get file types from arguments
FILE_TYPES=("$@")

# Ensure the backup directories exist
mkdir -p "$FULL_BACKUP_DIR" "$INCREMENTAL_BACKUP_DIR" "$DIFFERENTIAL_BACKUP_DIR"

# Variables to store the last backup times and counters
LAST_BACKUP_TIME=""
LAST_FULL_BACKUP_TIME=""
FULL_BACKUP_COUNT=0
INCREMENTAL_BACKUP_COUNT=0
DIFFERENTIAL_BACKUP_COUNT=0

# Function to perform a full backup
perform_full_backup() {
    TIMESTAMP=$(date +"%a %d %b%Y %I:%M:%S %p %Z")
    FULL_BACKUP_COUNT=$((FULL_BACKUP_COUNT + 1))
    BACKUP_FILE="${FULL_BACKUP_DIR}/cbup24s-${FULL_BACKUP_COUNT}.tar"

    if [ ${#FILE_TYPES[@]} -eq 0 ]; then
        find "$SOURCE_DIR" -type f ! -path "$BACKUP_DIR/*" -print0 | tar --null -cf "$BACKUP_FILE" --files-from=-
    else
        find "$SOURCE_DIR" -type f \( $(printf -- "-name '%s' " "${FILE_TYPES[@]}") \) ! -path "$BACKUP_DIR/" -print0 | tar --null -cf "$BACKUP_FILE" --files-from=-
    fi

    echo "$TIMESTAMP cbup24s-${FULL_BACKUP_COUNT}.tar was created" >> "$LOG_FILE"
    LAST_FULL_BACKUP_TIME=$(date +"%Y-%m-%d %H:%M:%S")
    LAST_BACKUP_TIME="$LAST_FULL_BACKUP_TIME"
}

# Function to perform an incremental backup
perform_incremental_backup() {
    TIMESTAMP=$(date +"%a %d %b%Y %I:%M:%S %p %Z")
    INCREMENTAL_BACKUP_COUNT=$((INCREMENTAL_BACKUP_COUNT + 1))
    BACKUP_FILE="${INCREMENTAL_BACKUP_DIR}/ibup24s-${INCREMENTAL_BACKUP_COUNT}.tar"

    if [ ${#FILE_TYPES[@]} -eq 0 ]; then
        MODIFIED_FILES=$(find "$SOURCE_DIR" -type f -newermt "$LAST_BACKUP_TIME" ! -path "$BACKUP_DIR/*" -print)
    else
        MODIFIED_FILES=$(find "$SOURCE_DIR" -type f \( $(printf -- "-name '%s' " "${FILE_TYPES[@]}") \) -newermt "$LAST_BACKUP_TIME" ! -path "$BACKUP_DIR/" -print)
    fi

    if [ -n "$MODIFIED_FILES" ]; then
        echo "$MODIFIED_FILES" | tar -cf "$BACKUP_FILE" -T -
        echo "$TIMESTAMP ibup24s-${INCREMENTAL_BACKUP_COUNT}.tar was created" >> "$LOG_FILE"
        LAST_BACKUP_TIME=$(date +"%Y-%m-%d %H:%M:%S")
    else
        echo "$TIMESTAMP No changes-Incremental backup was not created" >> "$LOG_FILE"
    fi
}

# Function to perform a differential backup
perform_differential_backup() {
    TIMESTAMP=$(date +"%a %d %b%Y %I:%M:%S %p %Z")
    DIFFERENTIAL_BACKUP_COUNT=$((DIFFERENTIAL_BACKUP_COUNT + 1))
    BACKUP_FILE="${DIFFERENTIAL_BACKUP_DIR}/dbup24s-${DIFFERENTIAL_BACKUP_COUNT}.tar"

    if [ ${#FILE_TYPES[@]} -eq 0 ]; then
        MODIFIED_FILES=$(find "$SOURCE_DIR" -type f -newermt "$LAST_FULL_BACKUP_TIME" ! -path "$BACKUP_DIR/*" -print)
    else
        MODIFIED_FILES=$(find "$SOURCE_DIR" -type f \( $(printf -- "-name '%s' " "${FILE_TYPES[@]}") \) -newermt "$LAST_FULL_BACKUP_TIME" ! -path "$BACKUP_DIR/" -print)
    fi

    if [ -n "$MODIFIED_FILES" ]; then
        echo "$MODIFIED_FILES" | tar -cf "$BACKUP_FILE" -T -
        echo "$TIMESTAMP dbup24s-${DIFFERENTIAL_BACKUP_COUNT}.tar was created" >> "$LOG_FILE"
    else
        echo "$TIMESTAMP No changes-Differential backup was not created" >> "$LOG_FILE"
    fi
}

# Run the script continuously
while true; do
    # Perform the initial full backup
    perform_full_backup

    # Sleep for 2 minutes (120 seconds)
    sleep 20

    # Perform an incremental backup after the full backup
    perform_incremental_backup

    # Sleep for 2 minutes (120 seconds)
    sleep 20

    # Perform a differential backup
    perform_differential_backup

    # Sleep for 2 minutes (120 seconds)
    sleep 20

    # Perform another incremental backup after the differential backup
    perform_incremental_backup

    # Sleep for 2 minutes (120 seconds)
    sleep 20
done