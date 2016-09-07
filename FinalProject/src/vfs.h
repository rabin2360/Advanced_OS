/*
 * vfs_integration.h
 */
//#pragma pack(push, 1)
#ifndef VFS_H_
#define VFS_H_

#include "llist.h"
#include "hash.h"
#include "VFS_BST.h"
#include "narytree.h"

#define C_OR_CPP 1

#define ERROR_WRITING_FILE 1
#define ERROR_READING_FILE 1
#define INVALID_FILE_SYSTEM_SIZE 1

#define ERROR_VFS_CORRUPTED 1

#define ERROR_NOT_MOUNTED 1
#define ERROR_WRITING_FILE_DESCRIPTORS 126

#define VFS_FILE_EXISTS 127

#define ERROR_MAX_FD_REACHED 2
#define ERROR_FILE_TOO_LARGE 3
#define ERROR_NO_FREE_BLOCKS 4

//Initially the extn was .fs
#define VFS_EXTN ""

//The minimum allowed file descriptors, used to check if size is valid
//By default there is always the root fd, so this should actually be = 2
#define MIN_REQUIRED_FD 1

int create(char* label,int size);
/* create a fileSystem with with extension .fs with the label specified. The size should be given in KB, minimum being 2KB. Following needs to be done -
1. No of Blocks Calculation = ((size*1024) - 20)/(sizeof(fd) + BLOCK_SIZE + 4);
2.Meta Header Calculation:  = 20 bytes
3.size of  header1 = no of blocks * size of fd
4.size of header2 = no of blocks *4
5.initialize max_fd
6. initialize current_fd_id
7. initialize no_of_fd
fd for root directory with name '/' and path '/' should be created. And the  meta header, header1 and header 2 needs to be written in the File System in binary format*/


int mount(char* label);
/* Open the file specified by the label. Read the global variables from the file. Populate the data structures (N-ARY TREE, HASH TABLE, BST). Also initialize the free block list. Read the block numbers into the list until -1 is encountered*/

int unmount(char* label);
/* Write the file descriptors and the free block list into the file. For the free block list, fill rest of the blocks (block numbers) with -1.(If sizeof the list is less than total number of blocks as calculated in create )  */


int add_file(char *path, char *filename, char *data);
/*adds a new file with data in the path specified*/

int create_dir(char *path, char *dirname);
/* adds a new directory in the path specified */

int remove_file(char *fullpath); /* deletes the file with the filename in the fullpath specified
1. Search the file in BST.

2. Retrieve blocks from the file.

3. Add the blocks to the free list.

Note: steps 2 and 3 are not performed, because we want to store it in recycle bin

4. Remove the file from N-ary Tree, Hash and BST.

5. Update the global variables.

*/

int remove_dir(char *path);

/*deletes the directory with the directory in the path specified

Remove the directory from n-ary tree only. Also, call remove_file recursively for files inside the directory.

*/

int list_files(char *path);

/* lists all the files in the directory specified in the path

Get the top level file descriptors. Print the files only. */

int get_file(char *fullpath, char *data);

/* set the content of the file specified in the fullpath in *data and return number of bytes. Return -1 if file not found

1. Search for the file.

2. retrieve the blocks.

3. Seek to corresponding offsets and read the data.

4. Data is stored in char* data passed.

*/

int search_files(char *fileName);
//searches for the files in the bst

char* get_file_name(char* fullpath);
char* get_parent_path(char* fullpath);
int isDirectory (char *path);
void my_free(void *ptr);
int persistToFile(char* label1);
float get_file_size ();

#endif /* VFS_H_ */
//#pragma pack(pop)
