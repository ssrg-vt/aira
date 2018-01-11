#define NUM_ACCUM 32

__kernel void local_accum(__global int* a,
											__local int* b,
											int size)
{
	int tid = get_global_id(0);
	int i;
	for(i = 0; i < 32; i++)
		b[tid] = a[tid + i];
}
