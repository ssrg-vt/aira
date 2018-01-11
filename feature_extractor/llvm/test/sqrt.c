#include <math.h>

void vec_sqrt(const float* a, float* b, int size)
{
	int i;
	for(i = 0; i < size; i++)
	{
		if(a[i] > 0)
			b[i] = sqrt(a[i]);
		else
			b[i] = 0;
	}
}
