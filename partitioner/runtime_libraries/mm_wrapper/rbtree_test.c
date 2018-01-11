#include <stdlib.h>
#include <stdio.h>
#include "rbtree.h"

#define TESTSIZE 10

/*
 * Function to test the RB-tree implementation.
 */
int main(int argc, char** argv)
{
	rbtree t = rbtree_create();
	void* pointers[TESTSIZE];
	int sizes[TESTSIZE];
	int i = 0, randVal = 0;

	//Malloc some values and store their pointers in the array
	for(i = 0; i < TESTSIZE; i++)
	{
		randVal = rand() % 1024;
		pointers[i] = (void*)malloc(randVal);
		sizes[i] = randVal;
		printf("Malloc'd pointer %p with size %d\n", pointers[i], sizes[i]);
	}

	//Add the pointers to the tree
	for(i = 0; i < TESTSIZE; i++)
		rbtree_insert(t, pointers[i], sizes[i]);

	//Check to make sure we can find them
	int size;
	for(i = 0; i < TESTSIZE; i++)
	{
		size = rbtree_lookup(t, pointers[i]);
		if(size != sizes[i])
			printf("Found incorrect size for pointer %p!\n",
				pointers[i]);
	}

	//Check for some in-between values
	void* test;
	for(i = 0; i < TESTSIZE; i++)
	{
		printf("Searching for pointer %p...", pointers[i] + 10);
		test = rbtree_lookup_pointer(t, pointers[i] + 10);
		if(test != pointers[i])
			printf("incorrect in-between value - returned %p!\n", test);
		else
			printf("returned %p\n", test);
	}

	//Remove the pointers from the tree
	for(i = 0; i < TESTSIZE; i++)
		rbtree_delete(t, pointers[i]);

	//Free the malloc'd pointers & the tree
	for(i = 0; i < TESTSIZE; i++)
		free(pointers[i]);
	rbtree_destroy(t);

	return 0;
}
