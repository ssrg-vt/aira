/*
 * Serial C implementation of vector add
 */
void add_vects(const int* a, const int* b, int* c, int size)
{
	int i;
	for(i = 0; i < size; i++)
		c[i] = a[i] + b[i];
}

