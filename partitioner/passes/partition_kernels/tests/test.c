#include <stdio.h>
void mySmallFunction(int array[]);
void mySmallerFunction(int *array);
#pragma popcorn compatibleArch ( x86 )
#pragma popcorn inputs (  )
#pragma popcorn globalInputs (  )
#pragma popcorn outputs (  )
#pragma popcorn globalOutputs (  )
#pragma popcorn functionsNeeded ( mySmallFunction, mySmallerFunction )

int main()
{
  int array[5UL];
  mySmallFunction(array);
  return 0;
}
#pragma popcorn compatibleArch ( x86 )
#pragma popcorn inputs ( array* )
#pragma popcorn globalInputs (  )
#pragma popcorn outputs ( array* )
#pragma popcorn globalOutputs (  )
#pragma popcorn functionsNeeded ( mySmallerFunction )

void mySmallFunction(int array[])
{
  printf("Calling sub-function...\n");
  mySmallerFunction(array);
}
#pragma popcorn arch ( gpu )
#pragma popcorn compatibleArch ( x86, tilera, gpu )
#pragma popcorn inputs ( array* )
#pragma popcorn globalInputs (  )
#pragma popcorn outputs ( array* )
#pragma popcorn globalOutputs (  )
#pragma popcorn functionsNeeded (  )

void mySmallerFunction(int *array)
{
   *array = 10;
}
