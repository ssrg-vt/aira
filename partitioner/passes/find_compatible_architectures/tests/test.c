#include <stdio.h>

void mySmallFunction(int array[]);
void mySmallerFunction(int* array);

int main()
{
	int array[5];
	mySmallFunction(array);
	return 0;
}

void mySmallFunction(int array[])
{
	printf("Calling sub-function...\n");
	mySmallerFunction(array);
}

void mySmallerFunction(int* array)
{
	*array = 10;
}
