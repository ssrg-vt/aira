/*
 * Strange loop to test LLVM's loop nest depth mechanism.
 */
int irregular_loop(const int* a, const int* b, int* c, int size)
{
	int counter;
	for(int i = 0; i < size; i++)
	{
		c[i] = a[i] + b[i];
		if(c[i] < 1000)
			counter++;
		else if(c[i] % 9 == 0)
			break;
	}
	return counter;
}
