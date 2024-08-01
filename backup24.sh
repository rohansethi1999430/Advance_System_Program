#!/bin/bash

# Get the user's home directory dynamically
USER_HOME=$(eval echo ~${USER})

# Directory paths
BACKUP_DIR="$USER_HOME/backup"
COMPLETE_BACKUP_DIR="$BACKUP_DIR/cbup24s"
INCREMENTAL_BACKUP_DIR="$BACKUP_DIR/ibup24s"
DIFFERENTIAL_BACKUP_DIR="$BACKUP_DIR/dbup24s"


# Create backup directories if they don't exist
mkdir -p "$COMPLETE_BACKUP_DIR" "$INCREMENTAL_BACKUP_DIR" "$DIFFERENTIAL_BACKUP_DIR"

# Ensure the log file exists
touch "$LOG_FILE"

# Function to log messages with timestamps
log_message() {
    echo "$(date '+%a %d %b %Y %I:%M:%S %p %Z') $1" >> "$LOG_FILE"
}

# Function to log error messages with timestamps
log_error() {
    echo "$(date '+%a %d %b %Y %I:%M:%S %p %Z') ERROR: $1" >> "$LOG_FILE"
}

# Function to create a complete backup
complete_backup() {
    TAR_FILE="$COMPLETE_BACKUP_DIR/cbup$(date '+%Y%m%d%H%M%S').tar"
    find "$USER_HOME" -type f \( $FILE_TYPES \) -print0 | tar --exclude="$BACKUP_DIR" --null --files-from=- -cvf "$TAR_FILE" &>/dev/null
    if [ $? -eq 0 ]; then
        log_message "$(basename $TAR_FILE) was created"
    else
        log_error "Error creating complete backup: $(find "$USER_HOME" -type f \( $FILE_TYPES \) -print0 | tar --exclude="$BACKUP_DIR" --null --files-from=- -cvf "$TAR_FILE" 2>&1)"
    fi
}

# Function to create an incremental backup
incremental_backup() {
    TAR_FILE="$INCREMENTAL_BACKUP_DIR/ibup$(date '+%Y%m%d%H%M%S').tar"
    TMP_FILE="/tmp/incremental_files.txt"
    
    # Constructing and logging the find command step by step
    log_message "Starting incremental backup..."
    log_message "Find Command Components:"
    
    FIND_USER_HOME="$USER_HOME"
    FIND_TYPE="-type f"
    FIND_FILE_TYPES="\( $FILE_TYPES \)"
    FIND_NEWER="-newer $LAST_BACKUP"
    FIND_PRINT0="-print0"
    
    log_message "User Home: $FIND_USER_HOME"
    log_message "Type: $FIND_TYPE"
    log_message "File Types: $FIND_FILE_TYPES"
    log_message "Newer Than: $FIND_NEWER"
    log_message "Print0: $FIND_PRINT0"
    
    FIND_COMMAND="find $FIND_USER_HOME $FIND_TYPE $FIND_FILE_TYPES $FIND_NEWER $FIND_PRINT0"
    log_message "Executing: $FIND_COMMAND"
    
    eval "$FIND_COMMAND" > "$TMP_FILE"
    log_message "Find command executed. Results:"
    cat "$TMP_FILE" >> "$LOG_FILE"
    
    if [ -s "$TMP_FILE" ]; then
        tar --exclude="$BACKUP_DIR" --null --files-from="$TMP_FILE" -cvf "$TAR_FILE" &>/dev/null
        if [ $? -eq 0 ]; then
            log_message "$(basename $TAR_FILE) was created"
            touch "$LAST_BACKUP"
        else
            log_error "Error creating incremental backup: $(tar --exclude="$BACKUP_DIR" --null --files-from="$TMP_FILE" -cvf "$TAR_FILE" 2>&1)"
        fi
    else
        log_message "No changes-Incremental backup was not created"
    fi
    rm -f "$TMP_FILE"
}

# Function to create a differential backup
differential_backup() {
    TAR_FILE="$DIFFERENTIAL_BACKUP_DIR/dbup$(date '+%Y%m%d%H%M%S').tar"
    TMP_FILE="/tmp/differential_files.txt"
    
    # Constructing and logging the find command step by step
    log_message "Starting differential backup..."
    log_message "Find Command Components:"
    
    FIND_USER_HOME="$USER_HOME"
    FIND_TYPE="-type f"
    FIND_FILE_TYPES="\( $FILE_TYPES \)"
    FIND_NEWER="-newer $FIRST_BACKUP"
    FIND_PRINT0="-print0"
    
    log_message "User Home: $FIND_USER_HOME"
    log_message "Type: $FIND_TYPE"
    log_message "File Types: $FIND_FILE_TYPES"
    log_message "Newer Than: $FIND_NEWER"
    log_message "Print0: $FIND_PRINT0"
    
    FIND_COMMAND="find $FIND_USER_HOME $FIND_TYPE $FIND_FILE_TYPES $FIND_NEWER $FIND_PRINT0"
    log_message "Executing: $FIND_COMMAND"
    
    eval "$FIND_COMMAND" > "$TMP_FILE"
    log_message "Find command executed. Results:"
    cat "$TMP_FILE" >> "$LOG_FILE"
    
    if [ -s "$TMP_FILE" ]; then
        tar --exclude="$BACKUP_DIR" --null --files-from="$TMP_FILE" -cvf "$TAR_FILE" &>/dev/null
        if [ $? -eq 0 ]; then
            log_message "$(basename $TAR_FILE) was created"
        else
            log_error "Error creating differential backup: $(tar --exclude="$BACKUP_DIR" --null --files-from="$TMP_FILE" -cvf "$TAR_FILE" 2>&1)"
        fi
    else
        log_message "No changes-Differential backup was not created"
    fi
    rm -f "$TMP_FILE"
}

# Determine file types to backup
if [ $# -eq 0 ]; then
    FILE_TYPES="-name '*'"
else
    FILE_TYPES=$(printf -- "-name '*.%s' -o " "${@#.}")
    FILE_TYPES="${FILE_TYPES% -o }"
fi

# Log the file types being used
log_message "File types to backup: $FILE_TYPES"

# Initial complete backup
FIRST_BACKUP="/tmp/first_backup_marker"
touch "$FIRST_BACKUP"
complete_backup

# Continuous loop to perform backups
while true; do
    LAST_BACKUP="/tmp/last_backup_marker"
    touch "$LAST_BACKUP"
    
    # Incremental backups
    sleep 30
    incremental_backup

    sleep 30
    incremental_backup

    # Differential backup
    sleep 30
    differential_backup

    # Incremental backup after differential
    sleep 30
    incremental_backup
done
