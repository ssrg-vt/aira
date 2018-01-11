#include <stdio.h>
void mySmallFunction(int array[]);
void mySmallerFunction(int *array,int numIncrement);
int globalInt = 0;
#pragma popcorn arch ( x86 )
#pragma popcorn inputs (  )
#pragma popcorn globalInputs ( globalInt )
#pragma popcorn outputs (  )
#pragma popcorn globalOutputs ( globalInt )
#pragma popcorn functionsNeeded ( mySmallFunction, mySmallerFunction )

int main()
{
  int array[5UL];
  mySmallFunction(array);
  return 0;
}
#pragma popcorn arch ( x86 )
#pragma popcorn inputs ( array* )
#pragma popcorn globalInputs ( globalInt )
#pragma popcorn outputs ( array* )
#pragma popcorn globalOutputs ( globalInt )
#pragma popcorn functionsNeeded ( mySmallerFunction )

void mySmallFunction(int array[])
{
  printf("Calling sub-function...\n");
  mySmallerFunction(array+2,10);
}
#pragma popcorn partitioned
#pragma popcorn arch ( x86, tilera, gpu )
#pragma popcorn inputs ( array*, numIncrement )
#pragma popcorn globalInputs ( globalInt )
#pragma popcorn outputs ( array* )
#pragma popcorn globalOutputs ( globalInt )
#pragma popcorn functionsNeeded (  )

void mySmallerFunction(int *array,int numIncrement)
{
  globalInt += numIncrement;
   *array = 10;
}
