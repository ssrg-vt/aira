#include <stdlib.h>

void kernel(int dim1, int dim2, int** arr)
{
	int i = 0, j = 0;
#pragma omp parallel for
	for(i = 0; i < dim1; i++)
		for(j = 0; j < dim2; j++)
			arr[i][j] = i + j;
}

int main(int argc, char** argv)
{
	int dim1 = 10, dim2 = 10, i = 0;
	int** arr = (int**)malloc(sizeof(int*) * dim1);
	int* arr_hidden = (int*)malloc(sizeof(int) * dim1 * dim2);
	for(i = 0; i < dim1; i++)
		arr[i] = &arr_hidden[dim2 * i];

	kernel(dim1, dim2, arr);

	return 0;
}
