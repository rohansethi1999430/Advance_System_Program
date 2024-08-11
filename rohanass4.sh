#!/bin/bash


# Name : Rohan Sethi
# Student Id : 110133645
# Section : 4

log_message() {
    # Format the log message with a timestamp and append it to the log file
    echo "$(date '+%a %d %b %Y %T %p %Z') $1" >> $HOME/backup/backup.log
}

# Initialize counters for backup files
complete_backup=1
increamental_backup=1
differencial_backup=1

# Create a directory in $HOME for tracking backups
TRACK_DIR="$HOME/backup_track"
mkdir -p $TRACK_DIR

# Functon t perform complete backup
#For_complete_backup

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

    FIND_COMMAND="find $FIND_USER_HOME $FIND_TYPE $FIND_FILE_TYPES $FIND_EXCLUDE"
    
    find_command_output=$(eval $FIND_COMMAND)
files=$(echo "$find_command_output" | xargs -0)
    
    num_files=$(echo "$files" | wc -w)
    # log_message "Number of files found for complete backup: $num_files"

if [ -z "$files" ]; then
# no file creation
    log_message "No files found for complete backup"
else
    tar_file="$HOME/backup/cbup24s/cbup24s-$complete_backup.tar"
    # log_message "Creating tar file: $tar_file"
    tar -cvf "$tar_file" $files
    log_message "cbup24s-$complete_backup.tar was created"
    touch "$TRACK_DIR/last_full_backup"
    complete_backup=$((complete_backup + 1))
fi
}

# Functin to perform 
# icbup


perform_incremental_backup() {
    FIND_USER_HOME="$HOME"
    FIND_TYPE="-type f"
    FIND_NEWER="-newer $TRACK_DIR/last_backup"
    FIND_EXCLUDE="-not -path \"$HOME/.vscode-server/*\" -not -path \"$HOME/.vscode-server/data/User/History/*\" -not -path \"$HOME/.cache/*\" -not -path \"$HOME/.local/share/Trash/*\""
    FIND_PRINT0="-print0"

    if [ -n "$file_types" ]; then
        FIND_FILE_TYPES="\( $file_types \)"
    else
        FIND_FILE_TYPES=""
    fi

    FIND_COMMAND="find $FIND_USER_HOME $FIND_TYPE $FIND_FILE_TYPES $FIND_NEWER $FIND_EXCLUDE"
    
    find_command_output=$(eval $FIND_COMMAND)
files=$(echo "$find_command_output" | xargs -0)
    
    num_files=$(echo "$files" | wc -w)
    # log_message "Number of files found for incremental backup: $num_files"

    if [ -n "$files" ]; then
        tar_file=$HOME/backup/ibup24s/ibup24s-$increamental_backup.tar
        # log_message "Creating tar file: $tar_file"
        tar -cvf $tar_file $files
        log_message "ibup24s-$increamental_backup.tar was created"
        touch $TRACK_DIR/last_backup
        increamental_backup=$((increamental_backup + 1))
    else
    # else no file created
        log_message "No changes - Incremental backup was not created"
    fi
}

# Funcion to perform 
# difbp


perform_differential_backup() {
    FIND_USER_HOME="$HOME"
    FIND_TYPE="-type f"
    FIND_NEWER="-newer $TRACK_DIR/last_diff_backup"
    FIND_EXCLUDE="-not -path \"$HOME/.vscode-server/*\" -not -path \"$HOME/.vscode-server/data/User/History/*\" -not -path \"$HOME/.cache/*\" -not -path \"$HOME/.local/share/Trash/*\""
    FIND_PRINT0="-print0"

    if [ -n "$file_types" ]; then
        FIND_FILE_TYPES="\( $file_types \)"
    else
        FIND_FILE_TYPES=""
    fi

    FIND_COMMAND="find $FIND_USER_HOME $FIND_TYPE $FIND_FILE_TYPES $FIND_NEWER $FIND_EXCLUDE"
    
    find_command_output=$(eval $FIND_COMMAND)
files=$(echo "$find_command_output" | xargs -0)
    
    num_files=$(echo "$files" | wc -w)
    # log_message "Number of files found for differential backup: $num_files"

if [ -z "$files" ]; then
# no chnge means no file creation
    log_message "No changes - Differential backup was not created"
else
    tar_file="$HOME/backup/dbup24s/dbup24s-$differencial_backup.tar"
    # log_message "Creating tar file: $tar_file"
    tar -cvf "$tar_file" $files
    # creating log msg
    log_message "dbup24s-$differencial_backup.tar was created"
    touch "$TRACK_DIR/last_diff_backup"
    differencial_backup=$((differencial_backup + 1))
fi
}

# Check and create necessary directories
mkdir -p $HOME/backup/cbup24s
mkdir -p $HOME/backup/ibup24s
mkdir -p $HOME/backup/dbup24s
touch  $HOME/backup/backup.log

# Determine file types to backup
file_types=""
for arg in "$@"; do
    # log_message "Adding file type parameter: -name '*$arg'"
    file_types="$file_types -o -name '*$arg'"
done

# Remove leading -o for find command
file_types="${file_types:3}"

# Main loop to perform backups
for (( ; ; ))
do
    perform_complete_backup
    touch $TRACK_DIR/last_backup
    touch $TRACK_DIR/last_diff_backup
    sleep 120  # Sleep for 20 seconds

    perform_incremental_backup
    sleep 120  # Sleep for 20 seconds

    perform_incremental_backup
    sleep 120  # Sleep for 20 seconds

    perform_differential_backup
    sleep 120  # Sleep for 20 seconds

    perform_incremental_backup
    sleep 120  # Sleep for 20 seconds
done