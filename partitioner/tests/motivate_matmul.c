/* gcc -std=c89 -fopenmp -Wall -O3 motivate_matmul.c -o motivate_matmul */
/* C89 to keep OpenMPC happy. */

/* To run:  ./motivate_matmul $MATRIX_DIMENSION */

#include <stdlib.h>
#include <assert.h>

typedef float TYPE;

static void fill(TYPE *matrix, int size, TYPE val)
{
  int loop;
  for (loop = 0; loop < size; loop++) {
    matrix[loop] = val;
  }
}

static void matmul(int dim, TYPE *C, const TYPE *A, const TYPE *B)
{
  int i,j,k;
  #pragma omp parallel for shared(C, A, B)
  for (i = 0; i < dim; i++) {
    for (j = 0; j < dim; j++) {
      TYPE accum = (TYPE)0;
      for (k = 0; k < dim; k++) {
        accum += A[i*dim + k] * B[k*dim + j];
      }
      C[i*dim + j] = accum;
    }
  }
}

int main(int argc, char **argv)
{
  /* "Parse" command line args. */
  assert(argc == 2);
  int dim = atoi(argv[1]);
  assert(dim > 0);

  /* Setup data. */
  int size = dim * dim;
	int result;
  TYPE *A = (TYPE *)malloc(size * sizeof(TYPE));
  TYPE *B = (TYPE *)malloc(size * sizeof(TYPE));
  TYPE *C = (TYPE *)malloc(size * sizeof(TYPE));
  fill(A, size, (TYPE)3);
  fill(B, size, (TYPE)4);

  /* The main show. */
  matmul(dim, C, A, B);
	result = C[size-1];

	/* Cleanup */
	free(A);
	free(B);
	free(C);

  return result;
}
