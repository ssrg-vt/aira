#include <stdlib.h>
#include <stdio.h>

static const int numDivisions = 100000000;
static double divisionWidth;

void calcAreas(double* areas)
{
	int i = 0;
	double y = 0.0;
	double curX = 0.0;

#pragma omp parallel for
	for(i = 0; i < numDivisions; i++)
	{
		curX = ((double)i + 0.5) * divisionWidth;
		y = 4.0/(1.0 + (curX * curX));
		areas[i] = divisionWidth * y;
	}
}

double sumAreas(double* areas)
{
	int i = 0;
	double sum = 0.0;

#pragma omp parallel for reduction(+:sum)
	for(i = 0; i < numDivisions; i++)
	{
		sum += areas[i];
	}
	return sum;
}

int main(int argc, char** argv)
{
	double* areas = (double*)malloc(sizeof(double) * numDivisions);
	double pi = 0.0;
	divisionWidth = 1.0/(double)numDivisions;

	printf("Calculating %d areas...\n", numDivisions);
	calcAreas(areas);
	printf("Summing %d areas...\n", numDivisions);
	pi = sumAreas(areas);
	printf("Pi: %1.16f\n", pi);

	free(areas);
	return 0;
}
