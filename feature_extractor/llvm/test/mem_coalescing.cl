__kernel void weird_access(__global int* a,
														__global int* b,
														int size)
{
	int tid = get_global_id(0);
	int double_tid = 2 * tid;
	if(tid < size)
	{
		b[tid] = a[double_tid] + a[double_tid + tid];
	}
}

#define NUM_ACCESSES 32

__kernel void loop_access(__global int* a,
													__global int* b,
													int size)
{
	int tid = get_global_id(0);
	int i;
	if(tid < size)
	{
		for(i = 0; i < NUM_ACCESSES; i++)
			b[tid] += a[tid + i];
	}
}

