#include <stdio.h>

long globalVal = 0;

int performStupid(int numRuns) {

	int i = 0, stupid = 0;
	for(i = 0; i < numRuns; i++) {
		stupid += i % 25;
		globalVal = 1 + 2 + 3 + 4;
	}
	return 5;
}

int main(int argc, char** argv) {

	int myNumber = 1234567;
	printf("Stupid number: %d\n", performStupid(myNumber));
	int i = performStupid(myNumber);
	i = performStupid(1234);
	return i;
}
/* vim: set noexpandtab shiftwidth=4 tabstop=4 softtabstop=4: */
