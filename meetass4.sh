#!/bin/bash

# Initialize variables
backup_location_directory="$HOME/backup"
complete_backup_count=1
incremental_backup_count=1
differential_backup_count=1
source_directory="/home/patel319"
log_file="$backup_location_directory/backup.log"

# Define the directory paths
folders=("cbw24" "ib24" "db24")

# Check if the backup directory exists
if [ ! -d "$backup_location_directory" ]; then
    echo "Backup directory not found. Creating it..."
    mkdir -p "$backup_location_directory"
fi

# Loop through the folders array
for folder in "${folders[@]}"; do
    # Check if the folder exists
    if [ ! -d "$backup_location_directory/$folder" ]; then
        echo "Folder $folder not found. Creating it..."
        mkdir -p "$backup_location_directory/$folder"
    fi
done

echo "Folders checked and created if necessary."

while true; do
    # STEP 1: Create complete backup
    complete_backup_time=$(date '+%a %d %b %Y %r %Z')
    complete_backup_path="$backup_location_directory/cbw24/cbw24-$(printf '%01d' $complete_backup_count).tar"
    tar -cf "$complete_backup_path" --exclude="$backup_location_directory" --exclude="$backup_location_directory/*" --exclude=".*" "$source_directory"
    echo "$complete_backup_time $complete_backup_path" >> "$log_file"
    ((complete_backup_count++))

    sleep 120

    # STEP 2: Create incremental backup after STEP 1
    incremental_backup_time_1=$(date '+%a %d %b %Y %r %Z')
    incremental_backup_path="$backup_location_directory/ib24/ib24-$(printf '%01d' $incremental_backup_count).tar"
    new_files=$(find "$source_directory" -newermt "$complete_backup_time" ! -path "$backup_location_directory/*" ! -path ".*" ! -type d)
    if [ -n "$new_files" ]; then
        tar -cf "$incremental_backup_path" $new_files
        echo "$incremental_backup_time_1 $incremental_backup_path" >> "$log_file"
        ((incremental_backup_count++))
    else
        echo "$incremental_backup_time_1 No new or modified files - Incremental backup was not created" >> "$log_file"
    fi

    sleep 120

    # STEP 3: Create incremental backup after STEP 2
    incremental_backup_time_2=$(date '+%a %d %b %Y %r %Z')
    incremental_backup_path="$backup_location_directory/ib24/ib24-$(printf '%01d' $incremental_backup_count).tar"
    new_files=$(find "$source_directory" -newermt "$incremental_backup_time_1" ! -path "$backup_location_directory/*" ! -path ".*" ! -type d)
    if [ -n "$new_files" ]; then
        tar -cf "$incremental_backup_path" $new_files
        echo "$incremental_backup_time_2 $incremental_backup_path" >> "$log_file"
        ((incremental_backup_count++))
    else
        echo "$incremental_backup_time_2 No new or modified files - Incremental backup was not created" >> "$log_file"
    fi

    sleep 120

    # STEP 4: Create differential backup after STEP 1
    differential_backup_time=$(date '+%a %d %b %Y %r %Z')
    differential_backup_path="$backup_location_directory/db24/db24-$(printf '%01d' $differential_backup_count).tar"
    new_files=$(find "$source_directory" -newermt "$complete_backup_time" ! -path "$backup_location_directory/*" ! -path ".*" ! -type d)
    if [ -n "$new_files" ]; then
        tar -cf "$differential_backup_path" $new_files
        echo "$differential_backup_time $differential_backup_path" >> "$log_file"
        ((differential_backup_count++))
    else
        echo "$differential_backup_time No new or modified files - Differential backup was not created" >> "$log_file"
    fi

    sleep 120

    # STEP 5: Create incremental backup after STEP 4
    incremental_backup_time=$(date '+%a %d %b %Y %r %Z')
    incremental_backup_path="$backup_location_directory/ib24/ib24-$(printf '%01d' $incremental_backup_count).tar"
    new_files=$(find "$source_directory" -newermt "$differential_backup_time" ! -path "$backup_location_directory/*" ! -path ".*" ! -type d)
    if [ -n "$new_files" ]; then
        tar -cf "$incremental_backup_path" $new_files
        echo "$incremental_backup_time $incremental_backup_path" >> "$log_file"
        ((incremental_backup_count++))
    else
        echo "$incremental_backup_time No new or modified files - Incremental backup was not created" >> "$log_file"
    fi

    sleep 120

done