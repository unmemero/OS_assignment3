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

#define FS_ID 0x4D595346
#define BLOCK_SIZE 4096
#define INODE_SIZE 128
#define MAX_FILENAME 255
#define MAX_INODES 1024
#define ROOT_INODE 0
#define MAX_DATA_BLOCKS 2528 // Added definition

/*Main info block of file system*/
typedef struct{
      uint32_t fs_id;
      size_t size;
      size_t root_inode;
      size_t free_inode_bitmap;
      size_t free_block_bitmap;
      size_t inode_table;
      size_t data_blocks;
      size_t max_data_blocks; // Added field
}fs_info_block;

/*Node in filesystem tree (directory or file)*/
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

typedef struct{
      char name[MAX_FILENAME + 1];
      size_t inode_offset;
}directory_entry;

/*Get offset to ptr*/
static void* offset_to_ptr(void *fsptr, size_t fssize, size_t offset){
      return (offset >= fssize) ? NULL : (char *)fsptr + offset;
}

/*Turn ptr into an offset*/
static size_t ptr_to_offset(void *fsptr, size_t fssize, void *ptr){
      char *base = (char*)fsptr;
      char *p = (char*)ptr;
      return (p < base || p >= base + fssize) ? (size_t)-1 : (size_t)(p - base);
}

/*Init the fs*/
static int init_fs(void *fsptr, size_t fssize){
      fs_info_block *info_block = (fs_info_block*)fsptr;
      
      /*FS already ini*/
      if(info_block->fs_id == FS_ID) return 1;

      /*Init block*/
      info_block->fs_id = FS_ID;
      info_block->size = fssize;
      info_block->root_inode = sizeof(fs_info_block);
      info_block->free_inode_bitmap = info_block->root_inode + INODE_SIZE;
      info_block->free_block_bitmap = info_block->free_inode_bitmap + (MAX_INODES / 8);
      info_block->inode_table = info_block->free_block_bitmap + (MAX_DATA_BLOCKS / 8);
      info_block->data_blocks = info_block->inode_table + (MAX_INODES * INODE_SIZE);
      info_block->max_data_blocks = MAX_DATA_BLOCKS; // Initialize max_data_blocks

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
      
      /* Mark root's data block as used */
      unsigned char *data_bitmap = (unsigned char*)offset_to_ptr(fsptr, fssize, info_block->free_block_bitmap);
      if (data_bitmap) {
          data_bitmap[0] |= 1; // Mark the first data block as used (root's data block)
      }

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
int __myfs_rmdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
  /* STUB */
  return -1;
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
  /* STUB */
  return -1;
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
  /* STUB */
  return -1;
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
  /* STUB */
  return -1;
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
  /* STUB */
  return -1;
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
  /* STUB */
  return -1;
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
  /* STUB */
  return -1;
}
