
/*
 * vfs_bst.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "VFS_BST.h"
#include "llist.h"


//This method is just for convention sake
b_node *bst_create() {
	return NULL;
}

/*searches the BST on the basis of fullpath and returns the searched node*/
int bst_search_file( b_node *bst_root, char *filename ) {

  /*while(bst_root != NULL) {

		int i = strlen(bst_root->full_path);
		//printf("length of the string is %d\n", i);
		//printf("path name %s\n", bst_root->full_path);

		while(bst_root->full_path[i] != '/')
		  {
		    i--;
		  }

		char * temp = (char *)malloc(1 * sizeof(char));
		//char * filename = (char *) malloc(1* sizeof(char));
		//filename = "test";
		temp = strncpy(temp, bst_root->full_path, i);
		temp[i] = 0;
		strcat(temp, "/");
		strcat(temp, filename);

		printf("temp value %s\n", temp);
		printf("full path %s\n", bst_root->full_path);
		
		int cmp = strcmp(temp, bst_root->full_path);
		
		if (cmp < 0) {
			bst_root = bst_root->left;
		} else if (cmp > 0) {
			bst_root = bst_root->right;
		}else {//implies we found the node
			return bst_root;
		}


	}
	return NULL;
  */

  int count = 0;
  b_node *listOfFds = NULL;
  //count =
  dfs_search(bst_root, filename, &count, &listOfFds);
  printf("Count after search: %d\n", count);
  return count;
}


/*searches the BST on the basis of fullpath and returns the searched node*/
b_node *bst_search( b_node *bst_root, char *fullpath ) {
    
	while(bst_root != NULL) {
		int cmp = strcmp(fullpath, bst_root->full_path);
		if (cmp < 0) {
			bst_root = bst_root->left;
		} else if (cmp > 0) {
			bst_root = bst_root->right;
		}else {//implies we found the node
			return bst_root;
		}
	}
	return NULL;
}



/* Creates a new node based on given data and inserts into the BST. Returns 0 on success,
 * 1 in case of duplicate entry; the root argument gets updated */
int bst_insert ( b_node **bst_root_ptr, fd *data ) {
	char fullPath[MAX_PATH_SIZE+MAX_FILENAME_SIZE+1];// +1 for '/'
	b_node *root;
	b_node *newNode;
	b_node *parent;
	//This stores the address of the left or right pointer of the parent
	b_node **insertAt;
	int cmp;

	//Some error check, may be unnecessary
	if (bst_root_ptr == NULL) {
		return BST_ROOT_IS_NULL;
	}

	strcpy(fullPath,data->path);
	//THis cond is when path = '/'
	if (data->path[strlen(data->path)-1] != '/')
		strcat(fullPath,"/");
	strcat(fullPath,data->file_name);

	root = *bst_root_ptr;
	parent = NULL;

	while (root != NULL) {
		parent = root;
		cmp = strcmp(fullPath, root->full_path);
		if (cmp < 0) {
			//This is the node into which we will store the new node
			insertAt = &(root->left);
			root = *insertAt;
		} else if (cmp > 0) {
			insertAt = &(root->right);
			root = *insertAt;
		} else {//Duplicate key
			return BST_DUPLICATE_KEY;
		}


	}
	//Allocate the memory to new node and populate the fields
	newNode = (b_node*)malloc(sizeof(b_node));
	strcpy(newNode->full_path,fullPath);
	newNode->file_desc = data;
	newNode->left = NULL;
	newNode->parent = parent;
	newNode->right = NULL;

	//We now need to assign the new node as the child of parent node
	//But before that we need to check if we infact have a root or not.
	if (*bst_root_ptr == NULL) {
		*bst_root_ptr = newNode;
	} else {
		*insertAt = newNode;
	}
	return BST_SUCCESS;

}

/* Creates a new node based on given data and inserts into the BST. Returns 0 on success,
 * 1 in case of duplicate entry; the root argument gets updated */
int bst_insert_with_ll ( b_node **bst_root_ptr, fd *data ) {

  char fullPath[MAX_PATH_SIZE+MAX_FILENAME_SIZE+1];// +1 for '/'
  char fileName[MAX_FILENAME_SIZE+1];
  b_node *root;
  b_node *newNode;
  b_node *parent;
  //This stores the address of the left or right pointer of the parent
  b_node **insertAt;
  int cmp, filecmp;

  //Some error check, may be unnecessary
  if (bst_root_ptr == NULL) {
    return BST_ROOT_IS_NULL;
  }

   strcpy(fullPath,data->path);	//THis cond is when path = '/'
   strcpy(fileName, data->file_name);

   //this isn't necessary for filename
   if (data->path[strlen(data->path)-1] != '/')
     strcat(fullPath,"/");

   //this isn't necessary for filename
   strcat(fullPath,data->file_name);

   root = *bst_root_ptr;
   parent = NULL;

   while (root != NULL) {
      parent = root;
      //cmp = strcmp(fullPath, root->full_path);
      filecmp = strcmp(fileName, root->file_desc->file_name);

      /*
      if (cmp < 0) {
	//This is the node into which we will store the new node
	insertAt = &(root->left);
	root = *insertAt;
      } else if (cmp > 0) {
	insertAt = &(root->right);
	root = *insertAt;
      } else {
	//Duplicate key
	return BST_DUPLICATE_KEY;
       }
      */
      //compare the file name and if same file name, compare the path to determine duplicates
      if (filecmp < 0){
	insertAt = &(root->left);
	root = *insertAt;
      }
      else if (filecmp > 0){
	insertAt = &(root->right);
	root = *insertAt;
      }
      else{

        //same file
	if (strcmp(data->path, root->file_desc->path) == 0)
	  {
	    printf("same name and same file (IGNORE)");
	    printf("filenames %s and %s\n", data->file_name, root->file_desc->file_name);
	  }
	//different files with the same name
	else
	  {
	    //insert in linked list
	    printf("same file but different paths (ADD) - same filename %s and %s\n", data->path, root->file_desc->path);

	    //root->nextFile = list_insert_tail(root->nextFile,data);
	  }

	return BST_DUPLICATE_KEY;
      }
   }
	//Allocate the memory to new node and populate the fields
	newNode = (b_node*)malloc(sizeof(b_node));
	strcpy(newNode->full_path,fullPath);
	newNode->file_desc = data;
	newNode->left = NULL;
	newNode->parent = parent;
	newNode->right = NULL;
	newNode->nextFile = NULL;
	
	//We now need to assign the new node as the child of parent node
	//But before that we need to check if we infact have a root or not.
	if (*bst_root_ptr == NULL) {
		*bst_root_ptr = newNode;
	} else {
		*insertAt = newNode;
	}
	return BST_SUCCESS;

}

/* deletes the bst node and returns root */
int bst_delete(b_node **bst_root_ptr, char *fullpath) {
	b_node *toDelete, *successor;
	b_node **toManipulate, **tempLink;

	//As usual first do error check
	if (bst_root_ptr == NULL || *bst_root_ptr == NULL)
		return BST_ROOT_IS_NULL;

	//Search if the fullpath exists in bst
	toDelete = bst_search(*bst_root_ptr,fullpath);
	if (toDelete == NULL)
		return BST_KEY_NOT_FOUND;

	//Now check if the node to be deleted is a left or right child
	//And store the corresponding pointer's address eg &(parent->left)

	if (toDelete->parent == NULL){
		//implies we are deleting the root node
		toManipulate = bst_root_ptr;
	}else if (toDelete->parent->left == toDelete) {
		toManipulate = &(toDelete->parent->left);
	} else {
		toManipulate = &(toDelete->parent->right);
	}

	//Now we need to address the three cases of deletion
	//1,2 - Case with no or only one child
	if (toDelete->left == NULL){
		*toManipulate = toDelete->right;
		//change the parent ptr of the child of toDelete
		if (toDelete->right != NULL){
			toDelete->right->parent = toDelete->parent;
		}
	} else if (toDelete->right == NULL){
		*toManipulate = toDelete->left;
		//change the parent ptr of the child of toDelete
		if (toDelete->left != NULL){
			toDelete->left->parent = toDelete->parent;
		}
	} else { //This is case-3 where there are both children
		//First find the in-order successor of toDelete
		tempLink = &(toDelete->right);
		successor = toDelete->right;
		while(successor->left != NULL) {
			tempLink = &(successor->left);
			successor = successor->left;
		}

		//Now replace the toDelete with successor
		//There are two ways to do this, either copy the contents
		//Or manipulate the links. We will try the latter.

		//So, first do the code as deleting the successor, and successor can
		//never have two children.
		*tempLink = successor->right;
		if (successor->right != NULL) {
			successor->right->parent = successor->parent;
		}

		//We now have removed the successor from the tree, so, we insert
		//it at the position where toDelete exists.
		*toManipulate = successor;
		successor->left = toDelete->left;
		successor->right = toDelete->right;
		successor->parent = toDelete->parent;

	}

	free(toDelete);
	return BST_SUCCESS;
}

void print_inorder (b_node *root){
	if (root == NULL) {
		return;
	}
	print_inorder(root->left);
	printf("%s\n",root->full_path);
	print_inorder(root->right);
}

void dfs_search(b_node *root, char *searchFile, int *foundCount, b_node ** foundElements)
{
  if(root ==NULL || searchFile == NULL)
    {
      printf("Either root or search file is not entered.\n");
      //return 0;
    }

  //if you have a left child traverse in that direction
  if(root->left !=NULL)
    {
      //foundCount =
      dfs_search(root->left, searchFile, foundCount, foundElements);
    }

  //same for the right child
  if(root->right != NULL)
    {
      //foundCount =
	dfs_search(root->right, searchFile, foundCount, foundElements);
    }

  //search if the input matches
  int i = strlen(root->full_path);
		//printf("length of the string is %d\n", i);
		//printf("path name %s\n", bst_root->full_path);

  while(root->full_path[i] != '/')
    {
      i--;
    }

		char * temp = (char *)malloc(1 * sizeof(char));
		//char * filename = (char *) malloc(1* sizeof(char));
		//filename = "test";
		temp = strncpy(temp, root->full_path, i);
		temp[i] = 0;
		strcat(temp, "/");
		strcat(temp, searchFile);

		//printf("temp value %s\n", temp);
		//printf("full path %s\n", root->full_path);
		
		int cmp = strcmp(temp, root->full_path);

		if (cmp == 0)
		  {
		    //foundCount++;
		    int temp = * foundCount;
		    temp++;
		    *foundCount = temp;
		    printf("found and count increased to %d\n", *foundCount);

		    
		    b_node * tempFd = malloc(1*sizeof(b_node));
		    tempFd = root;
		    *foundElements = malloc(1*sizeof(b_node));
		    *foundElements = tempFd;

		    printf("tempfd - %d\n", tempFd);
		    printf("foundElements - %d\n", *foundElements);
		    printf("foundElements own address - %d\n", foundElements);
		    printf("Found - file: %s", tempFd->file_desc->path);
		    //printf("%s\n", root->file_desc->file_name);
		    //return foundCount;
		  }
		else
		  {
		    //printf("Not Found\n");
		    //return foundCount;
		  }

}

