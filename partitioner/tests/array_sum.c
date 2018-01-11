#include <time.h>
#include <stdlib.h>
#include <stdio.h>

long doubleSum(long sum) {
	printf("Sum: %ld\n", sum * 2);
	return sum * 2;
}

long sumArray(int array[], int numElements) {
	long sum = 0;
	int i = 0;
	for(i = 0; i < numElements; i++) {
		sum += array[i];
	}
	sum = doubleSum(sum);
	return sum;
}

int main(int argc, char** argv) {
	int array[100];
	long sum = 0;
	int i = 0;
	srand(time(NULL));
	for(i = 0; i < 100; i++) {
		array[i] = rand(); 
	}
	sum = sumArray(array, 100);
	printf("Sum: %ld\n", sum);
	return 0;
}
