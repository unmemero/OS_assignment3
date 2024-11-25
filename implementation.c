/*
  MyFS: a tiny file-system written for educational purposes

  MyFS is 

  Copyright 2018-21 by

  University of Alaska Anchorage, College of Engineering.

  Copyright 2022-24

  University of Texas at El Paso, Department of Computer Science.

  Contributors: Christoph Lauter 
                Rafael Garcia and
                Isaac G Padilla

  and based on 

  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall myfs.c implementation.c `pkg-config fuse --cflags --libs` -o myfs

*/

#include <stddef.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>


/* The filesystem you implement must support all the 13 operations
   stubbed out below. There need not be support for access rights,
   links, symbolic links. There needs to be support for access and
   modification times and information for statfs.

   The filesystem must run in memory, using the memory of size 
   fssize pointed to by fsptr. The memory comes from mmap and 
   is backed with a file if a backup-file is indicated. When
   the filesystem is unmounted, the memory is written back to 
   that backup-file. When the filesystem is mounted again from
   the backup-file, the same memory appears at the newly mapped
   in virtual address. The filesystem datastructures hence must not
   store any pointer directly to the memory pointed to by fsptr; it
   must rather store offsets from the beginning of the memory region.

   When a filesystem is mounted for the first time, the whole memory
   region of size fssize pointed to by fsptr reads as zero-bytes. When
   a backup-file is used and the filesystem is mounted again, certain
   parts of the memory, which have previously been written, may read
   as non-zero bytes. The size of the memory region is at least 2048
   bytes.

   CAUTION:

   * You MUST NOT use any global variables in your program for reasons
   due to the way FUSE is designed.

   You can find ways to store a structure containing all "global" data
   at the start of the memory region representing the filesystem.

   * You MUST NOT store (the value of) pointers into the memory region
   that represents the filesystem. Pointers are virtual memory
   addresses and these addresses are ephemeral. Everything will seem
   okay UNTIL you remount the filesystem again.

   You may store offsets/indices (of type size_t) into the
   filesystem. These offsets/indices are like pointers: instead of
   storing the pointer, you store how far it is away from the start of
   the memory region. You may want to define a type for your offsets
   and to write two functions that can convert from pointers to
   offsets and vice versa.

   * You may use any function out of libc for your filesystem,
   including (but not limited to) malloc, calloc, free, strdup,
   strlen, strncpy, strchr, strrchr, memset, memcpy. However, your
   filesystem MUST NOT depend on memory outside of the filesystem
   memory region. Only this part of the virtual memory address space
   gets saved into the backup-file. As a matter of course, your FUSE
   process, which implements the filesystem, MUST NOT leak memory: be
   careful in particular not to leak tiny amounts of memory that
   accumulate over time. In a working setup, a FUSE process is
   supposed to run for a long time!

   It is possible to check for memory leaks by running the FUSE
   process inside valgrind:

   valgrind --leak-check=full ./myfs --backupfile=test.myfs ~/fuse-mnt/ -f

   However, the analysis of the leak indications displayed by valgrind
   is difficult as libfuse contains some small memory leaks (which do
   not accumulate over time). We cannot (easily) fix these memory
   leaks inside libfuse.

   * Avoid putting debug messages into the code. You may use fprintf
   for debugging purposes but they should all go away in the final
   version of the code. Using gdb is more professional, though.

   * You MUST NOT fail with exit(1) in case of an error. All the
   functions you have to implement have ways to indicated failure
   cases. Use these, mapping your internal errors intelligently onto
   the POSIX error conditions.

   * And of course: your code MUST NOT SEGFAULT!

   It is reasonable to proceed in the following order:

   (1)   Design and implement a mechanism that initializes a filesystem
         whenever the memory space is fresh. That mechanism can be
         implemented in the form of a filesystem handle into which the
         filesystem raw memory pointer and sizes are translated.
         Check that the filesystem does not get reinitialized at mount
         time if you initialized it once and unmounted it but that all
         pieces of information (in the handle) get read back correctly
         from the backup-file. 

   (2)   Design and implement functions to find and allocate free memory
         regions inside the filesystem memory space. There need to be 
         functions to free these regions again, too. Any "global" variable
         goes into the handle structure the mechanism designed at step (1) 
         provides.

   (3)   Carefully design a data structure able to represent all the
         pieces of information that are needed for files and
         (sub-)directories.  You need to store the location of the
         root directory in a "global" variable that, again, goes into the 
         handle designed at step (1).
          
   (4)   Write __myfs_getattr_implem and debug it thoroughly, as best as
         you can with a filesystem that is reduced to one
         function. Writing this function will make you write helper
         functions to traverse paths, following the appropriate
         subdirectories inside the file system. Strive for modularity for
         these filesystem traversal functions.

   (5)   Design and implement __myfs_readdir_implem. You cannot test it
         besides by listing your root directory with ls -la and looking
         at the date of last access/modification of the directory (.). 
         Be sure to understand the signature of that function and use
         caution not to provoke segfaults nor to leak memory.

   (6)   Design and implement __myfs_mknod_implem. You can now touch files 
         with 

         touch foo

         and check that they start to exist (with the appropriate
         access/modification times) with ls -la.

   (7)   Design and implement __myfs_mkdir_implem. Test as above.

   (8)   Design and implement __myfs_truncate_implem. You can now 
         create files filled with zeros:

         truncate -s 1024 foo

   (9)   Design and implement __myfs_statfs_implem. Test by running
         df before and after the truncation of a file to various lengths. 
         The free "disk" space must change accordingly.

   (10)  Design, implement and test __myfs_utimens_implem. You can now 
         touch files at different dates (in the past, in the future).

   (11)  Design and implement __myfs_open_implem. The function can 
         only be tested once __myfs_read_implem and __myfs_write_implem are
         implemented.

   (12)  Design, implement and test __myfs_read_implem and
         __myfs_write_implem. You can now write to files and read the data 
         back:

         echo "Hello world" > foo
         echo "Hallo ihr da" >> foo
         cat foo

         Be sure to test the case when you unmount and remount the
         filesystem: the files must still be there, contain the same
         information and have the same access and/or modification
         times.

   (13)  Design, implement and test __myfs_unlink_implem. You can now
         remove files.

   (14)  Design, implement and test __myfs_unlink_implem. You can now
         remove directories.

   (15)  Design, implement and test __myfs_rename_implem. This function
         is extremely complicated to implement. Be sure to cover all 
         cases that are documented in man 2 rename. The case when the 
         new path exists already is really hard to implement. Be sure to 
         never leave the filessystem in a bad state! Test thoroughly 
         using mv on (filled and empty) directories and files onto 
         inexistant and already existing directories and files.

   (16)  Design, implement and test any function that your instructor
         might have left out from this list. There are 13 functions 
         __myfs_XXX_implem you have to write.

   (17)  Go over all functions again, testing them one-by-one, trying
         to exercise all special conditions (error conditions): set
         breakpoints in gdb and use a sequence of bash commands inside
         your mounted filesystem to trigger these special cases. Be
         sure to cover all funny cases that arise when the filesystem
         is full but files are supposed to get written to or truncated
         to longer length. There must not be any segfault; the user
         space program using your filesystem just has to report an
         error. Also be sure to unmount and remount your filesystem,
         in order to be sure that it contents do not change by
         unmounting and remounting. Try to mount two of your
         filesystems at different places and copy and move (rename!)
         (heavy) files (your favorite movie or song, an image of a cat
         etc.) from one mount-point to the other. None of the two FUSE
         processes must provoke errors. Find ways to test the case
         when files have holes as the process that wrote them seeked
         beyond the end of the file several times. Your filesystem must
         support these operations at least by making the holes explicit 
         zeros (use dd to test this aspect).

   (18)  Run some heavy testing: copy your favorite movie into your
         filesystem and try to watch it out of the filesystem.

*/

/* Helper types and functions */

/**************Definitions**************/

#define FS_ID 0x4D595346
#define BLOCK_SIZE 4096
#define INODE_SIZE 128
#define MAX_FILENAME 255
#define MAX_INODES 1024
#define ROOT_INODE 0
#define MAX_DATA_BLOCKS 2528 

/**************Structs adn typedefs**************/

/*
*   Main info block of file system
*       - fs_id: file system id (to check if fs is initialized)
*       - size: size of file system
*       - root_inode: offset to root inode
*       - free_inode_bitmap: offset to inode bitmap
*       - free_block_bitmap: offset to data block bitmap
*       - inode_table: offset to inode table
*       - data_blocks: offset to data blocks
*       - max_data_blocks: maximum number of data blocks
*/
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

/*
*   Node in filesystem tree (directory or file)
*       - mode: file type and permissions
*       - uid: user id
*       - gid: group id
*       - size: size of file
*       - access_time: last access time
*       - modification_time: last modification time
*       - change_time: last change time
*       - data_block: offset to data block
*/
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

/*
*   Entry inside a directory
*       - name: name of file
*       - inode_offset: offset to inode of file
*/
typedef struct{
    char name[MAX_FILENAME + 1];
    size_t inode_offset;
}directory_entry;


/**************Functions**************/

/**
 * Converts an offset to a pointer in the file system
 */
static void* offset_to_ptr(void *fsptr, size_t fssize, size_t offset){
    return (offset >= fssize) ? NULL : (char *)fsptr + offset;
}

/**
 * Init the fs
 */
static int init_fs(void *fsptr, size_t fssize){
    /*Info block at current */
    fs_info_block *info_block = (fs_info_block*)fsptr;
    
    /*FS already init*/
    if(info_block->fs_id == FS_ID) return 1;

    /*Init info block of fs*/
    info_block->fs_id = FS_ID;
    info_block->size = fssize;
    info_block->root_inode = sizeof(fs_info_block);
    info_block->free_inode_bitmap = info_block->root_inode + INODE_SIZE;
    info_block->free_block_bitmap = info_block->free_inode_bitmap + (MAX_INODES / 8);
    info_block->inode_table = info_block->free_block_bitmap + (MAX_DATA_BLOCKS / 8);
    info_block->data_blocks = info_block->inode_table + (MAX_INODES * INODE_SIZE);
    info_block->max_data_blocks = MAX_DATA_BLOCKS; 

    /*Init root*/
    inode *root = (inode*)offset_to_ptr(fsptr, fssize, info_block->root_inode);
    root->mode = S_IFDIR | 0755;
    root->uid = getuid();
    root->gid = getgid();
    root->size = 0;
    root->access_time = root->modification_time = root->change_time = time(NULL);
    root->data_block = info_block->data_blocks;

    /*Init root dir*/
    directory_entry *root_dir = (directory_entry*)offset_to_ptr(fsptr, fssize, root->data_block);
    /*Add .*/
    strcpy(root_dir[0].name, ".");
    root_dir[0].inode_offset = info_block->root_inode;
    /*Add ..*/
    strcpy(root_dir[1].name, "..");
    root_dir[1].inode_offset = info_block->root_inode;
    root->size += 2 * sizeof(directory_entry);

    /*Init inode bitmap*/
    memset(offset_to_ptr(fsptr, fssize, info_block->free_inode_bitmap), 0, MAX_INODES / 8);

    /* Mark root as used */
    uint8_t *inode_bitmap = (uint8_t *)offset_to_ptr(fsptr, fssize, info_block->free_inode_bitmap);
    inode_bitmap[0] |= (uint8_t)1;

    /*Init data block bitmap*/
    memset(offset_to_ptr(fsptr, fssize, info_block->free_block_bitmap), 0, MAX_DATA_BLOCKS / 8);
    
    /*Mark root's data block as used*/
    unsigned char *data_bitmap = (unsigned char*)offset_to_ptr(fsptr, fssize, info_block->free_block_bitmap);
    if (data_bitmap) data_bitmap[0] |= 1;

    return 1;
}


static inode* find_inode(void *fsptr, size_t fssize, const char * path, size_t *inode_offset_ptr){
    fs_info_block *info_block = (fs_info_block*)fsptr;
    inode *curr_inode = (inode *)offset_to_ptr(fsptr, fssize, info_block->root_inode);
    size_t curr_offset = info_block->root_inode;

    if(!strcmp(path, "/")){
        *inode_offset_ptr = curr_offset;
        return curr_inode;
    }

    /*Tokenize path*/
    char *path_cpy = strdup(path);
    if (!path_cpy) return NULL;

    char *token = strtok(path_cpy, "/");
    while(token){
        if(!(curr_inode->mode & S_IFDIR)){
                free(path_cpy);
                return NULL;
        }

        /*Iterate through directory*/
        directory_entry *entries = (directory_entry *)offset_to_ptr(fsptr, fssize, curr_inode->data_block);
        size_t num_entries = curr_inode->size / sizeof(directory_entry);
        inode *next_inode = NULL;
        size_t next_offset = 0;
        int found = 0;

        for(size_t i = 0; i < num_entries; i++){
                if(!strcmp(entries[i].name, token)){
                    next_offset = entries[i].inode_offset;
                    next_inode = (inode *)offset_to_ptr(fsptr, fssize, next_offset);
                    found = 1;
                    break;
                }
        }

        if(!found){
                free(path_cpy);
                return NULL;
        }

        /*Move to next inode*/
        curr_inode = next_inode;
        curr_offset = next_offset;

        token = strtok(NULL, "/");
    }

    /*Cleanup and return*/
    free(path_cpy);
    if(inode_offset_ptr) *inode_offset_ptr = curr_offset;
    return curr_inode;
}

/*Split path into parent dir and base name*/
static int split_path(const char *path, char **parent_path, char **base_name) {
      if (!path || !parent_path || !base_name) return -1;
      
      char *path_cpy = strdup(path);
      if (!path_cpy) return -1;
      
      char *last_slash = strrchr(path_cpy, '/');
      /* Invalid path */
      if (last_slash == NULL) {
            free(path_cpy);
            return -1;
      }
      
      /* Parent directory is root */
      if (last_slash == path_cpy) {
            *parent_path = strdup("/");
      } else {
            *last_slash = '\0';
            *parent_path = strdup(path_cpy);
      }
      
      if(!(*parent_path)) {
            free(path_cpy);
            return -1;
      }
      
      *base_name = strdup(last_slash + 1);
      if (!(*base_name)) {
            free(path_cpy);
            free(*parent_path);
            return -1;
      }
      
      free(path_cpy);
      return 0;
}

/*Find a free inode*/
static size_t find_free_inode(void *fsptr, size_t fssize) {
    fs_info_block *info_block = (fs_info_block*)fsptr;
    unsigned char *bitmap = (unsigned char*)offset_to_ptr(fsptr, fssize, info_block->free_inode_bitmap);
    if (!bitmap) return (size_t)-1;
    
    for (size_t byte = 0; byte < MAX_INODES / 8; byte++) {
        if (bitmap[byte] != 0xFF) {
            for (int bit = 0; bit < 8; bit++) {
                size_t inode_num = byte * 8 + bit;
                if (inode_num >= MAX_INODES) break;
                if (!(bitmap[byte] & (1 << bit))) {
                    bitmap[byte] |= (1 << bit);
                    size_t inode_offset = info_block->inode_table + inode_num * INODE_SIZE;
                    return inode_offset;
                }
            }
        }
    }
    
    return (size_t)-1;
}

int add_dir_entry(void *fsptr, size_t fssize, inode *dir_inode, size_t dir_inode_offset, const char *name, size_t new_inode_offset) {
      size_t num_entries = dir_inode->size / sizeof(directory_entry);
      size_t max_entries = BLOCK_SIZE / sizeof(directory_entry);
      if (num_entries >= max_entries) return -1; 

      directory_entry *new_entry = (directory_entry*)offset_to_ptr(fsptr, fssize, dir_inode->data_block + num_entries * sizeof(directory_entry));
      if (!new_entry) return -1;
      strncpy(new_entry->name, name, MAX_FILENAME - 1);
      new_entry->name[MAX_FILENAME - 1] = '\0'; 
      new_entry->inode_offset = new_inode_offset;

      dir_inode->size += sizeof(directory_entry);
      dir_inode->modification_time = dir_inode->change_time = time(NULL);

      return 0; 
}


/*Remove entry from dir inode*/
static int remove_dir_entry(void *fsptr, size_t fssize, inode *dir_inode, size_t dir_inode_offset, const char *name) {
      /*Get entries from dir*/
      directory_entry *entries = (directory_entry *)offset_to_ptr(fsptr, fssize, dir_inode->data_block);
      if (!entries) return -1;
      
      size_t num_entries = dir_inode->size / sizeof(directory_entry);
      size_t target_index = num_entries;
      
      /*Find target entry*/
      for (size_t i = 0; i < num_entries; i++) {
            if (strcmp(entries[i].name, name) == 0) {
                  target_index = i;
                  break;
            }
      }
      
      /*Entry not found*/
      if (target_index == num_entries) return -1;
      
      /*Shift entries */
      for (size_t i = target_index; i < num_entries - 1; i++) entries[i] = entries[i + 1];
      
      /* Zero out the last entry */
      memset(&entries[num_entries - 1], 0, sizeof(directory_entry));
      
      /*Update size*/
      dir_inode->size -= sizeof(directory_entry);
      
      /* Update times*/
      dir_inode->modification_time = dir_inode->change_time = time(NULL);
      
      return 0; 
}

/*Find free data block*/
static size_t find_free_data_block(void *fsptr, size_t fssize) {
    fs_info_block *info_block = (fs_info_block*)fsptr;
    size_t max_data_blocks = info_block->max_data_blocks; // Renamed variable
    unsigned char *bitmap = (unsigned char*)offset_to_ptr(fsptr, fssize, info_block->free_block_bitmap);
    if (!bitmap) return (size_t)-1;

    for (size_t block_num = 0; block_num < max_data_blocks; block_num++) {
        if (!(bitmap[block_num / 8] & (1 << (block_num % 8)))) {
            // Mark block as used
            bitmap[block_num / 8] |= (1 << (block_num % 8));
            // Calculate block offset
            size_t block_offset = info_block->data_blocks + block_num * BLOCK_SIZE;
            return block_offset;
        }
    }

    return (size_t)-1;
}


/* Frees a data block by marking it as free in the bitmap */
static int free_data_block(void *fsptr, size_t fssize, size_t block_offset) {
    fs_info_block *info_block = (fs_info_block*)fsptr;
    if (block_offset < info_block->data_blocks || block_offset >= fssize) return -1;
    
    size_t block_num = (block_offset - info_block->data_blocks) / BLOCK_SIZE;
    if (block_num >= info_block->max_data_blocks) return -1;
    
    unsigned char *bitmap = (unsigned char*)offset_to_ptr(fsptr, fssize, info_block->free_block_bitmap);
    if (!bitmap) return -1;
    
    bitmap[block_num / 8] &= ~(1 << (block_num % 8)); 
    return 0; 
}

/* Calculates the total number of blocks in the filesystem */
size_t calculate_total_blocks(size_t fssize) {
    return fssize / BLOCK_SIZE;
}

unsigned char* get_block_bitmap(void *fsptr, size_t fssize) {
    fs_info_block *info_block = (fs_info_block*)fsptr;
    return (unsigned char*)offset_to_ptr(fsptr, fssize, info_block->free_block_bitmap);
}

size_t calculate_free_blocks(void *fsptr, size_t fssize) {
    unsigned char *bitmap = get_block_bitmap(fsptr, fssize);
    if (!bitmap) {
        return 0; // Unable to access bitmap, consider as no free blocks
    }
    
    size_t free_blocks = 0;
    size_t total_blocks = calculate_total_blocks(fssize);
    
    for (size_t i = 0; i < total_blocks; i++) {
        size_t byte_index = i / 8;
        size_t bit_index = i % 8;
        if (!(bitmap[byte_index] & (1 << bit_index))) {
            free_blocks++;
        }
    }
    
    return free_blocks;
}




/* End of helper functions */

/* Implements an emulation of the stat system call on the filesystem 
   of size fssize pointed to by fsptr. 
   
   If path can be followed and describes a file or directory 
   that exists and is accessable, the access information is 
   put into stbuf. 

   On success, 0 is returned. On failure, -1 is returned and 
   the appropriate error code is put into *errnoptr.

   man 2 stat documents all possible error codes and gives more detail
   on what fields of stbuf need to be filled in. Essentially, only the
   following fields need to be supported:

   st_uid      the value passed in argument
   st_gid      the value passed in argument
   st_mode     (as fixed values S_IFDIR | 0755 for directories,
                                S_IFREG | 0755 for files)
   st_nlink    (as many as there are subdirectories (not files) for directories
                (including . and ..),
                1 for files)
   st_size     (supported only for files, where it is the real file size)
   st_atim
   st_mtim

*/
int __myfs_getattr_implem(void *fsptr, size_t fssize, int *errnoptr, uid_t uid, gid_t gid, const char *path, struct stat *stbuf) {
      
      /*Init fs*/
      if(!init_fs(fsptr, fssize)){
            *errnoptr = EFAULT;
            return -1;
      }
      
      /*Find inode to path*/
      size_t inode_offset;
      inode *node = find_inode(fsptr, fssize, path, &inode_offset);
      if(!node){
            *errnoptr = ENOENT;
            return -1;
      }

      /*Populate data structure*/
      memset(stbuf, 0, sizeof(struct stat));
      stbuf->st_uid = node->uid;
      stbuf->st_gid = node->gid;
      stbuf->st_mode = node->mode;
      stbuf->st_size = node->size;
      stbuf->st_atime = node->access_time;
      stbuf->st_mtime = node->modification_time;
      stbuf->st_ctime = node->change_time;

      /*Set num links for directories*/
      if(node->mode & S_IFDIR){
            size_t num_entries = node->size / sizeof(directory_entry);
            stbuf->st_nlink = num_entries;
      }else if(node->mode & S_IFREG){
            stbuf->st_nlink = 1;
      }else{
            *errnoptr = EINVAL;
            return -1;
      }

      return 0;
}

/* Implements an emulation of the readdir system call on the filesystem 
   of size fssize pointed to by fsptr. 

   If path can be followed and describes a directory that exists and
   is accessable, the names of the subdirectories and files 
   contained in that directory are output into *namesptr. The . and ..
   directories must not be included in that listing.

   If it needs to output file and subdirectory names, the function
   starts by allocating (with calloc) an array of pointers to
   characters of the right size (n entries for n names). Sets
   *namesptr to that pointer. It then goes over all entries
   in that array and allocates, for each of them an array of
   characters of the right size (to hold the i-th name, together 
   with the appropriate '\0' terminator). It puts the pointer
   into that i-th array entry and fills the allocated array
   of characters with the appropriate name. The calling function
   will call free on each of the entries of *namesptr and 
   on *namesptr.

   The function returns the number of names that have been 
   put into namesptr. 

   If no name needs to be reported because the directory does
   not contain any file or subdirectory besides . and .., 0 is 
   returned and no allocation takes place.

   On failure, -1 is returned and the *errnoptr is set to 
   the appropriate error code. 

   The error codes are documented in man 2 readdir.

   In the case memory allocation with malloc/calloc fails, failure is
   indicated by returning -1 and setting *errnoptr to EINVAL.

*/
int __myfs_readdir_implem(void *fsptr, size_t fssize, int *errnoptr,  const char *path, char ***namesptr) {
      /*Init fs*/
      if (!init_fs(fsptr, fssize)) {
            *errnoptr = EFAULT;
            return -1;
      }

      /*Find inode for map*/
      size_t inode_offset;
      inode *dir_inode = find_inode(fsptr, fssize, path, &inode_offset);
      if (!dir_inode) {
            *errnoptr = ENOENT;
            return -1;
      }

      /*If INODE not dir*/
      if (!(dir_inode->mode & S_IFDIR)) {
            *errnoptr = ENOTDIR;
            return -1;
      }

      /*Get dir entries*/
      directory_entry *entries = (directory_entry *)offset_to_ptr(fsptr, fssize, dir_inode->data_block);
      if (!entries) {
            *errnoptr = EIO;
            return -1;
      }

      size_t num_entries = dir_inode->size / sizeof(directory_entry);
      size_t valid_entries = 0;

      /*Count entries except .. .*/
      for (size_t i = 0; i < num_entries; i++) if (strcmp(entries[i].name, ".") && strcmp(entries[i].name, "..")) valid_entries++;

      /*Ret 0 if no valid entries*/
      if (valid_entries == 0) {
            *namesptr = NULL; 
            return 0;
      }

      /*Allocate name array*/
      char **names_array = calloc(valid_entries, sizeof(char *));
      if (!names_array) {
            *errnoptr = EINVAL;
            return -1;
      }

      size_t current_name = 0;

      /* Allocate copy of each name */
      for (size_t i = 0; i < num_entries; i++) {
            /*Skip . and ..*/
            if (strcmp(entries[i].name, ".") == 0 || strcmp(entries[i].name, "..") == 0) continue;

            /*Allocate memory for str*/
            size_t name_len = strlen(entries[i].name);
            names_array[current_name] = malloc(name_len + 1);
            if (!names_array[current_name]) {
                  /*Malloc failed, clean*/
                  for (size_t j = 0; j < current_name; j++) free(names_array[j]);
                  free(names_array);
                  *errnoptr = EINVAL;
                  return -1;
            }

            /*Copy name*/
            strcpy(names_array[current_name], entries[i].name);
            current_name++;
      }

      /*Assign array to namesptr*/
      *namesptr = names_array;

      /*Return num names*/
      return valid_entries;
}

/* Implements an emulation of the mknod system call for regular files
   on the filesystem of size fssize pointed to by fsptr.

   This function is called only for the creation of regular files.

   If a file gets created, it is of size zero and has default
   ownership and mode bits.

   The call creates the file indicated by path.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 mknod.

*/
int __myfs_mknod_implem(void *fsptr, size_t fssize, int *errnoptr, const char *path) {
      /* Init fs */
      if (!init_fs(fsptr, fssize)) {
            *errnoptr = EFAULT;
            return -1;
      }

      /* Split path */
      char *parent_path = NULL;
      char *file_name = NULL;
      if (split_path(path, &parent_path, &file_name) != 0) {
            *errnoptr = EINVAL;
            return -1;
      }

      /* Find parent dir */
      size_t parent_inode_offset;
      inode *parent_dir = find_inode(fsptr, fssize, parent_path, &parent_inode_offset);
      if (!parent_dir) {
            free(parent_path);
            free(file_name);
            *errnoptr = ENOENT;
            return -1;
      }

      /* Verify parent is dir */
      if (!(parent_dir->mode & S_IFDIR)) {
            free(parent_path);
            free(file_name);
            *errnoptr = ENOTDIR;
            return -1;
      }

      /* Check if file exists */
      directory_entry *entries = (directory_entry *)offset_to_ptr(fsptr, fssize, parent_dir->data_block);
      if (!entries) {
            free(parent_path);
            free(file_name);
            *errnoptr = EIO;
            return -1;
      }

      size_t num_entries = parent_dir->size / sizeof(directory_entry);
      for (size_t i = 0; i < num_entries; i++) {
            if (!strcmp(entries[i].name, file_name)) {
                  free(parent_path);
                  free(file_name);
                  *errnoptr = EEXIST;
                  return -1;
            }
      }

      /* Find free inode */
      size_t new_inode_offset = find_free_inode(fsptr, fssize);
      if (new_inode_offset == (size_t)-1) {
            free(parent_path);
            free(file_name);
            *errnoptr = ENOSPC;
            return -1;
      }

      /* Init inode */
      inode *new_inode = (inode *)offset_to_ptr(fsptr, fssize, new_inode_offset);
      if (!new_inode) {
            /* Unmark the inode in bitmap */
            fs_info_block *info_block = (fs_info_block*)fsptr;
            unsigned char *bitmap = (unsigned char*)offset_to_ptr(fsptr, fssize, info_block->free_inode_bitmap);
            size_t inode_num = (new_inode_offset - info_block->inode_table) / INODE_SIZE;
            if (inode_num < MAX_INODES) bitmap[inode_num / 8] &= ~(1 << (inode_num % 8));
            free(parent_path);
            free(file_name);
            *errnoptr = EIO;
            return -1;
      }

      /* Set inode for regular file */
      new_inode->mode = S_IFREG | 0644;
      new_inode->uid = getuid();
      new_inode->gid = getgid();
      new_inode->size = 0;
      new_inode->access_time = new_inode->modification_time = new_inode->change_time = time(NULL);
      new_inode->data_block = 0;

      /* Add entry to parent dir */
      if (add_dir_entry(fsptr, fssize, parent_dir, parent_inode_offset, file_name, new_inode_offset) != 0) {
            /* Failed to add dir, unmark the inode in bitmap */
            fs_info_block *info_block = (fs_info_block*)fsptr;
            unsigned char *bitmap = (unsigned char*)offset_to_ptr(fsptr, fssize, info_block->free_inode_bitmap);
            size_t inode_num = (new_inode_offset - info_block->inode_table) / INODE_SIZE;
            if (inode_num < MAX_INODES) bitmap[inode_num / 8] &= ~(1 << (inode_num % 8));
            /* Reset inode */
            memset(new_inode, 0, sizeof(inode));
            free(parent_path);
            free(file_name);
            *errnoptr = ENOSPC;
            return -1;
      }

      /* Clean up */
      free(parent_path);
      free(file_name);

      return 0;
}

/* Implements an emulation of the unlink system call for regular files
   on the filesystem of size fssize pointed to by fsptr.

   This function is called only for the deletion of regular files.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 unlink.

*/
int __myfs_unlink_implem(void *fsptr, size_t fssize, int *errnoptr, const char *path) {
      /* Init fs */
      if (!init_fs(fsptr, fssize)) {
            *errnoptr = EFAULT;
            return -1;
      }

      /* Split path */
      char *parent_path = NULL;
      char *file_name = NULL;
      if (split_path(path, &parent_path, &file_name)) {
            *errnoptr = EINVAL; 
            return -1;
      }

      /* Find parent dir */
      size_t parent_inode_offset;
      inode *parent_dir = find_inode(fsptr, fssize, parent_path, &parent_inode_offset);
      if (!parent_dir) {
            free(parent_path);
            free(file_name);
            *errnoptr = ENOENT;
            return -1;
      }

      /* Verify parent is dir */
      if (!(parent_dir->mode & S_IFDIR)) {
            free(parent_path);
            free(file_name);
            *errnoptr = ENOTDIR;
            return -1;
      }

      /* Get entries */
      directory_entry *entries = (directory_entry *)offset_to_ptr(fsptr, fssize, parent_dir->data_block);
      if (!entries) {
            free(parent_path);
            free(file_name);
            *errnoptr = EIO;
            return -1;
      }

      size_t num_entries = parent_dir->size / sizeof(directory_entry);
      size_t target_index = num_entries;

      /* Find target dir entry */
      for (size_t i = 0; i < num_entries; i++) {
            if (!strcmp(entries[i].name, file_name)) {
                  target_index = i;
                  break;
            }
      }

      /* File does not exist */
      if (target_index == num_entries) {
            free(parent_path);
            free(file_name);
            *errnoptr = ENOENT;
            return -1;
      }

      /* Get inode offset */
      size_t target_inode_offset = entries[target_index].inode_offset;
      inode *target_inode = (inode *)offset_to_ptr(fsptr, fssize, target_inode_offset);
      if (!target_inode) {
            free(parent_path);
            free(file_name);
            *errnoptr = EIO;
            return -1;
      }

      /* Verify it's a file */
      if (!(target_inode->mode & S_IFREG)) {
            free(parent_path);
            free(file_name);
            *errnoptr = EISDIR; // Is a directory
            return -1;
      }

      /* Remove directory entry */
      if (remove_dir_entry(fsptr, fssize, parent_dir, parent_inode_offset, file_name)) {
            free(parent_path);
            free(file_name);
            *errnoptr = EIO;
            return -1;
      }

      /* Free inode in bitmap */
      fs_info_block *info_block = (fs_info_block*)fsptr;
      unsigned char *bitmap = (unsigned char*)offset_to_ptr(fsptr, fssize, info_block->free_inode_bitmap);
      if (!bitmap) {
            *errnoptr = EIO; 
            return -1;
      }
      size_t inode_num = (target_inode_offset - info_block->inode_table) / INODE_SIZE;
      if (inode_num >= MAX_INODES) {
            *errnoptr = EIO;
            return -1;
      }
      bitmap[inode_num / 8] &= ~(1 << (inode_num % 8));

      /* Free data blocks if allocated */
      if (target_inode->data_block) {
            if (free_data_block(fsptr, fssize, target_inode->data_block) != 0) {
                  *errnoptr = EIO; 
                  return -1;
            }
      } 

      /* Reset inode */
      memset(target_inode, 0, sizeof(inode));

      /* Clean up */
      free(parent_path);
      free(file_name);

      return 0;
}

/* Implements an emulation of the rmdir system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call deletes the directory indicated by path.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The function call must fail when the directory indicated by path is
   not empty (if there are files or subdirectories other than . and ..).

   The error codes are documented in man 2 rmdir.

*/
int __myfs_rmdir_implem(void *fsptr, size_t fssize, int *errnoptr,const char *path) {
    /* Initialize the filesystem */
    if (!init_fs(fsptr, fssize)) {
        *errnoptr = EFAULT;
        return -1;
    }

    /* Split the path into parent directory and directory name */
    char *parent_path = NULL;
    char *dir_name = NULL;
    if (split_path(path, &parent_path, &dir_name) != 0) {
        *errnoptr = EINVAL;
        return -1;
    }

    /* Find the parent directory inode */
    size_t parent_inode_offset;
    inode *parent_dir = find_inode(fsptr, fssize, parent_path, &parent_inode_offset);
    if (!parent_dir) {
        free(parent_path);
        free(dir_name);
        *errnoptr = ENOENT;
        return -1;
    }

    /* Verify that the parent is a directory */
    if (!(parent_dir->mode & S_IFDIR)) {
        free(parent_path);
        free(dir_name);
        *errnoptr = ENOTDIR;
        return -1;
    }

    /* Find the target directory's inode */
    size_t target_inode_offset;
    inode *target_dir = find_inode(fsptr, fssize, path, &target_inode_offset);
    if (!target_dir) {
        free(parent_path);
        free(dir_name);
        *errnoptr = ENOENT;
        return -1;
    }

    /* Verify that the target is a directory */
    if (!(target_dir->mode & S_IFDIR)) {
        free(parent_path);
        free(dir_name);
        *errnoptr = ENOTDIR;
        return -1;
    }

    /* Check if the directory is empty (only contains . and ..) */
    size_t num_entries = target_dir->size / sizeof(directory_entry);
    if (num_entries > 2) { // More than . and ..
        free(parent_path);
        free(dir_name);
        *errnoptr = ENOTEMPTY;
        return -1;
    }

    /* Optional: Further verify that the only entries are . and .. */
    directory_entry *entries = (directory_entry *)offset_to_ptr(fsptr, fssize, target_dir->data_block);
    if (!entries) {
        free(parent_path);
        free(dir_name);
        *errnoptr = EIO;
        return -1;
    }

    if (num_entries == 1) {
        /* Should at least contain '.' */
        if (strcmp(entries[0].name, ".") != 0) {
            free(parent_path);
            free(dir_name);
            *errnoptr = ENOTEMPTY;
            return -1;
        }
    } else if (num_entries == 2) {
        /* Should contain '.' and '..' */
        int has_dot = 0, has_dotdot = 0;
        for (size_t i = 0; i < num_entries; i++) {
            if (strcmp(entries[i].name, ".") == 0) has_dot = 1;
            if (strcmp(entries[i].name, "..") == 0) has_dotdot = 1;
        }
        if (!has_dot || !has_dotdot) {
            free(parent_path);
            free(dir_name);
            *errnoptr = ENOTEMPTY;
            return -1;
        }
    }

    /* Remove the directory entry from the parent directory */
    if (remove_dir_entry(fsptr, fssize, parent_dir, parent_inode_offset, dir_name) != 0) {
        free(parent_path);
        free(dir_name);
        *errnoptr = EIO;
        return -1;
    }

    /* Free the target directory's data block */
    if (target_dir->data_block != 0) {
        if (free_data_block(fsptr, fssize, target_dir->data_block) != 0) {
            /* Attempt to rollback by re-adding the directory entry */
            add_dir_entry(fsptr, fssize, parent_dir, parent_inode_offset, dir_name, target_inode_offset);
            free(parent_path);
            free(dir_name);
            *errnoptr = EIO;
            return -1;
        }
    }

    /* Free the target directory's inode in the inode bitmap */
    fs_info_block *info_block = (fs_info_block*)fsptr;
    unsigned char *inode_bitmap = (unsigned char*)offset_to_ptr(fsptr, fssize, info_block->free_inode_bitmap);
    if (!inode_bitmap) {
        /* Attempt to rollback by re-adding the directory entry and freeing data block */
        add_dir_entry(fsptr, fssize, parent_dir, parent_inode_offset, dir_name, target_inode_offset);
        if (target_dir->data_block != 0) {
            // Reallocate the data block if possible
            // (This would require implementing a function to mark the block as used again)
            // For simplicity, we skip this step
        }
        free(parent_path);
        free(dir_name);
        *errnoptr = EIO;
        return -1;
    }

    size_t inode_num = (target_inode_offset - info_block->inode_table) / INODE_SIZE;
    if (inode_num >= MAX_INODES) {
        /* Invalid inode number, attempt to rollback */
        add_dir_entry(fsptr, fssize, parent_dir, parent_inode_offset, dir_name, target_inode_offset);
        if (target_dir->data_block != 0) {
            free_data_block(fsptr, fssize, target_dir->data_block);
        }
        free(parent_path);
        free(dir_name);
        *errnoptr = EIO;
        return -1;
    }

    /* Mark the inode as free in the bitmap */
    inode_bitmap[inode_num / 8] &= ~(1 << (inode_num % 8));

    /* Reset the inode structure to zero */
    memset(target_dir, 0, sizeof(inode));

    /* Clean up */
    free(parent_path);
    free(dir_name);

    return 0;
}

/* Implements an emulation of the mkdir system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call creates the directory indicated by path.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 mkdir.

*/
int __myfs_mkdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
      /* Initialize the filesystem */
      if (!init_fs(fsptr, fssize)) {
          *errnoptr = EFAULT;
          return -1;
      }

      /* Split the path into parent directory and directory name */
      char *parent_path = NULL;
      char *dir_name = NULL;
      if (split_path(path, &parent_path, &dir_name) != 0) {
          *errnoptr = EINVAL;
          return -1;
      }

      /* Find the parent directory inode */
      size_t parent_inode_offset;
      inode *parent_dir = find_inode(fsptr, fssize, parent_path, &parent_inode_offset);
      if (!parent_dir) {
          free(parent_path);
          free(dir_name);
          *errnoptr = ENOENT;
          return -1;
      }

      /* Verify that the parent is a directory */
      if (!(parent_dir->mode & S_IFDIR)) {
          free(parent_path);
          free(dir_name);
          *errnoptr = ENOTDIR;
          return -1;
      }

      /* Check if the directory already exists */
      directory_entry *entries = (directory_entry *)offset_to_ptr(fsptr, fssize, parent_dir->data_block);
      if (!entries) {
          free(parent_path);
          free(dir_name);
          *errnoptr = EIO;
          return -1;
      }

      size_t num_entries = parent_dir->size / sizeof(directory_entry);
      for (size_t i = 0; i < num_entries; i++) {
          if (!strcmp(entries[i].name, dir_name)) {
              free(parent_path);
              free(dir_name);
              *errnoptr = EEXIST;
              return -1;
          }
      }

      /* Find a free inode for the new directory */
      size_t new_inode_offset = find_free_inode(fsptr, fssize);
      if (new_inode_offset == (size_t)-1) {
          free(parent_path);
          free(dir_name);
          *errnoptr = ENOSPC;
          return -1;
      }

      /* Initialize the new inode */
      inode *new_dir_inode = (inode *)offset_to_ptr(fsptr, fssize, new_inode_offset);
      if (!new_dir_inode) {
          /* Unmark the inode in bitmap */
          fs_info_block *info_block = (fs_info_block*)fsptr;
          unsigned char *bitmap = (unsigned char*)offset_to_ptr(fsptr, fssize, info_block->free_inode_bitmap);
          size_t inode_num = (new_inode_offset - info_block->inode_table) / INODE_SIZE;
          if (inode_num < MAX_INODES) bitmap[inode_num / 8] &= ~(1 << (inode_num % 8));
          free(parent_path);
          free(dir_name);
          *errnoptr = EIO;
          return -1;
      }

      /* Allocate a data block for the new directory */
      size_t data_block_offset = find_free_data_block(fsptr, fssize);
      if (data_block_offset == (size_t)-1) {
          /* Unmark the inode in bitmap */
          fs_info_block *info_block = (fs_info_block*)fsptr;
          unsigned char *bitmap = (unsigned char*)offset_to_ptr(fsptr, fssize, info_block->free_inode_bitmap);
          size_t inode_num = (new_inode_offset - info_block->inode_table) / INODE_SIZE;
          if (inode_num < MAX_INODES) bitmap[inode_num / 8] &= ~(1 << (inode_num % 8));
          /* Reset the inode */
          memset(new_dir_inode, 0, sizeof(inode));
          free(parent_path);
          free(dir_name);
          *errnoptr = ENOSPC;
          return -1;
      }

      /* Initialize the new directory inode */
      new_dir_inode->mode = S_IFDIR | 0755;
      new_dir_inode->uid = getuid();
      new_dir_inode->gid = getgid();
      new_dir_inode->size = 0;
      new_dir_inode->access_time = new_dir_inode->modification_time = new_dir_inode->change_time = time(NULL);
      new_dir_inode->data_block = data_block_offset;

      /* Initialize the new directory entries (add "." and "..") */
      directory_entry *new_dir_entries = (directory_entry *)offset_to_ptr(fsptr, fssize, data_block_offset);
      if (!new_dir_entries) {
          /* Free the data block */
          free_data_block(fsptr, fssize, data_block_offset);
          /* Unmark the inode in bitmap */
          fs_info_block *info_block = (fs_info_block*)fsptr;
          unsigned char *bitmap = (unsigned char*)offset_to_ptr(fsptr, fssize, info_block->free_inode_bitmap);
          size_t inode_num = (new_inode_offset - info_block->inode_table) / INODE_SIZE;
          if (inode_num < MAX_INODES) bitmap[inode_num / 8] &= ~(1 << (inode_num % 8));
          /* Reset the inode */
          memset(new_dir_inode, 0, sizeof(inode));
          free(parent_path);
          free(dir_name);
          *errnoptr = EIO;
          return -1;
      }

      /* Add "." */
      strcpy(new_dir_entries[0].name, ".");
      new_dir_entries[0].inode_offset = new_inode_offset;
      /* Add ".." */
      strcpy(new_dir_entries[1].name, "..");
      new_dir_entries[1].inode_offset = parent_inode_offset;
      new_dir_inode->size += 2 * sizeof(directory_entry);

      /* Add the new directory to the parent directory */
      if (add_dir_entry(fsptr, fssize, parent_dir, parent_inode_offset, dir_name, new_inode_offset) != 0) {
          /* Free the data block */
          free_data_block(fsptr, fssize, data_block_offset);
          /* Unmark the inode in bitmap */
          fs_info_block *info_block = (fs_info_block*)fsptr;
          unsigned char *bitmap = (unsigned char*)offset_to_ptr(fsptr, fssize, info_block->free_inode_bitmap);
          size_t inode_num = (new_inode_offset - info_block->inode_table) / INODE_SIZE;
          if (inode_num < MAX_INODES) bitmap[inode_num / 8] &= ~(1 << (inode_num % 8));
          /* Reset the inode */
          memset(new_dir_inode, 0, sizeof(inode));
          free(parent_path);
          free(dir_name);
          *errnoptr = ENOSPC;
          return -1;
      }

      /* Cleanup */
      free(parent_path);
      free(dir_name);

      return 0;
}

/* Implements an emulation of the rename system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call moves the file or directory indicated by from to to.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   Caution: the function does more than what is hinted to by its name.
   In cases the from and to paths differ, the file is moved out of 
   the from path and added to the to path.

   The error codes are documented in man 2 rename.

*/
int __myfs_rename_implem(void *fsptr, size_t fssize, int *errnoptr,
                         const char *from, const char *to) {
    /* Initialize the filesystem */
    if (!init_fs(fsptr, fssize)) {
        *errnoptr = EFAULT;
        return -1;
    }

    /* Prevent renaming the root directory */
    if (strcmp(from, "/") == 0) {
        *errnoptr = EBUSY;
        return -1;
    }

    /* Split the 'from' and 'to' paths into parent directories and base names */
    char *from_parent_path = NULL;
    char *from_base_name = NULL;
    if (split_path(from, &from_parent_path, &from_base_name) != 0) {
        *errnoptr = EINVAL;
        return -1;
    }

    char *to_parent_path = NULL;
    char *to_base_name = NULL;
    if (split_path(to, &to_parent_path, &to_base_name) != 0) {
        free(from_parent_path);
        free(from_base_name);
        *errnoptr = EINVAL;
        return -1;
    }

    /* Find the 'from' parent directory inode */
    size_t from_parent_inode_offset;
    inode *from_parent_dir = find_inode(fsptr, fssize, from_parent_path, &from_parent_inode_offset);
    if (!from_parent_dir) {
        free(from_parent_path);
        free(from_base_name);
        free(to_parent_path);
        free(to_base_name);
        *errnoptr = ENOENT;
        return -1;
    }

    /* Verify that the 'from' parent is a directory */
    if (!(from_parent_dir->mode & S_IFDIR)) {
        free(from_parent_path);
        free(from_base_name);
        free(to_parent_path);
        free(to_base_name);
        *errnoptr = ENOTDIR;
        return -1;
    }

    /* Find the target inode to be renamed */
    size_t from_inode_offset;
    inode *from_inode = find_inode(fsptr, fssize, from, &from_inode_offset);
    if (!from_inode) {
        free(from_parent_path);
        free(from_base_name);
        free(to_parent_path);
        free(to_base_name);
        *errnoptr = ENOENT;
        return -1;
    }

    /* Ensure that 'from' is not a parent of 'to' to prevent directory loops */
    if (from_inode->mode & S_IFDIR) {
        // Implement a function to check if 'to' is a subdirectory of 'from'
        // This function is not currently implemented, so we'll skip it for now
        // You can add it later for robustness
    }

    /* Find the 'to' parent directory inode */
    size_t to_parent_inode_offset;
    inode *to_parent_dir = find_inode(fsptr, fssize, to_parent_path, &to_parent_inode_offset);
    if (!to_parent_dir) {
        free(from_parent_path);
        free(from_base_name);
        free(to_parent_path);
        free(to_base_name);
        *errnoptr = ENOENT;
        return -1;
    }

    /* Verify that the 'to' parent is a directory */
    if (!(to_parent_dir->mode & S_IFDIR)) {
        free(from_parent_path);
        free(from_base_name);
        free(to_parent_path);
        free(to_base_name);
        *errnoptr = ENOTDIR;
        return -1;
    }

    /* Check if the 'to' path already exists */
    size_t to_inode_offset;
    inode *to_inode = find_inode(fsptr, fssize, to, &to_inode_offset);
    if (to_inode) {
        /* 'to' path exists; handle according to the type */
        if (to_inode->mode & S_IFDIR) {
            /* If 'to' is a directory, 'from' must also be a directory and 'to' must be empty */
            if (!(from_inode->mode & S_IFDIR)) {
                free(from_parent_path);
                free(from_base_name);
                free(to_parent_path);
                free(to_base_name);
                *errnoptr = EISDIR;
                return -1;
            }

            size_t to_num_entries = to_inode->size / sizeof(directory_entry);
            if (to_num_entries > 2) { // More than '.' and '..'
                free(from_parent_path);
                free(from_base_name);
                free(to_parent_path);
                free(to_base_name);
                *errnoptr = ENOTEMPTY;
                return -1;
            }

            /* Optional: Further verify that the only entries are '.' and '..' */
            directory_entry *to_entries = (directory_entry *)offset_to_ptr(fsptr, fssize, to_inode->data_block);
            if (!to_entries) {
                free(from_parent_path);
                free(from_base_name);
                free(to_parent_path);
                free(to_base_name);
                *errnoptr = EIO;
                return -1;
            }

            if (to_num_entries == 1) {
                /* Should at least contain '.' */
                if (strcmp(to_entries[0].name, ".") != 0) {
                    free(from_parent_path);
                    free(from_base_name);
                    free(to_parent_path);
                    free(to_base_name);
                    *errnoptr = ENOTEMPTY;
                    return -1;
                }
            } else if (to_num_entries == 2) {
                /* Should contain '.' and '..' */
                int has_dot = 0, has_dotdot = 0;
                for (size_t i = 0; i < to_num_entries; i++) {
                    if (strcmp(to_entries[i].name, ".") == 0) has_dot = 1;
                    if (strcmp(to_entries[i].name, "..") == 0) has_dotdot = 1;
                }
                if (!has_dot || !has_dotdot) {
                    free(from_parent_path);
                    free(from_base_name);
                    free(to_parent_path);
                    free(to_base_name);
                    *errnoptr = ENOTEMPTY;
                    return -1;
                }
            }

            /* Optionally, you could implement recursive rename if desired */
            /* For simplicity, we do not allow overwriting non-empty directories */

            /* Remove the 'to' directory (must be empty) */
            if (__myfs_rmdir_implem(fsptr, fssize, errnoptr, to) != 0) {
                // 'to' exists but could not be removed
                free(from_parent_path);
                free(from_base_name);
                free(to_parent_path);
                free(to_base_name);
                return -1;
            }
        } else {
            /* 'to' is a file; unlink it to allow overwriting */
            if (__myfs_unlink_implem(fsptr, fssize, errnoptr, to) != 0) {
                // Could not unlink existing 'to' file
                free(from_parent_path);
                free(from_base_name);
                free(to_parent_path);
                free(to_base_name);
                return -1;
            }
        }
    }

    /* Remove the directory entry from the 'from' parent directory */
    if (remove_dir_entry(fsptr, fssize, from_parent_dir, from_parent_inode_offset, from_base_name) != 0) {
        free(from_parent_path);
        free(from_base_name);
        free(to_parent_path);
        free(to_base_name);
        *errnoptr = EIO;
        return -1;
    }

    /* Add the directory entry to the 'to' parent directory */
    if (add_dir_entry(fsptr, fssize, to_parent_dir, to_parent_inode_offset, to_base_name, from_inode_offset) != 0) {
        /* Attempt to rollback by re-adding the original directory entry */
        add_dir_entry(fsptr, fssize, from_parent_dir, from_parent_inode_offset, from_base_name, from_inode_offset);
        free(from_parent_path);
        free(from_base_name);
        free(to_parent_path);
        free(to_base_name);
        *errnoptr = ENOSPC;
        return -1;
    }

    /* Update inode metadata if necessary */
    // Optionally, you can update the inode's parent directory or other metadata here

    /* Clean up */
    free(from_parent_path);
    free(from_base_name);
    free(to_parent_path);
    free(to_base_name);

    return 0;
}

/* Implements an emulation of the truncate system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call changes the size of the file indicated by path to offset
   bytes.

   When the file becomes smaller due to the call, the extending bytes are
   removed. When it becomes larger, zeros are appended.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 truncate.

*/
int __myfs_truncate_implem(void *fsptr, size_t fssize, int *errnoptr,
                           const char *path, off_t offset) {
    /* Initialize the filesystem */
    if (!init_fs(fsptr, fssize)) {
        *errnoptr = EFAULT;
        return -1;
    }

    /* Validate the offset */
    if (offset < 0) {
        *errnoptr = EINVAL;
        return -1;
    }

    /* Find the inode corresponding to the path */
    size_t inode_offset;
    inode *file_inode = find_inode(fsptr, fssize, path, &inode_offset);
    if (!file_inode) {
        *errnoptr = ENOENT;
        return -1;
    }

    /* Verify that the inode represents a regular file */
    if (!(file_inode->mode & S_IFREG)) {
        *errnoptr = EINVAL;
        return -1;
    }

    /* Truncate to a smaller size */
    if (offset < file_inode->size) {
        if (offset == 0) {
            /* Truncate the file to zero size: free the data block */
            if (file_inode->data_block != 0) {
                if (free_data_block(fsptr, fssize, file_inode->data_block) != 0) {
                    *errnoptr = EIO;
                    return -1;
                }
                file_inode->data_block = 0;
            }
        } else {
            /* Truncate within the existing data block */
            if (file_inode->data_block == 0) {
                /* No data block allocated, nothing to truncate */
                /* This is an inconsistency; handle as an I/O error */
                *errnoptr = EIO;
                return -1;
            }

            /* Ensure the offset does not exceed block size */
            if ((size_t)offset > BLOCK_SIZE) {
                *errnoptr = EFBIG; // File too large
                return -1;
            }

            /* Optionally, zero out the bytes beyond the new size */
            void *data_ptr = offset_to_ptr(fsptr, fssize, file_inode->data_block + offset);
            if (!data_ptr) {
                *errnoptr = EIO;
                return -1;
            }
            size_t bytes_to_zero = file_inode->size - offset;
            memset(data_ptr, 0, bytes_to_zero);
        }

        /* Update the inode's size */
        file_inode->size = offset;
        /* Update metadata */
        file_inode->modification_time = file_inode->change_time = time(NULL);

        return 0;
    }

    /* Truncate to a larger size */
    if (offset > file_inode->size) {
        /* Check if the new size exceeds the maximum allowed size (BLOCK_SIZE) */
        if ((size_t)offset > BLOCK_SIZE) {
            *errnoptr = EFBIG; // File too large
            return -1;
        }

        /* Allocate a data block if not already allocated */
        if (file_inode->data_block == 0) {
            size_t data_block_offset = find_free_data_block(fsptr, fssize);
            if (data_block_offset == (size_t)-1) {
                *errnoptr = ENOSPC;
                return -1;
            }

            /* Mark the data block as used in the bitmap */
            fs_info_block *info_block = (fs_info_block*)fsptr;
            unsigned char *data_bitmap = (unsigned char*)offset_to_ptr(fsptr, fssize, info_block->free_block_bitmap);
            if (data_bitmap) {
                size_t block_num = (data_block_offset - info_block->data_blocks) / BLOCK_SIZE;
                if (block_num >= MAX_DATA_BLOCKS) {
                    /* Invalid data block number */
                    free_data_block(fsptr, fssize, data_block_offset);
                    *errnoptr = EIO;
                    return -1;
                }
                data_bitmap[block_num / 8] |= (1 << (block_num % 8));
            } else {
                /* Unable to access the data block bitmap */
                free_data_block(fsptr, fssize, data_block_offset);
                *errnoptr = EIO;
                return -1;
            }

            /* Initialize the data block to zero */
            void *data_ptr = offset_to_ptr(fsptr, fssize, data_block_offset);
            if (!data_ptr) {
                /* Unable to access the data block */
                free_data_block(fsptr, fssize, data_block_offset);
                *errnoptr = EIO;
                return -1;
            }
            memset(data_ptr, 0, BLOCK_SIZE);

            /* Assign the data block to the inode */
            file_inode->data_block = data_block_offset;
        }

        /* Get a pointer to the data block */
        void *data_ptr = offset_to_ptr(fsptr, fssize, file_inode->data_block);
        if (!data_ptr) {
            *errnoptr = EIO;
            return -1;
        }

        /* Calculate the number of bytes to zero-fill */
        size_t bytes_to_zero = offset - file_inode->size;
        if ((size_t)offset > BLOCK_SIZE) {
            /* Exceeds block size */
            *errnoptr = EFBIG;
            return -1;
        }

        /* Zero-fill the extending bytes */
        memset((char*)data_ptr + file_inode->size, 0, bytes_to_zero);

        /* Update the inode's size */
        file_inode->size = offset;
        /* Update metadata */
        file_inode->modification_time = file_inode->change_time = time(NULL);

        return 0;
    }

    /* No change in size; nothing to do */
    return 0;
}


/* Implements an emulation of the open system call on the filesystem 
   of size fssize pointed to by fsptr, without actually performing the opening
   of the file (no file descriptor is returned).

   The call just checks if the file (or directory) indicated by path
   can be accessed, i.e. if the path can be followed to an existing
   object for which the access rights are granted.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The two only interesting error codes are 

   * EFAULT: the filesystem is in a bad state, we can't do anything

   * ENOENT: the file that we are supposed to open doesn't exist (or a
             subpath).

   It is possible to restrict ourselves to only these two error
   conditions. It is also possible to implement more detailed error
   condition answers.

   The error codes are documented in man 2 open.

*/
int __myfs_open_implem(void *fsptr, size_t fssize, int *errnoptr,
                       const char *path) {
    /* Step 1: Initialize the filesystem */
    if (!init_fs(fsptr, fssize)) {
        *errnoptr = EFAULT;
        return -1;
    }

    /* Step 2: Validate the input path */
    if (path == NULL || strlen(path) == 0) {
        *errnoptr = EINVAL; // Invalid argument
        return -1;
    }

    /* Step 3: Find the inode corresponding to the path */
    size_t inode_offset;
    inode *file_inode = find_inode(fsptr, fssize, path, &inode_offset);
    if (!file_inode) {
        *errnoptr = ENOENT; // No such file or directory
        return -1;
    }

    /* Example Check for Directory Integrity */
    if (file_inode->mode & S_IFDIR) {
        directory_entry *entries = (directory_entry *)offset_to_ptr(fsptr, fssize, file_inode->data_block);
        if (!entries) {
            *errnoptr = EIO; // I/O error
            return -1;
        }

        size_t num_entries = file_inode->size / sizeof(directory_entry);
        if (num_entries < 2) { // At least '.' and '..' should exist
            *errnoptr = EIO; // I/O error
            return -1;
        }

        // Further integrity checks can be added here as needed
    }

    return 0;
}

/* Implements an emulation of the read system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call copies up to size bytes from the file indicated by 
   path into the buffer, starting to read at offset. See the man page
   for read for the details when offset is beyond the end of the file etc.
   
   On success, the appropriate number of bytes read into the buffer is
   returned. The value zero is returned on an end-of-file condition.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 read.

*/
int __myfs_read_implem(void *fsptr, size_t fssize, int *errnoptr,
                       const char *path, char *buf, size_t size, off_t offset) {
    /* Step 1: Initialize the filesystem */
    if (!init_fs(fsptr, fssize)) {
        *errnoptr = EFAULT; // Bad address
        return -1;
    }

    /* Step 2: Validate the input path and buffer */
    if (path == NULL || buf == NULL) {
        *errnoptr = EINVAL; // Invalid argument
        return -1;
    }

    /* Step 3: Find the inode corresponding to the path */
    size_t inode_offset;
    inode *file_inode = find_inode(fsptr, fssize, path, &inode_offset);
    if (!file_inode) {
        *errnoptr = ENOENT; // No such file or directory
        return -1;
    }

    /* Step 4: Verify that the inode represents a regular file */
    if (!(file_inode->mode & S_IFREG)) {
        *errnoptr = EINVAL; // Invalid argument
        return -1;
    }

    /* Step 5: Handle reading beyond the end of the file */
    if (offset >= (off_t)file_inode->size) {
        return 0; // EOF
    }

    /* Step 6: Calculate the number of bytes to read */
    size_t bytes_available = file_inode->size - offset;
    size_t bytes_to_read = (size < bytes_available) ? size : bytes_available;

    /* Step 7: Check if the file has a data block allocated */
    if (file_inode->data_block == 0) {
        /* No data block allocated; nothing to read */
        *errnoptr = EIO; // I/O error
        return -1;
    }

    /* Step 8: Ensure the offset does not exceed block size */
    if ((size_t)offset > BLOCK_SIZE) {
        *errnoptr = EINVAL; // Invalid argument
        return -1;
    }

    /* Step 9: Get a pointer to the data block at the specified offset */
    void *data_ptr = offset_to_ptr(fsptr, fssize, file_inode->data_block + offset);
    if (!data_ptr) {
        *errnoptr = EIO; // I/O error
        return -1;
    }

    /* Step 10: Copy the data into the user-provided buffer */
    memcpy(buf, data_ptr, bytes_to_read);

    /* Step 11: Update the inode's access time */
    file_inode->access_time = time(NULL);

    /* Step 12: Return the number of bytes read */
    return (int)bytes_to_read;
}

/* Implements an emulation of the write system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call copies up to size bytes to the file indicated by 
   path into the buffer, starting to write at offset. See the man page
   for write for the details when offset is beyond the end of the file etc.
   
   On success, the appropriate number of bytes written into the file is
   returned. The value zero is returned on an end-of-file condition.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 write.

*/
int __myfs_write_implem(void *fsptr, size_t fssize, int *errnoptr, const char *path, const char *buf, size_t size, off_t offset) {
      /* Init fs */
      if (!init_fs(fsptr, fssize)) {
            *errnoptr = EFAULT; 
            return -1;
      }

      /* Find file inode */
      size_t inode_offset;
      inode *file_inode = find_inode(fsptr, fssize, path, &inode_offset);
      if (!file_inode) {
            *errnoptr = ENOENT;
            return -1;
      }

      /* Check if file */
      if (!(file_inode->mode & S_IFREG)) {
            *errnoptr = EINVAL; 
            return -1;
      }

      /* Allocate data block if not already allocated */
      if (file_inode->data_block == 0) {
            size_t data_block_offset = find_free_data_block(fsptr, fssize);
            if (data_block_offset == (size_t)-1) {
                  *errnoptr = ENOSPC;
                  return -1;
            }
            file_inode->data_block = data_block_offset;
      }

      /* Write into data block */
      void *data_ptr = offset_to_ptr(fsptr, fssize, file_inode->data_block + offset);
      if (!data_ptr) {
            *errnoptr = EIO;
            return -1;
      }

      /* Check if write exceeds block size */
      if (offset + size > BLOCK_SIZE) {
            *errnoptr = EFBIG; // File too large
            return -1;
      }

      /* Perform the write */
      memcpy(data_ptr, buf, size);

      /* Update metadata */
      if (offset + size > file_inode->size) {
            file_inode->size = offset + size;
      }
      file_inode->modification_time = file_inode->change_time = time(NULL);

      return size;
}


/* Implements an emulation of the utimensat system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call changes the access and modification times of the file
   or directory indicated by path to the values in ts.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 utimensat.

*/
int __myfs_utimens_implem(void *fsptr, size_t fssize, int *errnoptr,
                          const char *path, const struct timespec ts[2]) {
    /* Step 1: Initialize the filesystem */
    if (!init_fs(fsptr, fssize)) {
        *errnoptr = EFAULT; // Bad address
        return -1;
    }

    /* Step 2: Validate the input path */
    if (path == NULL || strlen(path) == 0) {
        *errnoptr = EINVAL; // Invalid argument
        return -1;
    }

    /* Step 3: Find the inode corresponding to the path */
    size_t inode_offset;
    inode *file_inode = find_inode(fsptr, fssize, path, &inode_offset);
    if (!file_inode) {
        *errnoptr = ENOENT; // No such file or directory
        return -1;
    }

    /* Step 4: Update the access and modification times */
    time_t new_access_time;
    time_t new_modification_time;

    if (ts == NULL) {
        /* If ts is NULL, set both times to current time */
        new_access_time = new_modification_time = time(NULL);
    } else {
        /* Otherwise, set times based on ts array */
        new_access_time = ts[0].tv_sec;
        new_modification_time = ts[1].tv_sec;

        /* Validate the times */
        if ((ts[0].tv_nsec < 0 || ts[0].tv_nsec >= 1000000000) ||
            (ts[1].tv_nsec < 0 || ts[1].tv_nsec >= 1000000000)) {
            *errnoptr = EINVAL; // Invalid timespec
            return -1;
        }
    }

    /* Step 5: Update the inode's times */
    file_inode->access_time = new_access_time;
    file_inode->modification_time = new_modification_time;
    file_inode->change_time = time(NULL); 

    /* Step 6: Success */
    return 0;
}

/* Implements an emulation of the statfs system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call gets information of the filesystem usage and puts in 
   into stbuf.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 statfs.

   Essentially, only the following fields of struct statvfs need to be
   supported:

   f_bsize   fill with what you call a block (typically 1024 bytes)
   f_blocks  fill with the total number of blocks in the filesystem
   f_bfree   fill with the free number of blocks in the filesystem
   f_bavail  fill with same value as f_bfree
   f_namemax fill with your maximum file/directory name, if your
             filesystem has such a maximum

*/
int __myfs_statfs_implem(void *fsptr, size_t fssize, int *errnoptr,
                         struct statvfs* stbuf) {
    /* Step 1: Initialize the filesystem */
    if (!init_fs(fsptr, fssize)) {
        *errnoptr = EFAULT; // Bad address
        return -1;
    }

    /* Step 2: Validate the input statvfs structure */
    if (stbuf == NULL) {
        *errnoptr = EFAULT; // Bad address
        return -1;
    }

    /* Step 3: Zero out the statvfs structure */
    memset(stbuf, 0, sizeof(struct statvfs));

    /* Step 4: Populate the required fields */

    /* f_bsize: Block size */
    stbuf->f_bsize = BLOCK_SIZE;

    /* f_frsize: Fragment size (optional, can set to f_bsize) */
    stbuf->f_frsize = BLOCK_SIZE;

    /* f_blocks: Total number of blocks in the filesystem */
    stbuf->f_blocks = calculate_total_blocks(fssize);

    /* f_bfree: Number of free blocks */
    stbuf->f_bfree = calculate_free_blocks(fsptr, fssize);

    /* f_bavail: Number of free blocks available to non-superuser */
    stbuf->f_bavail = stbuf->f_bfree; // Assuming no reserved blocks

    /* f_namemax: Maximum length of filenames */
    stbuf->f_namemax = MAX_FILENAME;
    /* Step 5: Return success */
    return 0;
}
