#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mm_wrapper.h"


int main(int argc, char** argv)
{
	//Initialize the wrappers
	INIT_MM_WRAPPER();

	printf("Attempting to do wrapped malloc...");
	void* testMalloc = malloc(2048);
	printf("Pointer/sizeof testMalloc: %p %d\n", testMalloc,
			get_size(testMalloc));
	free(testMalloc);

	printf("Attempting to do wrapped calloc...");
	testMalloc = calloc(512, 4);
	printf("Pointer/sizeof testMalloc: %p %d\n", testMalloc,
			get_size(testMalloc));
	int i = 0;
	for(i = 0; i < 2048; i++)
		if(((char*)testMalloc)[i] != 0)
			printf("Found %d instead of 0\n",
				((int*)testMalloc)[i]);
	free(testMalloc);

	printf("Attempting to do wrapped realloc...");
	testMalloc = malloc(2048);
	testMalloc = realloc(testMalloc, 4096);
	printf("Pointer/sizeof testMalloc: %p %d\n", testMalloc,
			get_size(testMalloc));
	free(testMalloc);

	printf("Finished!\n");
	return 0;
}
/* vim: set noexpandtab shiftwidth=4 tabstop=4 softtabstop=4: */
