# Assignment 3

**Contributors:** Rafael Garcia and Isaac G. Padilla

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

- To streamline the process of adding a new file or directory, we decided to condense that into its own function to improve code readability.

#### 7

```c
static int remove_dir_entry(void *fsptr, size_t fssize, inode *dir_inode, size_t dir_inode_offset, const char *name)
```

- We had a similar approach to removing files and directories from directories, since we just wanted tomething to improve code readability.

#### 8

```c
static size_t find_free_data_block(void *fsptr, size_t fssize)
```

- Since we determined some functions may need to find a free data block, we thought the best way to approach it is to put the process in its own function. The hardest part was figuring out how to mark the usage of the blocks in the bitmap and  getting the offset to that block, but once done, it proved to be useful in order to get a block of data to write on.

#### 9

```c
static int free_data_block(void *fsptr, size_t fssize, size_t block_offset) 
```

- Similar to previous functions, we used the bitmap to mark a block as free.

#### 10

```c
uint8_t * get_block_bitmap(void *fsptr, size_t fssize)
```

- Mainly to return bitmap to check if a block is free or not.

#### 11

```c
size_t calculate_free_blocks(void *fsptr, size_t fssize) 
```

- We needed something to check how many free blocks are available. Figuring out the operations on the bitmap was the complex part, but once that was just a matter of calculating the number of free blocks.

## Core functions

### 1. `__myfs_getattr_implem`

- Probably the most complex part is figuring was getting to know more on ehat to provide to stbuf, and to check if the filesystem is already there. Once that was figured out, it was a simple process of making a function to initialize the filesystem, findiing the current node, and populating its info on stbuf.

### 2. `__myfs_readdir_implem`

- The way we decided to implement this, is by a traversal of the fs with the `find_inode` function, getting all entries with an offset to `fsptr`, and populating the names array based on the number of entries of the directory requested. The most difficult part was the memory allocation, since we needed to implement many considerations in case the filesystem failed, but essentially it was keeping track of our allocations, and freeing our allocated space.

### 3. `__myfs_mknod_implem`

- In this point, since we were working on split fuinctions, we thought it would be a good idea implement a function that split the path into the child and parent components of a path. We determined for this function we required to get the parent directory first, get the entries, and check if the entry we want to add existed.
- If all was good with that, we just needed to create a new node at the offset marked by the inode bitmap on which node was free.
- We then populated the node with the file's information.
- To add it to the parent, we created `add_dir_entry`, since we were developing various functions in parallel, and determined that it would be simpler to keep that into a single function. In the case we couldn't add it to the parent directory, we just free the inode bitmap, and the rest of the allocs.
- After that, we cleanup allocated strings, and return 0 on success.

### 4. `__myfs_unlink_implem`

- We did a simillar process as in `mknod` initially, where we find the parent directory, got the entries of the parent, found the file, and got the offset of the inode.
- After this, since other functions that we were developing in parallel caused required the removal from a directory, we decided to implement [`remove_dir_entry`](#7) , to remove both files and directories from a parent directory.
- After this, we just clean the bitmap, set the inode to 0, and free allocated memory.

### 5. `__myfs_rmdir_implem`

- 

### 6. `__myfs_mkdir_implem`

### 7. `__myfs_rename_implem`

### 8. `__myfs_truncate_implem`

### 9. `__myfs_open_implem`

### 10. `__myfs_read_implem`

### 11. `__myfs_write_implem`

### 12. `__myfs_utimens_implem`

### 13. `__myfs_statfs_implem`

## Testing process

We used GDB with breakpoints to test each of the functions, as well as various linux filesystem command line tools.
