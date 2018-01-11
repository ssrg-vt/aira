/*
 * Simple vector add
 */

__kernel void add_vectors(__global const int* a,
													__global const int* b,
													__global int* c,
													int size)
{
	int tid = get_global_id(0);
	if(tid < size)
		c[tid] = a[tid] + b[tid];
}

/*
 * SIMD vector add
 */
__kernel void add_vectors_simd(__global const int4* a,
																__global const int4* b,
																__global int4* c,
																int size)
{
	int tid = get_global_id(0);
	if(tid < size)
		c[tid] = a[tid] + b[tid];
}
