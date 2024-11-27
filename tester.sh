#!/bin/bash

# Array containing all function names
fxs=("__myfs_getattr_implem" "__myfs_readdir_implem" "__myfs_mknod_implem" "__myfs_unlink_implem" "__myfs_rmdir_implem" "__myfs_mkdir_implem" "__myfs_rename_implem" "__myfs_truncate_implem" "__myfs_open_implem" "__myfs_read_implem" "__myfs_write_implem" "__myfs_write_implem" "__myfs_utimens_implem" "__myfs_statfs_implem" "offset_to_ptr" "init_fs" "find_inode" "split_path" "find_free_inode" "add_dir_entry" "remove_dir_entry" "find_free_data_block" "free_data_block" "get_block_bitmap" "calculate_free_blocks")

# Display available functions
echo "Functions available:"
for i in "${!fxs[@]}"; do
    echo "$((i + 1)). ${fxs[i]}"
done

# Prompt the user to select functions
read -p "Which functions do you want to test? (separate them by commas): " sel_fxs

# Parse selected functions and build the GDB command
IFS=',' read -ra indices <<< "$sel_fxs"  # Tokenize input by comma
breakpoints=""
for index in "${indices[@]}"; do
    if [[ $index =~ ^[0-9]+$ ]] && ((index >= 1 && index <= ${#fxs[@]})); then
        breakpoints+=" -ex 'break ${fxs[index-1]}'"
    else
        echo "Invalid selection: $index"
        exit 1
    fi
done

# Construct the GDB command
gdb_command="gdb${breakpoints} -ex 'run' --args ./myfs --backupfile=test.myfs /home/rgarcia117/fuse-mnt/ -f"

# Run the GDB command
echo "Executing: $gdb_command"
eval "$gdb_command"
