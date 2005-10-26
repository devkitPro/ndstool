struct Tree
{
	unsigned int dir_id;	// directory ID in case of directory entry
	char *name;				// file or directory name
	Tree *directory;		// nonzero indicates directory. first tree node is a dummy
	Tree *next;				// linked list

	Tree()
	{
		dir_id = 0;
		name = 0;
		directory = 0;
		next = 0;
	}
};

Tree *ReadDirectory(Tree *tree, char *path);
