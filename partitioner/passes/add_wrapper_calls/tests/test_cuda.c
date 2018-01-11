#include <cuda/cuda.h>

extern int globalInt;

void mySmallerFunction(int *array,int numIncrement)
{
  globalInt += numIncrement;
  *array = 10;
}
