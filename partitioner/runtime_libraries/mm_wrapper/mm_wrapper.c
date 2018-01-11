/*
 * C Memory Management Wrapper
 *
 * These functions wrap the standard C library memory functions malloc, calloc,
 * realloc and free.  They implement no memory management logic except to add
 * a bit of meta-data used by the partitioner at runtime.
 *
 * By wrapping these functions and adding some meta-data to the block managed
 * by these functions, we can determine at runtime how much data must be
 * transferred between partitions.  This meta-data at the start of the
 * allocated block helps the partitioner determine the size of the block from
 * the block pointer in constant time, allowing efficient retrieval of
 * allocated block sizes.
 *
 * As of right now, we do not wrap 'valloc' or 'memalign'.  If there seems to
 * be a need to do so, we will implement them.
 *
 * Author: Rob Lyerly <rlyerly@vt.edu>
 */
//TODO concurrent memory allocation/free?

#include <string.h>
#include "rbtree.h"

/* Record runtime statistics */
#ifdef _STATISTICS
#include <stdio.h>
#include "timer.h"

/* Get detailed debugging information (may add considerable overhead) */
#ifdef _DEBUG
#include <execinfo.h>
#endif
#endif

/*
 * Flag indicating whether or not to use the memory management wrappers. This
 * is set via calls to register_mm_wrappers() and unregister_mm_wrappers().
 */
#define TRUE 1
#define FALSE 0
static int use_wrapper = FALSE;

/*
 * Declarations (for ld) to the libc memory management functions.
 */
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void*, size_t);
void *__real_valloc(size_t);
void *__real_memalign(size_t, size_t);
void *__real_free(void*);

/*
 * Red-black tree, used to store the sizes of pointers.
 */
static rbtree tree;

#ifdef _STATISTICS
/*
 * Timer and counter for timing the overhead added by the wrapper.
 */
static timer mm_timer;
static uint64_t total_time = 0;

/*
 * Number of variables that have been alloc'd (malloc/calloc/realloc) & freed.
 * Does NOT indicate amount of memory alloc'd or freed.
 */
static uint64_t allocated = 0;
static uint64_t requests = 0;
static uint64_t freed = 0;

#ifdef _DEBUG
#define MAX_DEPTH 50

/*
 * Backtrace information
 */
static void* call_chain[MAX_DEPTH];
static size_t call_chain_length;
static char** func_names = NULL;
static const char* no_caller = "(could not find caller)";

/*
 * Return the name of the function that called the MM wrapper
 */
static const char* caller()
{
	use_wrapper = FALSE;
	if(func_names)
		free(func_names);
	call_chain_length = backtrace(call_chain, MAX_DEPTH);
	if(call_chain_length < 2)
		return no_caller;
	func_names = backtrace_symbols(call_chain, call_chain_length);

	size_t i;
	for(i = 0; i < call_chain_length; i++)
		printf("%s\n", func_names[i]);

	use_wrapper = TRUE;
	return (const char*)func_names[1];
}
#endif /* _DEBUG */
#endif /* _STATISTICS */

/*
 * Enable the wrappers.
 */
int register_mm_wrappers()
{
#ifdef _STATISTICS
	total_time = 0;
	allocated = 0;
	requests = 0;
	freed = 0;
	init_timer(&mm_timer);
	start_timer(&mm_timer);
#endif
	use_wrapper = FALSE;
	tree = rbtree_create();
	use_wrapper = TRUE;
#ifdef _STATISTICS
	stop_timer(&mm_timer);
	total_time += get_time(&mm_timer);
#ifdef _DEBUG
	printf("register_mm_wrappers() (called from %s) time: %lu\n", caller(),
		get_time(&mm_timer));
#endif
#endif
	return use_wrapper;
}

/*
 * Disable usage of the wrappers.
 *
 * Note: wrappers should be disabled before calling exit, because the process
 * cleanup routines require usage of the normal libc version of free.
 */
void unregister_mm_wrappers()
{
#ifdef _STATISTICS
	start_timer(&mm_timer);
#endif
	use_wrapper = FALSE;
	rbtree_destroy(tree);
#ifdef _STATISTICS
	stop_timer(&mm_timer);
	total_time += get_time(&mm_timer);
	printf("Number of alloc,request,free calls: %lu,%lu,%lu\n", allocated, requests,
		freed);
	printf("MM wrapper overhead: %lu ns\n", total_time);
#ifdef _DEBUG
	printf("unregister_mm_wrappers() (called from %s) time: %lu\n", caller(),
		get_time(&mm_timer));
	if(func_names)
		free(func_names);
#endif
#endif
}

/*
 * Return the size of the allocated data block in which ptr resides.
 */
int get_size(void *ptr)
{
#ifdef _STATISTICS
	start_timer(&mm_timer);
	requests++;
#endif
	int size = rbtree_lookup(tree, ptr);
#ifdef _STATISTICS
	stop_timer(&mm_timer);
	total_time += get_time(&mm_timer);
#ifdef _DEBUG
	printf("get_size() (called from %s) time: %lu\n", caller(),
		get_time(&mm_timer));
#endif
#endif
	return size;
}

/*
 * Return the size of the allocated data block which ptr points to.
 *
 * This method returns 0 if the pointer is NOT found in the tree.  This is
 * mainly used for debugging.
 */
int get_size_np(void *ptr)
{
#ifdef _STATISTICS
	start_timer(&mm_timer);
	requests++;
#endif
	int size = rbtree_lookup_np(tree, ptr);
#ifdef _STATISTICS
	stop_timer(&mm_timer);
	total_time += get_time(&mm_timer);
#ifdef _DEBUG
	printf("get_size_np() (called from %s) time: %lu\n", caller(),
		get_time(&mm_timer));
#endif
#endif
	return size;
}

/*
 * Return a pointer to the beginning of the data block in which ptr resides.
 */
void *get_pointer(void *ptr)
{
#ifdef _STATISTICS
	start_timer(&mm_timer);
	requests++;
#endif
	void *ret_ptr = rbtree_lookup_pointer(tree, ptr);
#ifdef _STATISTICS
	stop_timer(&mm_timer);
	total_time += get_time(&mm_timer);
#ifdef _DEBUG
	printf("get_pointer() (called from %s) time: %lu\n", caller(),
		get_time(&mm_timer));
#endif
#endif
	return ret_ptr;
}

/*
 * Register a pointer with specified data size in the red-black tree for
 * retrieval.
 */
void register_pointer(void *ptr, size_t size)
{
	if(use_wrapper)
	{
#ifdef _STATISTICS
		start_timer(&mm_timer);
		allocated++;
#endif
		use_wrapper = FALSE;
		rbtree_insert(tree, ptr, size);
		use_wrapper = TRUE;
#ifdef _STATISTICS
		stop_timer(&mm_timer);
		total_time += get_time(&mm_timer);
#ifdef _DEBUG
		printf("register_pointer() (called from %s) time: %lu\n", caller(),
			get_time(&mm_timer));
#endif
#endif
	}
	else
		rbtree_insert(tree, ptr, size);
}

/*
 * Delete the specified pointer from the red-black tree.
 */
void unregister_pointer(void *ptr)
{
	if(use_wrapper)
	{
#ifdef _STATISTICS
		start_timer(&mm_timer);
		freed++;
#endif
		use_wrapper = FALSE;
		rbtree_delete(tree, ptr);
		use_wrapper = TRUE;
#ifdef _STATISTICS
		stop_timer(&mm_timer);
		total_time += get_time(&mm_timer);
#ifdef _DEBUG
		printf("unregister_pointer() (called from %s) time: %lu\n", caller(),
			get_time(&mm_timer));
#endif
#endif
	}
	else
		rbtree_delete(tree, ptr);
}

/*
 * Wrapper for 'malloc'.
 *
 * Calls the libc malloc and saves the pointer + size in the red-black tree.
 */
void *__wrap_malloc(size_t size)
{
	if(use_wrapper)
	{
		use_wrapper = FALSE;
		void *block = __real_malloc(size);
#ifdef _STATISTICS
		start_timer(&mm_timer);
		allocated++;
#endif
		rbtree_insert(tree, block, size);
		use_wrapper = TRUE;
#ifdef _STATISTICS
		stop_timer(&mm_timer);
		total_time += get_time(&mm_timer);
#ifdef _DEBUG
		printf("malloc() (called from %s) time: %lu\n", caller(),
			get_time(&mm_timer));
#endif
#endif
		return block;
	}
	else
		return __real_malloc(size);
}

/*
 * Wrapper for 'calloc'.
 *
 * Calls the libc malloc, zeros the data and saves the pointer + size in the
 * red-black tree.
 */
void *__wrap_calloc(size_t nmemb, size_t size)
{
	if(use_wrapper)
	{
		use_wrapper = FALSE;
		void *block = __real_malloc(nmemb * size);
		memset(block, 0, size * nmemb);
#ifdef _STATISTICS
		start_timer(&mm_timer);
		allocated++;
#endif
		rbtree_insert(tree, block, nmemb * size);
		use_wrapper = TRUE;
#ifdef _STATISTICS
		stop_timer(&mm_timer);
		total_time += get_time(&mm_timer);
#ifdef _DEBUG
		printf("calloc() (called from %s) time: %lu\n", caller(),
			get_time(&mm_timer));
#endif
#endif
		return block;
	}
	else
		return __real_calloc(nmemb, size);
}

/*
 * Wrapper for 'realloc'.
 *
 * Deletes the old pointer from the red-black tree, calls the libc realloc and
 * saves the new pointer in the red-black tree.
 */
void *__wrap_realloc(void *ptr, size_t size)
{
	if(use_wrapper)
	{
#ifdef _STATISTICS
		start_timer(&mm_timer);
		allocated++;
#endif
		use_wrapper = FALSE;
		rbtree_delete(tree, ptr);
#ifdef _STATISTICS
		stop_timer(&mm_timer);
		total_time += get_time(&mm_timer);
#endif
		void *new_block = __real_realloc(ptr, size);
#ifdef _STATISTICS
		start_timer(&mm_timer);
#endif
		rbtree_insert(tree, new_block, size);
		use_wrapper = TRUE;
#ifdef _STATISTICS
		stop_timer(&mm_timer);
		total_time += get_time(&mm_timer);
#ifdef _DEBUG
		printf("realloc() (called from %s) time: %lu\n", caller(),
			get_time(&mm_timer));
#endif
#endif
		return new_block;
	}
	else
		return __real_realloc(ptr, size);
}

/*
 * Wrapper for 'free'.
 *
 * Calls the libc free and removes the pointer from the red-black tree.
 */
void __wrap_free(void *ptr)
{
	//Semantics of 'free'.  If 'ptr' is null, the function does nothing
	if(ptr == NULL)
		return;

	if(use_wrapper)
	{
#ifdef _STATISTICS
		start_timer(&mm_timer);
		freed++;
#endif
		use_wrapper = FALSE;
		rbtree_delete(tree, ptr);
#ifdef _STATISTICS
		stop_timer(&mm_timer);
		total_time += get_time(&mm_timer);
#ifdef _DEBUG
		printf("get_size() (called from %s) time: %lu\n", caller(),
			get_time(&mm_timer));
#endif
#endif
		__real_free(ptr);
		use_wrapper = TRUE;
	}
	else
		__real_free(ptr);
	return;
}

/*
 * NOTE: We do not currently wrap 'valloc' or 'memalign'.
 */
/* vim: set noexpandtab shiftwidth=4 tabstop=4 softtabstop=4: */
