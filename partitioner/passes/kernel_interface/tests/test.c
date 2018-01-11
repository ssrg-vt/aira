#include <stdio.h>
void mySmallFunction(int array[]);
void mySmallerFunction(int *array);
#pragma popcorn compatibleArch( x86 )

int main()
{
  int array[5UL];
  mySmallFunction(array);
  return 0;
}
#pragma popcorn compatibleArch( x86 )

void mySmallFunction(int array[])
{
  printf("Calling sub-function...\n");
  mySmallerFunction(array);
}
#pragma popcorn compatibleArch( x86, tilera, gpu )

void mySmallerFunction(int *array)
{
   *array = 10;
}
