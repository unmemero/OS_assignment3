# Assignment 3

**Contributors:** Rafael Garcia and Isaac G. Padilla\

## General Initial considerations

### Definitions

```c
#define FS_ID 0x4D595346
#define BLOCK_SIZE 4096
#define INODE_SIZE 128
#define MAX_FILENAME 255
#define MAX_INODES 1024
#define ROOT_INODE 0
#define MAX_DATA_BLOCKS 2528 
```

- `FS_ID`: We created an ID for the filesystem to verify it's properly initialized.
- `BLOCK_SIZE`: We assigned the size of each block to be 4KB, we're considering inceasing it.
- `INODE_SIZE`: We created the size of an node in our filesystem's tree.
- `MAX_FILENAME`: Gave limit of 255 chars for filename.
- `MAX_INODES`: Defined maximum number of inodes possible.
- `ROOT_INODE`: Offset to root inode (start from 0).
- `MAX_DATA_BLOCKS`: CAlculated based on `INODE_SIZE`, `MAX_INODES`, AND   `BLOCK_SIZE`

### Structs

```c
typedef struct{
    uint32_t fs_id;
    size_t size;
    size_t root_inode;
    size_t free_inode_bitmap;
    size_t free_block_bitmap;
    size_t inode_table;
    size_t data_blocks;
    size_t max_data_blocks;
}fs_info_block;
```

- We decided to create  a struct with the base information of the filesystem.
- It consists of:
- - `uint32_t fs_id`: The filesystem's ID to help us check if the filesystem is initialized.
- - `size_t size`: Contains the size of the filesystem (usually fssize, but it may be too much of a hassle adding multiple args into a new helper function, so it's easier to do it in a struct).
- - `size_t root_inode`: Offset to root inode from fsptr (should be 0).
- - `size_t free_inode_bitmap`: Bitmap to keep track of inode usage.
- - `size_t free_block_bitmap`: Bitmap to keep tracl of free blocks for files.
- - `size_t free_inode_table`: Offset to inodes;
- - `size_t max_datab_locks`: Max number of datablock in filesystem.

```c
typedef struct{
    mode_t mode;
    uid_t uid;
    gid_t gid;
    size_t size;
    time_t access_time;
    time_t modification_time;
    time_t change_time;
    size_t data_block;
}inode;
```

- We decided to create a node struct for every entry in the filesystem. Each entry will have:
- - `mode_t mode`: To tell if its a file or a directory.
- - `uid_t uid`: User ID of entry cretator.
- - `gid_t gid`: Group ID the creator belongs to.
- - `size_t size`: Size of the entry.
- - `time_t access time`: Time of last access of the entry.
- - `time_t modification_time`: Time of last modification to the entry.
- - `time_t change_time`: Time of most recent change to the entry
- - `time_t data_block`: block offset corresponding to the file's datablock.

```c
typedef struct{
    char name[MAX_FILENAME + 1];
    size_t inode_offset;
}directory_entry;
```

- Created for basic entry info such as:
- - `char name[MAX_FILENAME + 1]`: The name of the file + `\0`.
- - `size_t inode_offset`: Offset of the current entry with respect to the root.

### Helper functions

#### 1

```c
static void* offset_to_ptr(void *fsptr, size_t fssize, size_t offset)
```

- This returns a pointer from the root directory plus an offset to the current entry.
- We did this to simplify getting offsets from root, and to check the offset doesn't exceed `fssize`.

##### 2

```c
static int init_fs(void *fsptr, size_t fssize)
```

- We created it to determine if the filesystem was already initialized, and if not, to initialize it to the default values mentioned previously
- The main issue we faced is how to keep track of the content of the filesystem, which brought to the bitmap idea to keep it efficient in which we mark the usage of the data block.

#### 3

```c
static inode* find_inode(void *fsptr, size_t fssize, const char * path, size_t *inode_offset_ptr)
```

- Here, it was hard to figure out how we wanted to traverse the tree. In that case, we decided to use path tokenization with `/` as our delimiter, follow the path by iterating and comparing strings, and returning the inode if the current token matches the entry, else, if the last toke was reached, or a subdirectory wasn't found, it returns `NULL`.

#### 4

```c
static int split_path(const char *path, char **parent_path, char **base_name) 
```

- We just created this to simplify getting parent and child node names from the user provided path.

#### 5

```c
static size_t find_free_inode(void *fsptr, size_t fssize)
```

- It was difficult to see if we had availavility for a new element in a director. We created this function to simplify the lookup by using a bitmap that marked if a node was either free or in use.

#### 6

```c
int add_dir_entry(void *fsptr, size_t fssize, inode *dir_inode, size_t dir_inode_offset, const char *name, size_t new_inode_offset)
```

-

## Core functions

### 1. `__myfs_getattr_implem`

### 2. `__myfs_readdir_implem`

### 3. `__myfs_mknod_implem`

### 4. `__myfs_unlink_implem`

### 5. `__myfs_rmdir_implem`

### 6. `__myfs_mkdir_implem`

### 7. `__myfs_rename_implem`

### 8. `__myfs_truncate_implem`

### 9. `__myfs_open_implem`

### 10. `__myfs_read_implem`

### 11. `__myfs_write_implem`

### 12. `__myfs_utimens_implem`

### 13. `__myfs_statfs_implem`

## Testing process