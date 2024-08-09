#!/bin/bash

# Function to log messages with the specific date format
log_message() {
    local timestamp
    timestamp=$(date '+%a %d %b %Y %I:%M:%S %p %Z')
    echo "$timestamp $1" >> "$HOME/backup/backup.log"
}

# Function to create directories if they do not exist
setup_directories() {
    # Array of directory names
    dirs=("cbup24s" "ibup24s" "dbup24s")

    # Loop through each directory and create it if it doesn't exist
    for dir in "${dirs[@]}"; do
        mkdir -p "$HOME/backup/$dir"
    done
}

# Function to create the backup log file if it doesn't exist
setup_backup_log() {
    [ -f "$HOME/backup/backup.log" ] || touch "$HOME/backup/backup.log"
}

# Helper function to build the file types portion of the find command
build_file_types() {
    if [ -z "$1" ] && [ -z "$2" ] && [ -z "$3" ]; then
        echo "-name '*'"
    else
        local types=""
        [ -n "$1" ] && types="-name \"*${1}\""
        [ -n "$2" ] && types="$types -o -name \"*${2}\""
        [ -n "$3" ] && types="$types -o -name \"*${3}\""
        echo "$types"
    fi
}

# Function to get the latest backup timestamp file
get_latest_backup_file() {
    [ -f "$HOME/last_backup" ] && echo "$HOME/last_backup" || echo "$HOME/last_full_backup"
}

# Function to perform a complete backup
perform_complete_backup() {
    file_types=$(build_file_types "$1" "$2" "$3")
    cbup_count=$(ls $HOME/backup/cbup24s | wc -l)
    cbup_count=$((cbup_count + 1))

    FIND_COMMAND="find $HOME -path $HOME/.vscode-server -prune -o -type f \( $file_types \) -print0"
    files=$(eval $FIND_COMMAND | xargs -0)
    
create_tar_file() {
    local tar_file=$1
    shift
    local files=$@
    tar -cvf "$tar_file" $files
}

update_backup_markers() {
    touch "$HOME/last_full_backup"
    rm -f "$HOME/last_backup" # Remove the last incremental backup marker
}

log_backup_creation() {
    local tar_file=$1
    log_message "$(basename "$tar_file") was created"
}

create_complete_backup() {
    local files=$1
    local tar_file="$HOME/backup/cbup24s/cbup24s-$cbup_count.tar"

    create_tar_file "$tar_file" $files
    log_backup_creation "$tar_file"
    update_backup_markers
}

handle_no_changes_complete_backup() {
    log_message "No changes - Complete backup was not created"
}

files=$(eval $FIND_COMMAND | xargs -0)

[ -n "$files" ] && create_complete_backup "$files" || handle_no_changes_complete_backup
}

# Function to perform incremental backup
perform_incremental_backup() {
    file_types=$(build_file_types "$1" "$2" "$3")
    ibup_count=$(ls $HOME/backup/ibup24s | wc -l)
    ibup_count=$((ibup_count + 1))

    LATEST_BACKUP_FILE=$(get_latest_backup_file)
    FIND_COMMAND="find $HOME -path $HOME/.vscode-server -prune -o -type f \( $file_types \) -newer $LATEST_BACKUP_FILE -print0"
   files=$(eval $FIND_COMMAND | xargs -0)

create_tar_and_log() {
    local tar_file=$1
    local files=$2
    local log_message_text=$3
    
    tar -cvf "$tar_file" $files
    log_message "$log_message_text"
    touch "$HOME/last_backup"
}

handle_no_changes() {
    log_message "No changes - Incremental backup was not created"
}

files=$(eval $FIND_COMMAND | xargs -0)

[ -n "$files" ] && create_tar_and_log "$HOME/backup/ibup24s/ibup24s-$ibup_count.tar" "$files" "ibup24s-$ibup_count.tar was created" || handle_no_changes
}

perform_differential_backup() {
    file_types=$(build_file_types "$1" "$2" "$3")
    dbup_count=$(ls $HOME/backup/dbup24s | wc -l)
    dbup_count=$((dbup_count + 1))

    FIND_COMMAND="find $HOME -path $HOME/.vscode-server -prune -o -type f \( $file_types \) -newer $HOME/last_full_backup -print0"
    files=$(eval $FIND_COMMAND | xargs -0)
    
create_differential_backup() {
    local tar_file="$HOME/backup/dbup24s/dbup24s-$dbup_count.tar"
    local files=$1

    tar -cvf "$tar_file" $files
    log_message "dbup24s-$dbup_count.tar was created"
}

handle_no_changes_differential_backup() {
    log_message "No changes - Differential backup was not created"
}

files=$(eval $FIND_COMMAND | xargs -0)

[ -n "$files" ] && create_differential_backup "$files" || handle_no_changes_differential_backup
}

# Setup directories and backup log file
setup_directories
setup_backup_log

# Main loop
backup_operations=(
    "perform_complete_backup"
    "perform_incremental_backup"
    "perform_incremental_backup"
    "perform_differential_backup"
    "perform_incremental_backup"
)

while true; do
    for operation in "${backup_operations[@]}"; do
        $operation "$1" "$2" "$3"
        sleep 20
    done
done
