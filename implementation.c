/*

  MyFS: a tiny file-system written for educational purposes

  MyFS is 

  Copyright 2018-21 by

  University of Alaska Anchorage, College of Engineering.

  Copyright 2022-24

  University of Texas at El Paso, Department of Computer Science.

  Contributors: Christoph Lauter 
                Rafael Garcia and
                Isaac G. Padilla

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

#define FILE_TYPE 0
#define DIR_TYPE 1
#define BLOCK_SIZE 1024     
#define NAME_MAX_LENGTH 255
#define ISEMPTYDIR(dir) (dir->entry_count == 0)
#define MAX_DIR_SIZE (1<<13)
#define MAX(a,b) ((a) > (b) ? (a) : (b))


/*Directory entry*/
typedef struct{
      size_t offset;
      char name[256];
      int type;
} myfs_dir_entry;

/*Header struct at the beginning of fsptr*/
typedef struct{
      size_t root_dir_offset;
      size_t last_offset;
} myfs_header;

/*Struct for file entrie*/
typedef struct{
      size_t data_offset;
      size_t size;
      time_t access_time;
      time_t mod_time;
} myfs_file;

/*Directory struct*/
typedef struct{
      size_t offset;
      size_t num_links;
      time_t access_time;
      time_t mod_time;
      size_t entry_count;
      myfs_dir_entry entries[];
} myfs_dir;

/*Helper skelter*/

/*Convert offset to ptr*/
void *offset_to_ptr(void *fsptr, size_t offset){
      return (fsptr == NULL) ? NULL : (char *)fsptr + offset;
}

/*Locate a subdirectory or file within a directory*/
void *find_entry(myfs_dir *curr_dir, const char *name, void *fsptr, int *type){
      for(size_t i=0;i<curr_dir->entry_count;i++){
           if(strcmp(curr_dir->entries[i].name, name) == 0){
                  if(curr_dir->entries[i].type == FILE_TYPE){
                        *type = FILE_TYPE;
                        return offset_to_ptr(fsptr, curr_dir->entries[i].offset);
                  }else if(curr_dir->entries[i].type == DIR_TYPE){
                        *type = DIR_TYPE;
                        return offset_to_ptr(fsptr, curr_dir->entries[i].offset);
                  }
           }
      }
      return NULL;
}

/*Locate file*/
int find_path(void *fsptr, const char *path, myfs_file **file, myfs_dir **dir){
      myfs_header *header = (myfs_header *)fsptr;

      /*If root, ret root*/
      if(strcmp(path, "/") == 0){
            *dir = (myfs_dir *)offset_to_ptr(fsptr, header->root_dir_offset);
            *file = NULL;
            return 0;
      }

      /*Start from root*/
      myfs_dir *curr_dir = (myfs_dir *) offset_to_ptr(fsptr, header->root_dir_offset);
      if(!curr_dir) return -1;

      /*Dup path*/
      char path_cpy[256];
      strncpy(path_cpy, path, sizeof(path_cpy)-1);
      path_cpy[sizeof(path_cpy)-1] = '\0';

      /*Tok path*/
      char *token = strtok(path_cpy, "/");
      void *entry = NULL;
      int type = -1;

      while(token){
            /*Find next entry*/
            entry = find_entry(curr_dir,token, fsptr, &type);
            if(!entry) return -1;

            /*If entry is file*/
            if(type == FILE_TYPE){
                  *file = (myfs_file *)entry;
                  *dir = NULL;
                  return 0;
            }else{
                  /*Enrey is dir*/
                  curr_dir = (myfs_dir *)entry;
            }

            token = strtok(NULL, "/");
      }

      /*Path is dir*/
      *dir = curr_dir;
      *file = NULL;
      return 0;
}

/*Find dir*/
myfs_dir *find_dir(void *fsptr, const char *path){
      myfs_dir *dir = NULL;
      myfs_file *file = NULL;
      if(find_path(fsptr, path, &file, &dir) == -1)return NULL;
      return dir;
}

/*Get parent directory*/
myfs_dir *get_parent_dir(void *fsptr,const char *path, char *filename){
      if(path == NULL || filename == NULL) return NULL;

      /*Path is root*/
      if(strcmp(path,"/") == 0){
            strcpy(filename, "/");
            return find_dir(fsptr, "/");
      }

      /*Start at root*/
      myfs_dir *curr_dir = find_dir(fsptr, "/");
      if(!curr_dir) return NULL;

      /*Path dup*/
      char path_cpy[256];
      strncpy(path_cpy, path, sizeof(path_cpy)-1);
      path_cpy[sizeof(path_cpy)-1] = '\0';

      /*Tok path*/
      char *token = strtok(path_cpy, "/");
      char *next_token = strtok(NULL, "/");
      int type;
      void *entry = NULL;

      /*Find next entry*/
      while(next_token){
            type = -1;
            entry = find_entry(curr_dir, token, fsptr, &type);
            if(!entry || type != DIR_TYPE) return NULL;

            /*Next dir*/
            curr_dir = (myfs_dir *)entry;
            token = next_token;
            next_token = strtok(NULL, "/");
      }

      /*Filename*/
      strncpy(filename, token,255);
      filename[255] = '\0';
      return curr_dir;
}

/*CHeck if file exists*/
int file_exists(myfs_dir *dir, const char *filename){
      for(size_t i=0;i<dir->entry_count;i++) if(strcmp(dir->entries[i].name, filename) == 0) return 1;
      return 0;
}

/*Clear file*/
void clear_file_data(void *fsptr,size_t data_offset, size_t size){
      void *data = offset_to_ptr(fsptr, data_offset);
      memset(data, 0, size);
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

      myfs_file *file = NULL;
      myfs_dir *dir = NULL;

      memset(stbuf, 0, sizeof(struct stat));

      if (find_path(fsptr, path, &file, &dir) == -1) {
            *errnoptr = ENOENT;
            return -1;
      }

      stbuf->st_uid = uid;
      stbuf->st_gid = gid;

      if (dir) {
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = dir->num_links;
            stbuf->st_size = 0; 
            stbuf->st_atim.tv_sec = dir->access_time;
            stbuf->st_atim.tv_nsec = 0;
            stbuf->st_mtim.tv_sec = dir->mod_time;
            stbuf->st_mtim.tv_nsec = 0;
      } else if (file) {
            stbuf->st_mode = S_IFREG | 0755;
            stbuf->st_nlink = 1;
            stbuf->st_size = file->size;
            stbuf->st_atim.tv_sec = file->access_time;
            stbuf->st_atim.tv_nsec = 0;
            stbuf->st_mtim.tv_sec = file->mod_time;
            stbuf->st_mtim.tv_nsec = 0;
      } else {
            *errnoptr = ENOENT;
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
int __myfs_readdir_implem(void *fsptr, size_t fssize, int *errnoptr, const char *path, char ***namesptr) {

      myfs_dir *dir = find_dir(fsptr, path);

      if (!dir) {
            *errnoptr = ENOENT;
            return -1;
      }

      if (dir->entry_count == 0) {
            *namesptr = NULL;
            return 0;
      }
      
      size_t num_entries = dir->entry_count;
      *namesptr = (char **)calloc(num_entries, sizeof(char *));
      if (!*namesptr) {
            *errnoptr = ENOMEM;
            return -1;
      }

      size_t j = 0;
      for (size_t i = 0; i < dir->entry_count; i++) {
            if (strcmp(dir->entries[i].name, ".") == 0 || strcmp(dir->entries[i].name, "..") == 0) continue;

            (*namesptr)[j] = strdup(dir->entries[i].name);
            if (!(*namesptr)[j]) {
                  *errnoptr = ENOMEM;
                  for (size_t k = 0; k < j; k++) free((*namesptr)[k]);
                  free(*namesptr);
                  return -1;
            }
            j++;
      }

      if (j == 0) {
            free(*namesptr);
            *namesptr = NULL;
      }

      return (int)j;
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
      myfs_header *header = (myfs_header *)fsptr;
      char filename[256];
      myfs_dir *parent_dir = get_parent_dir(fsptr, path, filename);

      /*No parent directory*/
      if (!parent_dir) {
            *errnoptr = ENOENT;
            return -1;
      }

      /*File already exists*/
      int type = -1;
      if (find_entry(parent_dir, filename, fsptr, &type)) {
            *errnoptr = EEXIST;
            return -1;
      }

      /*Check parent dir space*/
      size_t max_entries = (MAX_DIR_SIZE - sizeof(myfs_dir)) / sizeof(myfs_dir_entry);
      if (parent_dir->entry_count >= max_entries) {
            *errnoptr = ENOSPC;
            return -1;
      }

      /*Make space for file*/
      size_t new_file_offset = header->last_offset;
      if (new_file_offset + sizeof(myfs_file) > fssize) {
            *errnoptr = ENOSPC;
            return -1;
      }

      /*Make new file*/
      myfs_file *new_file = (myfs_file *)offset_to_ptr(fsptr, new_file_offset);

      /*Init file*/
      memset(new_file, 0, sizeof(myfs_file));
      new_file->data_offset = 0;
      new_file->size = 0;
      new_file->access_time = time(NULL);
      new_file->mod_time = new_file->access_time;

      /*Update last offset*/
      header->last_offset += sizeof(myfs_file);

      /*Add to parent dir*/
      myfs_dir_entry *new_entry = &parent_dir->entries[parent_dir->entry_count];
      new_entry->offset = new_file_offset;
      strncpy(new_entry->name, filename, sizeof(new_entry->name) - 1);
      new_entry->name[sizeof(new_entry->name) - 1] = '\0';
      new_entry->type = FILE_TYPE;

      /*Update parent dir*/
      parent_dir->entry_count++;
      parent_dir->mod_time = new_file->mod_time;

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
      myfs_dir *parent_dir = NULL;
      char filename[256];

      /*Get parent dir and filename*/
      parent_dir = get_parent_dir(fsptr, path, filename);
      if (!parent_dir) {
            *errnoptr = ENOENT;
            return -1;
      }

      /*Check if file exists and its just file, not dir*/
      int type = -1;
      myfs_file *file_to_rm = (myfs_file *)find_entry(parent_dir, filename, fsptr, &type);
      if (!file_to_rm || type != FILE_TYPE) {
            *errnoptr = ENOENT;
            return -1;
      }

      /*Clear file data*/
      clear_file_data(fsptr, file_to_rm->data_offset, file_to_rm->size);

      /*Remove the filey*/
      int found = 0;
      for (size_t i = 0; i < parent_dir->entry_count; i++) {
            if (strcmp(parent_dir->entries[i].name, filename) == 0) {
                  found = 1;
                  /* Shift entries to fill the gap */
                  for (size_t j = i; j < parent_dir->entry_count - 1; j++) {
                  parent_dir->entries[j] = parent_dir->entries[j + 1];
                  }
                  parent_dir->entry_count--;
                  break;
            }
      }

      /*ENOENT*/
      if (!found) {
            *errnoptr = ENOENT;
            return -1;
      }

      /*Clear file info*/
      memset(file_to_rm, 0, sizeof(myfs_file));

      /*Update parentdir*/
      parent_dir->mod_time = time(NULL);
      parent_dir->access_time = parent_dir->mod_time;

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
int __myfs_rmdir_implem(void *fsptr, size_t fssize, int *errnoptr, const char *path) {
      char dirname[256];

      /*CAN'T DELETE ROOTIN' TOOTIN' ROOT*/
      if (strcmp(path, "/") == 0) {
            *errnoptr = EINVAL;
            return -1;
      }

      /*Get parent dir*/
      myfs_dir *parent_dir = get_parent_dir(fsptr, path, dirname);
      if (!parent_dir) {
            *errnoptr = ENOENT;
            return -1;
      }

      /*Check if direxists*/
      int type = -1;
      myfs_dir *dir_to_rm = (myfs_dir *)find_entry(parent_dir, dirname, fsptr, &type);
      if (!dir_to_rm || type != DIR_TYPE) {
            *errnoptr = ENOENT;
            return -1;
      }

      /*Check if dir is empty*/
      if (!ISEMPTYDIR(dir_to_rm)) {
            *errnoptr = ENOTEMPTY;
            return -1;
      }

      /*Remove dir from parent dir*/
      int found = 0;
      for (size_t i = 0; i < parent_dir->entry_count; i++) {
            if (strcmp(parent_dir->entries[i].name, dirname) == 0) {
                  found = 1;
                  /*Shift entries to fill the gap*/
                  for (size_t j = i; j < parent_dir->entry_count - 1; j++) parent_dir->entries[j] = parent_dir->entries[j + 1];
                  parent_dir->entry_count--;
                  break;
            }
      }

      /*DIE, no dir found*/
      if (!found) {
            *errnoptr = ENOENT;
            return -1;
      }

      /*Clear metadata*/
      memset(dir_to_rm, 0, sizeof(myfs_dir));

      /*Update parent dir access*/
      parent_dir->mod_time = time(NULL);
      parent_dir->access_time = parent_dir->mod_time;

      return 0;
}

/* Implements an emulation of the mkdir system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call creates the directory indicated by path.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 mkdir.

*/
int __myfs_mkdir_implem(void *fsptr, size_t fssize, int *errnoptr, const char *path) {
      char dirname[256];

      /*Get parent dir*/
      myfs_dir *parent_dir = get_parent_dir(fsptr, path, dirname);
      if (!parent_dir) {
            *errnoptr = ENOENT;
            return -1;
      }

      /*direxists*/
      int type = -1;
      if (find_entry(parent_dir, dirname, fsptr, &type)) {
            *errnoptr = EEXIST;
            return -1;
      }

      /*Max entry*/
      size_t max_entries = (MAX_DIR_SIZE - sizeof(myfs_dir)) / sizeof(myfs_dir_entry);

      /*Check space*/
      if (parent_dir->entry_count >= max_entries) {
            *errnoptr = ENOSPC;
            return -1;
      }

      /*Get FS header*/
      myfs_header *header = (myfs_header *)fsptr;

      /*Get space for new dir*/
      size_t new_dir_offset = header->last_offset;

      /*Get new dir size*/
      size_t new_dir_size = sizeof(myfs_dir) + max_entries * sizeof(myfs_dir_entry);

      /*Check filesystem for space*/
      if (new_dir_offset + new_dir_size > fssize) {
            *errnoptr = ENOSPC;  
            return -1;
      }

      /*Init new dir*/
      myfs_dir *new_dir = (myfs_dir *)offset_to_ptr(fsptr, new_dir_offset);
      memset(new_dir, 0, new_dir_size);
      new_dir->offset = new_dir_offset;
      new_dir->num_links = 2; 
      new_dir->access_time = time(NULL);
      new_dir->mod_time = new_dir->access_time;
      new_dir->entry_count = 0;

      /*Update FS header*/
      header->last_offset += new_dir_size;

      /*Add to parent dir*/
      myfs_dir_entry *new_entry = &parent_dir->entries[parent_dir->entry_count];
      new_entry->offset = new_dir_offset;
      strncpy(new_entry->name, dirname, sizeof(new_entry->name) - 1);
      new_entry->name[sizeof(new_entry->name) - 1] = '\0';
      new_entry->type = DIR_TYPE;

      /* Update parent metadata*/
      parent_dir->entry_count++;
      parent_dir->mod_time = new_dir->mod_time;

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
    char from_name[256], to_name[256];
    char child_from_path[512], child_to_path[512]; // Increased size for longer paths

    // Locate the parent directories of the "from" and "to" paths
    myfs_dir *from_parent = get_parent_dir(fsptr, from, from_name);
    myfs_dir *to_parent = get_parent_dir(fsptr, to, to_name);

    // Check directories exist
    if (!from_parent) {
        *errnoptr = ENOENT; 
        return -1;
    }

    if (!to_parent) {
        *errnoptr = ENOENT; 
        return -1;
    }

    // Get "from" entry
    int from_type = -1;
    void *from_entry = find_entry(from_parent, from_name, fsptr, &from_type);
    if (!from_entry) {
        *errnoptr = ENOENT;
        return -1;
    }

    // Check if "to" already exists
    int to_type = -1;
    void *to_entry = find_entry(to_parent, to_name, fsptr, &to_type);

    if (to_entry) {
        // Handle conflicts when "to" exists
        if (to_type != from_type) {
            *errnoptr = EISDIR; // Type mismatch: Cannot overwrite file with directory or vice versa
            return -1;
        }

        if (from_type == DIR_TYPE) {
            // Recursively merge directories
            myfs_dir *from_dir = (myfs_dir *)from_entry;

            for (size_t i = 0; i < from_dir->entry_count; i++) {
                myfs_dir_entry *child_entry = &from_dir->entries[i];

                // Construct new paths for children
                snprintf(child_from_path, sizeof(child_from_path), "%s/%s", from, child_entry->name);
                snprintf(child_to_path, sizeof(child_to_path), "%s/%s", to, child_entry->name);

                // Ensure no truncation occurred
                if (strlen(child_from_path) >= sizeof(child_from_path) || strlen(child_to_path) >= sizeof(child_to_path)) {
                    *errnoptr = ENAMETOOLONG; // Path too long
                    return -1;
                }

                // Recursively rename/merge child
                if (__myfs_rename_implem(fsptr, fssize, errnoptr, child_from_path, child_to_path) != 0) {
                    return -1; // Propagate error if renaming a child fails
                }
            }
        } else if (from_type == FILE_TYPE) {
            // Overwrite the file in "to"
            myfs_file *to_file = (myfs_file *)to_entry;
            myfs_file *from_file = (myfs_file *)from_entry;

            clear_file_data(fsptr, to_file->data_offset, to_file->size);
            memcpy(to_file, from_file, sizeof(myfs_file));
        }
    }

    // Remove "from" entry from its parent directory
    int found = 0;
    for (size_t i = 0; i < from_parent->entry_count; i++) {
        if (strcmp(from_parent->entries[i].name, from_name) == 0) {
            found = 1;
            // Shift remaining entries to fill the gap
            for (size_t j = i; j < from_parent->entry_count - 1; j++) {
                from_parent->entries[j] = from_parent->entries[j + 1];
            }
            from_parent->entry_count--;
            break;
        }
    }
    if (!found) {
        *errnoptr = ENOENT; // "from" entry was not found in the parent
        return -1;
    }

    // Add "from" entry to the "to" directory
    if (to_parent->entry_count >= (fssize / sizeof(myfs_dir_entry))) {
        *errnoptr = ENOSPC; // No space to add the new entry
        return -1;
    }

    myfs_dir_entry *new_entry = &to_parent->entries[to_parent->entry_count];
    new_entry->offset = (size_t)from_entry - (size_t)fsptr; // Offset from start
    strncpy(new_entry->name, to_name, sizeof(new_entry->name) - 1);
    new_entry->name[sizeof(new_entry->name) - 1] = '\0'; // Ensure null-termination
    new_entry->type = from_type;

    to_parent->entry_count++;

    return 0; // Success
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
  myfs_file *file = NULL;
    myfs_dir *dir = NULL;

    // Locate the file using the helper function
    if (find_path(fsptr, path, &file, &dir) == -1 || !file) {
        *errnoptr = ENOENT; // File not found
        return -1;
    }

    // Ensure the offset is valid
    if (offset < 0) {
        *errnoptr = EINVAL; // Invalid offset
        return -1;
    }

    size_t current_size = file->size;
    size_t data_offset = file->data_offset;

    if (offset < current_size) {
        // Shrinking the file: clear excess data
        clear_file_data(fsptr, data_offset + offset, current_size - offset);
        file->size = offset; // Update file size
    } else if (offset > current_size) {
        // Expanding the file
        size_t expand_amount = offset - current_size;

        // Check if there is enough space in the filesystem
        if (data_offset + offset > fssize) {
            *errnoptr = ENOSPC; // Not enough space
            return -1;
        }

        // Zero out the new space
        void *new_data = offset_to_ptr(fsptr, data_offset + current_size);
        memset(new_data, 0, expand_amount);

        file->size = offset; // Update file size
    }

    // Update file timestamps
    file->mod_time = time(NULL);
    file->access_time = time(NULL);

    return 0; // Success
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
int __myfs_open_implem(void *fsptr, size_t fssize, int *errnoptr, const char *path) {
      /*Check FS pointer*/
      if(!fsptr || !fssize){
            *errnoptr = EFAULT;
            return -1;
      }

      /*Check FS status*/
      myfs_header *header = (myfs_header *)fsptr;
      if(header->root_dir_offset >= fssize || header->last_offset > fssize){
            *errnoptr = EFAULT;
            return -1;
      }

      /*Locate file or directory*/
      myfs_file *file = NULL;
      myfs_dir *dir = NULL;
      int res = find_path(fsptr, path, &file, &dir);
      if(res < 0){
            *errnoptr = ENOENT;
            return -1;
      }

      /*Success*/
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
int __myfs_read_implem(void *fsptr, size_t fssize, int *errnoptr, const char *path, char *buf, size_t size, off_t offset) {
      /*Check if FS is in valid state*/
      if(!fsptr || !fssize){
            *errnoptr = EFAULT;
            return -1;
      }

      /*Find file*/
      myfs_file *file = NULL;
      myfs_dir *dir = NULL;
      if(find_path(fsptr, path, &file, &dir) == -1 || !file){
            *errnoptr = ENOENT;
            return -1;
      }

      /*Check if it's regular file*/
      if(dir){
            *errnoptr = EISDIR;    
            return -1;
      }

      /*Check offset*/
      if(offset < 0){
            *errnoptr = EINVAL;
            return -1;
      }

      /*Offset is beyond EOF, return 0*/
      if(offset >= (off_t)file->size) return 0;

      /*Adjust offset size*/
      size_t max_bytes = file->size - offset;
      size_t read_bytes = MAX(max_bytes,size);

      /*Check read not beyond FS*/
      if((file->data_offset + offset + read_bytes) > fssize){
            *errnoptr = EIO;
            return -1;
      }

      /*Copy data from mem to buffet*/
      void *data = offset_to_ptr(fsptr, file->data_offset + offset);
      memcpy(buf, data, read_bytes);

      /*Update access time*/
      file->access_time = time(NULL);
      
      /*Return read bytes*/
      return (int)read_bytes;
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
  myfs_file *file = NULL;
    myfs_dir *dir = NULL;

    // Locate the file to write to
    if (find_path(fsptr, path, &file, &dir) == -1 || !file) {
        *errnoptr = ENOENT; // File does not exist
        return -1;
    }

    // Ensure offset and size are valid
    if (offset < 0 || size <= 0) {
        *errnoptr = EINVAL; // Invalid offset or size
        return -1;
    }

    // Calculate the end of the write
    size_t end_offset = offset + size;

    // Ensure the write does not exceed the filesystem size
    if (file->data_offset + end_offset > fssize) {
        *errnoptr = ENOSPC; // Not enough space in the filesystem
        return -1;
    }

    // Expand the file if necessary
    if (end_offset > file->size) {
        size_t expand_amount = end_offset - file->size;
        void *new_data = offset_to_ptr(fsptr, file->data_offset + file->size);
        memset(new_data, 0, expand_amount); // Zero out newly allocated space
        file->size = end_offset; // Update file size
    }

    // Perform the write operation
    void *write_start = offset_to_ptr(fsptr, file->data_offset + offset);
    memcpy(write_start, buf, size);

    // Update file metadata
    file->mod_time = time(NULL); // Update modification time
    file->access_time = time(NULL); // Update access time

    return size; // Return the number of bytes written
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
  myfs_file *file = NULL;
    myfs_dir *dir = NULL;

    // Locate the file or directory by path
    if (find_path(fsptr, path, &file, &dir) == -1) {
        *errnoptr = ENOENT; // Path does not exist
        return -1;
    }

    // If times are provided, update the appropriate fields
    if (ts) {
        if (file) {
            // Update file access and modification times
            file->access_time = ts[0].tv_sec;
            file->mod_time = ts[1].tv_sec;
        } else if (dir) {
            // Update directory access and modification times
            dir->access_time = ts[0].tv_sec;
            dir->mod_time = ts[1].tv_sec;
        } else {
            *errnoptr = ENOENT; // Neither file nor directory found
            return -1;
        }
    } else {
        // If `ts` is NULL, set times to the current time
        time_t current_time = time(NULL);
        if (file) {
            file->access_time = current_time;
            file->mod_time = current_time;
        } else if (dir) {
            dir->access_time = current_time;
            dir->mod_time = current_time;
        }
    }

    return 0; // Success
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

    // Validate inputs
    if (!fsptr || !stbuf) {
        if (errnoptr) *errnoptr = EINVAL; // Invalid argument
        return -1;
    }

    // Zero out the statvfs structure to ensure clean data
    memset(stbuf, 0, sizeof(struct statvfs));

    // Calculate total number of blocks in the filesystem
    size_t total_blocks = fssize / BLOCK_SIZE;

    // Calculate used and free blocks
    size_t used_blocks = 0;
    size_t free_blocks = 0;

    // Traverse memory to determine block usage
    unsigned char *memory = (unsigned char *)fsptr;
    for (size_t block = 0; block < total_blocks; block++) {
        size_t block_start = block * BLOCK_SIZE;
        int block_used = 0;

        // Check if any byte in the block is non-zero
        for (size_t i = 0; i < BLOCK_SIZE; i++) {
            if (memory[block_start + i] != 0) {
                block_used = 1;
                break;
            }
        }

        if (block_used) {
            used_blocks++;
        }
    }

    free_blocks = total_blocks - used_blocks;

    // Populate the statvfs structure
    stbuf->f_bsize = BLOCK_SIZE;          // Block size in bytes
    stbuf->f_frsize = BLOCK_SIZE;         // Fragment size (same as block size)
    stbuf->f_blocks = total_blocks;       // Total number of blocks
    stbuf->f_bfree = free_blocks;         // Free blocks
    stbuf->f_bavail = free_blocks;        // Free blocks available to unprivileged users
    stbuf->f_namemax = NAME_MAX_LENGTH;   // Maximum length of file/directory name

    return 0; // Success
}