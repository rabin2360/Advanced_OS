/*
 * vfs_bst.h
 *
 *  Created on: Oct 21, 2009
 *      Author: Arun k
 */

#ifndef VFS_BST_H_
#define VFS_BST_H_

#include "globals.h"

#define C_OR_CPP 1

#define NIL '\0'
#define BST_TRUE 1
#define BST_FALSE 0
#define BST_SUCCESS 0
#define BST_EMPTY 1
//It was '2' earlier, but devsquare expects this to be 1.
#define BST_KEY_NOT_FOUND 1
#define BST_FAILURE 100
#define BST_ROOT_IS_NULL -1
#define BST_DUPLICATE_KEY 1

typedef struct bst_node
{
	struct bst_node *parent;
	struct bst_node *left;
	struct bst_node *right;
	char full_path[MAX_PATH_SIZE+MAX_FILENAME_SIZE];
	fd *file_desc;
}b_node;


b_node *bst_create();
/*Creates the bst, returns the root b_node*/

b_node *bst_search( b_node *bst_root, char *fullpath );

/*searches the BST on the basis of fullpath and returns the searched node*/
int bst_insert ( b_node **bst_root_ptr, fd *data );


/* insert the bst node and returns root */
int bst_delete(b_node **bst_root_ptr, char *fullpath);

/* deletes the bst node and returns root */
/*Prints the bst*/
void print_inorder(b_node *bst_root);


#endif /* VFS_BST_H_ */
