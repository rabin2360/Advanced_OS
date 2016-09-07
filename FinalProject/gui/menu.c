#include <gtk/gtk.h>
#include<string.h>
#include<stdlib.h>
#include <vfs.h>
#include <globals.h>

#include <vfs_error_codes.h>


#define IS_DIR_COLUMN 2

extern t_node *root;
extern int max_fd, vfs_size_in_kb, no_of_fd;
static GtkWidget *main_window;
static char *vfs_path = NULL;
static int is_mounted = 0;

static void show_message_dialog(gpointer window, GtkMessageType type, const gchar *msg)
{
  GtkWidget *dialog;
  dialog = gtk_message_dialog_new(window,
            GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_MODAL,
            type,
            GTK_BUTTONS_OK,
            msg);
  //gtk_window_set_title(GTK_WINDOW(dialog), "Information");
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

static GtkTreeModel *
create_empty_model ()
{
  GtkTreeStore *treestore;
  GtkTreeIter toplevel;

  treestore = gtk_tree_store_new(3,
                  G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

  gtk_tree_store_append(treestore, &toplevel, NULL);
  gtk_tree_store_set(treestore, &toplevel,
                     0, "/  (Root File System)", 1, "/",IS_DIR_COLUMN,1,
                     -1);

  return GTK_TREE_MODEL(treestore);
}

  GtkTreeStore *treestore;
static char temp[MAX_FILENAME_SIZE + MAX_PATH_SIZE + 5];
static int isFirst = 1;

static void populateModel (t_node *root, GtkTreeIter *parent){
	t_node *child;
	int len;
	GtkTreeIter current;

	if (root == NULL){
		return;
	}

	if (!isFirst) {
		temp[0]='\0';
		strcpy(temp,root->data->path);
		len = strlen(temp);
		if (temp[len-1] != '/')
				strcat (temp, "/");
		strcat(temp, root->data->file_name);

	  gtk_tree_store_append(treestore, &current, parent);
	  gtk_tree_store_set(treestore, &current,
                     0, root->data->file_name, 1, temp,IS_DIR_COLUMN, isDirectory(temp),-1);
	} else {
		current=*parent;
	}
	isFirst = 0;
	child = root->left_child;
	if (child != NULL) {
		populateModel(child, &current);
	} else {
		return;
	}
	child = child->right_siblings;
	while(child != NULL) {
		populateModel(child, &current);	
		child = child->right_siblings;
	}

	return;

}

static GtkTreeModel * load_model() {

  GtkTreeIter toplevel, child;

  treestore = gtk_tree_store_new(3,
                  G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

  gtk_tree_store_append(treestore, &toplevel, NULL);
  gtk_tree_store_set(treestore, &toplevel,
                     0, "/  (Root File System)", 1, "/",IS_DIR_COLUMN,1,
                     -1);

	isFirst = 1;
	populateModel(root, &toplevel);

  return GTK_TREE_MODEL(treestore);
}

static void select_the_root_node (gpointer     callback_data) {

	GtkTreeSelection  *selection;
	GtkTreeIter iter;

	//TODO : Select the first row by default
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(callback_data));
	gtk_tree_selection_unselect_all(selection);
	gtk_tree_model_get_iter_from_string(gtk_tree_view_get_model(GTK_TREE_VIEW(callback_data)), &iter, "0");
	gtk_tree_selection_select_iter(selection, &iter);
}

static void action_about( gpointer     callback_data,
                                                    guint            callback_action,
                                                    GtkWidget *menu_item ) {

show_message_dialog(main_window, GTK_MESSAGE_INFO, "\n\nDeveloper: Arun Kalyanasundaram\n\n Email: arun.k@iiitb.net\n");
}

static void action_emp_trash( gpointer     callback_data,
                                                    guint            callback_action,
                                                    GtkWidget *menu_item ) {

//Trash anyway gets emptied when unmounted and also automatic_empty_trash enabled
//empty_trash ();
show_message_dialog(main_window, GTK_MESSAGE_INFO, "Successfully emptied trash");

}

static void under_construction( gpointer     callback_data,
                                                    guint            callback_action,
                                                    GtkWidget *menu_item ) {

show_message_dialog(main_window, GTK_MESSAGE_INFO, "\n\nSorry! This Feature is still under construction.   \n\nDeveloper: Arun Kalyanasundaram\nEmail: arun.k@iiitb.net\n");
}


static void action_other_op( gpointer     callback_data,
                                                    guint            callback_action,
                                                    GtkWidget *menu_item ) {

show_message_dialog(main_window, GTK_MESSAGE_INFO, "\n\nOther operations can be performed using the context menu on each file / folder.");
}

static void action_view_prop( gpointer     callback_data,
                                                    guint            callback_action,
                                                    GtkWidget *menu_item ) {


char temp[200];

sprintf(temp,"\n\nVFS File: %s\n\n Size = %f Kilo Bytes\n\n Max no. of Files / Folders = %d\n\nCurrent no. of Files / Filders = %d\n",vfs_path, get_file_size(), max_fd, no_of_fd);

show_message_dialog(main_window, GTK_MESSAGE_INFO, temp);

}

//callback_data is the tree view
static void action_open_vfs( gpointer     callback_data,
                                                    guint            callback_action,
                                                    GtkWidget *menu_item )
{

	GtkWidget* dialog;
	gint result;
	GtkTreeModel *model;
	int ret,len;

	char *title = NULL;
	char *temp_path=NULL;

	dialog = gtk_file_chooser_dialog_new ("Choose the VFS File", GTK_WINDOW(main_window), GTK_FILE_CHOOSER_ACTION_OPEN,GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL );

	gtk_widget_set_size_request (dialog, 400, 250);
	gtk_widget_show_all (dialog);

	result = gtk_dialog_run (GTK_DIALOG (dialog));

	switch (result) {
		case GTK_RESPONSE_OK:

			//TODO: Warn the user, if required to save existing and do unmount
			//Unmount existing one
			//unmount(vfs_path);

			temp_path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
			ret = mount (temp_path);

			if (ret != 0) {
				show_message_dialog(dialog, GTK_MESSAGE_ERROR, "Error Opening VFS File!");
				printf("Error -  open\n");
				break;
			}

			//create the model in the tree view
			model = load_model();
			gtk_tree_view_set_model(GTK_TREE_VIEW(callback_data), model);
			g_object_unref(model);
			select_the_root_node(callback_data);

			len = strlen((char*)temp_path);
			my_free(vfs_path);
			vfs_path = (char*)malloc(len+1);
			strcpy(vfs_path,(char*)temp_path);

			//TODO : just show the file name in title not the whole vfs_path
			title = malloc(strlen(vfs_path)+50);
			strcpy(title, "Virtual File System - ");
			strcat (title, vfs_path);
		    gtk_window_set_title(GTK_WINDOW(main_window), title);

			gtk_tree_view_expand_row (GTK_TREE_VIEW(callback_data), gtk_tree_path_new_from_string("0"), FALSE);
			
			is_mounted = 1;
			break;
	}

	my_free(title); my_free(temp_path);
	gtk_widget_destroy(dialog);

}

//callback_data is the tree view
static void action_create_vfs( gpointer     callback_data,
                                                    guint            callback_action,
                                                    GtkWidget *menu_item )
{
	GtkWidget* dialog, *content_area, *label1, *path, *label2, *size;
	gint result;
	GtkTreeModel *model;

	const gchar *size_text, *path_text;
	int size_int, ret, len;
	FILE *fp = NULL;
	char *title = NULL;

	dialog = gtk_dialog_new_with_buttons ("Create VFS", NULL, GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         NULL);

	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	label1 = gtk_label_new ("  Enter the Path of the VFS file to be created:    ");
	gtk_box_pack_start(GTK_BOX(content_area), label1, FALSE, TRUE, 5);

	path = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(content_area), path, TRUE, TRUE, 5);


	label2 = gtk_label_new ("Enter the size in KiloBytes (KB):");
	gtk_box_pack_start(GTK_BOX(content_area), label2, FALSE, TRUE, 5);

	size = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(content_area), size, TRUE, TRUE, 5);

	gtk_widget_show_all (dialog);

	result = gtk_dialog_run (GTK_DIALOG (dialog));

	path_text = gtk_entry_get_text (GTK_ENTRY (path));
	size_text = gtk_entry_get_text (GTK_ENTRY (size));

	switch (result) {
		case GTK_RESPONSE_OK:
			sscanf(size_text,"%d", &size_int);
			printf("Path: %s\nSize: %d\n", path_text,size_int);

			//TODO: Warn the user, if required to save existing and do unmount
			//unmou existing
			//unmount(vfs_path);

			len = strlen((char*)path_text);
			my_free(vfs_path);
			vfs_path = (char*)malloc(len+1);
			strcpy(vfs_path,(char*)path_text);

			fp = fopen (vfs_path, "r");
			if ((fp != NULL)) {
				show_message_dialog(dialog, GTK_MESSAGE_ERROR, "Error file already exists!");
				printf("Error - file already exists\n");
				fclose(fp);
				break;
			} 

			ret = create(vfs_path,size_int);
			if (ret != 0) {
				show_message_dialog(dialog, GTK_MESSAGE_ERROR, "Error creating VFS file!");
				printf("Error - create\n");
				break;
			}
			ret = mount (vfs_path);
			if (ret != 0) {
				show_message_dialog(dialog, GTK_MESSAGE_ERROR, "Error Mounting VFS File!");
				printf("Error -  mount\n");
				break;
			}

			//create the model in the tree view
			model = create_empty_model();
			gtk_tree_view_set_model(GTK_TREE_VIEW(callback_data), model);
			g_object_unref(model);
			select_the_root_node(callback_data);

			title = malloc(strlen(vfs_path)+50);
			strcpy(title, "Virtual File System - ");
			strcat (title, vfs_path);
		    gtk_window_set_title(GTK_WINDOW(main_window), title);

			//show_message_dialog(dialog, GTK_MESSAGE_INFO, "Successfully created the Virtual File system");
			gtk_tree_view_expand_row (GTK_TREE_VIEW(callback_data), gtk_tree_path_new_from_string("0"), FALSE);
			is_mounted = 1;
			break;
	}
	my_free(title);
	gtk_widget_destroy(dialog);
}

//callback_data is the tree view
static void action_save_vfs( gpointer     callback_data,
                                                    guint            callback_action,
                                                    GtkWidget *menu_item )
{
	int ret;

	if (!is_mounted) {
		show_message_dialog(main_window, GTK_MESSAGE_ERROR, "No VFS available to save!");
		return;
	}
	ret = persistToFile(vfs_path);

	if (ret != 0) {
		show_message_dialog(main_window, GTK_MESSAGE_ERROR, "Error Saving VFS");
		return;
	}
}

//callback_data is the tree view
static void action_move_dir_trash( gpointer     callback_data,
                                                    guint            callback_action,
                                                    GtkWidget *menu_item )
{
    GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	char *full_path;
	int ret;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(callback_data));

	 gtk_tree_selection_get_selected(selection, &model, &iter);
	//Get the second col value which has the full path
	 gtk_tree_model_get (model, &iter, 1, &full_path, -1);
		if (strcmp(full_path,"/") == 0 )
		{
			show_message_dialog(main_window, GTK_MESSAGE_ERROR, "Cannot delete the entire file system!");
			return;
		}
	 ret = remove_dir(full_path);

	if (ret != 0 )
	{
		show_message_dialog(main_window, GTK_MESSAGE_ERROR, "Error deleting Folder");
		return;
	}
	gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
	show_message_dialog(main_window, GTK_MESSAGE_INFO, "Successfully deleted folder.\nNote: Restore from trash is not yet available.");
	select_the_root_node(callback_data);

}

static void action_move_file_trash( gpointer     callback_data,
                                                    guint            callback_action,
                                                    GtkWidget *menu_item )
{
    GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	char *full_path;
	int ret;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(callback_data));

	 gtk_tree_selection_get_selected(selection, &model, &iter);
	//Get the second col value which has the full path
	 gtk_tree_model_get (model, &iter, 1, &full_path, -1);
	 ret = remove_file(full_path);

	if (ret != 0 )
	{
		show_message_dialog(main_window, GTK_MESSAGE_ERROR, "Error deleting File");
		return;
	}
	gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
	show_message_dialog(main_window, GTK_MESSAGE_INFO, "Successfully deleted file.\nNote: Restore from trash is not yet available.");
	select_the_root_node(callback_data);
}


static void action_search_vfs( gpointer     callback_data,
                                                    guint            callback_action,
                                                    GtkWidget *menu_item )
{

	GtkWidget* dialog, *content_area, *label1, *filePath;
	
	gint result;
	GtkTreeModel *model;

	const gchar *size_text, *path_text;
	int size_int, ret, len;
	FILE *fp = NULL;
	char *title = NULL;

	dialog = gtk_dialog_new_with_buttons ("Search VFS", NULL, GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         NULL);

	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	label1 = gtk_label_new ("  Enter the name of the file to be searched:    ");
	gtk_box_pack_start(GTK_BOX(content_area), label1, FALSE, TRUE, 5);

	filePath = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(content_area), filePath, TRUE, TRUE, 5);

	gtk_widget_show_all (dialog);

	result = gtk_dialog_run (GTK_DIALOG (dialog));

	path_text = gtk_entry_get_text (GTK_ENTRY (filePath));

	int searchResult;
	switch (result) {
		case GTK_RESPONSE_OK:
		//printf("Ok button pressed\n");
		searchResult = search_files((char *)path_text);
		
		if(searchResult)
		  printf("Found the file!\n");
		else
		  printf("File not found!\n");
		break;
		
		default:
		
		break;
	}

	gtk_widget_destroy(dialog);
}

static GtkItemFactoryEntry menu_items[] = {
    { "/_File",                 NULL,                 NULL,                     0, "<Branch>" },
    { "/File/Create _New VFS",         "<control>N", action_create_vfs,        1, "<Item>" },
    { "/File/_Open VFS",         "<control>O", action_open_vfs,        1, "<Item>" },
    { "/File/_Save VFS",         "<control>S", action_save_vfs,        1, "<Item>" },
    { "/File/Save VFS _As",         "<control>A", under_construction,        1, "<Item>" },
    { "/File/E_xit",         NULL, NULL,        1, "<Item>" },
    { "/_Actions",                 NULL,                 NULL,                     0, "<Branch>" },
    { "/Actions/_Empty Trash",         NULL, action_emp_trash,        1, "<Item>" },
    { "/Actions/_Search Files",         NULL, action_search_vfs,        1, "<Item>" },
    { "/Actions/sep",   NULL,         NULL,           0, "<Separator>" },
    { "/Actions/_View Properties",         "<control>P", action_view_prop,        1, "<Item>" },
    { "/Actions/_Other Operations",         NULL, action_other_op,        1, "<Item>" },
    { "/_Help",                 NULL,                 NULL,                     0, "<Branch>" },
    { "/Help/_About",     NULL,                 action_about,                     1, "<Item>" },
};

/* Returns a menubar widget made from the above menu */
GtkWidget *get_menubar_menu( GtkWidget  *window, gpointer view )
{
 gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);
    GtkItemFactory *item_factory;
    GtkAccelGroup *accel_group;

	main_window = window;
    /* Make an accelerator group (shortcut keys) */
    accel_group = gtk_accel_group_new ();

    /* Make an ItemFactory (that makes a menubar) */
    item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<vfsmain>",
                                                                             accel_group);

    /* This function generates the menu items. Pass the item factory,
         the number of items in the array, the array itself, and any
         callback data for the the menu items. */
    gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, view);

    /* Attach the new accelerator group to the window. */
    gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

    /* Finally, return the actual menu bar created by the item factory. */
    return gtk_item_factory_get_widget (item_factory, "<vfsmain>");
}

//callback_data is the tree view
static void context_new_folder( gpointer     callback_data,
                                                    guint            callback_action,
                                                    GtkWidget *menu_item )
{

    GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	const gchar *full_path, *dir_name;
	char temp[MAX_FILENAME_SIZE + MAX_PATH_SIZE + 5];
	int ret,len;
	GtkWidget* dialog, *content_area, *label1, *name;
	gint result;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(callback_data));

	 gtk_tree_selection_get_selected(selection, &model, &iter);
	//Get the second col value which has the full path
	 gtk_tree_model_get (model, &iter, 1, &full_path, -1);

	dialog = gtk_dialog_new_with_buttons ("Create Directory", NULL, GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT, 
		GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         NULL);
	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	label1 = gtk_label_new ("  Enter the name of the directory to create:    ");
	gtk_box_pack_start(GTK_BOX(content_area), label1, FALSE, TRUE, 5);

	name = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(content_area), name, TRUE, TRUE, 5);

	gtk_widget_show_all (dialog);

	result = gtk_dialog_run (GTK_DIALOG (dialog));

	switch (result) {
		GtkTreeIter child;
		case GTK_RESPONSE_OK:
			dir_name = gtk_entry_get_text (GTK_ENTRY (name));
			ret = create_dir((char*)full_path, (char*)dir_name);
			if (ret != 0) {
				if (ret == ERROR_MAX_FD_REACHED)
					show_message_dialog(dialog, GTK_MESSAGE_ERROR, "Error: Max number of Files / Folders Reached.");
				else if (ret == ERROR_FILE_ALREADY_EXISTS)
					show_message_dialog(dialog, GTK_MESSAGE_ERROR, "Error: Folder already exists.");				
				else
					show_message_dialog(dialog, GTK_MESSAGE_ERROR, "Error creating Folder!");
				printf("Error - create folder %d\n", ret);
				break;
			}
			temp[0]='\0';
			len = strlen(full_path);
			strcat(temp,full_path);
			if (full_path[len-1] != '/')
				strcat (temp, "/");
			strcat(temp, dir_name);

			gtk_tree_store_append((GtkTreeStore*)model, &child, &iter);
			gtk_tree_store_set((GtkTreeStore*)model, &child,
                     0, dir_name, 1, temp,IS_DIR_COLUMN, isDirectory(temp), -1);
			gtk_tree_view_expand_row (GTK_TREE_VIEW(callback_data), 
					gtk_tree_path_new_from_string(gtk_tree_model_get_string_from_iter ((GtkTreeModel*)model, &iter)), FALSE);
			break;
			
	}
	gtk_widget_destroy(dialog);

}

//callback_data is the tree view
static void context_new_file( gpointer     callback_data,
                                                    guint            callback_action,
                                                    GtkWidget *menu_item )
{

    GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	const gchar *full_path, *file_name;
	char temp[MAX_FILENAME_SIZE + MAX_PATH_SIZE + 5];
	int ret,len;
	GtkWidget* dialog, *content_area, *label1, *name;
	gint result;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(callback_data));

	 gtk_tree_selection_get_selected(selection, &model, &iter);
	//Get the second col value which has the full path
	 gtk_tree_model_get (model, &iter, 1, &full_path, -1);

	dialog = gtk_dialog_new_with_buttons ("Create Directory", NULL, GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT, 
		GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         NULL);
	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	label1 = gtk_label_new ("  Enter the name of the file to create:    ");
	gtk_box_pack_start(GTK_BOX(content_area), label1, FALSE, TRUE, 5);

	name = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(content_area), name, TRUE, TRUE, 5);

	gtk_widget_show_all (dialog);

	result = gtk_dialog_run (GTK_DIALOG (dialog));

	switch (result) {
		GtkTreeIter child;
		case GTK_RESPONSE_OK:
			file_name = gtk_entry_get_text (GTK_ENTRY (name));
			ret = add_file((char*)full_path, (char*)file_name, "");
			if (ret != 0) {
				if (ret == ERROR_MAX_FD_REACHED)
					show_message_dialog(dialog, GTK_MESSAGE_ERROR, "Error: Max number of Files / Folders Reached.");
				else if (ret == ERROR_FILE_ALREADY_EXISTS)
					show_message_dialog(dialog, GTK_MESSAGE_ERROR, "Error: File already exists.");
				else
					show_message_dialog(dialog, GTK_MESSAGE_ERROR, "Error creating File!");
				printf("Error - create file\n");
				break;
			}
			temp[0]='\0';
			len = strlen(full_path);
			strcat(temp,full_path);
			if (full_path[len-1] != '/')
				strcat (temp, "/");
			strcat(temp, file_name);

			gtk_tree_store_append((GtkTreeStore*)model, &child, &iter);
			gtk_tree_store_set((GtkTreeStore*)model, &child,
                     0, file_name, 1, temp,IS_DIR_COLUMN, isDirectory(temp), -1);
			gtk_tree_view_expand_row (GTK_TREE_VIEW(callback_data), 
					gtk_tree_path_new_from_string(gtk_tree_model_get_string_from_iter ((GtkTreeModel*)model, &iter)), FALSE);
			break;
			
	}
	gtk_widget_destroy(dialog);

}

static GtkItemFactoryEntry dir_context_menu_items[] = {
    { "/_New",                 NULL,                 NULL,                     0, "<Branch>" },
    { "/New/_Folder",         "NULL", context_new_folder,        1, "<Item>" },
    { "/New/F_ile",         "NULL", context_new_file,        1, "<Item>" },
    { "/C_ut",         "NULL", under_construction,        1, "<Item>" },
    { "/_Copy",         "NULL", under_construction,        1, "<Item>" },
    { "/_Paste into Folder",         "NULL", under_construction,        1, "<Item>" },
 	{ "/sep1",     NULL,         NULL,           0, "<Separator>" },
    { "/_Rename",         "NULL", under_construction,        1, "<Item>" },
    { "/Move to _Trash",         "NULL", action_move_dir_trash,        1, "<Item>" }
};

    GtkWidget *dir_menu, *file_menu;
  void view_popup_menu_dir (GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
  {

     gtk_menu_popup(GTK_MENU(dir_menu), NULL, NULL, NULL, NULL,
                   (event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));
  }

static void context_open_file( gpointer     callback_data,
                                                    guint            callback_action,
                                                    GtkWidget *menu_item )
{

    GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	const gchar *full_path;
	gchar *new_text;
	char *parent_path, *file_name, *old_text;
	char temp[MAX_FILENAME_SIZE + MAX_PATH_SIZE + 50];
	int ret;
	gint len;
	GtkWidget* dialog, *content_area, *file_view,*sw;
	GtkTextBuffer *buffer;
	gint result;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(callback_data));

	 gtk_tree_selection_get_selected(selection, &model, &iter);
	//Get the second col value which has the full path
	 gtk_tree_model_get (model, &iter, 1, &full_path, -1);

	//Max_BLOCKS is max blocks per file which is 20
	old_text = (char*) malloc(((BLOCK_SIZE * MAX_BLOCKS) + 1024)*sizeof(char));// the file can be at max 20k
	len = get_file((char*)full_path, old_text);
	old_text[len]='\0';

	if (len < 0 ) {
		show_message_dialog(dialog, GTK_MESSAGE_ERROR, "Error Opening File!");
		printf("Error Opening File!");
		return;
	}

	strcpy (temp, "Edit File: ");
	strcat (temp, full_path);
	dialog = gtk_dialog_new_with_buttons (temp, NULL, GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT, 
		GTK_STOCK_SAVE, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         NULL);
	gtk_widget_set_size_request (dialog, 400, 350);


    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(sw),
                     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(sw),
                        GTK_SHADOW_ETCHED_IN);


	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	file_view = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(sw), file_view);
	gtk_box_pack_start(GTK_BOX(content_area), sw, TRUE, TRUE, 5);


	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(file_view));
	gtk_text_buffer_insert_at_cursor(buffer, (const gchar*)old_text, len);
	gtk_widget_show_all (dialog);

	result = gtk_dialog_run (GTK_DIALOG (dialog));

	switch (result) {
		GtkTextIter start,end;
		case GTK_RESPONSE_OK:

			parent_path = get_parent_path((char*)full_path);
			file_name = get_file_name ((char*)full_path);

			//First remove the file
			remove_file((char*)full_path);

			//Then add the file with new text
			buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(file_view));
			gtk_text_buffer_get_iter_at_offset(buffer, &start,0);
			gtk_text_buffer_get_iter_at_offset(buffer, &end,-1);
			new_text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

			ret = add_file(parent_path, file_name, (char*)new_text );
			
			if (ret != 0) {
				show_message_dialog(dialog, GTK_MESSAGE_ERROR, "Error saving file!");
				printf("Error - save file\n");
				ret = add_file(parent_path, file_name, old_text );
				break;
			}
			break;
			
	}
	free(old_text);
	gtk_widget_destroy(dialog);

}

static GtkItemFactoryEntry file_context_menu_items[] = {
    { "/_Open",                 NULL,                 context_open_file,                     1, "<Item>" },
    { "/C_ut",         "NULL", under_construction,        1, "<Item>" },
    { "/_Copy",         "NULL", under_construction,        1, "<Item>" },
 	{ "/sep1",     NULL,         NULL,           0, "<Separator>" },
    { "/_Rename",         "NULL", under_construction,        1, "<Item>" },
    { "/Move to _Trash",         "NULL", action_move_file_trash,        1, "<Item>" }
};

  void view_popup_menu_file (GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
  {


     gtk_menu_popup(GTK_MENU(file_menu), NULL, NULL, NULL, NULL,
                   (event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));
  }

  gboolean view_onButtonPressed (GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
  {
    /* single click with the right mouse button? */
    if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
    {

        GtkTreeSelection *selection;
		GtkTreeModel     *model;
		GtkTreeIter       iter;

        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

        GtkTreePath *path;
        /* Get tree path for row that was clicked */
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),
                                             (gint) event->x, 
                                             (gint) event->y,
                                             &path, NULL, NULL, NULL))
		{

			 gchar *full_path;

             gtk_tree_selection_unselect_all(selection);
             gtk_tree_selection_select_path(selection, path);
             gtk_tree_path_free(path);
			//Atleast one has to be selected because in it is in browse mode
			 gtk_tree_selection_get_selected(selection, &model, &iter);
			//Get the second col value which has the full path
			 gtk_tree_model_get (model, &iter, 1, &full_path, -1);
			 if (isDirectory(full_path)) {
			      view_popup_menu_dir(treeview, event, userdata);
			 } else {
			      view_popup_menu_file(treeview, event, userdata);
			}

		} else {
			return TRUE; //nothing to do
		}

      return TRUE; /* we handled this */
    }

    return FALSE; /* we did not handle this */
  }


  gboolean view_onPopupMenu (GtkWidget *treeview, gpointer userdata)
  {
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	gchar *full_path;

  /* This will only work in single or browse selection mode! */

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{

		 gtk_tree_model_get (model, &iter, 1, &full_path, -1);
		 if (isDirectory(full_path)) {
		      view_popup_menu_dir(treeview, NULL, userdata);
		 } else {
		      view_popup_menu_file(treeview, NULL, userdata);
		}
	}

    return TRUE;
  }

void view_onRowActivated (GtkTreeView        *treeview,
                       GtkTreePath        *path,
                       GtkTreeViewColumn  *col,
                       gpointer            userdata)
  {
    GtkTreeModel *model;
    GtkTreeIter   iter;
    GtkTreeSelection *selection;

    model = gtk_tree_view_get_model(treeview);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
       gchar *full_path;

       gtk_tree_model_get(model, &iter, 1, &full_path, -1);

		 if (isDirectory(full_path)) {
			if (gtk_tree_view_row_expanded (treeview, path)) {
				gtk_tree_view_collapse_row (treeview, path);
			} else {
				gtk_tree_view_expand_row (treeview, path, FALSE);
			}			
		 } else {
			context_open_file(treeview,0,NULL );
		}
	}
 }


void setup_context_menu(GtkWidget *view) {

	gint nmenu_items;
   GtkItemFactory *item_factory;

	//Creating dir context menu
	nmenu_items = sizeof (dir_context_menu_items) / sizeof (dir_context_menu_items[0]);
  
   item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<dircontext>",
                                        NULL);
   gtk_item_factory_create_items (item_factory, nmenu_items, dir_context_menu_items, view);
   dir_menu = gtk_item_factory_get_widget (item_factory, "<dircontext>");

	//Creating File context menu  
	nmenu_items = sizeof (file_context_menu_items) / sizeof (file_context_menu_items[0]);
   item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<filecontext>",
                                        NULL);
   gtk_item_factory_create_items (item_factory, nmenu_items, file_context_menu_items, view);
   file_menu = gtk_item_factory_get_widget (item_factory, "<filecontext>");


	g_signal_connect(view, "row-activated", (GCallback) view_onRowActivated, NULL);

    g_signal_connect(view, "button-press-event", (GCallback) view_onButtonPressed, NULL);
    g_signal_connect(view, "popup-menu", (GCallback) view_onPopupMenu, NULL);



}

