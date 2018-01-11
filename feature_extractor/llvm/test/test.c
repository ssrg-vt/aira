#include <stdio.h>
#include <math.h>

#define NUM_DIVS 1000000
#define NUM_ITS 100

double calc_pi()
{
	double div_width = 1.0/NUM_DIVS;
	double curx = 0.0;
	double cury = 0.0;
	double sum = 0.0;
	int i = 0;

//#pragma omp parallel for private(curx, cury) reduction(+: sum)
	for(i = 0; i < NUM_DIVS; i++)
	{
		curx = ((double)i + 0.5) * div_width;
		cury = 4.0 / (1 + sin(curx));
		sum += div_width * cury;
	}

	return sum;
}

int main(int argc, char** argv)
{
	int i = 0;
	double pi = 0.0;

	for(i = 0; i < NUM_ITS; i++)
	{
		pi = calc_pi();
	}

	printf("Pi: %1.16f\n", pi);

	return 0;
}
