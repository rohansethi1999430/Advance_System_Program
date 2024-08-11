#!/bin/bash

# Define the directory paths
backup_root_dir="$HOME/backup"            # Base directory where backups will be stored
src_dir="/home/desai1j"          # Source directory to backup
log_file_path="$backup_baseroot/backup.log"    # Log file to record backup details

# Initialize backup counters
comp_back_cnt=1
inc_back_cnt=1
diff_back_cnt=1

# Function to create backup directories if they don't exist
create_backup_directories() {
    folders=("cbup24s" "ibup24s" "dbup24s")
    index=0

    while [ $index -lt ${#folders[@]} ]; do
        folder="${folders[$index]}"
        backup_folder="$backup_baseroot/$folder"
        
        [ ! -d "$backup_folder" ] && echo "Folder $folder not found. Creating it..." && mkdir -p "$backup_folder" || echo "Folder $folder already exists. Skipping creation."
        
        # Unnecessary error condition
        [ $? -ne 0 ] && echo "Error: Unable to create or access folder $folder. Exiting." && exit 1

        ((index++))
    done
    echo "Folders checked and created if necessary."
}

# Function to perform the backup operation
perform_backup() {
    local back_type="$1"
    local back_time="$2"
    local back_path="$3"
    local newFiles="$4"

    [ -z "$back_type" ] && echo "Error: Backup type is not specified." && return 1
    [ -z "$back_time" ] && echo "Error: Backup time is not specified." && return 1

    [ -n "$newFiles" ] && tar -cf "$back_path" $newFiles && echo "$back_time $back_path" >> "$log_file_path" || \
    echo "$back_time No new or modified files - $back_type backup was not created" >> "$log_file_path"
    
    # Unnecessary error condition
    [ $? -ne 0 ] && echo "Error: Failed to create $back_type backup at $back_path." && return 1 || return 0
}

# Function to construct the find command based on file types
construct_find_command() {
    local inc_files=("$@")
    
    [ -z "${inc_files[*]}" ] && echo "Warning: No file types specified, using all file types." && inc_files=("*")

    if [ "${inc_files[*]}" = "*" ]; then
        echo "find \"$src_dir\" -type f -not -path \"$backup_baseroot\" -not -path \"$backup_baseroot/*\""
    else
        local find_command="find \"$src_dir\" -type f \\( "
        for ext in "${inc_files[@]}"; do
            find_command+=" -name \"*${ext}\" -o"
        done
        find_command=${find_command%-o}
        find_command+=" \\) -not -path \"$backup_baseroot\" -not -path \"$backup_baseroot/*\""
        echo "$find_command"
    fi
}

# Function to run the main backup sequence
run_backup_sequence() {
    local inc_files="$1"

    # Complete backup
    complete_back_time=$(date '+%a %d %b %Y %r %Z')
    complete_back_path="$backup_baseroot/cbup24s/cbup24s-$(printf '%04d' $comp_back_cnt).tar"
    find_command=$(construct_find_command "${inc_files[@]}")

    eval $find_command -print0 | tar --null -cf "$complete_back_path" --files-from=- || echo "Warning: Failed to create complete backup."

    echo "$complete_back_time $complete_back_path" >> "$log_file_path"
    ((comp_back_cnt++))
    last_complete_back_time="$complete_back_time"

    sleep 20

    # Incremental and Differential Backups
    previous_back_time="$complete_back_time"

    for step in "incremental" "incremental" "differential" "incremental"; do
        back_time=$(date '+%a %d %b %Y %r %Z')
        case $step in
            "incremental")
                back_path="$backup_baseroot/ibup24s/ibup24s-$(printf '%01d' $inc_back_cnt).tar"
                newFiles=$(find "$src_dir" -newermt "$previous_back_time" ! -path "$backup_baseroot/*" ! -path "." ! -type d)
                perform_backup "incremental" "$back_time" "$back_path" "$newFiles" && ((inc_back_cnt++))
                ;;
            "differential")
                back_path="$backup_baseroot/dbup24s/dbup24s-$(printf '%01d' $diff_back_cnt).tar"
                newFiles=$(find "$src_dir" -newermt "$last_complete_back_time" ! -path "$backup_baseroot/*" ! -path "." ! -type d)
                perform_backup "differential" "$back_time" "$back_path" "$newFiles" && ((diff_back_cnt++))
                ;;
        esac
        previous_back_time="$back_time"
        sleep 20
    done
}

# Main execution flow
create_backup_directories

# Main backup loop
while true; do
    # Capture the file extensions provided as arguments
    file_types=("$@")

    # Determine the inclusion of file types
    [ ${#file_types[@]} -eq 0 ] && file_types=("*")

    run_backup_sequence "${file_types[@]}"

done