/*
 * vfs_integration.c
 *
 */

//#pragma pack(push, 1)
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "globals.h"
#include "vfs_error_codes.h"
#include "vfs.h"
#include "narytree.h"
#include "VFS_BST.h"
#include "llist.h"
#include "hash.h"


//dev square related global variables
l_node *start;
t_node *root;
b_node *bst_root;
l_node *h_bucket[NO_OF_BUCKETS];

char *label = NULL;

int vfs_size_in_kb;
int size_meta_header;
int size_of_header_1;
int size_of_header_2;
int max_fd;
int current_fd_id;
int no_of_fd;

//Variable used to automatically clear the trash when a new file cannot be accomodated in free list
//Later set it to 0, when we have the code to prompt user if he wants to go ahead and empty the trash
static int automatic_empty_trash = 1;

//Used in write_nary method, Note: THe file is not kept open all the time, for each io operation,
//the file is opened and closed, hence it is not advised to initialize this everytime on use.
static FILE *temp_file_pointer = NULL;
static int fd_count;

/*The reason why we declare these structs in the source file is because we dont want
 * these to be exposed to the api user.
 */

//==============META HEADER: to be calculated in the function create_VFS()=============
/*	 size header_1 = max_fd * sizeof(fd), we will write zero into header1 and header2 at the beginning of any
	 * write operation, and then once the write op is complete, we write back the computed value
	 * This way when we mount the volume again we could come to know if the vfs is corrupted, common case
	 * of corruption could be when the user kills the application when it is writing to the vfs.

	 maximum number of fds the filesystem can support it should be equal to the number of blocks
	 (size_of_filesystem - 20) / (size_of_fd + BLOCK_SIZE + 4)
	 Actually it should be (size_of_filesystem -20 - sizeof(fd)) / (size_of_fd + BLOCK_SIZE + 4) +1, because
	 * by default we will have one fd for root dir.
*/


//The inbuilt fclose complains when fp is null, hence this helper
static void my_fclose(FILE *fp){
	if (fp != NULL)
		fclose(fp);
}

int is_file_exists(char *filename) {
	FILE *fp;
	fp = fopen(filename, "rb");

	//File does not exist
	if (!fp){
		return 0;
	}
	my_fclose(fp);

	//File does exist
	return 1;
}

static int populate_size_meta_header(){
	return size_meta_header=20;
}

//size is in KB
static int get_max_fd(int size){

	//Actually this should be (size - sizeof(META_HEADER) - sizeof(fd)) / (sizeof(fd) + BLOCK_SIZE + 4) +1
	/*
	 * For eg: if the size = 10K then actual max_fd will be 8.45 + 1 = 9.45 = 9
	 * But with the below formula it will be 8.53 = 8. Hence even though we cud store 9 files
	 * each of 1K but instead we will be able to only store 8 files of 1k each.
	 * But it will fail for size = 182
	 */
	return (size*1024 - size_meta_header) / (sizeof(fd) + BLOCK_SIZE + sizeof(int));

}

float get_file_size () {
	return (max_fd*(sizeof(fd) + BLOCK_SIZE + sizeof(int)) + size_meta_header)/1024.0;

}

//Create a file descriptor, returns appropriate error
static int createFD(char *path, char *fileName, int isDir, int size, fd **fd_data){
	fd *data;
	int i,count;

	data = (fd*)malloc(sizeof(fd));

	strcpy(data->path,path);
	strcpy(data->file_name,fileName);
	data->is_dir = isDir;
	data->fd_id = current_fd_id;
	//size used in case of files only
	data->size = size;

	//For files populate the blocks with -1 before storing any data
	if (isDir != 1){
		//This will be the max number of blocks of data that can be stored, ie 20
		count = sizeof(data->blocks)/sizeof(int);
		for (i = 0; i < count; ++i) {
			data->blocks[i]=-1;
		}
	}
	//No idea what do we do with this fd_id
	//Only when the insertion of fd is success, we increment the metaheader

	*fd_data = data;

	return 0;
}

//This is used to write a wrong data and then the correct value is written, used fo corruption detection.
static void write_meta_header_incorrect(FILE *fp){
	int temp = 0;

	//Meta header should be at the top, so seek the fp to beginning
	fseek(fp,0,SEEK_SET);

	fwrite(&temp,sizeof(int),1,fp);
	fwrite(&temp,sizeof(int),1,fp);
	fwrite(&max_fd,sizeof(int),1,fp);
	fwrite(&current_fd_id,sizeof(int),1,fp);
	fwrite(&no_of_fd,sizeof(int),1,fp);
}

static void write_meta_header(FILE *fp){

	//Meta header should be at the top, so seek the fp to beginning
	fseek(fp,0,SEEK_SET);

	fwrite(&size_of_header_1,sizeof(int),1,fp);
	fwrite(&size_of_header_2,sizeof(int),1,fp);
	fwrite(&max_fd,sizeof(int),1,fp);
	fwrite(&current_fd_id,sizeof(int),1,fp);
	fwrite(&no_of_fd,sizeof(int),1,fp);
}

//Need to return success of failure
void read_meta_header(FILE *fp){
	//Meta header should be at the top, so seek the fp to beginning
	fseek(fp,0,SEEK_SET);

	fread(&size_of_header_1,sizeof(int),1,fp);
	fread(&size_of_header_2,sizeof(int),1,fp);
	fread(&max_fd,sizeof(int),1,fp);
	fread(&current_fd_id,sizeof(int),1,fp);
	fread(&no_of_fd,sizeof(int),1,fp);
}


static int get_size_header_1(int max_fd){
	return max_fd*sizeof(fd);
}

static int get_size_header_2(int max_fd){
	return max_fd*sizeof(int);
}

static void computeHeaderSize(){
	size_of_header_1 = get_size_header_1(max_fd);
	size_of_header_2 = get_size_header_2(max_fd);
}

//The inbuilt free method complains when ptr is null, hence this helper function
void my_free(void *ptr){
	if (ptr != NULL)
		free(ptr);
	ptr=NULL;
}

static char* get_vfs_file_name(char *label) {

	char *vfs_file_name = (char*)malloc(strlen(label)*sizeof(char)+(strlen(VFS_EXTN)*sizeof(char)) + 1);
	strcpy(vfs_file_name,label);
	strcat(vfs_file_name,VFS_EXTN);
	return vfs_file_name;
}

static void populate_vfs_file_name(char *label1){

	my_free(label);

	//The vfs file ends with .fs, not anymore
	label = get_vfs_file_name(label1);
}

/*
 * Create header and file blocks, assume size is in KiloBytes
 * IMPORTANT: If a file exists with the same label, then it will be overwritten by create_vfs,
 * hence the caller of this API should make sure that this condition is handles.
 * In create we overwrite all the global data, hence they shld make sure it is called after nmount
 */
int create(char *label1, int size){

	FILE *fp = NULL;
	fd *root_fd;
	//Array used to write zero to the blocks in our vfs
	char blocks[BLOCK_SIZE]={0};
	int i, retVal = 0, unused_size;

	//Error check for label
	if (label1 == NULL) {
		retVal = -1;
		goto FINISH;
	}

	populate_size_meta_header();

	//Check if the size is valid, ie the max_fd should be greate than 2k and less than 4GB
	//Max size an int can take is 4GB = 2 ^ 22 KB, hence our computation will overflow
	if (size < 0 || size > 4 * 1024 * 1024) {
		retVal = INVALID_FILE_SYSTEM_SIZE;
		goto FINISH;
	}
	max_fd = get_max_fd(size);

	if (max_fd < MIN_REQUIRED_FD) {
		retVal = INVALID_FILE_SYSTEM_SIZE;
		goto FINISH;
	}

	populate_vfs_file_name(label1);


	//Now create the file, note: it will be overwritten if it already exists.
	fp = fopen(label,"wb");

	if (!fp) {
		retVal = ERROR_WRITING_FILE;
		goto FINISH;
	}

	//Initialize the meta headers, and calculate other, max_fd already computed above
	current_fd_id = 0;
	no_of_fd = 0;

	//Compute header1 from max_fd, yes it is not required to persist it,
	//but we could use this for corruption detection. See header file.
	computeHeaderSize();

	//Create the Default FD which is the root node and increment the metaheader
	createFD("/","/",1, 0, &root_fd);
	current_fd_id++;
	no_of_fd++;


	//First write the metaheader into the file
	write_meta_header(fp);

	//Now write the header1, which is nothing but just the root FD
	//fwrite returns the number of elements written
	if (fwrite(root_fd,sizeof(fd),1,fp) != 1){
		retVal =ERROR_WRITING_FILE;
		goto FINISH;
	}

	//Fill the remainder of header-1 with zeros, or better just fill it with the root fd
	//Since our no_of_fd is 1 we wont read beyond the first file descriptor when we mount the vol
	for (i = 1; i < max_fd; ++i) {
		if (fwrite(root_fd,sizeof(fd),1,fp) != 1){
			retVal = ERROR_WRITING_FILE;
			goto FINISH;
		}
	}

	//Now file pointer already points to the start of header2.

	//Header-2 contains the freelist, since we have just created the filesystem, all blocks are free
	//Hence just populate header-2 with 0 - max_fd-1. But if a particular block is not free, then we write -1
	//And no of blocks = max_fd
	for (i = 0; i < max_fd; ++i) {
		if (fwrite(&i,sizeof(int),1,fp) != 1){
			retVal = ERROR_WRITING_FILE;
			goto FINISH;
		}
	}

	//We are done with header2, Just fill the blocks with zeros
	for (i = 0; i < max_fd; ++i) {
		if (fwrite(blocks,sizeof(char),BLOCK_SIZE,fp) != BLOCK_SIZE) {
			retVal = ERROR_WRITING_FILE;
			goto FINISH;
		}
	}

	/*We now have filled the entire vfs file, but it may not be equal to the size, because
	 * there will be some extra size < BLOCK_SIZE which will be unused, hence there is no point
	 * in adding that extra space in our VFS file, since we cant use it anyway.
	 * Eg: if size = 4k, then the vfs file size will be only 3716 Bytes ~ 3.6 KB [sizeof(fd) = 204]
	 */
	//But, better fill this unused size, otherwise dev-square might cry...
	unused_size = (size*1024) - (size_meta_header+size_of_header_1 + size_of_header_2 + BLOCK_SIZE*max_fd);
	if (fwrite(&blocks[0],sizeof(char),unused_size,fp) != unused_size){
		retVal = ERROR_WRITING_FILE;
		goto FINISH;
	}

	FINISH:
	//Finally dont forget to free mem and close the file
	my_fclose(fp);
	if (retVal == 0)
		vfs_size_in_kb = size;
	return retVal;
}

static void resetRootNodes() {
	start = NULL;
	root = NULL;
	bst_root = NULL;

	//we know that hash_initialize just sets it to nulls
	hash_initialize(h_bucket);
}

//To be called in unmount and when insert file descriptor fails
static void free_all_nodes(){
	//todo, free all datastructures

	//Reset the values once your free is complete.
	resetRootNodes();
}

static void initializeRootNodes(){

	//Nothing wrong in freeing the already allocated memory before initializing
	free_all_nodes();

	bst_root = bst_create();
	root = NULL;
	start = NULL;
	hash_initialize(h_bucket);
}

/*
 * on_delete_from trash takes care of checking if the node is in recycle bin,
 * if so it will permanently delete it and add the blocks to free list.
 * This is basically an action handler to be called on each node, that u want to permanently delete,
 * Note: this is not recursive, so if you delete a dir, then better call this on each of its children
*/
static int on_delete_from_trash(t_node *node){
	fd *data;
	int i,count, *free_block;

	//Actually this check is not needed, but we need to reuse this function for empty trash, write_nary
	//Hence it is better to check if it is really inside recycle bin, before deleting.
	if(node->is_deleted == 1) {
		node->is_deleted = PERMANENTLY_DELETED;
	} else {//either not deleted or already permanently deleted
		return 0;
	}

	data = node->data;
	//Nothing to do if it is a directory
	if(data->is_dir == 1)
		return 0;

	//Now add the blocks of fd to free list
	//This logic has been changed for devsqr... see below, may be because -1 is not written to file properly
	/*count = sizeof(data->blocks)/sizeof(int);
	i=0;
	while(data->blocks[i] != -1 && i < count) {
		free_block = (int*)malloc(sizeof(int));
		*free_block = data->blocks[i];
		start = list_insert_tail(start,free_block);

		//Just to make sure that we dont do something silly when we runover this node again
		data->blocks[i] = -1;
		i++;
	}
*/
	i=0;
	count = data->size/BLOCK_SIZE;
	//not multiple of blocksize
	if (data->size%BLOCK_SIZE != 0)
		count++;

	while(i < count) {
		if(data->blocks[i] < 0) {
			break;
		}
		free_block = (int*)malloc(sizeof(int));
		*free_block = data->blocks[i];

		//devsqr does not work if we use insert_tail here, how ridiculous
		start = list_insert_tail(start,free_block);

		//Just to make sure that we dont do something silly when we runover this node again
		data->blocks[i] = -1;
		i++;
	}

	//Again, just to make sure we dont end up re-freeing, just a precautionary measure
	data->size=0;

	//Dont free the fd here, because the nary node still exists, free it later, probably DURING unmount
	//my_free(data);
	return 0;
}

static int empty_node_from_trash(t_node *node) {
	return nary_preOrder_process(node,on_delete_from_trash);
}

//If any of the insert fails we return error, could be the vfs file is corrupt
static int insert_file_descriptor(fd *data)
{

	int ret = 0;

	if (data == NULL) {
		return -1;
	}

	//First insert to N-ary tree.
	ret = insert_node_with_action(&(root),data,empty_node_from_trash);
	if (ret != 0) {
		goto FINISH;
	}

	//Dir dont go into bst and hash i guess,
	//Put this block in a separate function
	if (data->is_dir != 1) {
		//Now insert into bst
		//Not sure if we need to do this only for files
		ret = bst_insert(&(bst_root),data);
		if (ret != 0) {
			goto FINISH;
		}

		//Insert to the hash table
		//Not sure if we need to do this only for files.
		ret = hash_insert(h_bucket,data);
		if (ret != 0) {
			goto FINISH;
		}
	}
	FINISH:
	//may be free any temp memory here...
	return ret;
}

/*
 * Populates all the data structues from the file system
 * Note: we do not check if there is already a label mounted,
 * The caller has to make sure that any existing label is unmounted before calling this.
 */
int mount (char *label1){

	FILE *fp = NULL;
	fd *data = NULL;
	int i,*free_block = NULL, retVal = 0;

	if (label1 == NULL)
		return -1;

	//Populate some variables
	populate_size_meta_header();

	//Does the label end with .fs or not...
	//Assume it does not, hence append it
	populate_vfs_file_name(label1);

	fp = fopen(label,"rb");

	if (!fp){
		retVal = ERROR_READING_FILE;
		goto FINISH;
	}

	//Create the root node of the data structures
	//Note: All previously allocated memory should be freed inside this.
	//Generally the memory will be freed when unmount is called, but to be safe...
	initializeRootNodes();

	//Read the meta header and check success of failure
	read_meta_header(fp);

	//Do the corruption check, we use header 1 and header 2 size for doing this
	if (size_of_header_1 != get_size_header_1(max_fd) || size_of_header_2 != get_size_header_2(max_fd)){
		retVal = ERROR_VFS_CORRUPTED;
		goto FINISH;
	}

	//Other sanity check is the no of fd < max_fd
	if (no_of_fd > max_fd) {
		retVal = ERROR_VFS_CORRUPTED;
		goto FINISH;
	}

	//Read the file descriptors and insert to the data structures
	for (i = 0; i < no_of_fd; ++i) {
		data = (fd*)malloc(sizeof(fd));
		fread(data,sizeof(fd),1,fp);
		retVal = insert_file_descriptor(data);
		if (retVal != 0) {
			retVal = ERROR_VFS_CORRUPTED;
			my_free(data);
			goto FINISH;
		}
	}

	//Now take the fp to the start of header-2
	fseek(fp,size_meta_header+size_of_header_1,SEEK_SET);

	for (i = 0; i < max_fd; ++i) {
		free_block = (int*)malloc(sizeof(int));
		fread(free_block,sizeof(int),1,fp);

		//Insert it to the linked list,
		//Not sure where to insert, so just insert to tail
		start = list_insert_tail(start,free_block);

	}

	//All metadata and headers have been successfully loaded

	FINISH:
	//Finally dont forget to free mem and close the file
	my_fclose(fp);

	//Free the root nodes if mount was not success
	if (retVal != 0) {
		free_all_nodes();
	}

	return retVal;
}


//This will actually be called from within the nary_preOrder function
static int write_nary_helper (t_node *node) {

	//on_delete_from trash takes care of checking if the node is in recycle bin,
	//if so it will permanently delete it and add the blocks to free list
	on_delete_from_trash(node);

	//If deleted just return, dont write
	if (node->is_deleted != 0)
		return 0;

	//global variable initialized in node_write_data
	fd_count++;

	//printf(" %s %s\n", node->data->path,node->data->file_name);

	//Assume when this is called the temp_file_pointer is not null
	return fwrite(node->data,sizeof(fd),1,temp_file_pointer) - 1;

	//Remember fwrite returns the number of records written, it returns 1 in this case,
	//hence the function returns 0 on success.
}


/*
 *This will invoke write the fd's to the file in pre-order
 * Returns the number of nodes written. Isn't it cool...
 */
static int write_nary_data(FILE *fp){

	//Global vars, which will be used inside the write_nary_helper
	temp_file_pointer = fp;
	fd_count = 0;

	//Pass the helper function as a function pointer to preOrder
	if (nary_preOrder_process(root,write_nary_helper) != 0) {
		return -1;
	}
	return fd_count;
}

int persistToFile(char* label1){

	FILE *fp = NULL;
	int retVal = 0, free_list_count, i, temp;

	if (label1 == NULL)
		return -1;

	//Populating this for devsqr i guess
	populate_size_meta_header();

	//Check if the fs is mounted or not, dont think its a good idea to check this here, anyway
	if (root == NULL) {
		return ERROR_NOT_MOUNTED;
	}

	populate_vfs_file_name(label1);

	//Open the file for writing in r+ mode, which enables to overwrite the bytes
	fp = fopen(label,"rb+");
	if (!fp) {
		retVal = ERROR_READING_FILE;
		goto FINISH;
	}

	//Corruption prevention - write 0 in place of size of header1 and header2 in meta header,
	//Some values are set to 0 before any write op, then set to the right value after write is complete
	write_meta_header_incorrect(fp);

	//Write header-1
	//Before writing nary, we need to empty trash, but no worries, it is already taken care in write_nary
	temp = write_nary_data(fp);
	if (temp == -1 || temp > max_fd){
		retVal = ERROR_WRITING_FILE_DESCRIPTORS;
		goto FINISH;
	}
	//actually no_of_fd should have been already incremented in add/remove file/dir
	no_of_fd = temp;

	//Before we write the header-2, set the fp to start of header-2
	fseek(fp,size_meta_header+size_of_header_1,SEEK_SET);

	//Write the header-2
	free_list_count = list_write_data_int(start,fp);

	if (free_list_count == -1) {
		retVal = ERROR_WRITING_FILE;
		goto FINISH;
	}

	//Fill the remaining free list with -1
	temp = -1;
	for (i = free_list_count; i < max_fd; ++i) {
		if (fwrite(&temp,sizeof(int),1,fp) != 1) {
			retVal = ERROR_WRITING_FILE;
			goto FINISH;
		}
	}

	//Re-write the meta header, with the correct value
	write_meta_header(fp);

	FINISH:
	//Finally dont forget to free mem and close the file
	my_fclose(fp);

	return retVal;
}

/* Write the file descriptors and the free block list into the file. For the free block list, fill rest of the blocks (block numbers) with -1.(If sizeof the list is less than total number of blocks as calculated in create )  */
int unmount(char* label1){

	int retVal = 0;

	retVal = persistToFile (label1);

	//Need to free memory of all root nodes, if success
	if (retVal == 0) {
		my_free(label);	
		free_all_nodes();
	}
	return retVal;
}


int create_dir(char *path, char *dirname) {

	fd *data = NULL;
	int ret = 0;

	//Need to check if the filename starts with char or number, atleast in main

	//Assume fs already mounted

	//Check if the no of fd limit is reached
	if (no_of_fd >= max_fd) {
		return ERROR_MAX_FD_REACHED;
	}

	createFD(path,dirname,1, 0, &data);

	//todo: Check if the node is in recycle bin, if so prompt user to clear from bin
	//because nary insert will permanently clear the file if it is in the recycle bin
	ret = insert_file_descriptor(data);

	if (ret != 0) {//failure
		my_free(data);
	} else {
		//Only increment the metaheader on success
		current_fd_id++;
		no_of_fd++;
	}

	return ret;

}

//Expose the empty trash methods later...
int empty_trash () {

	return empty_node_from_trash(root);
}

static void file_seek_to_block(FILE *fp, int block_number){
	fseek(fp,size_meta_header+size_of_header_1+size_of_header_2+(block_number*1024),SEEK_SET);
}

static int write_data_to_blocks(fd *data, char *file_data) {

	int no_of_bytes, len, count, *free_block;
	FILE *fp;

	len = data->size;

	fp = fopen(label,"rb+");
	if (fp == NULL)
		return 1;

	no_of_bytes = 0;
	count = 0;

	//We now go and actually write the data to the file
	//At this point we are sure that we have enuf free blocks, hence no need to worry abt end of freelist
	while(len > 0){
		no_of_bytes =(len > BLOCK_SIZE) ? BLOCK_SIZE : len;
		free_block = ((int*)start->data);

		//Fseek to that particular block
		file_seek_to_block(fp,*free_block);

		//Write the data and If we have not written expected number of bytes, some IO exception
		if (fwrite(file_data,sizeof(char),no_of_bytes,fp) != no_of_bytes ){
			my_fclose(fp);
			return ERROR_IOEXCEPTION;
		}

		//set the block number in fd
		data->blocks[count++]=*free_block;

		//remove it from free list
		start = list_delete_head(start);

		free(free_block);//just 4 bytes, but why let it leak unnecessarily
		file_data = file_data + no_of_bytes;
		len = len - no_of_bytes;
	}
	my_fclose(fp);
	return 0;
}

static int accomodate_data_in_blocks(fd *data) {

	int i;
	int len = data->size;
	l_node *list_head;

	//Check if the size is within 20 * blocks size ie 20K
	if(len > (sizeof(data->blocks)/sizeof(int)) * BLOCK_SIZE) {
		return ERROR_FILE_TOO_LARGE;//ERROR
	}


	//Check the size of file data, whether we can accomodate in freelist
	//Implies no more free blocks or we have managed to fit in our data in the blocks
	list_head = start;
	while(list_head != NULL && len > 0) {
		i = *((int*)(list_head->data));

		len = len - BLOCK_SIZE;

		list_head = list_head->next;
	}

	//Implies the available size in free list is less than data
	if (len > 0) {
		return ERROR_NO_FREE_BLOCKS;
	}

	return 0;
}

/*adds a new file with data in the path specified*/
//Just for testing
int add_file(char *path, char *filename, char *file_data) {

	fd *data = NULL;
	int ret = 0, len;

	if (file_data == NULL ){
		return 1;
	}

	//Need to check if the filename starts with char or number, atleast in main

	//Assume fs already mounted
	//Check if the no of fd limit is reached
	if (no_of_fd >= max_fd) {
		return ERROR_MAX_FD_REACHED;
	}

	len = strlen(file_data);
	createFD(path,filename,0, len, &data);

	//See if we can store the file_data to the file system
	ret = accomodate_data_in_blocks(data);

	//todo: If ret == ERROR_NO_FREE_BLOCKS, then we need to prompt user, if he wants to empty trash
	//But automatic empty trash will do it without users consent
	if (ret == ERROR_NO_FREE_BLOCKS && automatic_empty_trash) {
		empty_trash();
		//now give it a second try, after emptying the trash
		ret = accomodate_data_in_blocks(data);
	}

	//If still unable to accomodate data then, not enuf space
	if (ret != 0) {
		goto FINISH;
	}

	//Here we need to prompt user, if the same file exists in recyle bin, then should we delete it

	ret = insert_file_descriptor(data);

	if (ret != 0) {//failure, may be file alredy exists etc...
		goto FINISH;
	} else {
		//All set to store the data into the file...
		ret = write_data_to_blocks(data,file_data);
	}

	FINISH:
	//io error, we cant do much, but ask the user to delete the file manually
	if (ret != 0 && ret != ERROR_IOEXCEPTION) {
		my_free(data);
	} else {
		//Only increment the metaheader on success
		current_fd_id++;
		no_of_fd++;
	}

	return ret;

}

static int remove_from_bst_hash(fd *data){
	int ret;
	char *fullpath ;
	fullpath = (char*)malloc(sizeof(char)*(strlen(data->path) + strlen(data->file_name) + 2));//including '/' inbetween
	strcpy(fullpath,data->path);

	//Cond if path = root...
	if (data->path[strlen(data->path)-1]!='/')
		strcat(fullpath,"/");

	strcat(fullpath,data->file_name);

	//Now remove from the bst, this will always return success i guess
	//But comment this for devsqr, crazy isnt it...
	ret = bst_delete(&bst_root,fullpath);

	//To remove from hash, get the filename from fd
	ret = hash_delete(h_bucket,data->path,data->file_name);

	my_free(fullpath);
	return ret;
}

int remove_file(char *fullpath){

	int ret;
	fd *data;
	b_node *bst_node;


	if (fullpath == NULL )
		return -1;

	bst_node = bst_search(bst_root,fullpath);

	//Cud be that the file is already deleted, may be in recycle bin.
	if (bst_node == NULL){
		return ERROR_FILE_NOT_FOUND;
	}

	data = bst_node->file_desc;

	//we are supposed to remove a file in this method
	if (data->is_dir){
		return ERROR_IT_IS_A_DIR;
	}

	//mark as deleted in the nary, ie move it to trash
	//Hence no need to retrieve the blocks and add to free list
	ret = delete_node(&root,fullpath);

	//Hope is a good thing, and i hope this condition never occurs, hence commented
/*	if (ret!=0){
		printf("FATAL ERROR - Node present in bst but not in nary\n");
	}*/

	//Now remove from the bst and hash
	ret = remove_from_bst_hash(data);

	//But if devsqr demands, to free the list, then i will have to call empty trash here
	//empty_trash();

	//Update the meta header, we have just removed only one file, hence sub1
	no_of_fd--;

	//Dont free the FD, because we may need to restore it when we un-delete a dir
	//In fact we dont free fd even when permanently deleting a file
	return ret;
}

int remove_dir_helper(t_node *node){
	int ret = 0;

	//Decrement the fd only when the node is not deleted already, eg we cud have /home as not deleted
	//But /home/def cud be deleted and /home/abc may not be deleted
	if (node->is_deleted == 0) {
		ret = nary_deleteHelper(node);
		//if it is a file, then remove from bst, hash
		if (!node->data->is_dir)
			ret = remove_from_bst_hash(node->data);
		no_of_fd--;
	}
	return ret;
}

int remove_dir(char *path){

	int ret;
	t_node *node;

	if (path == NULL )
		return -1;

	node = returnnode(root,path);

	//We cudnt find the node or it is already deleted
	if (node == NULL || node->is_deleted != 0) {
		return ERROR_FILE_NOT_FOUND;
	}

	//we are supposed to remove a directory in this method
	if (!node->data->is_dir){
		return ERROR_IT_IS_A_FILE;
	}

	//mark as deleted in the nary, ie move it to trash
	//Hence no need to retrieve the blocks and add to free list
	ret = nary_preOrder_process(node,remove_dir_helper);

	//Call empty trash here if devsqr checks the free-list
	//empty_trash();

	//Dont free the FD, because we may need to restore it when we un-delete a dir

	return ret;
}

char* get_file_name(char* fullpath) {
	fd *data = get_node(&root,fullpath);
	if (data == NULL)
		return NULL;
	return data->file_name;
}

char* get_parent_path(char* fullpath) {
	fd *data = get_node(&root,fullpath);
	if (data == NULL)
		return NULL;
	return data->path;
	
}

int isDirectory (char *path) {

	fd* data;

	data = get_node(&root,path);
	if (data == NULL)	{
		return -1;
	}

	if (data->is_dir)
		return 1;

	return 0;
}

/* lists all the files in the directory specified in the path
Get the top level file descriptors. Print the files only. */
int list_files(char *path){
	int count, i;
	fd **data = NULL;

	count = get_children(&root,path,&data);

	if (count == -1) {
		printf("Invalid Path : %s\n",path);
		//Actually we need to return -1 here so that the caller knows that path is invalid, but
		//it all depends on the dev-ill-sqr
		return 0;
	}
	//Print the files only
	for (i = 0; i < count; ++i) {
		if (!data[i]->is_dir) {
			printf("FILE: %s\n",data[i]->file_name);
		} else {
			printf("DIR: %s\n",data[i]->file_name);
		}
	}

	//Dilemma, whether to free the mem allocated by get_children
	my_free(data);
	return count;
}

int get_file(char *fullpath, char *data) {

	fd *fd_data;
	int count, i, no_of_blocks_to_read;
	FILE *fp;

	if (data == NULL)
		return -1;

	//Assume enuf memory in data, ie atleast 20k
	//First get the node, if it exists and it shld be a file
	fd_data = get_node(&root,fullpath);
	if (fd_data == NULL || fd_data->is_dir){
		return -1;
	}


	//Idea is to read n-1 blocks with 1k size and only the last block with size - i*block-size
	fp = fopen(label,"rb");

	//This will floor the value, hence it will be n or n-1
	no_of_blocks_to_read = (fd_data->size / BLOCK_SIZE);

	count = 0;
	for (i = 0; i < no_of_blocks_to_read; ++i) {
		file_seek_to_block(fp,fd_data->blocks[i]);
		//Return -1 if error in reading the file
		fread(data+count,sizeof(char),BLOCK_SIZE,fp);
		//dont worry, this wont modify the pointer address at the caller
		count = count + BLOCK_SIZE;
	}

	//Now we read the last block, provided we have something to read
	if (fd_data->size - count > 0) {
		file_seek_to_block(fp,fd_data->blocks[i]);

		fread(data+count,sizeof(char),fd_data->size - count,fp);
	}

	//?? do we need to put '\0' at the last character of data
	//data[fd_data->size]='\0';

	//iam not sure if it is a good idea to keep the file open throughout
	my_fclose(fp);

	return fd_data->size;

}

int search_files(char *fileName)
{
  int searchResult = 0;
  printf("Name of the file entered %s\n", fileName);

  b_node *bst_node;


 if (fileName == NULL )
	return 0;

 bst_node = bst_search(bst_root,fileName);
 
 if (bst_node == NULL)
	return 0;
 else
	return 1;
	
 return searchResult;
}

